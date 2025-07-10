#pragma once

#include <string_view>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "json.h"

namespace stat_reader {

class StatReader {
public:
    explicit StatReader(const transport_catalogue::TransportCatalogue& catalogue);

    void ProcessQuery(const json::Node& query) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};

} // namespace stat_reader