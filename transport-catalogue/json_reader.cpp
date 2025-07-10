#include "json_reader.h"
#include "domain.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <iomanip> // Для std::setprecision

namespace json_reader {

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void JsonReader::LoadData(const json::Node& data) {
    if (!data.IsMap()) {
        std::cerr << "Error: JSON data is not a map\n";
        return;
    }

    // Проверяем наличие ключа "base_requests"
    if (data.AsMap().find("base_requests") == data.AsMap().end()) {
        std::cerr << "Error: 'base_requests' key not found in JSON data\n";
        return;
    }

    const auto& base_requests = data.AsMap().at("base_requests").AsArray();

    // Первый проход: добавляем только остановки (Stop)
    for (size_t i = 0; i < base_requests.size(); ++i) {
        const auto& request = base_requests[i];
        if (!request.IsMap()) {
            std::cerr << "Error: Request is not a map\n";
            continue;
        }

        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) {
            std::cerr << "Error: 'type' key not found in request\n";
            continue;
        }

        std::string type = request_map.at("type").AsString();
        if (type == "Stop") {
            if (request_map.find("name") == request_map.end() ||
                request_map.find("latitude") == request_map.end() ||
                request_map.find("longitude") == request_map.end() ||
                request_map.find("road_distances") == request_map.end()) {
                std::cerr << "Error: Missing required fields in Stop request\n";
                continue;
            }

            std::string stop_name = request_map.at("name").AsString();
            double lat = request_map.at("latitude").AsDouble();
            double lng = request_map.at("longitude").AsDouble();
            catalogue_.AddStop(stop_name, {lat, lng});

            const auto& road_distances = request_map.at("road_distances").AsMap();
            for (const auto& [neighbor_stop_name, distance] : road_distances) {
                if (!distance.IsInt()) {
                    std::cerr << "Error: Distance is not an integer\n";
                    continue;
                }

                int dist = distance.AsInt();
                const auto* from_stop = catalogue_.FindStop(stop_name);
                const auto* to_stop = catalogue_.FindStop(neighbor_stop_name);

                // Если вторая остановка не найдена, добавляем её с нулевыми координатами
                if (to_stop == nullptr) {
                    catalogue_.AddStop(neighbor_stop_name, {0.0, 0.0});
                    to_stop = catalogue_.FindStop(neighbor_stop_name);
                }

                if (from_stop && to_stop) {
                    catalogue_.SetDistance(from_stop, to_stop, dist);
                } else {
                    std::cerr << "Error: One or both stops not found\n";
                }
            }
        }
    }

    // Второй проход: добавляем автобусы (Bus)
    for (size_t i = 0; i < base_requests.size(); ++i) {
        const auto& request = base_requests[i];
        if (!request.IsMap()) {
            std::cerr << "Error: Request is not a map\n";
            continue;
        }

        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) {
            std::cerr << "Error: 'type' key not found in request\n";
            continue;
        }

        std::string type = request_map.at("type").AsString();
        if (type == "Bus") {
            if (request_map.find("name") == request_map.end() ||
                request_map.find("is_roundtrip") == request_map.end() ||
                request_map.find("stops") == request_map.end()) {
                std::cerr << "Error: Missing required fields in Bus request\n";
                continue;
            }

            std::string bus_name = request_map.at("name").AsString();
            bool is_roundtrip = request_map.at("is_roundtrip").AsBool();
            const auto& stops_array = request_map.at("stops").AsArray();

            std::vector<const transport_catalogue::Stop*> stops;
            for (size_t j = 0; j < stops_array.size(); ++j) {
                if (!stops_array[j].IsString()) {
                    std::cerr << "Error: Stop name is not a string\n";
                    continue;
                }

                const auto* stop = catalogue_.FindStop(stops_array[j].AsString());
                if (stop) {
                    stops.push_back(stop);
                } else {
                    std::cerr << "Error: Stop not found: " << stops_array[j].AsString() << "\n";
                }
            }

            catalogue_.AddBus(bus_name, stops, is_roundtrip);
        }
    }
}

