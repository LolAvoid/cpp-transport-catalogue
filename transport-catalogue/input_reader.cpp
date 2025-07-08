#include "input_reader.h"
#include <string>
#include <vector>

namespace input_reader {

void InputReader::ReadInput(std::istream& input) {
    int command_count;
    input >> command_count;
    input.ignore();

    for (int i = 0; i < command_count; ++i) {
        std::string line;
        std::getline(input, line);
        commands_.push_back(std::move(line));
    }
}

void InputReader::ParseLine(std::string_view line) {
    commands_.emplace_back(line);
}

void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {

}

} 
