#include "map_renderer.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <iterator>

namespace map_renderer {

// Реализация SphereProjector
SphereProjector::SphereProjector(const std::vector<geo::Coordinates>& points, double max_width, double max_height, double padding)
    : max_width_(max_width), max_height_(max_height), padding_(padding) {
    if (points.empty()) {
        return;
    }

    auto [min_lon_it, max_lon_it] = std::minmax_element(points.begin(), points.end(),
        [](const geo::Coordinates& lhs, const geo::Coordinates& rhs) {
            return lhs.lng < rhs.lng;
        });
    auto [min_lat_it, max_lat_it] = std::minmax_element(points.begin(), points.end(),
        [](const geo::Coordinates& lhs, const geo::Coordinates& rhs) {
            return lhs.lat < rhs.lat;
        });

    min_lon_ = min_lon_it->lng;
    max_lat_ = max_lat_it->lat;

    double zoom_coef_width = (max_width - 2 * padding) / (max_lon_it->lng - min_lon_);
    double zoom_coef_height = (max_height - 2 * padding) / (max_lat_ - min_lat_it->lat);

    zoom_coef_ = std::min(zoom_coef_width, zoom_coef_height);
}

// Проекция географических координат на плоскость SVG
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    double x = (coords.lng - min_lon_) * zoom_coef_ + padding_;
    double y = (max_lat_ - coords.lat) * zoom_coef_ + padding_;
    return {x, y};
}

// Реализация MapRenderer
MapRenderer::MapRenderer(const transport_catalogue::TransportCatalogue& catalogue, const RenderSettings& settings)
    : catalogue_(catalogue), settings_(settings) {}

// Реализация MapRenderer
void MapRenderer::Render(std::ostream& out) const {
    svg::Document doc;

    // Получаем все маршруты из каталога
    const auto& all_buses = catalogue_.GetAllBuses();

    // Если маршрутов нет, ничего не рисуем
    if (all_buses.empty()) {
        doc.Render(out); // Выводим пустой SVG-документ
        return;
    }

    // Создаем множество остановок, которые принадлежат хотя бы одному маршруту
    std::unordered_set<std::string> stops_in_routes;
    for (const auto& bus : all_buses) {
        for (const auto& stop : bus.stops) {
            stops_in_routes.insert(stop->name);
        }
    }

    // Собираем координаты только тех остановок, которые принадлежат маршрутам
    std::vector<geo::Coordinates> all_coordinates;
    for (const auto& stop : catalogue_.GetAllStops()) {
        if (stops_in_routes.find(stop.name) != stops_in_routes.end()) {
            all_coordinates.push_back(stop.coordinates);
        }
    }

    // Создаем проектор для преобразования координат
    SphereProjector projector(all_coordinates, settings_.width, settings_.height, settings_.padding);

    // Отрисовываем ломаные линии маршрутов
    DrawRoutes(doc, projector);

    // Отрисовываем названия маршрутов
    DrawRouteLabels(doc, projector);

    // Отрисовываем круги остановок
    DrawStopCircles(doc, projector);

    // Отрисовываем названия остановок
    DrawStopLabels(doc, projector);

    // Выводим SVG-документ
    doc.Render(out);
}

// Отрисовка всех маршрутов
void MapRenderer::DrawRoutes(svg::Document& doc, const SphereProjector& projector) const {
    auto buses = catalogue_.GetAllBuses();
    std::sort(buses.begin(), buses.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name; // Сортировка по названию маршрута
    });

    size_t color_index = 0;
    for (const auto& bus : buses) {
        if (!bus.stops.empty()) {
            DrawRoute(doc, bus, projector, GetRouteColor(color_index));
            ++color_index;
        } else {
            // Если у маршрута нет остановок, следующий маршрут использует тот же индекс цвета
            continue;
        }
    }
}

// Отрисовка одного маршрута
void MapRenderer::DrawRoute(svg::Document& doc, const transport_catalogue::Bus& bus, const SphereProjector& projector, const Color& color) const {
    svg::Polyline polyline;
    for (const auto& stop : bus.stops) {
        auto point = projector(stop->coordinates);
        polyline.AddPoint(point);
    }
    if (!bus.is_round_trip) {
        // Для некольцевых маршрутов добавляем точки в обратном порядке, кроме последней остановки
        for (auto it = std::next(bus.stops.rbegin()); it != bus.stops.rend(); ++it) {
            auto point = projector((*it)->coordinates);
            polyline.AddPoint(point);
        }
    }
    polyline.SetFillColor("none");
    polyline.SetStrokeColor(ConvertColor(color));
    polyline.SetStrokeWidth(settings_.line_width);
    polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(polyline);
}

Color MapRenderer::GetRouteColor(size_t index) const {
    if (settings_.color_palette.empty()) {
        return "black";
    }
    return settings_.color_palette[index % settings_.color_palette.size()];
}