json::Node JsonReader::ProcessRequests(const json::Node& requests, const json::Node& render_settings) {
    std::vector<json::Node> responses;

    if (!requests.IsArray()) {
        std::cerr << "Error: Requests data is not an array\n";
        return json::Node(responses);
    }

    const auto& requests_array = requests.AsArray();

    // Обрабатываем запросы "Stop" и "Bus", "Map"
    for (size_t i = 0; i < requests_array.size(); ++i) {
        const auto& request = requests_array[i];
        if (!request.IsMap()) {
            std::cerr << "Error: Request is not a map\n";
            continue;
        }

        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) {
            std::cerr << "Error: 'type' key not found in request\n";
            continue;
        }

        std::string type = request_map.at("type").AsString();
        int id = request_map.at("id").AsInt();

        if (type == "Stop") {
            std::string stop_name = request_map.at("name").AsString();
            const auto* stop = catalogue_.FindStop(stop_name);

            if (!stop) {
                json::Dict response;
                response["request_id"] = json::Node(id);
                response["error_message"] = json::Node("not found");
                responses.push_back(json::Node(response));
            } else {
                const auto& buses = catalogue_.GetBusesByStop(stop_name);
                std::set<std::string> bus_names; // Используем set для автоматической сортировки

                for (const transport_catalogue::Bus* bus : buses) {
                    bus_names.insert(bus->name); // Добавляем имена автобусов в set
                }

                // Преобразуем отсортированные имена обратно в std::vector<json::Node>
                std::vector<json::Node> buses_array;
                for (const std::string& name : bus_names) {
                    buses_array.push_back(json::Node(name));
                }

                json::Dict response;
                response["request_id"] = json::Node(id);
                response["buses"] = json::Node(buses_array);
                responses.push_back(json::Node(response));
            }

        } else if (type == "Bus") {
            std::string bus_name = request_map.at("name").AsString();
            const auto* bus = catalogue_.FindBus(bus_name);

            if (!bus) {
                json::Dict response;
                response["request_id"] = json::Node(id);
                response["error_message"] = json::Node("not found");
                responses.push_back(json::Node(response));
            } else {
                auto bus_info = catalogue_.GetBusInfo(bus_name, id);

                json::Dict response;
                response["request_id"] = json::Node(bus_info.request_id);
                response["stop_count"] = json::Node(bus_info.stop_count);
                response["unique_stop_count"] = json::Node(bus_info.unique_stop_count);
                response["route_length"] = json::Node(bus_info.route_length);
                response["curvature"] = json::Node(bus_info.curvature);
                responses.push_back(json::Node(response));
            }
            
        } else if (type == "Map") {
            // Генерация SVG-изображения
            std::ostringstream svg_stream;
            map_renderer::MapRenderer renderer(catalogue_, map_renderer::ParseRenderSettings(render_settings));
            renderer.Render(svg_stream);
            std::string svg_content = svg_stream.str();

            // Формирование ответа
            json::Dict response;
            response["map"] = json::Node(svg_content);
            response["request_id"] = json::Node(id);
            responses.push_back(json::Node(response));
        }
    }
    return json::Node(responses);
}

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const json::Node& query) const {
    const auto& query_map = query.AsMap();
    const std::string& type = query_map.at("type").AsString();
    int request_id = query_map.at("id").AsInt();

    json::Dict response;

    if (type == "Bus") {
        const std::string& bus_name = query_map.at("name").AsString();
        response = ProcessBusQuery(bus_name, request_id);
    } else if (type == "Stop") {
        const std::string& stop_name = query_map.at("name").AsString();
        response = ProcessStopQuery(stop_name, request_id);
    } else if (type == "Map") {
        response = ProcessMapQuery(request_id);
    } else {
        response = {
            {"request_id", json::Node(request_id)},
            {"error_message", json::Node("invalid query type")}
        };
    }

    json::Print(json::Node(response), std::cout, 0);
    std::cout << "\n";
}

json::Dict StatReader::ProcessBusQuery(const std::string& bus_name, int request_id) const {
    const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

    if (!bus) {
        return {
            {"request_id", json::Node(request_id)},
            {"error_message", json::Node("not found")}
        };
    }

    transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name, request_id);

    return {
        {"request_id", json::Node(bus_info.request_id)},
        {"curvature", json::Node(bus_info.curvature)},
        {"route_length", json::Node(bus_info.route_length)},
        {"stop_count", json::Node(bus_info.stop_count)},
        {"unique_stop_count", json::Node(bus_info.unique_stop_count)}
    };
}

json::Dict StatReader::ProcessStopQuery(const std::string& stop_name, int request_id) const {
    const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

    if (!stop) {
        return {
            {"request_id", json::Node(request_id)},
            {"error_message", json::Node("not found")}
        };
    }

    const auto& buses = catalogue_.GetBusesByStop(stop_name);
    std::vector<json::Node> buses_array;
    for (const transport_catalogue::Bus* bus : buses) {
        buses_array.push_back(json::Node(bus->name));
    }

    return {
        {"request_id", json::Node(request_id)},
        {"buses", json::Node(buses_array)}
    };
}

json::Dict StatReader::ProcessMapQuery(int request_id) const {
    return {
        {"request_id", json::Node(request_id)},
        {"error_message", json::Node("map rendering not implemented")}
    };
}

} // namespace json_reader