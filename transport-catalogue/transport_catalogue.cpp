#include "transport_catalogue.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string_view name, const geo::Coordinates& coordinates) {
    string_storage_.emplace_back(name);
    auto& stored_name = string_storage_.back();
    stops_.emplace_back(Stop{stored_name, coordinates});
    stops_map_[stored_name] = &stops_.back();
}

void TransportCatalogue::AddBus(const std::string_view name, const std::vector<const Stop*>& stops, bool is_round_trip) {
    string_storage_.emplace_back(name);
    auto& stored_name = string_storage_.back();
    buses_.emplace_back(Bus{stored_name, stops, is_round_trip});
    buses_map_[stored_name] = &buses_.back();

    for (const auto* stop : stops) {
        stop_to_buses_map_[stop->name].insert(stored_name);
    }
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    auto it = buses_map_.find(name);
    return it != buses_map_.end() ? it->second : nullptr;
}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    auto it = stops_map_.find(name);
    return it != stops_map_.end() ? it->second : nullptr;
}

BusInfo TransportCatalogue::GetBusInfo(std::string_view name) const {
    for (const auto& bus : buses_) {
        if (bus.name == name) {
            BusInfo bus_info;

            if (bus.is_round_trip) {
                bus_info.stop_count = static_cast<int>(bus.stops.size());
            } else {
                bus_info.stop_count = static_cast<int>((bus.stops.size() * 2) - 1);
            }

            std::unordered_set<std::string> unique_stops;
            for (const auto& stop : bus.stops) {
                unique_stops.insert(stop->name);
            }
            
            bus_info.unique_stop_count = static_cast<int>(unique_stops.size());

            double length = 0.0;
            if (bus.is_round_trip) {
                for (size_t i = 0; i < bus.stops.size(); ++i) {
                    const Stop* from = bus.stops[i];
                    const Stop* to = bus.stops[(i + 1) % bus.stops.size()];
                    length += geo::ComputeDistance(from->coordinates, to->coordinates);
                }
            } else {
                for (size_t i = 0; i < bus.stops.size() - 1; ++i) {
                    const Stop* from = bus.stops[i];
                    const Stop* to = bus.stops[i + 1];
                    length += geo::ComputeDistance(from->coordinates, to->coordinates);
                }
                length *= 2;
            }
            bus_info.route_length = length;

            return bus_info;
        }
    }
    return {0, 0, 0.0};
}

const std::set<std::string_view>& TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
    auto it = stop_to_buses_map_.find(stop_name);
    if (it != stop_to_buses_map_.end()) {
        return it->second;
    }
    
    static const std::set<std::string_view> empty_set;
    return empty_set;
}

} 
