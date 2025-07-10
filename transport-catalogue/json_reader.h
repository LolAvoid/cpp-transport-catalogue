#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"
#include <sstream>
#include <string>
#include <vector>

namespace json_reader {

class JsonReader {
public:
    JsonReader(transport_catalogue::TransportCatalogue& catalogue);

    // Загрузка данных из JSON
    void LoadData(const json::Node& data);

    // Обработка запросов и формирование ответа
    json::Node ProcessRequests(const json::Node& requests, const json::Node& render_settings);

private:
    transport_catalogue::TransportCatalogue& catalogue_;
};

class StatReader {
public:
    explicit StatReader(const transport_catalogue::TransportCatalogue& catalogue);

    void ProcessQuery(const json::Node& query) const;
    json::Dict ProcessBusQuery(const std::string& bus_name, int request_id) const;
    json::Dict ProcessStopQuery(const std::string& stop_name, int request_id) const;
    json::Dict ProcessMapQuery(int request_id) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};

} // namespace json_reader