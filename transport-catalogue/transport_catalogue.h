#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <vector>
#include <functional> 
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
    int stop_count;            
    int unique_stop_count;     
    double route_length;       
};

class TransportCatalogue {
public:
    void AddStop(const std::string_view name, const geo::Coordinates& coordinates);
    void AddBus(const std::string_view name, const std::vector<const Stop*>& stops, bool is_round_trip);
    const Bus* FindBus(std::string_view name) const;
    const Stop* FindStop(std::string_view name) const;
    BusInfo GetBusInfo(std::string_view name) const;
    const std::unordered_set<std::string_view>& GetBusesByStop(std::string_view stop_name) const;

private:
    std::deque<std::string> string_storage_; 
    std::deque<Stop> stops_; 
    std::deque<Bus> buses_;   
    std::unordered_map<std::string_view, const Stop*> stops_map_; 
    std::unordered_map<std::string_view, const Bus*> buses_map_; 
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_map_; 
};

} 
