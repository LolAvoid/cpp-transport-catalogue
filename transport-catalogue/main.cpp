#include <iostream>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    input_reader::InputReader input_reader;

    int n;
    std::cin >> n;
    std::cin.ignore(); 
    
    for (int i = 0; i < n; ++i) {
        std::string line;
        std::getline(std::cin, line);
        input_reader.ParseLine(line);
    }
    
    input_reader.ApplyCommands(catalogue);       

    int q;
    std::cin >> q;
    std::cin.ignore(); 

    stat_reader::StatReader stat_reader(catalogue);

    for (int i = 0; i < q; ++i) {
        std::string line;
        std::getline(std::cin, line);
        stat_reader.ProcessQuery(line);
    }       

    return 0;
}
