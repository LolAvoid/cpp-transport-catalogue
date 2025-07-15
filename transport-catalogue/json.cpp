#include "json.h"
#include <cassert>
#include <sstream>
#include <iomanip>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input, char first_char) {
    std::vector<Node> result;
    char c = first_char;

    while (true) {
        input >> c;  // Пропускаем пробелы и читаем следующий символ
        
        if (c == ']') {
            break;  // Массив закончился
        }

        if (c == ',') {
            // Если это запятая, продолжаем считывать
            continue;
        }

        // Возвращаем символ в поток, если это не запятая и не закрывающая скобка
        input.putback(c);  
        result.push_back(LoadNode(input));  // Загружаем элемент массива
    }
    return Node(move(result));
}

Node LoadNumber(istream& input, char first_char) {
    string parsed_num;
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream");
        }
    };

    if (first_char == '-') {
        parsed_num += first_char;
    } else {
        input.putback(first_char);
    }

    while (isdigit(input.peek())) {
        read_char();
    }

    if (input.peek() == '.') {
        read_char();
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    if (input.peek() == 'e' || input.peek() == 'E') {
        read_char();
        if (input.peek() == '+' || input.peek() == '-') {
            read_char();
        }
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    try {
        if (parsed_num.find('.') != string::npos || parsed_num.find('e') != string::npos || parsed_num.find('E') != string::npos) {
            return Node(stod(parsed_num));
        } else {
            return Node(stoi(parsed_num));
        }
    } catch (const std::invalid_argument&) {
        throw ParsingError("Invalid number format");
    } catch (const std::out_of_range&) {
        throw ParsingError("Number out of range");
    }
}

Node LoadString(istream& input, char first_char) {
    string line;
    char c = first_char;

    //std::cout << "LoadString: start reading string" << std::endl;  // Отладочный вывод
    //std::cout << "LoadString: current character = " << c << std::endl;  // Отладочный вывод

    if (c != '"') {
        // Если первый символ не кавычка, выбрасываем исключение с информацией о прочитанном символе
        throw ParsingError("Expected '\"' at the start of a string, but got: " + string(1, c));
    }

    while (input.get(c)) {
        //std::cout << "LoadString: current character = " << c << std::endl;  // Отладочный вывод
        if (c == '"') {
            // Если встретили закрывающую кавычку, завершаем чтение
            //std::cout << "LoadString: final string = " << line << std::endl;  // Отладочный вывод
            return Node(move(line));
        }

        if (c == '\\') {
            // Обрабатываем экранированные символы
            input.get(c);
            //std::cout << "LoadString: escape sequence = \\" << c << std::endl;  // Отладочный вывод
            switch (c) {
                case 'n': line += '\n'; break;
                case 'r': line += '\r'; break;
                case 't': line += '\t'; break;
                case '"': line += '"'; break;
                case '\\': line += '\\'; break;
                default:
                throw ParsingError("Invalid escape sequence: \\" + string(1, c) + " in string: \"" + line + "\"");
            }
        } else {
            line += c;
        }
    }

    // Если поток закончился, а закрывающая кавычка не найдена, выбрасываем исключение
    throw ParsingError("ParsingError exception is expected on '\"" + line + "'");
}

Node LoadDict(istream& input, char first_char) {
    Dict result;
    char c = first_char;

    while (true) {
        input >> c;  // Пропускаем пробелы и читаем следующий символ
        //std::cout << "LoadDict: current character = " << c << std::endl;  // Отладочный вывод

        if (c == '}') {
            break;  // Словарь закончился
        }
        if (c == ',') {
            input >> c;  // Пропускаем запятую и читаем следующий символ
        }

        // Возвращаем символ в поток, если это не запятая и не закрывающая скобка
        input.putback(c);

        // Загружаем ключ
        input >> c;  // Считываем первый символ ключа
        if (c != '"') {
            // Если первый символ не кавычка, возвращаем символ в поток и выбрасываем ошибку
            input.putback(c);
            throw ParsingError("Expected '\"' at the start of a key in dictionary");
        }
        string key = LoadString(input, c).AsString();  // Передаем кавычку в LoadString
        //std::cout << "LoadDict: key = " << key << std::endl;  // Отладочный вывод

        input >> c;  // Считываем ':'
        //std::cout << "LoadDict: after ':', current character = " << c << std::endl;  // Отладочный вывод
        
        // Загружаем значение
        result.insert({move(key), LoadNode(input)});
    }
    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input, c);
    } else if (c == '{') {
        return LoadDict(input, c);
    } else if (c == '"') {
        return LoadString(input, c);
    } else if (c == 't' || c == 'f' || c == 'n') {
        input.putback(c);
        string word;
        char next_char;

        while (input.get(next_char)) {
            if (next_char == ',' || next_char == '}' || next_char == ']' || isspace(next_char)) {
                input.putback(next_char);
                break;
            }
            word += next_char;
        }

        if (word == "true" || word == "false" || word == "null" || word == "nullptr") {
            char next_char = input.peek();
            if (next_char != ',' && next_char != '}' && next_char != ']' && !isspace(next_char) && !input.eof()) {
                throw ParsingError("Invalid value: unexpected character after '" + word + "'");
            }

            if (word == "true") {
                return Node(true);
            } else if (word == "false") {
                return Node(false);
            } else {
                return Node(nullptr);
            }
        } else {
            throw ParsingError("Invalid value: expected 'true', 'false', 'null', or 'nullptr', but got: " + word);
        }
    } else {
        return LoadNumber(input, c);
    }
}

}  // namespace

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

