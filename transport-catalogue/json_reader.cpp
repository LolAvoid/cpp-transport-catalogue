#include "json_reader.h"
#include "domain.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <iomanip>

namespace json_reader {

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void JsonReader::ProcessStopRequest(const json::Node& request_map) {
    if (request_map.find("name") == request_map.end() ||
        request_map.find("latitude") == request_map.end() ||
        request_map.find("longitude") == request_map.end() ||
        request_map.find("road_distances") == request_map.end()) {
        std::cerr << "Error: Missing required fields in Stop request\n";
        return;
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

void JsonReader::ProcessBusRequest(const json::Node& request_map) {
    if (request_map.find("name") == request_map.end() ||
        request_map.find("is_roundtrip") == request_map.end() ||
        request_map.find("stops") == request_map.end()) {
        std::cerr << "Error: Missing required fields in Bus request\n";
        return;
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

void JsonReader::LoadData(const json::Node& data) {
    if (!data.IsMap()) {
        std::cerr << "Error: JSON data is not a map\n";
        return;
    }

    if (data.AsMap().find("base_requests") == data.AsMap().end()) {
        std::cerr << "Error: 'base_requests' key not found in JSON data\n";
        return;
    }

    const auto& base_requests = data.AsMap().at("base_requests").AsArray();

    // First pass: process stops
    for (const auto& request : base_requests) {
        if (!request.IsMap()) continue;
        
        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) continue;
        
        if (request_map.at("type").AsString() == "Stop") {
            ProcessStopRequest(request_map);
        }
    }

    // Second pass: process buses
    for (const auto& request : base_requests) {
        if (!request.IsMap()) continue;
        
        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) continue;
        
        if (request_map.at("type").AsString() == "Bus") {
            ProcessBusRequest(request_map);
        }
    }
}

json::Node JsonReader::ProcessStopRequest(const json::Node& request, int id) const {
    std::string stop_name = request.AsMap().at("name").AsString();
    const auto* stop = catalogue_.FindStop(stop_name);

    if (!stop) {
        return json::Builder{}
            .StartDict()
                .Key("request_id").Value(id)
                .Key("error_message").Value("not found")
            .EndDict()
            .Build();
    }

    const auto& buses = catalogue_.GetBusesByStop(stop_name);
    std::set<std::string> bus_names;
    for (const auto* bus : buses) {
        bus_names.insert(bus->name);
    }

    std::vector<json::Node> buses_array;
    for (const auto& name : bus_names) {
        buses_array.push_back(json::Node(name));
    }

    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(id)
            .Key("buses").Value(buses_array)
        .EndDict()
        .Build();
}

json::Node JsonReader::ProcessBusRequest(const json::Node& request, int id) const {
    std::string bus_name = request.AsMap().at("name").AsString();
    const auto* bus = catalogue_.FindBus(bus_name);

    if (!bus) {
        return json::Builder{}
            .StartDict()
                .Key("request_id").Value(id)
                .Key("error_message").Value("not found")
            .EndDict()
            .Build();
    }

    auto bus_info = catalogue_.GetBusInfo(bus_name, id);

    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(bus_info.request_id)
            .Key("stop_count").Value(bus_info.stop_count)
            .Key("unique_stop_count").Value(bus_info.unique_stop_count)
            .Key("route_length").Value(bus_info.route_length)
            .Key("curvature").Value(bus_info.curvature)
        .EndDict()
        .Build();
}

json::Node JsonReader::ProcessMapRequest(const json::Node& request, int id, const json::Node& render_settings) const {
    std::ostringstream svg_stream;
    map_renderer::MapRenderer renderer(catalogue_, map_renderer::ParseRenderSettings(render_settings));
    renderer.Render(svg_stream);

    return json::Builder{}
        .StartDict()
            .Key("map").Value(svg_stream.str())
            .Key("request_id").Value(id)
        .EndDict()
        .Build();
}

json::Node JsonReader::ProcessRequests(const json::Node& requests, const json::Node& render_settings) {
    std::vector<json::Node> responses;

    if (!requests.IsArray()) {
        std::cerr << "Error: Requests data is not an array\n";
        return json::Node(responses);
    }

    for (const auto& request : requests.AsArray()) {
        if (!request.IsMap()) continue;
        
        const auto& request_map = request.AsMap();
        if (request_map.find("type") == request_map.end()) continue;
        
        std::string type = request_map.at("type").AsString();
        int id = request_map.at("id").AsInt();

        if (type == "Stop") {
            responses.push_back(ProcessStopRequest(request, id));
        } else if (type == "Bus") {
            responses.push_back(ProcessBusRequest(request, id));
        } else if (type == "Map") {
            responses.push_back(ProcessMapRequest(request, id, render_settings));
        }
    }

    return json::Node(responses);
}

} // namespace json_reader
