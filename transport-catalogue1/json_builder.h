#pragma once

#include "json.h"

#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

namespace json {

class Builder {
public:
    Builder() = default;

    // Вложенный базовый класс для контекстов
    class BaseContext {
    public:
        BaseContext(Builder& builder) : builder_(builder) {}

    protected:
        Builder& builder_;
    };

    // Вложенные классы для контекстов
    class KeyContext;
    class ArrayItemContext;

    class DictItemContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        KeyContext Key(std::string key);
        Builder& EndDict();

        // Запрещаем методы, которые не должны быть доступны в контексте словаря
        ArrayItemContext Value(Node::Value value) = delete;
        ArrayItemContext StartArray() = delete;
        Builder& EndArray() = delete;
    };

    class ArrayItemContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        ArrayItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();

        // Запрещаем методы, которые не должны быть доступны в контексте массива
        KeyContext Key(std::string key) = delete;
        Builder& EndDict() = delete;
    };

    class KeyContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        DictItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();

        // Запрещаем методы, которые не должны быть доступны в контексте ключа
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
    };

    // Методы для построения JSON
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    KeyContext Key(std::string key);
    Builder& Value(Node::Value value);
    Builder& EndDict();
    Builder& EndArray();

    // Завершает построение JSON и возвращает результат
    Node Build();

private:
    // Вспомогательные методы и структуры
    void EnsureNotBuilt();
    void EnsureInDictContext();
    void EnsureInArrayOrDictContext();
    void EnsureInArrayContext();
    void Reset();
    void AddNode(Node node);

    // Стек для отслеживания текущего контекста
    std::vector<Node*> nodes_stack_;
    Node root_;
    bool is_built_ = false;
    std::string current_key_;  // Текущий ключ для словаря
    bool key_is_entered_ = false;

    // Счетчики для отслеживания открытых контейнеров
    int open_dicts_ = 0;
    int open_arrays_ = 0;
};

}  // namespace json