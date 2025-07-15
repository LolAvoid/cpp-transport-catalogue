#include "json_reader.h"
#include "domain.h"
#include "json_builder.h"
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
            ProcessStopRequest(request_map);
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
            ProcessBusRequest(request_map);
        }
    }
}

void JsonReader::ProcessStopRequest(const json::Dict& request_map) {
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

void JsonReader::ProcessBusRequest(const json::Dict& request_map) {
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

json::Node JsonReader::ProcessRequests(const json::Node& requests, const json::Node& render_settings) {
    json::Builder builder;
    builder.StartArray();  // Начинаем массив ответов

    if (!requests.IsArray()) {
        std::cerr << "Error: Requests data is not an array\n";
        return builder.Build();
    }

    const auto& requests_array = requests.AsArray();

    // Обрабатываем запросы "Stop", "Bus" и "Map"
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
            ProcessStopResponse(builder, request_map, id);
        } else if (type == "Bus") {
            ProcessBusResponse(builder, request_map, id);
        } else if (type == "Map") {
            ProcessMapResponse(builder, id, render_settings);
        }
    }

    builder.EndArray();  // Завершаем массив ответов
    return builder.Build();  // Возвращаем построенный JSON
}

void JsonReader::ProcessStopResponse(json::Builder& builder, const json::Dict& request_map, int id) {
    std::string stop_name = request_map.at("name").AsString();
    const auto* stop = catalogue_.FindStop(stop_name);

    if (!stop) {
        builder.StartDict()
            .Key("request_id").Value(id)
            .Key("error_message").Value("not found")
            .EndDict();
    } else {
        const auto& buses = catalogue_.GetBusesByStop(stop_name);
        std::set<std::string> bus_names; // Используем set для автоматической сортировки

        for (const transport_catalogue::Bus* bus : buses) {
            bus_names.insert(bus->name); // Добавляем имена автобусов в set
        }

        builder.StartDict()
            .Key("request_id").Value(id)
            .Key("buses").StartArray();
        for (const std::string& name : bus_names) {
            builder.Value(name);
        }
        builder.EndArray().EndDict();
    }
}

void JsonReader::ProcessBusResponse(json::Builder& builder, const json::Dict& request_map, int id) {
    std::string bus_name = request_map.at("name").AsString();
    const auto* bus = catalogue_.FindBus(bus_name);

    if (!bus) {
        builder.StartDict()
            .Key("request_id").Value(id)
            .Key("error_message").Value("not found")
            .EndDict();
    } else {
        auto bus_info = catalogue_.GetBusInfo(bus_name, id);

        builder.StartDict()
            .Key("request_id").Value(bus_info.request_id)
            .Key("stop_count").Value(bus_info.stop_count)
            .Key("unique_stop_count").Value(bus_info.unique_stop_count)
            .Key("route_length").Value(bus_info.route_length)
            .Key("curvature").Value(bus_info.curvature)
            .EndDict();
    }
}

void JsonReader::ProcessMapResponse(json::Builder& builder, int id, const json::Node& render_settings) {
    // Генерация SVG-изображения
    std::ostringstream svg_stream;
    map_renderer::MapRenderer renderer(catalogue_, map_renderer::ParseRenderSettings(render_settings));
    renderer.Render(svg_stream);
    std::string svg_content = svg_stream.str();

    // Формирование ответа
    builder.StartDict()
        .Key("request_id").Value(id)
        .Key("map").Value(svg_content)
        .EndDict();
}

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const json::Node& query) const {
    const auto& query_map = query.AsMap();
    const std::string& type = query_map.at("type").AsString();
    int request_id = query_map.at("id").AsInt();

    json::Builder builder;

    if (type == "Bus") {
        ProcessBusQuery(builder, query_map, request_id);
    } else if (type == "Stop") {
        ProcessStopQuery(builder, query_map, request_id);
    } else if (type == "Map") {
        ProcessMapQuery(builder, request_id);
    } else {
        builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value("invalid query type")
            .EndDict();
    }

    json::Print(builder.Build(), std::cout, 0);
    std::cout << "\n";
}

void StatReader::ProcessBusQuery(json::Builder& builder, const json::Dict& query_map, int request_id) const {
    const std::string& bus_name = query_map.at("name").AsString();
    const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

    if (!bus) {
        builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value("not found")
            .EndDict();
    } else {
        transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name, request_id);

        builder.StartDict()
            .Key("request_id").Value(bus_info.request_id)
            .Key("curvature").Value(bus_info.curvature)
            .Key("route_length").Value(bus_info.route_length)
            .Key("stop_count").Value(bus_info.stop_count)
            .Key("unique_stop_count").Value(bus_info.unique_stop_count)
            .EndDict();
    }
}

void StatReader::ProcessStopQuery(json::Builder& builder, const json::Dict& query_map, int request_id) const {
    const std::string& stop_name = query_map.at("name").AsString();
    const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

    if (!stop) {
        builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("error_message").Value("not found")
            .EndDict();
    } else {
        const auto& buses = catalogue_.GetBusesByStop(stop_name);
        builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("buses").StartArray();
        for (const transport_catalogue::Bus* bus : buses) {
            builder.Value(bus->name);
        }
        builder.EndArray().EndDict();
    }
}

void StatReader::ProcessMapQuery(json::Builder& builder, int request_id) const {
    builder.StartDict()
        .Key("request_id").Value(request_id)
        .Key("error_message").Value("map rendering not implemented")
        .EndDict();
}

} // namespace json_reader