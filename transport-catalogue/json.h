#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
#include <optional>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::monostate, std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node() : value_(std::monostate{}) {}  // Инициализация std::monostate
    Node(Dict map) : value_(map) {}
    Node(int value) : value_(value) {}
    Node(double value) : value_(value) {}
    Node(bool value) : value_(value) {}
    Node(const std::string& value) : value_(value) {}
    Node(const char* value) : value_(std::string(value)) {}
    Node(std::nullptr_t) : value_(nullptr) {}
    Node(const Array& array) : value_(array) {}

    // Конструктор копирования
    Node(const Node& other) : value_(other.value_) {}
    
    Node(Node&& other) noexcept : value_(std::move(other.value_)) {
        other.value_ = std::monostate{};
    }

    Node& operator=(Node&& other) noexcept {
        if (this != &other) {
            value_ = std::move(other.value_);
            other.value_ = std::monostate{};
        }
        return *this;
    }

    bool IsNull() const {
        return std::holds_alternative<std::nullptr_t>(value_);
    }

    bool IsInt() const { return std::holds_alternative<int>(value_); }
    bool IsDouble() const { return IsInt() || IsPureDouble(); }
    bool IsPureDouble() const { return std::holds_alternative<double>(value_); }
    bool IsBool() const { return std::holds_alternative<bool>(value_); }
    bool IsString() const { return std::holds_alternative<std::string>(value_); }
    bool IsArray() const { return std::holds_alternative<std::vector<Node>>(value_); }
    bool IsMap() const { return std::holds_alternative<Dict>(value_); }

    int AsInt() const {
        if (IsInt()) return std::get<int>(value_);
        throw std::logic_error("Not an int");
    }

    bool AsBool() const {
        if (IsBool()) return std::get<bool>(value_);
        throw std::logic_error("Not a bool");
    }

    double AsDouble() const {
        if (IsPureDouble()) {
            return std::get<double>(value_);
        }
        if (IsInt()) {
            return static_cast<double>(std::get<int>(value_));
        }
        throw std::logic_error("Not a double");
    }

    const std::string& AsString() const {
        if (IsString()) return std::get<std::string>(value_);
        throw std::logic_error("Not a string");
    }

    const std::vector<Node>& AsArray() const {
        if (IsArray()) return std::get<std::vector<Node>>(value_);
        throw std::logic_error("Not an array");
    }

    const Dict& AsMap() const {
        if (IsMap()) return std::get<Dict>(value_);
        throw std::logic_error("Not a map");
    }

    const Value& GetValue() const { return value_; }

    bool operator==(const Node& other) const {
        return value_ == other.value_;
    }

    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

private:
    Value value_;
};

class Document {
public:
    // Конструктор с инициализацией корня
    explicit Document(Node root) : root_(std::move(root)) {}

    // Конструктор по умолчанию
    Document() : root_(Node()) {} // Создаем пустой Node или можно использовать nullptr

    // Метод для получения корня
    const Node& GetRoot() const { return root_; }

    // Метод для установки нового корня
    void SetRoot(Node newRoot) { root_ = std::move(newRoot); }

    // Оператор сравнения ==
    bool operator==(const Document& other) const {
        return this->GetRoot() == other.GetRoot(); // Сравниваем корни документов
    }

private:
    Node root_;
};


Document LoadJSON(const std::string& json);
Document Load(std::istream& input);

void Print(const Node& node, std::ostream& output, int indent);
}  // namespace json