#include "input_reader.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

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
    for (const auto& command : commands_) {
        std::istringstream iss(command);
        std::string type;
        iss >> type;

        if (type == "Stop") {
            std::string name;
            iss >> std::ws;
            std::getline(iss, name, ':');

            double lat, lng;
            iss >> lat;
            iss.ignore();
            iss >> lng;

            geo::Coordinates coordinates{lat, lng};
            catalogue.AddStop(name, coordinates);
        } else if (type == "Bus") {
            std::string name;
            iss >> std::ws;
            std::getline(iss, name, ':');

            std::string route;
            iss >> std::ws;
            std::getline(iss, route);

            bool is_round_trip = false;
            if (route.find('>') != std::string::npos) {
                is_round_trip = true;
            }

            std::vector<const transport_catalogue::Stop*> stops;
            std::string stop_name;
            char delimiter = is_round_trip ? '>' : '-';
            std::replace(route.begin(), route.end(), delimiter, ' ');

            std::istringstream route_iss(route);
            while (route_iss >> stop_name) {
                if (stop_name.back() == ',') {
                    stop_name.pop_back();
                }
                const auto* stop = catalogue.FindStop(stop_name);
                if (stop) {
                    stops.push_back(stop);
                }
            }

            if (!is_round_trip) {
                for (auto it = stops.rbegin() + 1; it != stops.rend(); ++it) {
                    stops.push_back(*it);
                }
            }

            catalogue.AddBus(name, stops, is_round_trip);
        }
    }
}

} // namespace input_reader