// Отрисовка всех остановок
void MapRenderer::DrawStops(svg::Document& doc, const SphereProjector& projector) const {
    if (!settings_.render_stops) {
        return; // Если остановки не нужно рисовать, выходим из метода
    }

    // Получаем все остановки из каталога
    const auto& all_stops = catalogue_.GetAllStops();

    // Получаем все маршруты из каталога
    const auto& all_buses = catalogue_.GetAllBuses();

    // Создаем множество остановок, которые принадлежат хотя бы одному маршруту
    std::unordered_set<std::string> stops_in_routes;
    for (const auto& bus : all_buses) {
        for (const auto& stop : bus.stops) {
            stops_in_routes.insert(stop->name);
        }
    }

    for (const auto& stop : all_stops) {
        // Проверяем, существует ли остановка и принадлежит ли она хотя бы одному маршруту
        if (stops_in_routes.find(stop.name) == stops_in_routes.end()) {
            continue; // Если остановка не принадлежит ни одному маршруту, пропускаем её
        }

        auto point = projector(stop.coordinates);
        svg::Circle circle;
        circle.SetCenter(point);
        circle.SetRadius(settings_.stop_radius);
        circle.SetFillColor("white");
        doc.Add(circle);

        // Добавляем текстовую метку для остановки
        svg::Text label;
        label.SetPosition(point);

        // Преобразуем std::array<double, 2> в svg::Point
        svg::Point offset{settings_.stop_label_offset[0], settings_.stop_label_offset[1]};
        label.SetOffset(offset);

        label.SetFontSize(settings_.stop_label_font_size);
        label.SetFontFamily("Verdana");
        label.SetData(stop.name);
        label.SetFillColor("black");
        doc.Add(label);
    }
}

// Отрисовка одной остановки
void MapRenderer::DrawStop(svg::Document& doc, const transport_catalogue::Stop& stop, const SphereProjector& projector) const {
    auto point = projector(stop.coordinates);
    svg::Circle circle;
    circle.SetCenter(point);
    circle.SetRadius(settings_.stop_radius);
    circle.SetFillColor("white");
    doc.Add(circle);
}

void MapRenderer::DrawStopCircles(svg::Document& doc, const SphereProjector& projector) const {
    // Получаем все маршруты из каталога
    const auto& all_buses = catalogue_.GetAllBuses();

    // Создаем множество остановок, которые принадлежат хотя бы одному маршруту
    std::unordered_set<std::string> stops_in_routes;
    for (const auto& bus : all_buses) {
        for (const auto& stop : bus.stops) {
            stops_in_routes.insert(stop->name);
        }
    }

    // Получаем все остановки из каталога
    auto stops = catalogue_.GetAllStops();
    std::sort(stops.begin(), stops.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name; // Сортировка по названию остановки
    });

    for (const auto& stop : stops) {
        // Проверяем, принадлежит ли остановка хотя бы одному маршруту
        if (stops_in_routes.find(stop.name) == stops_in_routes.end()) {
            continue; // Если остановка не принадлежит ни одному маршруту, пропускаем её
        }

        auto point = projector(stop.coordinates);
        svg::Circle circle;
        circle.SetCenter(point);
        circle.SetRadius(settings_.stop_radius);
        circle.SetFillColor("white");
        doc.Add(circle);
    }
}

void MapRenderer::DrawStopLabels(svg::Document& doc, const SphereProjector& projector) const {
    // Получаем все маршруты из каталога
    const auto& all_buses = catalogue_.GetAllBuses();

    // Создаем множество остановок, которые принадлежат хотя бы одному маршруту
    std::unordered_set<std::string> stops_in_routes;
    for (const auto& bus : all_buses) {
        for (const auto& stop : bus.stops) {
            stops_in_routes.insert(stop->name);
        }
    }

    // Получаем все остановки из каталога
    auto stops = catalogue_.GetAllStops();
    std::sort(stops.begin(), stops.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name; // Сортировка по названию остановки
    });

    for (const auto& stop : stops) {
        // Проверяем, принадлежит ли остановка хотя бы одному маршруту
        if (stops_in_routes.find(stop.name) == stops_in_routes.end()) {
            continue; // Если остановка не принадлежит ни одному маршруту, пропускаем её
        }

        auto point = projector(stop.coordinates);

        // Подложка
        svg::Text underlayer;
        underlayer.SetPosition(point);
        underlayer.SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]});
        underlayer.SetFontSize(settings_.stop_label_font_size);
        underlayer.SetFontFamily("Verdana");
        underlayer.SetData(stop.name);
        underlayer.SetFillColor(ConvertColor(settings_.underlayer_color));
        underlayer.SetStrokeColor(ConvertColor(settings_.underlayer_color));
        underlayer.SetStrokeWidth(settings_.underlayer_width);
        underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(underlayer);

        // Надпись
        svg::Text label;
        label.SetPosition(point);
        label.SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]});
        label.SetFontSize(settings_.stop_label_font_size);
        label.SetFontFamily("Verdana");
        label.SetData(stop.name);
        label.SetFillColor("black");
        doc.Add(label);
    }
}

