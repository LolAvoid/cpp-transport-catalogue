#include "stat_reader.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>

namespace stat_reader {

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const std::string_view& name) const {
    if (name.substr(0, 4) == "Bus ") {
        std::string bus_name = std::string(name.substr(4));
        const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

        if (!bus) {
            std::cout << "Bus " << bus_name << ": not found\n"; 
            return;
        }
        
        transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name); 

        int R = bus_info.stop_count; 
        int U = bus_info.unique_stop_count; 
        double length = bus_info.route_length; 
        double curvature = bus_info.curvature; 

        std::cout << "Bus " << bus_name << ": " << R << " stops on route, " << U << " unique stops, ";

        if (length < 1000000) {
            std::cout << std::fixed << std::setprecision(0) << length;
        } else {
            std::cout << std::scientific << std::setprecision(5) << length; 
        }
        std::cout << " route length, " << std::fixed << std::setprecision(5) << curvature << " curvature\n";

    } else if (name.substr(0, 5) == "Stop ") {
        std::string stop_name = std::string(name.substr(5));
        
        const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

        if (!stop) {
            std::cout << "Stop " << stop_name << ": not found\n"; 
            return;
        }

        const auto& buses = catalogue_.GetBusesByStop(stop_name); 

        if (buses.empty()) {
            std::cout << "Stop " << stop_name << ": no buses\n";
        } else {
            std::vector<std::string> sorted_buses;

            for (const transport_catalogue::Bus* bus : buses) {
                sorted_buses.push_back(bus->name);
            }

            std::sort(sorted_buses.begin(), sorted_buses.end()); 
            std::cout << "Stop " << stop_name << ": buses ";
            for (size_t i = 0; i < sorted_buses.size(); ++i) {
                std::cout << sorted_buses[i];
                if (i < sorted_buses.size() - 1) {
                    std::cout << " ";
                }
            }
            std::cout << "\n";
        }
    } else {
        std::cout << name << ": not a valid query\n";
    }
}
} // namespace stat_reader