Document LoadJSON(const std::string& json) {
    std::istringstream input(json); // Создаем поток из строки
    Node rootNode = LoadNode(input); // Загружаем корневой узел
    return Document(std::move(rootNode)); // Возвращаем Document
}

std::string EscapeString(const std::string& str) {
    std::string result;
    for (char ch : str) {
        switch (ch) {
            case '"':  // Экранируем кавычки
                result += "\\\"";
                break;
            case '\\':  // Экранируем обратный слэш
                result += "\\\\";
                break;
            case '\n':  // Экранируем перевод строки
                result += "\\n";
                break;
            case '\r':  // Экранируем возврат каретки
                result += "\\r";
                break;
            case '\t':  // Экранируем табуляцию
                result += "\\t";
                break;
            default:
                result += ch;
                break;
        }
    }
    return result;
}

std::string FormatDouble(double value) {
    std::ostringstream oss;
    //oss << std::fixed << std::setprecision(4) << value; // Выводим с фиксированной точностью
    oss << value;
    std::string str = oss.str();

    // Убираем лишние нули после запятой
    size_t dot_pos = str.find('.');
    if (dot_pos != std::string::npos) {
        // Удаляем нули с конца строки
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        // Если после удаления нулей осталась только точка, удаляем и её
        if (str.back() == '.') {
            str.pop_back();
        }
    }

    return str;
}

void Print(const Node& node, std::ostream& output, int indent = 0) {
    const std::string indent_str(indent, ' ');  // Строка с отступами

    if (node.IsNull()) {
        output << "null";
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsDouble()) {
        // Форматируем число с плавающей точкой
        output << FormatDouble(node.AsDouble());
    } else if (node.IsBool()) {
        output << (node.AsBool() ? "true" : "false");
    } else if (node.IsString()) {
        output << "\"" << EscapeString(node.AsString()) << "\"";
    } else if (node.IsArray()) {
        output << "[\n";
        const std::vector<Node>& arr = node.AsArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) output << ",\n";
            output << indent_str << "    ";
            Print(arr[i], output, indent + 4);  // Рекурсивный вызов с увеличенным отступом
        }
        output << "\n" << indent_str << "]";
    } else if (node.IsMap()) {
        output << "{\n";
        const Dict& dict = node.AsMap();
        size_t i = 0;
        for (const auto& [key, value] : dict) {
            if (i > 0) output << ",\n";
            output << indent_str << "    \"" << EscapeString(key) << "\": ";
            Print(value, output, indent + 4);  // Рекурсивный вызов с увеличенным отступом
            ++i;
        }
        output << "\n" << indent_str << "}";
    }
}

std::string Print(const Node& doc) {
    std::ostringstream oss;
    Print(doc, oss);  // Используем существующую реализацию
    return oss.str();  // Возвращаем строку
}

}  // namespace json