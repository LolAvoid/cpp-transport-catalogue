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
    std::vector<const Stop*> stops; // Используем вектор для хранения указателей на остановки
    bool is_round_trip;

    const std::string& GetName() const {
        return name; // Добавим метод для доступа к имени автобуса
    }

    bool operator==(const Bus& other) const {
        return this->name == other.name;
    }
};

struct BusInfo {
    int stop_count;            // Количество остановок
    int unique_stop_count;     // Количество уникальных остановок
    double route_length;       // Дистанция маршрута
    double curvature;          // Извилистость маршрута
    int request_id;            // ID запроса
};

struct CustomHash {
    // Хеш-функция для Bus
    std::size_t operator()(const Bus* bus) const {
        return std::hash<std::string>()(bus->GetName());
    }

    // Хеш-функция для Stop
    std::size_t operator()(const Stop* stop) const {
        return std::hash<const Stop*>()(stop);
    }

    // Хеш-функция для std::string
    std::size_t operator()(const std::string& str) const {
        return std::hash<std::string>()(str);
    }

    // Хеш-функция для std::string_view
    std::size_t operator()(std::string_view str) const {
        return std::hash<std::string_view>()(str);
    }

    // Хеш-функция для пар остановок
    std::size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const {
        auto hash1 = std::hash<const Stop*>()(stops.first);
        auto hash2 = std::hash<const Stop*>()(stops.second);
        return hash1 ^ (hash2 << 1); // Комбинируем хеши
    }
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, const geo::Coordinates& coordinates);
    void AddBus(const std::string& name, const std::vector<const Stop*>& stops, bool is_round_trip);
    void SetDistance(const Stop* from, const Stop* to, int distance);
    const Bus* FindBus(const std::string& name) const;
    const Stop* FindStop(const std::string& name) const;
    BusInfo GetBusInfo(const std::string& name, int request_id) const;
    const std::unordered_set<const Bus*, CustomHash>& GetBusesByStop(const std::string& stop_name) const;
    const std::deque<Stop>& GetAllStops() const;
    const std::deque<Bus>& GetAllBuses() const;

private:
    std::deque<Stop> stops_; // Дек объектов Stop
    std::deque<Bus> buses_;   // Дек объектов Bus
    std::unordered_map<std::string, const Stop*> stops_map_;
    std::unordered_map<std::string, const Bus*> buses_map_;
    std::unordered_map<const Stop*, std::unordered_set<const Bus*, CustomHash>, CustomHash> stop_to_buses_map_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, CustomHash> between_stops_distance_;
};

} // namespace transport_catalogue