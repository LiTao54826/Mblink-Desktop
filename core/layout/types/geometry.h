/**
 * @file geometry.h
 * @brief Geometric primitives for layout computation
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/geometry.rs
 */

#pragma once

#include <optional>
#include <algorithm>
#include <cmath>

namespace mbink {

//------------------------------------------------------------------------------
// Axis Types
//------------------------------------------------------------------------------

/// The simple absolute horizontal and vertical axis
enum class AbsoluteAxis {
    Horizontal,
    Vertical
};

/// Returns the other axis
inline AbsoluteAxis OtherAxis(AbsoluteAxis axis) {
    return axis == AbsoluteAxis::Horizontal ? AbsoluteAxis::Vertical : AbsoluteAxis::Horizontal;
}

/// The CSS abstract axis
/// https://www.w3.org/TR/css-writing-modes-3/#abstract-axes
enum class AbstractAxis {
    /// The axis in the inline dimension (horizontal in horizontal writing modes)
    Inline,
    /// The axis in the block dimension (vertical in horizontal writing modes)
    Block
};

/// Returns the other abstract axis
inline AbstractAxis OtherAxis(AbstractAxis axis) {
    return axis == AbstractAxis::Inline ? AbstractAxis::Block : AbstractAxis::Inline;
}

/// Convert AbstractAxis to AbsoluteAxis (assuming horizontal writing mode)
inline AbsoluteAxis ToAbsoluteAxis(AbstractAxis axis) {
    return axis == AbstractAxis::Inline ? AbsoluteAxis::Horizontal : AbsoluteAxis::Vertical;
}

//------------------------------------------------------------------------------
// FlexDirection (forward declaration for axis methods)
//------------------------------------------------------------------------------

enum class FlexDirection {
    Row,
    Column,
    RowReverse,
    ColumnReverse
};

inline bool IsRow(FlexDirection dir) {
    return dir == FlexDirection::Row || dir == FlexDirection::RowReverse;
}

inline bool IsColumn(FlexDirection dir) {
    return dir == FlexDirection::Column || dir == FlexDirection::ColumnReverse;
}

inline bool IsReverse(FlexDirection dir) {
    return dir == FlexDirection::RowReverse || dir == FlexDirection::ColumnReverse;
}

//------------------------------------------------------------------------------
// Point<T> - A 2D coordinate
//------------------------------------------------------------------------------

template<typename T>
struct Point {
    T x;
    T y;

    static Point Zero() { return Point{T{}, T{}}; }

    /// Get component based on FlexDirection
    T Main(FlexDirection dir) const {
        return IsRow(dir) ? x : y;
    }

    T Cross(FlexDirection dir) const {
        return IsRow(dir) ? y : x;
    }

    /// Get component based on AbstractAxis
    T Get(AbstractAxis axis) const {
        return axis == AbstractAxis::Inline ? x : y;
    }

    void Set(AbstractAxis axis, T value) {
        if (axis == AbstractAxis::Inline) x = value;
        else y = value;
    }

    /// Map function over both components
    template<typename F>
    auto Map(F f) const -> Point<decltype(f(x))> {
        return Point<decltype(f(x))>{f(x), f(y)};
    }

    Point Transpose() const {
        return Point{y, x};
    }
};

template<typename T, typename U>
Point<decltype(T{} + U{})> operator+(const Point<T>& a, const Point<U>& b) {
    return Point<decltype(T{} + U{})>{a.x + b.x, a.y + b.y};
}

//------------------------------------------------------------------------------
// Size<T> - Width and height
//------------------------------------------------------------------------------

template<typename T>
struct Size {
    T width;
    T height;

    static Size Zero() { return Size{T{}, T{}}; }

    /// Get component based on FlexDirection
    T Main(FlexDirection dir) const {
        return IsRow(dir) ? width : height;
    }

    T Cross(FlexDirection dir) const {
        return IsRow(dir) ? height : width;
    }

    void SetMain(FlexDirection dir, T value) {
        if (IsRow(dir)) width = value;
        else height = value;
    }

    void SetCross(FlexDirection dir, T value) {
        if (IsRow(dir)) height = value;
        else width = value;
    }

    Size WithMain(FlexDirection dir, T value) const {
        Size result = *this;
        result.SetMain(dir, value);
        return result;
    }

    Size WithCross(FlexDirection dir, T value) const {
        Size result = *this;
        result.SetCross(dir, value);
        return result;
    }

    /// Get component based on AbstractAxis
    T Get(AbstractAxis axis) const {
        return axis == AbstractAxis::Inline ? width : height;
    }

    void Set(AbstractAxis axis, T value) {
        if (axis == AbstractAxis::Inline) width = value;
        else height = value;
    }

    /// Get component based on AbsoluteAxis
    T GetAbs(AbsoluteAxis axis) const {
        return axis == AbsoluteAxis::Horizontal ? width : height;
    }

    /// Map function over both components
    template<typename F>
    auto Map(F f) const -> Size<decltype(f(width))> {
        return Size<decltype(f(width))>{f(width), f(height)};
    }

    /// Zip two sizes and apply function
    template<typename U, typename F>
    auto ZipMap(const Size<U>& other, F f) const -> Size<decltype(f(width, other.width))> {
        return Size<decltype(f(width, other.width))>{
            f(width, other.width),
            f(height, other.height)
        };
    }

    bool operator==(const Size& other) const {
        return width == other.width && height == other.height;
    }

