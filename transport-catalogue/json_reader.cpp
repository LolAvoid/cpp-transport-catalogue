#include "json_reader.h"
#include "domain.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <iomanip>

namespace json_reader {

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue) 
    : catalogue_(catalogue) {}

void JsonReader::LoadData(const json::Node& data) {
    if (!ValidateBaseData(data)) {
        return;
    }

    const auto& base_requests = data.AsMap().at("base_requests").AsArray();
    ProcessStops(base_requests);
    ProcessBuses(base_requests);
}

bool JsonReader::ValidateBaseData(const json::Node& data) const {
    if (!data.IsMap()) {
        std::cerr << "Error: JSON data is not a map\n";
        return false;
    }

    if (data.AsMap().find("base_requests") == data.AsMap().end()) {
        std::cerr << "Error: 'base_requests' key not found in JSON data\n";
        return false;
    }
    return true;
}

void JsonReader::ProcessStops(const std::vector<json::Node>& requests) {
    for (const auto& request : requests) {
        if (!IsValidRequest(request, "Stop")) {
            continue;
        }

        const auto& request_map = request.AsMap();
        AddStopToCatalogue(request_map);
        ProcessStopDistances(request_map);
    }
}

void JsonReader::ProcessBuses(const std::vector<json::Node>& requests) {
    for (const auto& request : requests) {
        if (!IsValidRequest(request, "Bus")) {
            continue;
        }

        const auto& request_map = request.AsMap();
        AddBusToCatalogue(request_map);
    }
}

bool JsonReader::IsValidRequest(const json::Node& request, const std::string& expected_type) const {
    if (!request.IsMap()) {
        std::cerr << "Error: Request is not a map\n";
        return false;
    }

    const auto& request_map = request.AsMap();
    if (request_map.find("type") == request_map.end()) {
        std::cerr << "Error: 'type' key not found in request\n";
        return false;
    }

    if (request_map.at("type").AsString() != expected_type) {
        return false;
    }

    return true;
}

void JsonReader::AddStopToCatalogue(const json::Dict& stop_info) {
    std::string stop_name = stop_info.at("name").AsString();
    double lat = stop_info.at("latitude").AsDouble();
    double lng = stop_info.at("longitude").AsDouble();
    catalogue_.AddStop(stop_name, {lat, lng});
}

void JsonReader::ProcessStopDistances(const json::Dict& stop_info) {
    std::string stop_name = stop_info.at("name").AsString();
    const auto& road_distances = stop_info.at("road_distances").AsMap();

    for (const auto& [neighbor_stop_name, distance] : road_distances) {
        if (!distance.IsInt()) {
            std::cerr << "Error: Distance is not an integer\n";
            continue;
        }

        int dist = distance.AsInt();
        const auto* from_stop = catalogue_.FindStop(stop_name);
        const auto* to_stop = catalogue_.FindStop(neighbor_stop_name);

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

void JsonReader::AddBusToCatalogue(const json::Dict& bus_info) {
    std::string bus_name = bus_info.at("name").AsString();
    bool is_roundtrip = bus_info.at("is_roundtrip").AsBool();
    const auto& stops_array = bus_info.at("stops").AsArray();

    std::vector<const transport_catalogue::Stop*> stops;
    for (const auto& stop_node : stops_array) {
        if (!stop_node.IsString()) {
            std::cerr << "Error: Stop name is not a string\n";
            continue;
        }

        const auto* stop = catalogue_.FindStop(stop_node.AsString());
        if (stop) {
            stops.push_back(stop);
        } else {
            std::cerr << "Error: Stop not found: " << stop_node.AsString() << "\n";
        }
    }

    catalogue_.AddBus(bus_name, stops, is_roundtrip);
}

json::Node JsonReader::ProcessRequests(const json::Node& requests, const json::Node& render_settings) {
    std::vector<json::Node> responses;

    if (!requests.IsArray()) {
        std::cerr << "Error: Requests data is not an array\n";
        return json::Node(responses);
    }

    for (const auto& request : requests.AsArray()) {
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
            responses.push_back(ProcessStopRequest(request_map, id));
        } else if (type == "Bus") {
            responses.push_back(ProcessBusRequest(request_map, id));
        } else if (type == "Map") {
            responses.push_back(ProcessMapRequest(id, render_settings));
        }
    }
    return json::Node(responses);
}

json::Node JsonReader::ProcessStopRequest(const json::Dict& request, int id) {
    std::string stop_name = request.at("name").AsString();
    const auto* stop = catalogue_.FindStop(stop_name);

    if (!stop) {
        return json::Node(json::Dict{
            {"request_id", json::Node(id)},
            {"error_message", json::Node("not found")}
        });
    }

    const auto& buses = catalogue_.GetBusesByStop(stop_name);
    std::set<std::string> bus_names;
    for (const transport_catalogue::Bus* bus : buses) {
        bus_names.insert(bus->name);
    }

    std::vector<json::Node> buses_array;
    for (const std::string& name : bus_names) {
        buses_array.push_back(json::Node(name));
    }

    return json::Node(json::Dict{
        {"request_id", json::Node(id)},
        {"buses", json::Node(buses_array)}
    });
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request, int id) {
    std::string bus_name = request.at("name").AsString();
    const auto* bus = catalogue_.FindBus(bus_name);

    if (!bus) {
        return json::Node(json::Dict{
            {"request_id", json::Node(id)},
            {"error_message", json::Node("not found")}
        });
    }

    auto bus_info = catalogue_.GetBusInfo(bus_name, id);
    return json::Node(json::Dict{
        {"request_id", json::Node(bus_info.request_id)},
        {"stop_count", json::Node(bus_info.stop_count)},
        {"unique_stop_count", json::Node(bus_info.unique_stop_count)},
        {"route_length", json::Node(bus_info.route_length)},
        {"curvature", json::Node(bus_info.curvature)}
    });
}

json::Node JsonReader::ProcessMapRequest(int id, const json::Node& render_settings) {
    std::ostringstream svg_stream;
    map_renderer::MapRenderer renderer(catalogue_, map_renderer::ParseRenderSettings(render_settings));
    renderer.Render(svg_stream);

    return json::Node(json::Dict{
        {"map", json::Node(svg_stream.str())},
        {"request_id", json::Node(id)}
    });
}

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) 
    : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const json::Node& query) const {
    const auto& query_map = query.AsMap();
    const std::string& type = query_map.at("type").AsString();
    int request_id = query_map.at("id").AsInt();

    json::Dict response;
    if (type == "Bus") {
        response = ProcessBusQuery(query_map.at("name").AsString(), request_id);
    } else if (type == "Stop") {
        response = ProcessStopQuery(query_map.at("name").AsString(), request_id);
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
