#include "json_builder.h"

namespace json {

void Builder::AddNode(Node node) {
    if (nodes_stack_.empty()) {
        root_ = std::move(node);
        nodes_stack_.push_back(&root_);
    } else if (nodes_stack_.back()->IsArray()) {
        const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(std::move(node));
        nodes_stack_.push_back(&const_cast<Array&>(nodes_stack_.back()->AsArray()).back());
    } else if (nodes_stack_.back()->IsMap()) {
        if (current_key_.empty() && !key_is_entered_) {
            Reset();
            throw std::logic_error("StartDict() or StartArray() called after Key() without a value");
        }
        const_cast<Dict&>(nodes_stack_.back()->AsMap())[current_key_] = std::move(node);
        nodes_stack_.push_back(&const_cast<Dict&>(nodes_stack_.back()->AsMap())[current_key_]);
        current_key_.clear();
        key_is_entered_ = false;
    }
}

Builder::ArrayItemContext Builder::StartArray() {
    EnsureNotBuilt();
    EnsureInArrayOrDictContext();

    Node node(Array{});
    AddNode(std::move(node));

    open_arrays_++;
    return ArrayItemContext(*this);
}

Builder::DictItemContext Builder::StartDict() {
    EnsureNotBuilt();
    EnsureInArrayOrDictContext();

    Node node(Dict{});
    AddNode(std::move(node));

    open_dicts_++;
    return DictItemContext(*this);
}

Builder::KeyContext Builder::Key(std::string key) {
    EnsureNotBuilt();
    EnsureInDictContext();

    if (!current_key_.empty()) {
        Reset();
        throw std::logic_error("Key() called after Key() without a value");
    }

    current_key_ = std::move(key);
    key_is_entered_ = true;
    return KeyContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    EnsureNotBuilt();
    EnsureInArrayOrDictContext();

    Node node = std::visit([](auto&& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
            return Node();
        } else {
            return Node(arg);
        }
    }, value);

    if (nodes_stack_.empty()) {
        root_ = std::move(node);
        is_built_ = true;
        key_is_entered_ = true;
    } else if (nodes_stack_.back()->IsArray()) {
        const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(std::move(node));
    } else if (nodes_stack_.back()->IsMap()) {
        if (current_key_.empty() && !key_is_entered_) {
            Reset();
            throw std::logic_error("Value() called after Key() without a value");
        }
        const_cast<Dict&>(nodes_stack_.back()->AsMap())[current_key_] = std::move(node);
        current_key_.clear();
        key_is_entered_ = false;
    }

    return *this;
}

Builder& Builder::EndDict() {
    EnsureNotBuilt();
    EnsureInDictContext();

    nodes_stack_.pop_back();
    open_dicts_--;
    return *this;
}

Builder& Builder::EndArray() {
    EnsureNotBuilt();
    EnsureInArrayContext();

    nodes_stack_.pop_back();
    open_arrays_--;
    return *this;
}

Node Builder::Build() {
    if (nodes_stack_.empty() && is_built_) {
        Node result = std::move(root_);
        Reset();
        return result;
    }

    if (open_dicts_ != 0 || open_arrays_ != 0) {
        Reset();
        throw std::logic_error("Build() called with unfinished containers");
    }

    if (nodes_stack_.empty()) {
        is_built_ = true;
        Node result = std::move(root_);
        Reset();
        return result;
    }

    Reset();
    throw std::logic_error("Build() called on empty builder");
}

void Builder::Reset() {
    nodes_stack_.clear();
    root_ = Node();
    is_built_ = false;
    current_key_.clear();
    key_is_entered_ = false;
    open_dicts_ = 0;
    open_arrays_ = 0;
}

void Builder::EnsureNotBuilt() {
    if (is_built_) {
        Reset();
        throw std::logic_error("Builder is already built");
    }
}

void Builder::EnsureInDictContext() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
        Reset();
        throw std::logic_error("Key() called outside of a dictionary");
    }
}

void Builder::EnsureInArrayOrDictContext() {
    if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsMap()) {
        Reset();
        throw std::logic_error("Value() called in invalid context");
    }
}

void Builder::EnsureInArrayContext() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        Reset();
        throw std::logic_error("EndArray() called outside of an array");
    }
}

// Реализация методов вспомогательных классов

Builder::KeyContext Builder::DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& Builder::DictItemContext::EndDict() {
    return builder_.EndDict();
}

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return *this;
}

Builder::DictItemContext Builder::ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

Builder::ArrayItemContext Builder::ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& Builder::ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

Builder::DictItemContext Builder::KeyContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

Builder::DictItemContext Builder::KeyContext::StartDict() {
    return builder_.StartDict();
}

Builder::ArrayItemContext Builder::KeyContext::StartArray() {
    return builder_.StartArray();
}

}  // namespace json