void MapRenderer::DrawRouteLabel(svg::Document& doc, const SphereProjector& projector, geo::Coordinates coords, const std::string& name, const Color& color) const {
    auto point = projector(coords);

    // Подложка
    svg::Text underlayer;
    underlayer.SetPosition(point);
    underlayer.SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
    underlayer.SetFontSize(settings_.bus_label_font_size);
    underlayer.SetFontFamily("Verdana");
    underlayer.SetFontWeight("bold");
    underlayer.SetData(name);
    underlayer.SetFillColor(ConvertColor(settings_.underlayer_color));
    underlayer.SetStrokeColor(ConvertColor(settings_.underlayer_color));
    underlayer.SetStrokeWidth(settings_.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(underlayer);

    // Надпись
    svg::Text label;
    label.SetPosition(point);
    label.SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
    label.SetFontSize(settings_.bus_label_font_size);
    label.SetFontFamily("Verdana");
    label.SetFontWeight("bold");
    label.SetData(name);
    label.SetFillColor(ConvertColor(color));
    doc.Add(label);
}

void MapRenderer::DrawRouteLabels(svg::Document& doc, const SphereProjector& projector) const {
    auto buses = catalogue_.GetAllBuses();
    std::sort(buses.begin(), buses.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name; // Сортировка по названию маршрута
    });

    for (const auto& bus : buses) {
        if (bus.stops.empty()) {
            continue; // Если у маршрута нет остановок, пропускаем
        }

        // Получаем цвет маршрута
        auto color = GetRouteColor(std::distance(buses.begin(), std::find(buses.begin(), buses.end(), bus)));

        // Если маршрут некольцевой и первая и последняя остановки совпадают, выводим имя только один раз
        if (!bus.is_round_trip && bus.stops.front()->name == bus.stops.back()->name) {
            DrawRouteLabel(doc, projector, bus.stops.front()->coordinates, bus.name, color);
            } 
        else {
            // Отрисовываем название маршрута у первой конечной остановки
            DrawRouteLabel(doc, projector, bus.stops.front()->coordinates, bus.name, color);

            // Если маршрут некольцевой, отрисовываем название у второй конечной остановки
            if (!bus.is_round_trip) {
                DrawRouteLabel(doc, projector, bus.stops.back()->coordinates, bus.name, color);
            }
        }
    }
}

// Парсинг настроек визуализации из JSON
    RenderSettings ParseRenderSettings(const json::Node& render_settings_node) {
    RenderSettings settings;
    settings.width = render_settings_node.AsMap().at("width").AsDouble();
    settings.height = render_settings_node.AsMap().at("height").AsDouble();
    settings.padding = render_settings_node.AsMap().at("padding").AsDouble();
    settings.line_width = render_settings_node.AsMap().at("line_width").AsDouble();
    settings.stop_radius = render_settings_node.AsMap().at("stop_radius").AsDouble();
    settings.bus_label_font_size = render_settings_node.AsMap().at("bus_label_font_size").AsInt();
    settings.bus_label_offset = {render_settings_node.AsMap().at("bus_label_offset").AsArray()[0].AsDouble(),
                                render_settings_node.AsMap().at("bus_label_offset").AsArray()[1].AsDouble()};
    settings.stop_label_font_size = render_settings_node.AsMap().at("stop_label_font_size").AsInt();
    settings.stop_label_offset = {render_settings_node.AsMap().at("stop_label_offset").AsArray()[0].AsDouble(),
                                 render_settings_node.AsMap().at("stop_label_offset").AsArray()[1].AsDouble()};
    settings.underlayer_color = ParseColor(render_settings_node.AsMap().at("underlayer_color"));
    settings.underlayer_width = render_settings_node.AsMap().at("underlayer_width").AsDouble();
    for (const auto& color_node : render_settings_node.AsMap().at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color_node));
    }

    // Устанавливаем флаг render_stops в зависимости от наличия остановок в запросе
    settings.render_stops = render_settings_node.AsMap().count("stops") > 0;

    return settings;
}

svg::Color ConvertColor(const Color& color) {
    return std::visit([](const auto& value) -> svg::Color {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return svg::NoneColor;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, svg::Rgb>) {
            return value;
        } else if constexpr (std::is_same_v<T, svg::Rgba>) {
            return value;
        }
    }, color);
}

Color ParseColor(const json::Node& color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const auto& color_array = color_node.AsArray();
        if (color_array.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(color_array[0].AsInt()),
                static_cast<uint8_t>(color_array[1].AsInt()),
                static_cast<uint8_t>(color_array[2].AsInt())
            };
        } else if (color_array.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(color_array[0].AsInt()),
                static_cast<uint8_t>(color_array[1].AsInt()),
                static_cast<uint8_t>(color_array[2].AsInt()),
                color_array[3].AsDouble()
            };
        }
    }
    return "black"; // Default color
}

}  // namespace map_renderer