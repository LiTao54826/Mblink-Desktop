/**
 * @file test_object_fit_properties.cpp
 * @brief Property-based tests for CSS object-fit and object-position properties
 * 
 * This file implements property-based testing for object-fit and object-position
 * functionality. Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: css-media-layout-properties**
 * **Validates: Requirements 1.1-1.6, 2.1-2.5**
 */

#include <gtest/gtest.h>
#include "core/render/css/style_resolver.h"
#include "core/render/objects/render_object.h"
#include "core/render/image/image_fit.h"
#include "core/dom/element.h"
#include "core/dom/document.h"
#include <random>
#include <vector>
#include <string>
#include <cmath>

using namespace mbink;

// Random number generator for property tests
class ObjectFitPropertyTestRng {
public:
    ObjectFitPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    float randFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen_);
    }
    
    std::string randObjectFitValue() {
        static const std::vector<std::string> values = {
            "fill", "contain", "cover", "none", "scale-down"
        };
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randInvalidObjectFitValue() {
        // Note: CSS parsers typically trim whitespace, so " contain" would become "contain"
        // which is valid. We only test truly invalid values here.
        static const std::vector<std::string> values = {
            "stretch", "fit", "auto", "inherit", "initial", "unset",
            "FILL", "CONTAIN", "Cover", "NONE", "Scale-Down",
            "cover cover", "123", "fill-contain", "none-cover"
        };
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randObjectPositionKeyword() {
        static const std::vector<std::string> values = {
            "center", "top", "bottom", "left", "right",
            "top left", "top right", "bottom left", "bottom right",
            "left top", "right top", "left bottom", "right bottom",
            "center center", "center top", "center bottom",
            "left center", "right center"
        };
        return values[randInt(0, static_cast<int>(values.size()) - 1)];
    }
    
    std::string randObjectPositionPercentage() {
        int x = randInt(0, 100);
        int y = randInt(0, 100);
        return std::to_string(x) + "% " + std::to_string(y) + "%";
    }
    
    std::string randObjectPositionLength() {
        int x = randInt(0, 500);
        int y = randInt(0, 500);
        return std::to_string(x) + "px " + std::to_string(y) + "px";
    }
    
    std::string randObjectPositionMixed() {
        static const std::vector<std::string> keywords = {"center", "left", "right"};
        std::string keyword = keywords[randInt(0, static_cast<int>(keywords.size()) - 1)];
        int px = randInt(0, 100);
        return keyword + " " + std::to_string(px) + "px";
    }
    
private:
    std::mt19937 gen_;
};

class ObjectFitPropertyTest : public ::testing::Test {
protected:
    ObjectFitPropertyTestRng rng_;
    StyleResolver style_resolver_;
    static constexpr int NUM_ITERATIONS = 100;
    
    // Helper to create a mock element for testing
    std::shared_ptr<Element> CreateMockElement(const std::string& tag_name) {
        auto doc = std::make_shared<Document>();
        return doc->CreateElement(tag_name);
    }
    
    // Helper to apply inline style and resolve
    ComputedStyle ApplyStyleAndResolve(std::shared_ptr<Element> element, 
                                        const std::string& style_str) {
        element->SetAttribute("style", style_str);
        return style_resolver_.ResolveStyle(element, nullptr);
    }
};

/**
 * **Feature: css-media-layout-properties, Property 1: Object Fit Contain Preserves Aspect Ratio**
 * 
 * For any valid object-fit value, the StyleResolver should correctly parse
 * and store the value in ComputedStyle.
 * 
 * **Validates: Requirements 1.1-1.6**
 */
TEST_F(ObjectFitPropertyTest, ValidObjectFitValuesAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string object_fit_value = rng_.randObjectFitValue();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-fit: " + object_fit_value);
        
        EXPECT_EQ(style.object_fit, object_fit_value)
            << "object-fit should be parsed correctly. "
            << "Input: \"" << object_fit_value << "\", "
            << "Got: \"" << style.object_fit << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 1: Object Fit Contain Preserves Aspect Ratio**
 * 
 * For any invalid object-fit value, the StyleResolver should ignore it
 * and keep the default value "fill".
 * 
 * **Validates: Requirements 1.6**
 */
TEST_F(ObjectFitPropertyTest, InvalidObjectFitValuesAreIgnored) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string invalid_value = rng_.randInvalidObjectFitValue();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-fit: " + invalid_value);
        
        EXPECT_EQ(style.object_fit, "fill")
            << "Invalid object-fit should be ignored, keeping default 'fill'. "
            << "Input: \"" << invalid_value << "\", "
            << "Got: \"" << style.object_fit << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 3: Object Position Centers by Default**
 * 
 * For any element without explicit object-position, the default value
 * should be "50% 50%" (centered).
 * 
 * **Validates: Requirements 2.5**
 */
TEST_F(ObjectFitPropertyTest, ObjectPositionDefaultsToCenter) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Apply only object-fit, not object-position
        std::string object_fit_value = rng_.randObjectFitValue();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-fit: " + object_fit_value);
        
        EXPECT_EQ(style.object_position, "50% 50%")
            << "object-position should default to '50% 50%' (centered). "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 3: Object Position Centers by Default**
 * 
 * For any valid object-position keyword value, the StyleResolver should
 * correctly parse and store the value.
 * 
 * **Validates: Requirements 2.1**
 */
TEST_F(ObjectFitPropertyTest, ObjectPositionKeywordsAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string position_value = rng_.randObjectPositionKeyword();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-position: " + position_value);
        
        EXPECT_EQ(style.object_position, position_value)
            << "object-position keywords should be parsed correctly. "
            << "Input: \"" << position_value << "\", "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 3: Object Position Centers by Default**
 * 
 * For any valid object-position percentage value, the StyleResolver should
 * correctly parse and store the value.
 * 
 * **Validates: Requirements 2.2**
 */
TEST_F(ObjectFitPropertyTest, ObjectPositionPercentagesAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string position_value = rng_.randObjectPositionPercentage();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-position: " + position_value);
        
        EXPECT_EQ(style.object_position, position_value)
            << "object-position percentages should be parsed correctly. "
            << "Input: \"" << position_value << "\", "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 3: Object Position Centers by Default**
 * 
 * For any valid object-position length value, the StyleResolver should
 * correctly parse and store the value.
 * 
 * **Validates: Requirements 2.3**
 */
TEST_F(ObjectFitPropertyTest, ObjectPositionLengthsAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string position_value = rng_.randObjectPositionLength();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-position: " + position_value);
        
        EXPECT_EQ(style.object_position, position_value)
            << "object-position lengths should be parsed correctly. "
            << "Input: \"" << position_value << "\", "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 3: Object Position Centers by Default**
 * 
 * For any valid object-position mixed value (keyword + length), the StyleResolver
 * should correctly parse and store the value.
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(ObjectFitPropertyTest, ObjectPositionMixedValuesAreParsed) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string position_value = rng_.randObjectPositionMixed();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-position: " + position_value);
        
        EXPECT_EQ(style.object_position, position_value)
            << "object-position mixed values should be parsed correctly. "
            << "Input: \"" << position_value << "\", "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 1 & 3: Combined object-fit and object-position**
 * 
 * For any combination of valid object-fit and object-position values,
 * both should be correctly parsed and stored independently.
 * 
 * **Validates: Requirements 1.1-1.6, 2.1-2.5**
 */
TEST_F(ObjectFitPropertyTest, ObjectFitAndPositionCombined) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string fit_value = rng_.randObjectFitValue();
        std::string position_value = rng_.randObjectPositionKeyword();
        
        auto element = CreateMockElement("img");
        ComputedStyle style = ApplyStyleAndResolve(element, 
            "object-fit: " + fit_value + "; object-position: " + position_value);
        
        EXPECT_EQ(style.object_fit, fit_value)
            << "object-fit should be parsed correctly in combined style. "
            << "Input: \"" << fit_value << "\", "
            << "Got: \"" << style.object_fit << "\"";
        
        EXPECT_EQ(style.object_position, position_value)
            << "object-position should be parsed correctly in combined style. "
            << "Input: \"" << position_value << "\", "
            << "Got: \"" << style.object_position << "\"";
    }
}

/**
 * **Feature: css-media-layout-properties, Property 1: Object Fit Default Value**
 * 
 * For any element without explicit object-fit, the default value should be "fill".
 * 
 * **Validates: Requirements 1.6**
 */
TEST_F(ObjectFitPropertyTest, ObjectFitDefaultsToFill) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto element = CreateMockElement("img");
        // Apply some other style, but not object-fit
        ComputedStyle style = ApplyStyleAndResolve(element, "width: 100px");
        
        EXPECT_EQ(style.object_fit, "fill")
            << "object-fit should default to 'fill'. "
            << "Got: \"" << style.object_fit << "\"";
    }
}


/**
 * **Feature: css-media-layout-properties, Property 2: Object Fit Cover Fills Container**
 * 
 * For any image with object-fit: cover, the displayed image should completely
 * cover the container with no empty space. The destination rectangle should
 * always equal the container rectangle.
 * 
 * **Validates: Requirements 1.3**
 */
TEST_F(ObjectFitPropertyTest, ObjectFitCoverFillsContainer) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random image dimensions (10-1000 pixels)
        float image_width = static_cast<float>(rng_.randInt(10, 1000));
        float image_height = static_cast<float>(rng_.randInt(10, 1000));
        
        // Generate random container dimensions (10-1000 pixels)
        float container_x = static_cast<float>(rng_.randInt(0, 500));
        float container_y = static_cast<float>(rng_.randInt(0, 500));
        float container_width = static_cast<float>(rng_.randInt(10, 1000));
        float container_height = static_cast<float>(rng_.randInt(10, 1000));
        
        SkRect container_rect = SkRect::MakeXYWH(
            container_x, container_y, container_width, container_height);
        
        // Calculate object-fit: cover result
        ObjectFitResult result = CalculateObjectFit(
            image_width, image_height, container_rect, "cover", "50% 50%");
        
        // Property: destination rect should exactly match container rect
        // (cover means the image completely fills the container)
        EXPECT_FLOAT_EQ(result.dst_rect.x(), container_rect.x())
            << "object-fit: cover destination x should match container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
        
        EXPECT_FLOAT_EQ(result.dst_rect.y(), container_rect.y())
            << "object-fit: cover destination y should match container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
        
        EXPECT_FLOAT_EQ(result.dst_rect.width(), container_rect.width())
            << "object-fit: cover destination width should match container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
        
        EXPECT_FLOAT_EQ(result.dst_rect.height(), container_rect.height())
            << "object-fit: cover destination height should match container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
        
        // Property: source rect should have the same aspect ratio as container
        // (since we're cropping the image to fill the container)
        if (result.src_rect.width() > 0 && result.src_rect.height() > 0) {
            float src_aspect = result.src_rect.width() / result.src_rect.height();
            float container_aspect = container_width / container_height;
            
            EXPECT_NEAR(src_aspect, container_aspect, 0.001f)
                << "object-fit: cover source rect aspect ratio should match container. "
                << "Image: " << image_width << "x" << image_height << ", "
                << "Container: " << container_width << "x" << container_height << ", "
                << "Source aspect: " << src_aspect << ", Container aspect: " << container_aspect;
        }
    }
}

