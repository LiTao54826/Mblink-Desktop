/**
 * @file math.h
 * @brief Math utilities for layout computation
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/util/math.rs
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

namespace mbink {

//------------------------------------------------------------------------------
// Float utilities
//------------------------------------------------------------------------------

/// Returns the maximum of two floats, handling NaN
inline float f32_max(float a, float b) {
    if (std::isnan(a)) return b;
    if (std::isnan(b)) return a;
    return std::max(a, b);
}

/// Returns the minimum of two floats, handling NaN
inline float f32_min(float a, float b) {
    if (std::isnan(a)) return b;
    if (std::isnan(b)) return a;
    return std::min(a, b);
}

/// Clamp a value between min and max
inline float f32_clamp(float value, float min_val, float max_val) {
    return f32_max(min_val, f32_min(value, max_val));
}

//------------------------------------------------------------------------------
// Optional utilities
//------------------------------------------------------------------------------

/// Maybe add two optional values
inline std::optional<float> MaybeAdd(std::optional<float> a, std::optional<float> b) {
    if (a.has_value() && b.has_value()) {
        return *a + *b;
    }
    return a.has_value() ? a : b;
}

/// Maybe subtract two optional values
inline std::optional<float> MaybeSub(std::optional<float> a, std::optional<float> b) {
    if (a.has_value() && b.has_value()) {
        return *a - *b;
    }
    return a;
}

/// Maybe max of two optional values
inline std::optional<float> MaybeMax(std::optional<float> a, std::optional<float> b) {
    if (a.has_value() && b.has_value()) {
        return f32_max(*a, *b);
    }
    return a.has_value() ? a : b;
}

/// Maybe min of two optional values
inline std::optional<float> MaybeMin(std::optional<float> a, std::optional<float> b) {
    if (a.has_value() && b.has_value()) {
        return f32_min(*a, *b);
    }
    return a.has_value() ? a : b;
}

/// Unwrap optional or return default
inline float UnwrapOr(std::optional<float> opt, float def) {
    return opt.value_or(def);
}

/// Unwrap optional or return 0
inline float UnwrapOrZero(std::optional<float> opt) {
    return opt.value_or(0.0f);
}

//------------------------------------------------------------------------------
// MaybeMath trait implementation
//------------------------------------------------------------------------------

/// Extension methods for optional float operations
struct MaybeMath {
    /// Maybe clamp value between min and max
    /// CSS spec: when min > max, min wins (apply max first, then min)
    static std::optional<float> MaybeClamp(
        std::optional<float> value,
        std::optional<float> min_val,
        std::optional<float> max_val
    ) {
        if (!value.has_value()) return std::nullopt;

        float result = *value;
        // Apply max first, then min - ensures min wins when min > max
        if (max_val.has_value()) {
            result = f32_min(result, *max_val);
        }
        if (min_val.has_value()) {
            result = f32_max(result, *min_val);
        }
        return result;
    }

    /// Maybe add a definite value to an optional
    static std::optional<float> MaybeAdd(std::optional<float> opt, float val) {
        return opt.has_value() ? std::optional<float>(*opt + val) : std::nullopt;
    }

    /// Maybe subtract a definite value from an optional
    static std::optional<float> MaybeSub(std::optional<float> opt, float val) {
        return opt.has_value() ? std::optional<float>(*opt - val) : std::nullopt;
    }
};

//------------------------------------------------------------------------------
// Rounding utilities
//------------------------------------------------------------------------------

/// Round a float to the nearest integer
inline float Round(float value) {
    return std::round(value);
}

/// Round a float to a specific scale (for subpixel rendering)
inline float RoundToScale(float value, float scale) {
    return std::round(value * scale) / scale;
}

/// Floor a float
inline float Floor(float value) {
    return std::floor(value);
}

/// Ceil a float
inline float Ceil(float value) {
    return std::ceil(value);
}

//------------------------------------------------------------------------------
// Aspect ratio utilities
//------------------------------------------------------------------------------

/// Apply aspect ratio to compute width from height
inline std::optional<float> ApplyAspectRatioWidth(
    std::optional<float> height,
    std::optional<float> aspect_ratio
) {
    if (height.has_value() && aspect_ratio.has_value()) {
        return *height * *aspect_ratio;
    }
    return std::nullopt;
}

/// Apply aspect ratio to compute height from width
inline std::optional<float> ApplyAspectRatioHeight(
    std::optional<float> width,
    std::optional<float> aspect_ratio
) {
    if (width.has_value() && aspect_ratio.has_value()) {
        return *width / *aspect_ratio;
    }
    return std::nullopt;
}

} // namespace mbink

