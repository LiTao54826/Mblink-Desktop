/**
 * @file image_fit.h
 * @brief CSS object-fit and object-position helper functions
 * 
 * Implements the CSS object-fit and object-position properties for replaced elements
 * (images, videos). These properties control how content is sized and positioned
 * within its container.
 * 
 * CSS Specification:
 * - object-fit: https://www.w3.org/TR/css-images-3/#the-object-fit
 * - object-position: https://www.w3.org/TR/css-images-3/#the-object-position
 */

#pragma once

#include <string>
#include "include/core/SkRect.h"
#include "include/core/SkSize.h"

namespace lightui {

/**
 * @brief Result of object-fit calculation
 * 
 * Contains the source rectangle (portion of image to use) and
 * destination rectangle (where to draw on canvas).
 */
struct ObjectFitResult {
    SkRect src_rect;   ///< Source rectangle in image coordinates
    SkRect dst_rect;   ///< Destination rectangle in container coordinates
};

/**
 * @brief Parsed object-position values
 */
struct ObjectPositionOffset {
    float x;  ///< X offset (can be negative)
    float y;  ///< Y offset (can be negative)
};

/**
 * @brief Calculate source and destination rectangles based on object-fit
 * 
 * This function implements the CSS object-fit property, which determines how
 * replaced content (like images) should be resized to fit its container.
 * 
 * @param image_width Original image width
 * @param image_height Original image height
 * @param container_rect The container rectangle to fit the image into
 * @param object_fit The object-fit value: "fill", "contain", "cover", "none", "scale-down"
 * @param object_position The object-position value (e.g., "50% 50%", "center", "top left")
 * @return ObjectFitResult containing source and destination rectangles
 * 
 * Object-fit values:
 * - fill: Stretch to fill container, may distort aspect ratio (default)
 * - contain: Scale to fit within container, preserve aspect ratio, may leave empty space
 * - cover: Scale to cover container, preserve aspect ratio, may clip content
 * - none: Display at natural size, may clip or leave empty space
 * - scale-down: Use 'none' or 'contain', whichever results in smaller size
 */
ObjectFitResult CalculateObjectFit(
    float image_width,
    float image_height,
    const SkRect& container_rect,
    const std::string& object_fit,
    const std::string& object_position
);

/**
 * @brief Parse object-position string to x/y offsets
 * 
 * Parses CSS object-position values and converts them to pixel offsets.
 * 
 * @param object_position The object-position value string
 * @param available_width Available width for percentage calculations
 * @param available_height Available height for percentage calculations
 * @return ObjectPositionOffset containing x and y offsets
 * 
 * Supported formats:
 * - Keywords: "center", "top", "bottom", "left", "right"
 * - Two keywords: "top left", "center center", etc.
 * - Percentages: "50% 50%", "0% 100%"
 * - Lengths: "10px 20px"
 * - Mixed: "center 10px", "left 50%"
 */
ObjectPositionOffset ParseObjectPosition(
    const std::string& object_position,
    float available_width,
    float available_height
);

} // namespace lightui
