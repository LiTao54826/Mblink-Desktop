/**
 * @file resolve.h
 * @brief Resolution utilities for CSS values
 * 
 * Translated from Taffy (https://github.com/DioxusLabs/taffy)
 * Original: src/util/resolve.rs
 */

#pragma once

#include "../types/style.h"
#include "../types/geometry.h"
#include "math.h"
#include <optional>
#include <functional>

namespace mblink {

//------------------------------------------------------------------------------
// MaybeResolve - Resolve optional values
//------------------------------------------------------------------------------

/// Resolve LengthPercentage to optional float
inline std::optional<float> MaybeResolve(
    const LengthPercentage& value,
    std::optional<float> context
) {
    if (context.has_value()) {
        return value.ResolveToOption(*context);
    }
    switch (value.tag) {
        case LengthTag::Length:
            return value.value;
        case LengthTag::Min: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            return (a && b) ? std::optional<float>(std::min(*a, *b)) : std::nullopt;
        }
        case LengthTag::Max: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            return (a && b) ? std::optional<float>(std::max(*a, *b)) : std::nullopt;
        }
        case LengthTag::Clamp: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            auto c = value.func_c ? MaybeResolve(*value.func_c, std::nullopt) : std::nullopt;
            return (a && b && c) ? std::optional<float>(std::max(*a, std::min(*b, *c))) : std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

/// Resolve LengthPercentageAuto to optional float
inline std::optional<float> MaybeResolve(
    const LengthPercentageAuto& value,
    std::optional<float> context
) {
    if (value.tag == LengthTag::Auto) {
        return std::nullopt;
    }
    if (context.has_value()) {
        return value.ResolveToOption(*context);
    }
    switch (value.tag) {
        case LengthTag::Length:
            return value.value;
        case LengthTag::Min: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            return (a && b) ? std::optional<float>(std::min(*a, *b)) : std::nullopt;
        }
        case LengthTag::Max: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            return (a && b) ? std::optional<float>(std::max(*a, *b)) : std::nullopt;
        }
        case LengthTag::Clamp: {
            auto a = value.func_a ? MaybeResolve(*value.func_a, std::nullopt) : std::nullopt;
            auto b = value.func_b ? MaybeResolve(*value.func_b, std::nullopt) : std::nullopt;
            auto c = value.func_c ? MaybeResolve(*value.func_c, std::nullopt) : std::nullopt;
            return (a && b && c) ? std::optional<float>(std::max(*a, std::min(*b, *c))) : std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

// Note: Dimension is an alias for LengthPercentageAuto, so no separate overload needed

//------------------------------------------------------------------------------
// ResolveOrZero - Resolve to float, defaulting to 0
//------------------------------------------------------------------------------

/// Resolve LengthPercentage to float, defaulting to 0
inline float ResolveOrZero(
    const LengthPercentage& value,
    std::optional<float> context
) {
    return MaybeResolve(value, context).value_or(0.0f);
}

/// Resolve LengthPercentageAuto to float, defaulting to 0
inline float ResolveOrZero(
    const LengthPercentageAuto& value,
    std::optional<float> context
) {
    return MaybeResolve(value, context).value_or(0.0f);
}

//------------------------------------------------------------------------------
// Rect resolution
//------------------------------------------------------------------------------

/// Resolve Rect<LengthPercentage> to Rect<float>
inline Rect<float> ResolveOrZero(
    const Rect<LengthPercentage>& rect,
    std::optional<float> context
) {
    return Rect<float>{
        ResolveOrZero(rect.left, context),
        ResolveOrZero(rect.right, context),
        ResolveOrZero(rect.top, context),
        ResolveOrZero(rect.bottom, context)
    };
}

/// Resolve Rect<LengthPercentageAuto> to Rect<float>
inline Rect<float> ResolveOrZero(
    const Rect<LengthPercentageAuto>& rect,
    std::optional<float> context
) {
    return Rect<float>{
        ResolveOrZero(rect.left, context),
        ResolveOrZero(rect.right, context),
        ResolveOrZero(rect.top, context),
        ResolveOrZero(rect.bottom, context)
    };
}

/// Resolve Rect<LengthPercentageAuto> to Rect<optional<float>>
inline Rect<std::optional<float>> MaybeResolve(
    const Rect<LengthPercentageAuto>& rect,
    std::optional<float> context
) {
    return Rect<std::optional<float>>{
        MaybeResolve(rect.left, context),
        MaybeResolve(rect.right, context),
        MaybeResolve(rect.top, context),
        MaybeResolve(rect.bottom, context)
    };
}

//------------------------------------------------------------------------------
// Size resolution
//------------------------------------------------------------------------------

/// Resolve Size<Dimension> to Size<optional<float>>
inline Size<std::optional<float>> MaybeResolve(
    const Size<Dimension>& size,
    Size<std::optional<float>> context
) {
    return Size<std::optional<float>>{
        MaybeResolve(size.width, context.width),
        MaybeResolve(size.height, context.height)
    };
}

/// Resolve Size<LengthPercentage> to Size<float>
inline Size<float> ResolveOrZero(
    const Size<LengthPercentage>& size,
    std::optional<float> context
) {
    return Size<float>{
        ResolveOrZero(size.width, context),
        ResolveOrZero(size.height, context)
    };
}

//------------------------------------------------------------------------------
// Apply aspect ratio
//------------------------------------------------------------------------------

/// Apply aspect ratio to size
inline Size<std::optional<float>> MaybeApplyAspectRatio(
    Size<std::optional<float>> size,
    std::optional<float> aspect_ratio
) {
    if (!aspect_ratio.has_value()) {
        return size;
    }

    // If both dimensions are set, return as-is
    if (size.width.has_value() && size.height.has_value()) {
        return size;
    }

    // If width is set, compute height
    if (size.width.has_value()) {
        return Size<std::optional<float>>{
            size.width,
            std::optional<float>(*size.width / *aspect_ratio)
        };
    }

    // If height is set, compute width
    if (size.height.has_value()) {
        return Size<std::optional<float>>{
            std::optional<float>(*size.height * *aspect_ratio),
            size.height
        };
    }

    return size;
}

//------------------------------------------------------------------------------
// Clamp utilities
//------------------------------------------------------------------------------

/// Clamp size between min and max
inline Size<std::optional<float>> MaybeClamp(
    Size<std::optional<float>> size,
    Size<std::optional<float>> min_size,
    Size<std::optional<float>> max_size
) {
    return Size<std::optional<float>>{
        MaybeMath::MaybeClamp(size.width, min_size.width, max_size.width),
        MaybeMath::MaybeClamp(size.height, min_size.height, max_size.height)
    };
}

/// Clamp float size between optional min and max
inline Size<float> Clamp(
    Size<float> size,
    Size<std::optional<float>> min_size,
    Size<std::optional<float>> max_size
) {
    return Size<float>{
        f32_clamp(
            size.width,
            min_size.width.value_or(-INFINITY),
            max_size.width.value_or(INFINITY)
        ),
        f32_clamp(
            size.height,
            min_size.height.value_or(-INFINITY),
            max_size.height.value_or(INFINITY)
        )
    };
}

//------------------------------------------------------------------------------
// Size operations with optional
//------------------------------------------------------------------------------

/// Add optional size to size
inline Size<std::optional<float>> MaybeAdd(
    Size<std::optional<float>> size,
    Size<float> other
) {
    return Size<std::optional<float>>{
        size.width.has_value() ? std::optional<float>(*size.width + other.width) : std::nullopt,
        size.height.has_value() ? std::optional<float>(*size.height + other.height) : std::nullopt
    };
}

/// Subtract size from optional size
inline Size<std::optional<float>> MaybeSub(
    Size<std::optional<float>> size,
    Size<float> other
) {
    return Size<std::optional<float>>{
        size.width.has_value() ? std::optional<float>(*size.width - other.width) : std::nullopt,
        size.height.has_value() ? std::optional<float>(*size.height - other.height) : std::nullopt
    };
}

/// Max of optional size with definite size
inline Size<std::optional<float>> MaybeMax(
    Size<std::optional<float>> size,
    Size<float> other
) {
    return Size<std::optional<float>>{
        size.width.has_value() ? std::optional<float>(f32_max(*size.width, other.width)) : std::nullopt,
        size.height.has_value() ? std::optional<float>(f32_max(*size.height, other.height)) : std::nullopt
    };
}

/// Unwrap optional size or return default
inline Size<float> UnwrapOr(
    Size<std::optional<float>> size,
    Size<float> def
) {
    return Size<float>{
        size.width.value_or(def.width),
        size.height.value_or(def.height)
    };
}

/// Or operation for optional sizes
inline Size<std::optional<float>> MaybeOr(
    Size<std::optional<float>> a,
    Size<std::optional<float>> b
) {
    return Size<std::optional<float>>{
        a.width.has_value() ? a.width : b.width,
        a.height.has_value() ? a.height : b.height
    };
}

/// Max of two optional sizes
/// Note: If left-hand value is None, returns None (following Taffy's MaybeMath semantics)
inline Size<std::optional<float>> MaybeMaxOpt(
    Size<std::optional<float>> a,
    Size<std::optional<float>> b
) {
    auto max_opt = [](std::optional<float> x, std::optional<float> y) -> std::optional<float> {
        if (x.has_value() && y.has_value()) {
            return f32_max(*x, *y);
        }
        // If x (left-hand) is None, return None (not y)
        // If x has value but y is None, return x
        return x;
    };
    return Size<std::optional<float>>{
        max_opt(a.width, b.width),
        max_opt(a.height, b.height)
    };
}

/// Add two Rect<float>
inline Rect<float> RectAdd(const Rect<float>& a, const Rect<float>& b) {
    return Rect<float>{
        a.left + b.left,
        a.right + b.right,
        a.top + b.top,
        a.bottom + b.bottom
    };
}

} // namespace mblink

