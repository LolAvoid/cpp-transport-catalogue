#include "stat_reader.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>

namespace stat_reader {

StatReader::StatReader(const transport_catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void StatReader::ProcessQuery(const std::string& name) const {
    if (name.substr(0, 4) == "Bus ") {
        std::string bus_name = name.substr(4);
        const transport_catalogue::Bus* bus = catalogue_.FindBus(bus_name);

        if (!bus) {
            std::cout << name << ": not found\n"; 
            return;
        }
        
        transport_catalogue::BusInfo bus_info = catalogue_.GetBusInfo(bus_name); 
        
        int R = bus_info.stop_count; 
        int U = bus_info.unique_stop_count; 
        double length = bus_info.route_length; 

        std::cout << "Bus " << bus_name << ": " << R << " stops on route, " << U << " unique stops, ";

        if (length < 10000) {
            std::cout << std::fixed << std::setprecision(2) << length;
        } else if (length < 100000) {
            std::cout << std::fixed << std::setprecision(1) << length;
        } else if (length < 1000000) {
            std::cout << std::fixed << std::setprecision(0) << length; 
        } else {
            std::cout << std::scientific << std::setprecision(5) << length; 
        }
        std::cout << " route length\n";

    } else if (name.substr(0, 5) == "Stop ") {
        std::string stop_name = name.substr(5);
        
        const transport_catalogue::Stop* stop = catalogue_.FindStop(stop_name);

        if (!stop) {
            std::cout << "Stop " << stop_name << ": not found\n"; 
            return;
        }

        const auto& buses = catalogue_.GetBusesByStop(stop_name);

    if (buses.empty()) {
        std::cout << "Stop " << stop_name << ": no buses\n";
    } else {
        std::vector<std::string> sorted_buses(buses.begin(), buses.end()); 
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

} 
