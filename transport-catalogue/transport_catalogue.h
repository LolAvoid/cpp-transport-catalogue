#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include "geo.h"

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_round_trip;
};

struct BusInfo {
    int stop_count = 0;
    int unique_stop_count = 0;
    double route_length = 0.0;
};

class TransportCatalogue {
public:
    void AddStop(std::string_view name, const geo::Coordinates& coordinates);
    void AddBus(std::string_view name, const std::vector<const Stop*>& stops, bool is_round_trip);
    
    const Bus* FindBus(std::string_view name) const;
    const Stop* FindStop(std::string_view name) const;
    
    BusInfo GetBusInfo(std::string_view name) const;
    const std::set<std::string_view>& GetBusesByStop(std::string_view stop_name) const;

private:
    std::vector<std::string> string_storage_;
    std::vector<Stop> stops_;
    std::vector<Bus> buses_;
    
    std::unordered_map<std::string_view, Stop*> stops_map_;
    std::unordered_map<std::string_view, Bus*> buses_map_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_map_;
};

} 
