#pragma once

namespace geo {

// Структура для хранения географических координат
struct Coordinates {
    double lat;  // Широта в градусах
    double lng;  // Долгота в градусах
    
    // Операторы сравнения для удобства тестирования
    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

// Вычисляет расстояние между двумя точками на Земле (в метрах)
double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
