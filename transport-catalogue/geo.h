#pragma once

namespace geo {

struct Coordinates {
    double lat; // Широта
    double lng; // Долгота
};

double ComputeDistance(const Coordinates from, const Coordinates to);

}  // namespace geo