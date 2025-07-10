#pragma once

#include <vector>
#include <string>
#include <map>
#include <variant>
#include <array>
#include <optional>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <cmath>
#include <algorithm>
#include "svg.h"
#include "geo.h"
#include "json_reader.h"
#include "json.h"
#include "transport_catalogue.h"

namespace map_renderer {

// Определение типа Color
using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

// Функция для преобразования map_renderer::Color в svg::Color
svg::Color ConvertColor(const Color& color);

// Функция для парсинга цвета из JSON
Color ParseColor(const json::Node& color_node);

// Структура для хранения настроек визуализации
struct RenderSettings {
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    int bus_label_font_size;
    std::array<double, 2> bus_label_offset;
    int stop_label_font_size;
    std::array<double, 2> stop_label_offset;
    Color underlayer_color;
    double underlayer_width;
    std::vector<Color> color_palette;
    bool render_stops; // Флаг для отрисовки остановок
};

// Класс для проекции географических координат на плоскость
class SphereProjector {
public:
    SphereProjector(const std::vector<geo::Coordinates>& points, double max_width, double max_height, double padding);

    svg::Point operator()(geo::Coordinates coords) const;

private:
    double max_width_;
    double max_height_;
    double padding_;
    double min_lon_;
    double max_lat_;
    double zoom_coef_;
};

// Основной класс для отрисовки карты
class MapRenderer {
public:
    MapRenderer(const transport_catalogue::TransportCatalogue& catalogue, const RenderSettings& settings);

    void Render(std::ostream& out) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    RenderSettings settings_;

    void DrawRoutes(svg::Document& doc, const SphereProjector& projector) const;
    void DrawRoute(svg::Document& doc, const transport_catalogue::Bus& bus, const SphereProjector& projector, const Color& color) const;
    void DrawStops(svg::Document& doc, const SphereProjector& projector) const;
    void DrawStop(svg::Document& doc, const transport_catalogue::Stop& stop, const SphereProjector& projector) const;
    void DrawStopCircles(svg::Document& doc, const SphereProjector& projector) const;
    void DrawStopLabels(svg::Document& doc, const SphereProjector& projector) const;
    void DrawRouteLabels(svg::Document& doc, const SphereProjector& projector) const;
    void DrawRouteLabel(svg::Document& doc, const SphereProjector& projector, geo::Coordinates coords, const std::string& name, const Color& color) const;

    Color GetRouteColor(size_t index) const;
};

// Функция для парсинга настроек визуализации из JSON
RenderSettings ParseRenderSettings(const json::Node& render_settings_node);

}  // namespace map_renderer