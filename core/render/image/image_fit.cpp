/**
 * @file image_fit.cpp
 * @brief CSS object-fit and object-position implementation
 * 
 * Implements the CSS object-fit and object-position properties for replaced elements.
 * 
 * Requirements:
 * - 1.1: object-fit: fill - stretch to fill, may distort
 * - 1.2: object-fit: contain - fit within, preserve aspect ratio
 * - 1.3: object-fit: cover - cover container, preserve aspect ratio, may clip
 * - 1.4: object-fit: none - natural size, no scaling
 * - 1.5: object-fit: scale-down - smaller of none or contain
 * - 2.1-2.4: object-position - keywords, percentages, lengths, mixed
 */

#include "image_fit.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace mbink {

namespace {

/**
 * @brief Trim whitespace from string
 */
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

/**
 * @brief Parse a single position value (keyword, percentage, or length)
 * @param value The value string
 * @param available_size Available size for percentage calculations
 * @param is_horizontal True if this is for horizontal axis
 * @return The offset value in pixels
 */
float ParseSinglePositionValue(const std::string& value, float available_size, bool is_horizontal) {
    std::string trimmed = Trim(value);
    
    // Handle keywords
    if (trimmed == "center") {
        return available_size * 0.5f;
    }
    if (trimmed == "left" || trimmed == "top") {
        return 0.0f;
    }
    if (trimmed == "right" || trimmed == "bottom") {
        return available_size;
    }
    
    // Handle percentage
    if (trimmed.back() == '%') {
        try {
            float percent = std::stof(trimmed.substr(0, trimmed.length() - 1));
            return available_size * (percent / 100.0f);
        } catch (...) {
            return available_size * 0.5f;  // Default to center on parse error
        }
    }
    
    // Handle length (px)
    size_t px_pos = trimmed.find("px");
    if (px_pos != std::string::npos) {
        try {
            return std::stof(trimmed.substr(0, px_pos));
        } catch (...) {
            return available_size * 0.5f;  // Default to center on parse error
        }
    }
    
    // Try to parse as plain number (treat as pixels)
    try {
        return std::stof(trimmed);
    } catch (...) {
        return available_size * 0.5f;  // Default to center on parse error
    }
}

/**
 * @brief Split position string into two parts
 */
std::pair<std::string, std::string> SplitPositionString(const std::string& position) {
    std::string trimmed = Trim(position);
    
    // Find the split point - look for space that separates two values
    // Handle cases like "50% 50%", "center center", "10px 20px"
    size_t split_pos = std::string::npos;
    int paren_depth = 0;
    
    for (size_t i = 0; i < trimmed.length(); ++i) {
        char c = trimmed[i];
        if (c == '(') paren_depth++;
        else if (c == ')') paren_depth--;
        else if (c == ' ' && paren_depth == 0) {
            // Check if this is a meaningful split (not just extra whitespace)
            size_t next_non_space = trimmed.find_first_not_of(' ', i);
            if (next_non_space != std::string::npos) {
                split_pos = i;
                break;
            }
        }
    }
    
    if (split_pos == std::string::npos) {
        // Single value - use it for both axes or apply CSS rules
        std::string single = trimmed;
        
        // For single keywords, CSS has specific rules
        if (single == "center") {
            return {"center", "center"};
        }
        if (single == "left" || single == "right") {
            return {single, "center"};
        }
        if (single == "top" || single == "bottom") {
            return {"center", single};
        }
        
        // For single percentage or length, apply to horizontal, center vertical
        return {single, "center"};
    }
    
    std::string first = Trim(trimmed.substr(0, split_pos));
    std::string second = Trim(trimmed.substr(split_pos + 1));
    
    return {first, second};
}

} // anonymous namespace