    bool operator!=(const Size& other) const {
        return !(*this == other);
    }
};

template<typename T, typename U>
Size<decltype(T{} + U{})> operator+(const Size<T>& a, const Size<U>& b) {
    return Size<decltype(T{} + U{})>{a.width + b.width, a.height + b.height};
}

template<typename T, typename U>
Size<decltype(T{} - U{})> operator-(const Size<T>& a, const Size<U>& b) {
    return Size<decltype(T{} - U{})>{a.width - b.width, a.height - b.height};
}

// Specialization for Size<float>
template<>
inline Size<float> Size<float>::Zero() { return Size{0.0f, 0.0f}; }

/// Component-wise max
inline Size<float> Max(const Size<float>& a, const Size<float>& b) {
    return Size<float>{std::max(a.width, b.width), std::max(a.height, b.height)};
}

/// Component-wise min  
inline Size<float> Min(const Size<float>& a, const Size<float>& b) {
    return Size<float>{std::min(a.width, b.width), std::min(a.height, b.height)};
}

//------------------------------------------------------------------------------
// Line<T> - Start and end
//------------------------------------------------------------------------------

template<typename T>
struct Line {
    T start;
    T end;

    /// Map function
    template<typename F>
    auto Map(F f) const -> Line<decltype(f(start))> {
        return Line<decltype(f(start))>{f(start), f(end)};
    }
};

// Line<bool> constants
inline Line<bool> LineBoolTrue() { return Line<bool>{true, true}; }
inline Line<bool> LineBoolFalse() { return Line<bool>{false, false}; }

// Sum only for types that support addition
template<typename T>
inline auto LineSum(const Line<T>& line) -> decltype(line.start + line.end) {
    return line.start + line.end;
}

//------------------------------------------------------------------------------
// Rect<T> - Left, right, top, bottom
//------------------------------------------------------------------------------

template<typename T>
struct Rect {
    T left;
    T right;
    T top;
    T bottom;

    static Rect Zero() { return Rect{T{}, T{}, T{}, T{}}; }

    /// Get main start
    T MainStart(FlexDirection dir) const {
        return IsRow(dir) ? left : top;
    }

    T MainEnd(FlexDirection dir) const {
        return IsRow(dir) ? right : bottom;
    }

    T CrossStart(FlexDirection dir) const {
        return IsRow(dir) ? top : left;
    }

    T CrossEnd(FlexDirection dir) const {
        return IsRow(dir) ? bottom : right;
    }

    /// Set main start
    void SetMainStart(FlexDirection dir, T value) {
        if (IsRow(dir)) {
            left = value;
        } else {
            top = value;
        }
    }

    /// Set main end
    void SetMainEnd(FlexDirection dir, T value) {
        if (IsRow(dir)) {
            right = value;
        } else {
            bottom = value;
        }
    }

    /// Set cross start
    void SetCrossStart(FlexDirection dir, T value) {
        if (IsRow(dir)) {
            top = value;
        } else {
            left = value;
        }
    }

    /// Set cross end
    void SetCrossEnd(FlexDirection dir, T value) {
        if (IsRow(dir)) {
            bottom = value;
        } else {
            right = value;
        }
    }

    /// Get horizontal components as Line
    Line<T> HorizontalComponents() const {
        return Line<T>{left, right};
    }

    /// Get vertical components as Line
    Line<T> VerticalComponents() const {
        return Line<T>{top, bottom};
    }

    /// Map function
    template<typename F>
    auto Map(F f) const -> Rect<decltype(f(left))> {
        return Rect<decltype(f(left))>{f(left), f(right), f(top), f(bottom)};
    }

    bool operator==(const Rect& other) const {
        return left == other.left && right == other.right &&
               top == other.top && bottom == other.bottom;
    }

    bool operator!=(const Rect& other) const {
        return !(*this == other);
    }
};

// Operator + only for Rect<float>
inline Rect<float> operator+(const Rect<float>& a, const Rect<float>& b) {
    return Rect<float>{
        a.left + b.left, a.right + b.right,
        a.top + b.top, a.bottom + b.bottom
    };
}

// Specialization for Rect<float>
template<>
inline Rect<float> Rect<float>::Zero() {
    return Rect{0.0f, 0.0f, 0.0f, 0.0f};
}

// Sum functions only for Rect<float>
inline float RectHorizontalAxisSum(const Rect<float>& rect) {
    return rect.left + rect.right;
}

inline float RectVerticalAxisSum(const Rect<float>& rect) {
    return rect.top + rect.bottom;
}

inline Size<float> RectSumAxes(const Rect<float>& rect) {
    return Size<float>{RectHorizontalAxisSum(rect), RectVerticalAxisSum(rect)};
}

inline float RectMainAxisSum(const Rect<float>& rect, FlexDirection dir) {
    return IsRow(dir) ? RectHorizontalAxisSum(rect) : RectVerticalAxisSum(rect);
}

inline float RectCrossAxisSum(const Rect<float>& rect, FlexDirection dir) {
    return IsRow(dir) ? RectVerticalAxisSum(rect) : RectHorizontalAxisSum(rect);
}

inline float RectGridAxisSum(const Rect<float>& rect, AbsoluteAxis axis) {
    return axis == AbsoluteAxis::Horizontal ? RectHorizontalAxisSum(rect) : RectVerticalAxisSum(rect);
}

//------------------------------------------------------------------------------
// MinMax<Min, Max>
//------------------------------------------------------------------------------

template<typename Min, typename Max>
struct MinMax {
    Min min;
    Max max;
};

} // namespace mbink

