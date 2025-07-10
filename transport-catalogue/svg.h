#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <variant>
#include <string>
#include <cstdint>

namespace svg {

// Объявление структур Rgb и Rgba
struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{std::monostate{}};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);
std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

inline std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit([&out](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            out << "none";
        } else if constexpr (std::is_same_v<T, std::string>) {
            out << value;
        } else if constexpr (std::is_same_v<T, Rgb>) {
            out << "rgb(" << static_cast<int>(value.red) << ","
                << static_cast<int>(value.green) << ","
                << static_cast<int>(value.blue) << ")";
        } else if constexpr (std::is_same_v<T, Rgba>) {
            out << "rgba(" << static_cast<int>(value.red) << ","
                << static_cast<int>(value.green) << ","
                << static_cast<int>(value.blue) << ","
                << value.opacity << ")";
        }
    }, color);
    return out;
}

class PathProps_T {
public:
    PathProps_T& SetFillColor(Color color);
    PathProps_T& SetStrokeColor(Color color);
    PathProps_T& SetStrokeWidth(double width);
    PathProps_T& SetStrokeLineCap(StrokeLineCap line_cap);
    PathProps_T& SetStrokeLineJoin(StrokeLineJoin line_join);
    void RenderAttrs(std::ostream& out) const;

protected:  // Изменено с private на protected
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

template<typename T>
class PathProps : public PathProps_T {
public:
    T& SetFillColor(Color color) {
        PathProps_T::SetFillColor(std::move(color));
        return static_cast<T&>(*this);
    }

    T& SetStrokeColor(Color color) {
        PathProps_T::SetStrokeColor(std::move(color));
        return static_cast<T&>(*this);
    }

    T& SetStrokeWidth(double width) {
        PathProps_T::SetStrokeWidth(width);
        return static_cast<T&>(*this);
    }

    T& SetStrokeLineCap(StrokeLineCap line_cap) {
        PathProps_T::SetStrokeLineCap(line_cap);
        return static_cast<T&>(*this);
    }

    T& SetStrokeLineJoin(StrokeLineJoin line_join) {
        PathProps_T::SetStrokeLineJoin(line_join);
        return static_cast<T&>(*this);
    }
};


// Forward declarations
class Object;
class ObjectContainer;

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out, int indent_step = 0, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    virtual ~Object() = default;
    void Render(const RenderContext& context) const;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

    Circle& SetFillColor(Color color);
    Circle& SetStrokeColor(Color color);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);

    Polyline& SetFillColor(Color color);
    Polyline& SetStrokeColor(Color color);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);

    Text& SetFillColor(Color color);
    Text& SetStrokeColor(Color color);

private:
    void RenderObject(const RenderContext& context) const override;

    Point position_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void Draw(ObjectContainer& container) const = 0;
};

class ObjectContainer {
public:
    virtual ~ObjectContainer() = default;

    template <typename Obj>
    void Add(Obj obj) {
        static_assert(std::is_base_of_v<Object, Obj>, "Obj must inherit from Object");
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override {
        objects_.push_back(std::move(obj));
    }

    void Render(std::ostream& out) const;

private:
    std::vector<std::unique_ptr<Object>> objects_;
};

} // namespace svg