/**
 * @file list_marker.cpp
 * @brief List marker rendering utilities implementation
 */

#include "list_marker.h"
#include "render_object.h"
#include "core/render/painters/box_renderer.h"
#include "core/render/utils/color.h"
#include "text/font_manager.h"
#include "image/image_loader.h"
#include "image/image_cache.h"
#include <algorithm>
#include <cctype>

namespace mblink {

ListMarkerType ParseListStyleType(const std::string& type_str) {
    if (type_str == "none") return ListMarkerType::NONE;
    if (type_str == "disc") return ListMarkerType::DISC;
    if (type_str == "circle") return ListMarkerType::CIRCLE;
    if (type_str == "square") return ListMarkerType::SQUARE;
    if (type_str == "decimal") return ListMarkerType::DECIMAL;
    if (type_str == "decimal-leading-zero") return ListMarkerType::DECIMAL_LEADING_ZERO;
    if (type_str == "lower-roman") return ListMarkerType::LOWER_ROMAN;
    if (type_str == "upper-roman") return ListMarkerType::UPPER_ROMAN;
    if (type_str == "lower-alpha") return ListMarkerType::LOWER_ALPHA;
    if (type_str == "upper-alpha") return ListMarkerType::UPPER_ALPHA;
    
    // Default to disc for unrecognized values
    return ListMarkerType::DISC;
}

std::string ToLowerRoman(int num) {
    if (num <= 0 || num > 3999) {
        // Fall back to decimal for out-of-range values
        return std::to_string(num);
    }
    
    static const std::vector<std::pair<int, std::string>> roman = {
        {1000, "m"}, {900, "cm"}, {500, "d"}, {400, "cd"},
        {100, "c"}, {90, "xc"}, {50, "l"}, {40, "xl"},
        {10, "x"}, {9, "ix"}, {5, "v"}, {4, "iv"}, {1, "i"}
    };
    
    std::string result;
    for (const auto& [value, symbol] : roman) {
        while (num >= value) {
            result += symbol;
            num -= value;
        }
    }
    return result;
}

std::string ToUpperRoman(int num) {
    std::string lower = ToLowerRoman(num);
    std::string upper;
    upper.reserve(lower.size());
    for (char c : lower) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return upper;
}

std::string ToLowerAlpha(int num) {
    if (num <= 0) {
        return std::to_string(num);
    }
    
    std::string result;
    while (num > 0) {
        num--;  // Convert to 0-based
        result = static_cast<char>('a' + (num % 26)) + result;
        num /= 26;
    }
    return result;
}

std::string ToUpperAlpha(int num) {
    std::string lower = ToLowerAlpha(num);
    std::string upper;
    upper.reserve(lower.size());
    for (char c : lower) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return upper;
}

std::string GenerateMarkerText(ListMarkerType type, int index) {
    switch (type) {
        case ListMarkerType::NONE:
            return "";
        case ListMarkerType::DECIMAL:
            return std::to_string(index) + ".";
        case ListMarkerType::DECIMAL_LEADING_ZERO:
            if (index < 10) {
                return "0" + std::to_string(index) + ".";
            }
            return std::to_string(index) + ".";
        case ListMarkerType::LOWER_ROMAN:
            return ToLowerRoman(index) + ".";
        case ListMarkerType::UPPER_ROMAN:
            return ToUpperRoman(index) + ".";
        case ListMarkerType::LOWER_ALPHA:
            return ToLowerAlpha(index) + ".";
        case ListMarkerType::UPPER_ALPHA:
            return ToUpperAlpha(index) + ".";
        default:
            // Graphical markers don't have text
            return "";
    }
}

void PaintListMarker(
    SkCanvas* canvas,
    const ComputedStyle& style,
    const LayoutInfo& layout,
    const Box& box,
    int item_index,
    const std::string& parent_tag
) {
    if (!canvas) return;
    
    // Get list-style properties from computed style
    ListMarkerType marker_type = ParseListStyleType(style.list_style_type);
    
    // If list-style-type is none, don't render any marker
    if (marker_type == ListMarkerType::NONE) {
        return;
    }
    
    // Determine default marker type based on parent tag if not explicitly set
    // For <ul>, default is disc; for <ol>, default is decimal
    if (style.list_style_type == "disc" && parent_tag == "ol") {
        // If using default disc but parent is ol, use decimal instead
        marker_type = ListMarkerType::DECIMAL;
    }
    
    bool is_inside = (style.list_style_position == "inside");
    
    // Set up paint
    SkPaint paint;
    if (!style.color.empty()) {
        paint.setColor(Color::Parse(style.color));
    } else {
        paint.setColor(SK_ColorBLACK);
    }
    paint.setAntiAlias(true);
    
    // Calculate marker position
    // For "outside" position: marker is outside the content box
    // For "inside" position: marker is inside the content box
    float marker_y = box.content_y + style.font_size * 0.8f;  // Baseline position
    
    // Try to render list-style-image first
    if (!style.list_style_image.empty()) {
        // Load the image from cache or file/URL
        auto& image_cache = ImageCache::GetInstance();
        auto image = image_cache.Get(style.list_style_image);
        
        if (!image) {
            // Try to load the image (supports local files and network URLs)
            auto loaded_image = ImageLoader::LoadFromUrl(style.list_style_image);
            if (loaded_image) {
                image_cache.Put(style.list_style_image, loaded_image);
                image = loaded_image;
            }
        }
        
        if (image) {
            // Draw the image as marker
            float img_size = style.font_size;  // Use font size as marker size
            float marker_x;
            
            if (is_inside) {
                marker_x = box.content_x;
            } else {
                marker_x = box.content_x - img_size - 8.0f;
            }
            
            float img_y = marker_y - img_size * 0.8f;
            
            SkRect dest_rect = SkRect::MakeXYWH(marker_x, img_y, img_size, img_size);
            canvas->drawImageRect(image, dest_rect, SkSamplingOptions());
            return;  // Image rendered, don't render text/shape marker
        }
        // If image failed to load, fall through to render the fallback marker type
    }
    
    // Check if this is a graphical marker (disc, circle, square)
    bool is_graphical = (marker_type == ListMarkerType::DISC ||
                         marker_type == ListMarkerType::CIRCLE ||
                         marker_type == ListMarkerType::SQUARE);
    
    if (is_graphical) {
        // Render graphical marker
        float bullet_radius = 3.0f;
        float bullet_x, bullet_y;
        
        if (is_inside) {
            bullet_x = box.content_x + bullet_radius + 2.0f;
        } else {
            bullet_x = box.content_x - 8.0f - bullet_radius;
        }
        bullet_y = marker_y - style.font_size * 0.3f;
        
        switch (marker_type) {
            case ListMarkerType::DISC:
                paint.setStyle(SkPaint::kFill_Style);
                canvas->drawCircle(bullet_x, bullet_y, bullet_radius, paint);
                break;
                
            case ListMarkerType::CIRCLE:
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setStrokeWidth(1.5f);
                canvas->drawCircle(bullet_x, bullet_y, bullet_radius, paint);
                break;
                
            case ListMarkerType::SQUARE:
                paint.setStyle(SkPaint::kFill_Style);
                {
                    SkRect rect = SkRect::MakeXYWH(
                        bullet_x - bullet_radius,
                        bullet_y - bullet_radius,
                        bullet_radius * 2,
                        bullet_radius * 2
                    );
                    canvas->drawRect(rect, paint);
                }
                break;
                
            default:
                break;
        }
    } else {
        // Render text marker
        std::string marker_text = GenerateMarkerText(marker_type, item_index);
        
        if (!marker_text.empty()) {
            // Set up font
            FontDescriptor desc;
            desc.family = style.font_family;
            desc.size = style.font_size;
            desc.weight = FontWeight::NORMAL;
            desc.style = FontStyle::NORMAL;
            SkFont font = FontManager::GetInstance().LoadFont(desc);
            
            paint.setStyle(SkPaint::kFill_Style);
            
            float marker_x;
            if (is_inside) {
                marker_x = box.content_x;
            } else {
                // Right-align the marker text to the left of content
                // Measure text width
                SkRect text_bounds;
                font.measureText(marker_text.c_str(), marker_text.size(), SkTextEncoding::kUTF8, &text_bounds);
                float text_width = text_bounds.width();
                marker_x = box.content_x - text_width - 5.0f;
            }
            
            canvas->drawString(marker_text.c_str(), marker_x, marker_y, font, paint);
        }
    }
}

} // namespace mblink
