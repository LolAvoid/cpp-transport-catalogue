#include "transport_router.h"
#include <algorithm>

namespace transport {

void Router::BuildGraph(const transport_catalogue::TransportCatalogue& catalogue) {
    const auto& all_stops = catalogue.GetSortedAllStops();
    graph::DirectedWeightedGraph<double> stops_graph(all_stops.size() * 2);
    std::unordered_map<std::string, graph::VertexId> stop_ids;
    graph::VertexId vertex_id = 0;

    // Создаем вершины и ребра ожидания
    for (const auto& [stop_name, stop_info] : all_stops) {
        stop_ids[stop_info->name] = vertex_id;
        stops_graph.AddEdge({
            stop_info->name,
            0,
            vertex_id,
            vertex_id + 1,
            static_cast<double>(settings_.bus_wait_time)
        });
        vertex_id += 2;
    }
    stop_ids_ = std::move(stop_ids);

    // Предварительно рассчитываем коэффициент скорости
    const double velocity_coef = settings_.bus_velocity * 1000.0 / 60.0;

    // Создаем ребра поездки на автобусе
    const auto& all_buses = catalogue.GetSortedAllBuses();
    for (const auto& [bus_name, bus_info] : all_buses) {
        const auto& stops = bus_info->stops;
        const size_t stops_count = stops.size();
        
        for (size_t i = 0; i < stops_count; ++i) {
            for (size_t j = i + 1; j < stops_count; ++j) {
                const auto* stop_from = stops[i];
                const auto* stop_to = stops[j];
                
                // Рассчитываем расстояние для прямого направления
                int distance = 0;
                for (size_t k = i + 1; k <= j; ++k) {
                    distance += catalogue.GetDistance(stops[k - 1], stops[k]);
                }                
                // Добавляем прямое ребро
                AddBusEdge(stops_graph, bus_info->name, static_cast<size_t>(j - i), 
                                stop_from->name, stop_to->name, distance, velocity_coef);

                // Для некольцевых маршрутов добавляем обратное ребро
                if (!bus_info->is_round_trip) {
                    int reverse_distance = 0;
                    for (size_t k = j; k > i; --k) {
                        reverse_distance += catalogue.GetDistance(stops[k], stops[k - 1]);
                    }
                    
                    AddBusEdge(stops_graph, bus_info->name, static_cast<size_t>(j - i),
                              stop_to->name, stop_from->name, reverse_distance, velocity_coef);
                }
            }
        }
    }
    
    graph_ = std::move(stops_graph);
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

void Router::AddBusEdge(graph::DirectedWeightedGraph<double>& graph,
                       const std::string& bus_name,
                       size_t span_count,  
                       const std::string& from_stop,
                       const std::string& to_stop,
                       double distance,
                       double velocity) const {
    double time = distance / velocity;
    graph.AddEdge({
        bus_name,
        span_count,  
        stop_ids_.at(from_stop) + 1,
        stop_ids_.at(to_stop),
        time
    });
}

std::optional<RouteInfo> Router::GetRouteInfo(std::string_view stop_from, std::string_view stop_to) const {
    if (!router_) {
        return std::nullopt;
    }

    auto route = router_->BuildRoute(
        stop_ids_.at(std::string(stop_from)),
        stop_ids_.at(std::string(stop_to))
    );

    if (!route) {
        return std::nullopt;
    }

    RouteInfo result;
    result.total_time = route->weight;

    for (const auto edge_id : route->edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        if (edge.span_count == 0) {
            result.items.push_back(WaitItem{edge.name, edge.weight});
        } else {
            result.items.push_back(BusItem{edge.name, edge.span_count, edge.weight});
        }
    }

    return result;
}

} // namespace transport