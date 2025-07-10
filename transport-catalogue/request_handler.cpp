#include "request_handler.h"
#include <algorithm>
#include <iostream>
#include <iomanip> // Для std::setprecision
#include <vector>

namespace stat_reader {

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const json::Node& query) const {
    const auto& query_map = query.AsMap();
    const std::string& type = query_map.at("type").AsString();
    int request_id = query_map.at("id").AsInt();

    if (type == "Bus") {
        const std::string& bus_name = query_map.at("name").AsString();
        const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

        if (!bus) {
            json::Dict response;
            response["request_id"] = json::Node(request_id);
            response["error_message"] = json::Node("not found");
            json::Print(json::Node(response), std::cout, 0);
            std::cout << "\n";
            return;
        }

        transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name, request_id);

        json::Dict response;
        response["request_id"] = json::Node(bus_info.request_id);
        response["curvature"] = json::Node(bus_info.curvature);
        response["route_length"] = json::Node(bus_info.route_length);
        response["stop_count"] = json::Node(bus_info.stop_count);
        response["unique_stop_count"] = json::Node(bus_info.unique_stop_count);

        json::Print(json::Node(response), std::cout, 0);
        std::cout << "\n";
    } else if (type == "Stop") {
        const std::string& stop_name = query_map.at("name").AsString();
        const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

        if (!stop) {
            json::Dict response;
            response["request_id"] = json::Node(request_id);
            response["error_message"] = json::Node("not found");
            json::Print(json::Node(response), std::cout, 0);
            std::cout << "\n";
            return;
        }

        const auto& buses = catalogue_.GetBusesByStop(stop_name);
        json::Array buses_array;
        for (const transport_catalogue::Bus* bus : buses) {
            buses_array.push_back(json::Node(bus->name));
        }

        json::Dict response;
        response["request_id"] = json::Node(request_id);
        response["buses"] = json::Node(buses_array);

        json::Print(json::Node(response), std::cout, 0);
        std::cout << "\n";
    } else {
        json::Dict response;
        response["request_id"] = json::Node(request_id);
        response["error_message"] = json::Node("invalid query type");
        json::Print(json::Node(response), std::cout, 0);
        std::cout << "\n";
    }
}

} // namespace stat_reader