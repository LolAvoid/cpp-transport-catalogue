#pragma once

#include "transport_catalogue.h"
#include <iostream>

namespace stat_reader {

class StatReader {
public:
    explicit StatReader(const transport_catalogue::TransportCatalogue& catalogue);
    
    void ProcessQueries(std::istream& input, std::ostream& output) const;
    void ProcessQuery(const std::string& name, std::ostream& output) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};

} 
