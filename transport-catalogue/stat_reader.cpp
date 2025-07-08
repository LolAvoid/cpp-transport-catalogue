#include "stat_reader.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>

namespace stat_reader {

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) 
    : catalogue_(catalogue) {}

void StatReader::ProcessQueries(std::istream& input, std::ostream& output) const {
    int query_count;
    input >> query_count;
    input.ignore();

    for (int i = 0; i < query_count; ++i) {
        std::string line;
        std::getline(input, line);
        ProcessQuery(line, output);
    }
}

void StatReader::ProcessQuery(const std::string& name, std::ostream& output) const {
    const double SHORT_ROUTE_LENGTH = 10000;
    const double MEDIUM_ROUTE_LENGTH = 100000;
    const double LONG_ROUTE_LENGTH = 1000000;

    if (name.substr(0, 4) == "Bus ") {
        std::string bus_name = name.substr(4);
        const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

        if (!bus) {
            output << name << ": not found\n";
            return;
        }
        
        transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name);
        
        output << "Bus " << bus_name << ": " 
               << bus_info.stop_count << " stops on route, "
               << bus_info.unique_stop_count << " unique stops, ";

        if (bus_info.route_length < SHORT_ROUTE_LENGTH) {
            output << std::fixed << std::setprecision(2) << bus_info.route_length;
        } else if (bus_info.route_length < MEDIUM_ROUTE_LENGTH) {
            output << std::fixed << std::setprecision(1) << bus_info.route_length;
        } else if (bus_info.route_length < LONG_ROUTE_LENGTH) {
            output << std::fixed << std::setprecision(0) << bus_info.route_length;
        } else {
            output << std::scientific << std::setprecision(5) << bus_info.route_length;
        }
        output << " route length\n";
    } else if (name.substr(0, 5) == "Stop ") {
        std::string stop_name = name.substr(5);
        const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

        if (!stop) {
            output << "Stop " << stop_name << ": not found\n";
            return;
        }

        const auto& buses = catalogue_.GetBusesByStop(stop_name);
        if (buses.empty()) {
            output << "Stop " << stop_name << ": no buses\n";
        } else {
            output << "Stop " << stop_name << ": buses ";
            bool first = true;
            for (const auto& bus : buses) {
                if (!first) {
                    output << " ";
                }
                output << bus;
                first = false;
            }
            output << "\n";
        }
    } else {
        output << name << ": not a valid query\n";
    }
}

} // namespace stat_reader
