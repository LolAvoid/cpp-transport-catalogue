#include "transport_catalogue.h"
#include <chrono>
#include <iostream>
#include <cmath> // Для std::round

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates& coordinates) {
    // Проверяем, существует ли уже запись в stops_map_
    auto it = stops_map_.find(name);
    if (it != stops_map_.end()) {
        // Если запись с таким именем уже существует, обновляем соответствующий объект Stop
        const Stop* existing_stop = it->second; // Получаем указатель на уже существующий объект Stop
        const_cast<Stop*>(existing_stop)->coordinates = coordinates;
    } else {
        // Если записи не существует, добавляем новый объект Stop и обновляем карту
        stops_.emplace_back(Stop{name, coordinates}); // Добавляем в дек
        stops_map_[name] = &stops_.back(); // Обновляем мапу, устанавливая ссылку на только что добавленный элемент
    }
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<const Stop*>& stops, bool is_round_trip) {
    buses_.emplace_back(Bus{name, stops, is_round_trip});
    buses_map_[name] = &buses_.back(); // Используем name

    for (const auto* stop : stops) {
        stop_to_buses_map_[stop].insert(&buses_.back()); // Используем указатели
    }
}
    
void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance) {
    if (from != nullptr && to != nullptr) {
        between_stops_distance_[{from, to}] = distance; // Добавляем расстояние в мапу
    }
    //else std::cout << "\n Error SetDistance \n";// Сюда попадать не должны
}

const Bus* TransportCatalogue::FindBus(const std::string& name) const {
    auto it = buses_map_.find(name);
    return it != buses_map_.end() ? it->second : nullptr;
}

const Stop* TransportCatalogue::FindStop(const std::string& name) const {
    auto it = stops_map_.find(name);
    return it != stops_map_.end() ? it->second : nullptr;
}

BusInfo TransportCatalogue::GetBusInfo(const std::string& name, int request_id) const {
    const Bus* bus = FindBus(name); // Используем FindBus для поиска автобуса
    if (bus) {
        BusInfo bus_info;
        bus_info.request_id = request_id; // Устанавливаем request_id
        // Количество остановок
        bus_info.stop_count = bus->is_round_trip ? static_cast<int>(bus->stops.size()) : static_cast<int>(bus->stops.size() * 2 - 1);

        // Подсчет уникальных остановок без копирования
        std::unordered_map<std::string, int> stop_counter;
        for (const auto& stop : bus->stops) {
            stop_counter[stop->name]++;
        }
        bus_info.unique_stop_count = static_cast<int>(stop_counter.size());

        // Длина маршрута
        double route_length = 0.0;
        double geographical_distance = 0.0; 

        // Вектор для хранения развернутого маршрута
        std::vector<const Stop*> expanded_stops= bus->stops;        
        if (!bus->is_round_trip) {
            // Добавляем остановки в обратном порядке
            for (size_t i = bus->stops.size()-1; i > 0; --i) {
            expanded_stops.push_back(bus->stops[i - 1]);
            }
        }
        
        // Теперь у нас есть развернутый маршрут в expanded_stops
        for (size_t i = 0; i < expanded_stops.size() - 1; i++) {
            const auto& from_stop = expanded_stops[i];
            const auto& to_stop = expanded_stops[i + 1];

            // Получаем расстояние от текущей остановки до следующей
            auto it = between_stops_distance_.find({from_stop, to_stop});
            auto reverse_it = between_stops_distance_.find({to_stop, from_stop});

            if (it != between_stops_distance_.end() && it->second > 0) {
                route_length += it->second; // Добавляем расстояние
            } else if (reverse_it != between_stops_distance_.end() && reverse_it->second > 0) {
                route_length += reverse_it->second; // Добавляем расстояние в обратном направлении
            }
            
            // Добавляем географическое расстояние
            geographical_distance += ComputeDistance(from_stop->coordinates, to_stop->coordinates);
        }

        // Устанавливаем общую длину маршрута
        bus_info.route_length = route_length;

        // Вычисляем коэффициент извилистости (C)
        if (geographical_distance > 0.0) {
            bus_info.curvature = route_length / geographical_distance; // C = L / D
        } else {
            bus_info.curvature = 1.0L; // Если географическое расстояние равно 0, устанавливаем извилистость в 1
        }
           
        return bus_info; // Возвращаем уникальный указатель
    }
    return BusInfo{}; // Если автобус не найден
}

const std::unordered_set<const Bus*, CustomHash>& TransportCatalogue::GetBusesByStop(const std::string& stop_name) const {
    auto stop_it = stops_map_.find(stop_name); // Используем stop_name
    if (stop_it != stops_map_.end()) {
        const Stop* stop = stop_it->second;
        auto bus_it = stop_to_buses_map_.find(stop);
        if (bus_it != stop_to_buses_map_.end()) {
            return bus_it->second;
        }
    }
    static const std::unordered_set<const Bus*, CustomHash> empty_set;
    return empty_set;
}

const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
    return stops_;
}

const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
    return buses_;
}

} // namespace transport_catalogue