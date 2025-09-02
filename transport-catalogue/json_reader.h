#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"
#include "json_builder.h"
#include "transport_router.h"
#include <sstream>
#include <string>
#include <vector>
#include "graph.h"

namespace json_reader {

class JsonReader {
public:
    JsonReader(transport_catalogue::TransportCatalogue& catalogue);

    void LoadData(const json::Node& data);
    json::Node ProcessRequests(const json::Node& requests, const json::Node& render_settings);
    void LoadRoutingSettings(const json::Node& settings_node);
    void SetDefaultRoutingSettings();

private:
    transport_catalogue::TransportCatalogue& catalogue_;
    std::optional<graph::DirectedWeightedGraph<double>> cached_graph_;
    std::unique_ptr<transport::Router> cached_router_;

    void ProcessStopRequest(const json::Dict& request_map);
    void ProcessBusRequest(const json::Dict& request_map);
    void ProcessStopResponse(json::Builder& builder, const json::Dict& request_map, int id);
    void ProcessBusResponse(json::Builder& builder, const json::Dict& request_map, int id);
    void ProcessMapResponse(json::Builder& builder, int id, const json::Node& render_settings);
    void ProcessRouteResponse(json::Builder& builder, const json::Dict& request_map, int id);
};

class StatReader {
public:
    explicit StatReader(const transport_catalogue::TransportCatalogue& catalogue);
    void ProcessQuery(const json::Node& query) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;

    void ProcessBusQuery(json::Builder& builder, const json::Dict& query_map, int request_id) const;
    void ProcessStopQuery(json::Builder& builder, const json::Dict& query_map, int request_id) const;
    void ProcessMapQuery(json::Builder& builder, int request_id) const;
};

} // namespace json_reader