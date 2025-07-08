#include <iostream>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    input_reader::InputReader input_reader;
    stat_reader::StatReader stat_reader(catalogue);

    input_reader.ReadInput(std::cin);
    input_reader.ApplyCommands(catalogue);

    stat_reader.ProcessQueries(std::cin, std::cout);

    return 0;
}
