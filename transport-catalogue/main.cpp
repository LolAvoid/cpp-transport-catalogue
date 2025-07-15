#include <iostream>
#include <string>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "json.h"
#include "map_renderer.h"
#include <sstream>
#include "svg.h"

std::string LoadDocumentFromStream(std::istream& input) {
    std::string input_json;
    int brace_count = 0; // Счетчик фигурных скобок
    char ch;

    while (input.get(ch)) {
        input_json += ch;

        if (ch == '{') {
            brace_count++;
        } else if (ch == '}') {
            brace_count--;
        }

        // Если число открытых и закрытых скобок совпало, завершаем чтение
        if (brace_count == 0) {
            break;
        }
    }

    return input_json;
}

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::JsonReader json_reader(catalogue);

    // Загружаем JSON-документ из стандартного ввода
    std::string input_json = LoadDocumentFromStream(std::cin);
    json::Document input_data = json::LoadJSON(input_json);

    // Загрузка данных в каталог
    json_reader.LoadData(input_data.GetRoot());

    // Получаем настройки визуализации
    const auto& render_settings = input_data.GetRoot().AsMap().at("render_settings");

    // Обработка запросов и формирование ответа
    const auto& stat_requests = input_data.GetRoot().AsMap().at("stat_requests").AsArray();
    json::Node responses = json_reader.ProcessRequests(stat_requests, render_settings);

    // Вывод ответа в формате JSON
    Print(responses, std::cout, 4);

    return 0;
}