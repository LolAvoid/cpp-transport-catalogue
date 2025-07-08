#pragma once

#include "transport_catalogue.h"
#include <iostream>

namespace input_reader {

class InputReader {
public:
    void ReadInput(std::istream& input);
    void ParseLine(std::string_view line);
    void ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const;

private:
    std::vector<std::string> commands_;
};

} 