ObjectPositionOffset ParseObjectPosition(
    const std::string& object_position,
    float available_width,
    float available_height
) {
    ObjectPositionOffset result = {0.0f, 0.0f};
    
    if (object_position.empty()) {
        // Default: 50% 50% (centered)
        result.x = available_width * 0.5f;
        result.y = available_height * 0.5f;
        return result;
    }
    
    auto [x_str, y_str] = SplitPositionString(object_position);
    
    // Check if we need to swap based on keywords
    // CSS allows "top left" which means x=left, y=top
    bool x_is_vertical = (x_str == "top" || x_str == "bottom");
    bool y_is_horizontal = (y_str == "left" || y_str == "right");
    
    if (x_is_vertical && y_is_horizontal) {
        std::swap(x_str, y_str);
    }
    
    result.x = ParseSinglePositionValue(x_str, available_width, true);
    result.y = ParseSinglePositionValue(y_str, available_height, false);
    
    return result;
}

ObjectFitResult CalculateObjectFit(
    float image_width,
    float image_height,
    const SkRect& container_rect,
    const std::string& object_fit,
    const std::string& object_position
) {
    ObjectFitResult result;
    
    // Handle edge cases
    if (image_width <= 0 || image_height <= 0) {
        result.src_rect = SkRect::MakeEmpty();
        result.dst_rect = SkRect::MakeEmpty();
        return result;
    }
    
    float container_width = container_rect.width();
    float container_height = container_rect.height();
    
    if (container_width <= 0 || container_height <= 0) {
        result.src_rect = SkRect::MakeEmpty();
        result.dst_rect = SkRect::MakeEmpty();
        return result;
    }
    
    // Source rect is always the full image for most cases
    result.src_rect = SkRect::MakeWH(image_width, image_height);
    
    if (object_fit == "fill") {
        // Stretch to fill container, ignore aspect ratio
        result.dst_rect = container_rect;
    }
    else if (object_fit == "contain") {
        // Fit within container, preserve aspect ratio
        float scale = std::min(
            container_width / image_width,
            container_height / image_height
        );
        float scaled_width = image_width * scale;
        float scaled_height = image_height * scale;
        
        // Calculate position offset
        float extra_width = container_width - scaled_width;
        float extra_height = container_height - scaled_height;
        
        ObjectPositionOffset offset = ParseObjectPosition(
            object_position, extra_width, extra_height);
        
        result.dst_rect = SkRect::MakeXYWH(
            container_rect.x() + offset.x,
            container_rect.y() + offset.y,
            scaled_width,
            scaled_height
        );
    }
    else if (object_fit == "cover") {
        // Cover container, preserve aspect ratio, clip excess
        float scale = std::max(
            container_width / image_width,
            container_height / image_height
        );
        float scaled_width = image_width * scale;
        float scaled_height = image_height * scale;
        
        // Calculate how much we need to clip
        float excess_width = scaled_width - container_width;
        float excess_height = scaled_height - container_height;
        
        // Get position offset for the excess area
        ObjectPositionOffset offset = ParseObjectPosition(
            object_position, excess_width, excess_height);
        
        // Calculate source rect (the portion of image to show)
        float src_x = offset.x / scale;
        float src_y = offset.y / scale;
        float src_width = container_width / scale;
        float src_height = container_height / scale;
        
        result.src_rect = SkRect::MakeXYWH(src_x, src_y, src_width, src_height);
        result.dst_rect = container_rect;
    }
    else if (object_fit == "none") {
        // Display at natural size, no scaling
        float extra_width = container_width - image_width;
        float extra_height = container_height - image_height;
        
        ObjectPositionOffset offset = ParseObjectPosition(
            object_position, extra_width, extra_height);
        
        result.dst_rect = SkRect::MakeXYWH(
            container_rect.x() + offset.x,
            container_rect.y() + offset.y,
            image_width,
            image_height
        );
    }
    else if (object_fit == "scale-down") {
        // Use 'none' or 'contain', whichever results in smaller size
        if (image_width <= container_width && image_height <= container_height) {
            // Image fits naturally, use 'none'
            return CalculateObjectFit(image_width, image_height, container_rect, 
                                      "none", object_position);
        } else {
            // Image too large, use 'contain'
            return CalculateObjectFit(image_width, image_height, container_rect, 
                                      "contain", object_position);
        }
    }
    else {
        // Unknown value, default to "fill"
        result.dst_rect = container_rect;
    }
    
    return result;
}

} // namespace mbink
