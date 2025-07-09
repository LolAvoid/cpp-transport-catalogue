#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <utility>
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

    const std::string& GetName() const {
        return name; 
    }
};

struct BusInfo {
    int stop_count;            
    int unique_stop_count;     
    double route_length;      
    double curvature;          
};

struct CustomHash {
    std::size_t operator()(const Bus* bus) const {
        return std::hash<std::string>()(bus->GetName());
    }

    std::size_t operator()(const Stop* stop) const {
        return std::hash<const Stop*>()(stop);
    }

    std::size_t operator()(const std::string& str) const {
        return std::hash<std::string>()(str);
    }

    std::size_t operator()(std::string_view str) const {
        return std::hash<std::string_view>()(str);
    }

    std::size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const {
        auto hash1 = std::hash<const Stop*>()(stops.first);
        auto hash2 = std::hash<const Stop*>()(stops.second);
        return hash1 ^ (hash2 << 1); 
    }
};


class TransportCatalogue {
public:
    void AddStop(const std::string_view name, const geo::Coordinates& coordinates);
    void AddBus(const std::string_view name, const std::vector<const Stop*>& stops, bool is_round_trip);
    void SetDistance(const Stop* from, const Stop* to, int distance);
    const Bus* FindBus(std::string_view name) const;
    const Stop* FindStop(std::string_view name) const;
    BusInfo GetBusInfo(std::string_view name) const;
    const std::unordered_set<const Bus*, CustomHash>& GetBusesByStop(std::string_view stop_name) const;

private:
    std::deque<std::string> string_storage_; 
    std::deque<Stop> stops_; 
    std::deque<Bus> buses_;   
    std::unordered_map<std::string_view, const Stop*> stops_map_;
    std::unordered_map<std::string_view, const Bus*> buses_map_;
    std::unordered_map<const Stop*, std::unordered_set<const Bus*, CustomHash>, CustomHash> stop_to_buses_map_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, CustomHash> between_stops_distance_;
};

} // namespace transport_catalogue
