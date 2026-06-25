/**
 * @file list_marker.h
 * @brief List marker rendering utilities
 * 
 * Provides utilities for rendering CSS list markers including:
 * - Roman numeral conversion (lower-roman, upper-roman)
 * - Alpha conversion (lower-alpha, upper-alpha)
 * - Marker text generation for various list-style-type values
 */

#pragma once

#include <string>
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"

namespace mblink {

// Forward declarations
struct ComputedStyle;
struct LayoutInfo;
struct Box;

/**
 * @brief List marker type enumeration
 */
enum class ListMarkerType {
    NONE,
    DISC,
    CIRCLE,
    SQUARE,
    DECIMAL,
    DECIMAL_LEADING_ZERO,
    LOWER_ROMAN,
    UPPER_ROMAN,
    LOWER_ALPHA,
    UPPER_ALPHA
};

/**
 * @brief Convert list-style-type string to enum
 * @param type_str The CSS list-style-type value
 * @return The corresponding ListMarkerType enum value
 */
ListMarkerType ParseListStyleType(const std::string& type_str);

/**
 * @brief Convert a number to lowercase Roman numerals
 * @param num The number to convert (1-3999)
 * @return The Roman numeral string (e.g., "i", "ii", "iii", "iv")
 */
std::string ToLowerRoman(int num);

/**
 * @brief Convert a number to uppercase Roman numerals
 * @param num The number to convert (1-3999)
 * @return The Roman numeral string (e.g., "I", "II", "III", "IV")
 */
std::string ToUpperRoman(int num);

/**
 * @brief Convert a number to lowercase alphabetic marker
 * @param num The number to convert (1-based)
 * @return The alphabetic string (e.g., "a", "b", ..., "z", "aa", "ab")
 */
std::string ToLowerAlpha(int num);

/**
 * @brief Convert a number to uppercase alphabetic marker
 * @param num The number to convert (1-based)
 * @return The alphabetic string (e.g., "A", "B", ..., "Z", "AA", "AB")
 */
std::string ToUpperAlpha(int num);

/**
 * @brief Generate marker text for a given list-style-type and index
 * @param type The list marker type
 * @param index The 1-based item index
 * @return The marker text (e.g., "1.", "i.", "a.")
 */
std::string GenerateMarkerText(ListMarkerType type, int index);

/**
 * @brief Paint a list marker for an <li> element
 * 
 * This function handles all list marker rendering including:
 * - Graphical markers (disc, circle, square)
 * - Text markers (decimal, roman, alpha)
 * - list-style-position (inside/outside)
 * - list-style-image (custom marker images)
 * 
 * @param canvas The Skia canvas to draw on
 * @param style The computed style of the <li> element
 * @param layout The layout info of the <li> element
 * @param box The box model of the <li> element
 * @param item_index The 1-based index of the item in the list
 * @param parent_tag The tag name of the parent list element ("ul" or "ol")
 */
void PaintListMarker(
    SkCanvas* canvas,
    const ComputedStyle& style,
    const LayoutInfo& layout,
    const Box& box,
    int item_index,
    const std::string& parent_tag
);

} // namespace mblink