/**
 * **Feature: css-media-layout-properties, Property 1: Object Fit Contain Preserves Aspect Ratio**
 * 
 * For any image with object-fit: contain, the displayed image should preserve
 * its original aspect ratio while fitting within the container.
 * 
 * **Validates: Requirements 1.2**
 */
TEST_F(ObjectFitPropertyTest, ObjectFitContainPreservesAspectRatio) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random image dimensions (10-1000 pixels)
        float image_width = static_cast<float>(rng_.randInt(10, 1000));
        float image_height = static_cast<float>(rng_.randInt(10, 1000));
        
        // Generate random container dimensions (10-1000 pixels)
        float container_x = static_cast<float>(rng_.randInt(0, 500));
        float container_y = static_cast<float>(rng_.randInt(0, 500));
        float container_width = static_cast<float>(rng_.randInt(10, 1000));
        float container_height = static_cast<float>(rng_.randInt(10, 1000));
        
        SkRect container_rect = SkRect::MakeXYWH(
            container_x, container_y, container_width, container_height);
        
        // Calculate object-fit: contain result
        ObjectFitResult result = CalculateObjectFit(
            image_width, image_height, container_rect, "contain", "50% 50%");
        
        // Property: destination rect should preserve original aspect ratio
        float original_aspect = image_width / image_height;
        float result_aspect = result.dst_rect.width() / result.dst_rect.height();
        
        EXPECT_NEAR(result_aspect, original_aspect, 0.001f)
            << "object-fit: contain should preserve aspect ratio. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height << ", "
            << "Original aspect: " << original_aspect << ", Result aspect: " << result_aspect;
        
        // Property: destination rect should fit within container
        EXPECT_LE(result.dst_rect.width(), container_width + 0.001f)
            << "object-fit: contain width should fit within container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
        
        EXPECT_LE(result.dst_rect.height(), container_height + 0.001f)
            << "object-fit: contain height should fit within container. "
            << "Image: " << image_width << "x" << image_height << ", "
            << "Container: " << container_width << "x" << container_height;
    }
}
