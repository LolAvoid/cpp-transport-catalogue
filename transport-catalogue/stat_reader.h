#pragma once

#include <string>
#include "transport_catalogue.h"

namespace stat_reader {

class StatReader {
public:
    explicit StatReader(const transport_catalogue::TransportCatalogue& catalogue);
    void ProcessQuery(const std::string& bus_name) const;
    void PrintAllBuses() const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};

} 
