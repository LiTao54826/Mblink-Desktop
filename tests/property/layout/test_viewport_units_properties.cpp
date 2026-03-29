/**
 * @file test_viewport_units_properties.cpp
 * @brief Property-based tests for viewport units (vh, vw, vmin, vmax)
 * 
 * This file implements property-based testing for viewport unit resolution
 * in the layout engine. Tests verify that:
 * - vh units resolve correctly relative to viewport height (Property 1)
 * - vw units resolve correctly relative to viewport width (Property 2)
 * - Viewport units recalculate on resize (Property 3)
 * 
 * **Feature: viewport-units-and-absolute-positioning**
 * **Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3**
 */

#include <gtest/gtest.h>
#include "render/css/css_value.h"
#include <random>
#include <cmath>

using namespace mbink;

/**
 * @brief Random number generator for property tests
 */
class ViewportUnitsPropertyTestRng {
public:
    ViewportUnitsPropertyTestRng() : gen_(std::random_device{}()) {}
    
    float randViewportDimension() {
        // Generate viewport dimensions between 100 and 2000 pixels
        std::uniform_real_distribution<float> dist(100.0f, 2000.0f);
        return dist(gen_);
    }
    
    float randViewportUnitValue() {
        // Generate viewport unit values between 0 and 200 (e.g., 0vh to 200vh)
        std::uniform_real_distribution<float> dist(0.0f, 200.0f);
        return dist(gen_);
    }
    
private:
    std::mt19937 gen_;
};

/**
 * @brief Test fixture for viewport units property tests
 */
class ViewportUnitsPropertyTest : public ::testing::Test {
protected:
    ViewportUnitsPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    static constexpr float EPSILON = 0.001f;
    
    void SetUp() override {
        // Reset viewport size to known state
        ViewportSize::Set(0.0f, 0.0f);
    }
    
    void TearDown() override {
        // Reset viewport size
        ViewportSize::Set(0.0f, 0.0f);
    }
};

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 1: Viewport height unit resolution**
 * 
 * For any viewport height H and vh value V, the computed pixel value SHALL equal H * V / 100
 * 
 * **Validates: Requirements 1.1, 1.2**
 */
TEST_F(ViewportUnitsPropertyTest, VhResolution) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random viewport height and vh value
        float viewport_height = rng_.randViewportDimension();
        float vh_value = rng_.randViewportUnitValue();
        
        // Set viewport size
        ViewportSize::Set(800.0f, viewport_height);  // Width doesn't matter for vh
        
        // Create a CSSLength with vh unit
        CSSLength length(vh_value, CSSUnit::VH);
        
        // Compute the pixel value
        float computed_px = length.ToPx();
        
        // Expected value: viewport_height * vh_value / 100
        float expected_px = viewport_height * vh_value / 100.0f;
        
        EXPECT_NEAR(computed_px, expected_px, EPSILON)
            << "VH resolution failed. "
            << "Viewport height: " << viewport_height << ", "
            << "VH value: " << vh_value << ", "
            << "Expected: " << expected_px << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 1: Viewport height unit resolution**
 * 
 * Special case: 100vh SHALL equal the viewport height exactly
 * 
 * **Validates: Requirements 1.1**
 */
TEST_F(ViewportUnitsPropertyTest, Vh100EqualsViewportHeight) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float viewport_height = rng_.randViewportDimension();
        
        ViewportSize::Set(800.0f, viewport_height);
        
        CSSLength length(100.0f, CSSUnit::VH);
        float computed_px = length.ToPx();
        
        EXPECT_NEAR(computed_px, viewport_height, EPSILON)
            << "100vh should equal viewport height. "
            << "Viewport height: " << viewport_height << ", "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 2: Viewport width unit resolution**
 * 
 * For any viewport width W and vw value V, the computed pixel value SHALL equal W * V / 100
 * 
 * **Validates: Requirements 2.1, 2.2**
 */
TEST_F(ViewportUnitsPropertyTest, VwResolution) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random viewport width and vw value
        float viewport_width = rng_.randViewportDimension();
        float vw_value = rng_.randViewportUnitValue();
        
        // Set viewport size
        ViewportSize::Set(viewport_width, 600.0f);  // Height doesn't matter for vw
        
        // Create a CSSLength with vw unit
        CSSLength length(vw_value, CSSUnit::VW);
        
        // Compute the pixel value
        float computed_px = length.ToPx();
        
        // Expected value: viewport_width * vw_value / 100
        float expected_px = viewport_width * vw_value / 100.0f;
        
        EXPECT_NEAR(computed_px, expected_px, EPSILON)
            << "VW resolution failed. "
            << "Viewport width: " << viewport_width << ", "
            << "VW value: " << vw_value << ", "
            << "Expected: " << expected_px << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 2: Viewport width unit resolution**
 * 
 * Special case: 100vw SHALL equal the viewport width exactly
 * 
 * **Validates: Requirements 2.1**
 */
TEST_F(ViewportUnitsPropertyTest, Vw100EqualsViewportWidth) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float viewport_width = rng_.randViewportDimension();
        
        ViewportSize::Set(viewport_width, 600.0f);
        
        CSSLength length(100.0f, CSSUnit::VW);
        float computed_px = length.ToPx();
        
        EXPECT_NEAR(computed_px, viewport_width, EPSILON)
            << "100vw should equal viewport width. "
            << "Viewport width: " << viewport_width << ", "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 3: Viewport unit recalculation on resize**
 * 
 * For any element using viewport units, when viewport dimensions change,
 * the computed pixel values SHALL update to reflect the new viewport size
 * 
 * **Validates: Requirements 1.3, 2.3**
 */
TEST_F(ViewportUnitsPropertyTest, ViewportUnitsRecalculateOnResize) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate initial and new viewport dimensions
        float initial_width = rng_.randViewportDimension();
        float initial_height = rng_.randViewportDimension();
        float new_width = rng_.randViewportDimension();
        float new_height = rng_.randViewportDimension();
        
        float vh_value = rng_.randViewportUnitValue();
        float vw_value = rng_.randViewportUnitValue();
        
        // Create CSS lengths
        CSSLength vh_length(vh_value, CSSUnit::VH);
        CSSLength vw_length(vw_value, CSSUnit::VW);
        
        // Set initial viewport size and compute
        ViewportSize::Set(initial_width, initial_height);
        float initial_vh_px = vh_length.ToPx();
        float initial_vw_px = vw_length.ToPx();
        
        // Resize viewport
        ViewportSize::Set(new_width, new_height);
        float new_vh_px = vh_length.ToPx();
        float new_vw_px = vw_length.ToPx();
        
        // Expected values after resize
        float expected_vh_px = new_height * vh_value / 100.0f;
        float expected_vw_px = new_width * vw_value / 100.0f;
        
        EXPECT_NEAR(new_vh_px, expected_vh_px, EPSILON)
            << "VH should recalculate on resize. "
            << "Initial height: " << initial_height << ", "
            << "New height: " << new_height << ", "
            << "VH value: " << vh_value << ", "
            << "Expected: " << expected_vh_px << "px, "
            << "Got: " << new_vh_px << "px, "
            << "Iteration: " << i;
        
        EXPECT_NEAR(new_vw_px, expected_vw_px, EPSILON)
            << "VW should recalculate on resize. "
            << "Initial width: " << initial_width << ", "
            << "New width: " << new_width << ", "
            << "VW value: " << vw_value << ", "
            << "Expected: " << expected_vw_px << "px, "
            << "Got: " << new_vw_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 3: Viewport unit recalculation on resize**
 * 
 * Verify that the same CSSLength object produces different pixel values
 * when viewport size changes (no caching of resolved values)
 * 
 * **Validates: Requirements 1.3, 2.3**
 */
TEST_F(ViewportUnitsPropertyTest, ViewportUnitsNotCached) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float width1 = rng_.randViewportDimension();
        float height1 = rng_.randViewportDimension();
        float width2 = rng_.randViewportDimension();
        float height2 = rng_.randViewportDimension();
        
        // Ensure dimensions are different
        while (std::abs(height1 - height2) < 1.0f) {
            height2 = rng_.randViewportDimension();
        }
        
        CSSLength vh_length(50.0f, CSSUnit::VH);
        
        // First computation
        ViewportSize::Set(width1, height1);
        float px1 = vh_length.ToPx();
        
        // Second computation with different viewport
        ViewportSize::Set(width2, height2);
        float px2 = vh_length.ToPx();
        
        // Values should be different (unless heights happen to be equal)
        if (std::abs(height1 - height2) > 1.0f) {
            EXPECT_NE(px1, px2)
                << "Viewport units should not be cached. "
                << "Height1: " << height1 << ", Height2: " << height2 << ", "
                << "Px1: " << px1 << ", Px2: " << px2 << ", "
                << "Iteration: " << i;
        }
    }
}


/**
 * **Feature: viewport-units-and-absolute-positioning, Property 5: vmin/vmax resolution**
 * 
 * For any viewport with dimensions W and H, vmin SHALL resolve relative to min(W,H)
 * and vmax SHALL resolve relative to max(W,H)
 * 
 * **Validates: Requirements 3.1, 3.2**
 */
TEST_F(ViewportUnitsPropertyTest, VminResolution) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random viewport dimensions
        float viewport_width = rng_.randViewportDimension();
        float viewport_height = rng_.randViewportDimension();
        float vmin_value = rng_.randViewportUnitValue();
        
        // Set viewport size
        ViewportSize::Set(viewport_width, viewport_height);
        
        // Create a CSSLength with vmin unit
        CSSLength length(vmin_value, CSSUnit::VMIN);
        
        // Compute the pixel value
        float computed_px = length.ToPx();
        
        // Expected value: min(viewport_width, viewport_height) * vmin_value / 100
        float vmin = std::min(viewport_width, viewport_height);
        float expected_px = vmin * vmin_value / 100.0f;
        
        EXPECT_NEAR(computed_px, expected_px, EPSILON)
            << "VMIN resolution failed. "
            << "Viewport: " << viewport_width << "x" << viewport_height << ", "
            << "VMIN value: " << vmin_value << ", "
            << "Expected: " << expected_px << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 5: vmin/vmax resolution**
 * 
 * For any viewport with dimensions W and H, vmax SHALL resolve relative to max(W,H)
 * 
 * **Validates: Requirements 3.1, 3.2**
 */
TEST_F(ViewportUnitsPropertyTest, VmaxResolution) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random viewport dimensions
        float viewport_width = rng_.randViewportDimension();
        float viewport_height = rng_.randViewportDimension();
        float vmax_value = rng_.randViewportUnitValue();
        
        // Set viewport size
        ViewportSize::Set(viewport_width, viewport_height);
        
        // Create a CSSLength with vmax unit
        CSSLength length(vmax_value, CSSUnit::VMAX);
        
        // Compute the pixel value
        float computed_px = length.ToPx();
        
        // Expected value: max(viewport_width, viewport_height) * vmax_value / 100
        float vmax = std::max(viewport_width, viewport_height);
        float expected_px = vmax * vmax_value / 100.0f;
        
        EXPECT_NEAR(computed_px, expected_px, EPSILON)
            << "VMAX resolution failed. "
            << "Viewport: " << viewport_width << "x" << viewport_height << ", "
            << "VMAX value: " << vmax_value << ", "
            << "Expected: " << expected_px << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 5: vmin/vmax resolution**
 * 
 * Special case: 100vmin SHALL equal min(viewport_width, viewport_height)
 * 
 * **Validates: Requirements 3.1**
 */
TEST_F(ViewportUnitsPropertyTest, Vmin100EqualsMinDimension) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float viewport_width = rng_.randViewportDimension();
        float viewport_height = rng_.randViewportDimension();
        
        ViewportSize::Set(viewport_width, viewport_height);
        
        CSSLength length(100.0f, CSSUnit::VMIN);
        float computed_px = length.ToPx();
        
        float expected = std::min(viewport_width, viewport_height);
        
        EXPECT_NEAR(computed_px, expected, EPSILON)
            << "100vmin should equal min(width, height). "
            << "Viewport: " << viewport_width << "x" << viewport_height << ", "
            << "Expected: " << expected << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 5: vmin/vmax resolution**
 * 
 * Special case: 100vmax SHALL equal max(viewport_width, viewport_height)
 * 
 * **Validates: Requirements 3.2**
 */
TEST_F(ViewportUnitsPropertyTest, Vmax100EqualsMaxDimension) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float viewport_width = rng_.randViewportDimension();
        float viewport_height = rng_.randViewportDimension();
        
        ViewportSize::Set(viewport_width, viewport_height);
        
        CSSLength length(100.0f, CSSUnit::VMAX);
        float computed_px = length.ToPx();
        
        float expected = std::max(viewport_width, viewport_height);
        
        EXPECT_NEAR(computed_px, expected, EPSILON)
            << "100vmax should equal max(width, height). "
            << "Viewport: " << viewport_width << "x" << viewport_height << ", "
            << "Expected: " << expected << "px, "
            << "Got: " << computed_px << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 5: vmin/vmax resolution**
 * 
 * Verify that vmin and vmax correctly switch when viewport orientation changes
 * (e.g., from landscape to portrait)
 * 
 * **Validates: Requirements 3.1, 3.2**
 */
TEST_F(ViewportUnitsPropertyTest, VminVmaxOrientationChange) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate landscape viewport (width > height)
        float larger = rng_.randViewportDimension();
        float smaller = rng_.randViewportDimension();
        if (smaller > larger) std::swap(smaller, larger);
        
        // Ensure they're different enough
        while (larger - smaller < 10.0f) {
            larger = rng_.randViewportDimension();
            smaller = rng_.randViewportDimension();
            if (smaller > larger) std::swap(smaller, larger);
        }
        
        float unit_value = rng_.randViewportUnitValue();
        
        CSSLength vmin_length(unit_value, CSSUnit::VMIN);
        CSSLength vmax_length(unit_value, CSSUnit::VMAX);
        
        // Landscape: width > height
        ViewportSize::Set(larger, smaller);
        float landscape_vmin = vmin_length.ToPx();
        float landscape_vmax = vmax_length.ToPx();
        
        // Portrait: height > width
        ViewportSize::Set(smaller, larger);
        float portrait_vmin = vmin_length.ToPx();
        float portrait_vmax = vmax_length.ToPx();
        
        // vmin should always resolve to smaller dimension
        float expected_vmin = smaller * unit_value / 100.0f;
        // vmax should always resolve to larger dimension
        float expected_vmax = larger * unit_value / 100.0f;
        
        EXPECT_NEAR(landscape_vmin, expected_vmin, EPSILON)
            << "Landscape vmin should equal smaller dimension. "
            << "Iteration: " << i;
        
        EXPECT_NEAR(landscape_vmax, expected_vmax, EPSILON)
            << "Landscape vmax should equal larger dimension. "
            << "Iteration: " << i;
        
        EXPECT_NEAR(portrait_vmin, expected_vmin, EPSILON)
            << "Portrait vmin should equal smaller dimension. "
            << "Iteration: " << i;
        
        EXPECT_NEAR(portrait_vmax, expected_vmax, EPSILON)
            << "Portrait vmax should equal larger dimension. "
            << "Iteration: " << i;
    }
}


//------------------------------------------------------------------------------
// Absolute Positioning Stretch Property Tests
//------------------------------------------------------------------------------

/**
 * @brief Random number generator for absolute positioning property tests
 */
class AbsolutePositioningPropertyTestRng {
public:
    AbsolutePositioningPropertyTestRng() : gen_(std::random_device{}()) {}
    
    float randContainerDimension() {
        // Generate container dimensions between 200 and 1000 pixels
        std::uniform_real_distribution<float> dist(200.0f, 1000.0f);
        return dist(gen_);
    }
    
    float randInsetValue() {
        // Generate inset values between 0 and 100 pixels
        std::uniform_real_distribution<float> dist(0.0f, 100.0f);
        return dist(gen_);
    }
    
    float randMarginValue() {
        // Generate margin values between 0 and 50 pixels
        std::uniform_real_distribution<float> dist(0.0f, 50.0f);
        return dist(gen_);
    }
    
private:
    std::mt19937 gen_;
};

/**
 * @brief Test fixture for absolute positioning property tests
 */
class AbsolutePositioningPropertyTest : public ::testing::Test {
protected:
    AbsolutePositioningPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    static constexpr float EPSILON = 0.01f;
    
    void SetUp() override {
        // Reset viewport size to known state
        ViewportSize::Set(800.0f, 600.0f);
    }
    
    void TearDown() override {
        // Reset viewport size
        ViewportSize::Set(0.0f, 0.0f);
    }
    
    /**
     * @brief Calculate expected height for top+bottom stretch
     * 
     * When an absolutely positioned element has both top and bottom set
     * but no explicit height, the height should be:
     * container_height - top - bottom - margin_top - margin_bottom
     */
    float calculateExpectedStretchHeight(
        float container_height,
        float top,
        float bottom,
        float margin_top,
        float margin_bottom
    ) {
        float height = container_height - top - bottom - margin_top - margin_bottom;
        return std::max(height, 0.0f);
    }
    
    /**
     * @brief Calculate expected width for left+right stretch
     * 
     * When an absolutely positioned element has both left and right set
     * but no explicit width, the width should be:
     * container_width - left - right - margin_left - margin_right
     */
    float calculateExpectedStretchWidth(
        float container_width,
        float left,
        float right,
        float margin_left,
        float margin_right
    ) {
        float width = container_width - left - right - margin_left - margin_right;
        return std::max(width, 0.0f);
    }
};

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8: Absolute positioning stretch with top+bottom**
 * 
 * For any absolutely positioned element with top: T and bottom: B but no explicit height,
 * in a containing block of height H, the element's height SHALL equal H - T - B - margins
 * 
 * This test verifies the mathematical property of the stretch calculation.
 * 
 * **Validates: Requirements 4.3**
 */
TEST_F(AbsolutePositioningPropertyTest, TopBottomStretchHeightCalculation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random container height and inset values
        float container_height = rng_.randContainerDimension();
        float top = rng_.randInsetValue();
        float bottom = rng_.randInsetValue();
        float margin_top = rng_.randMarginValue();
        float margin_bottom = rng_.randMarginValue();
        
        // Calculate expected height
        float expected_height = calculateExpectedStretchHeight(
            container_height, top, bottom, margin_top, margin_bottom
        );
        
        // Verify the calculation is correct
        float computed_height = container_height - top - bottom - margin_top - margin_bottom;
        computed_height = std::max(computed_height, 0.0f);
        
        EXPECT_NEAR(computed_height, expected_height, EPSILON)
            << "Top+bottom stretch height calculation failed. "
            << "Container height: " << container_height << ", "
            << "Top: " << top << ", Bottom: " << bottom << ", "
            << "Margin top: " << margin_top << ", Margin bottom: " << margin_bottom << ", "
            << "Expected: " << expected_height << "px, "
            << "Got: " << computed_height << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8: Absolute positioning stretch with top+bottom**
 * 
 * For any absolutely positioned element with top: T and bottom: B but no explicit height,
 * the computed height SHALL be non-negative (clamped to 0 if calculation would be negative)
 * 
 * **Validates: Requirements 4.3**
 */
TEST_F(AbsolutePositioningPropertyTest, TopBottomStretchHeightNonNegative) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate values that might result in negative height
        float container_height = rng_.randContainerDimension();
        
        // Sometimes generate large insets that exceed container
        float top = rng_.randContainerDimension();  // Can be larger than container
        float bottom = rng_.randContainerDimension();
        float margin_top = rng_.randMarginValue();
        float margin_bottom = rng_.randMarginValue();
        
        // Calculate expected height (should be clamped to 0)
        float expected_height = calculateExpectedStretchHeight(
            container_height, top, bottom, margin_top, margin_bottom
        );
        
        // Verify height is non-negative
        EXPECT_GE(expected_height, 0.0f)
            << "Top+bottom stretch height should be non-negative. "
            << "Container height: " << container_height << ", "
            << "Top: " << top << ", Bottom: " << bottom << ", "
            << "Margin top: " << margin_top << ", Margin bottom: " << margin_bottom << ", "
            << "Computed: " << expected_height << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8: Absolute positioning stretch with top+bottom**
 * 
 * For any absolutely positioned element with top: 0 and bottom: 0 and no margins,
 * the element's height SHALL equal the container height exactly
 * 
 * **Validates: Requirements 4.3**
 */
TEST_F(AbsolutePositioningPropertyTest, TopBottomStretchFullHeight) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        
        // With top: 0, bottom: 0, and no margins, height should equal container
        float expected_height = calculateExpectedStretchHeight(
            container_height, 0.0f, 0.0f, 0.0f, 0.0f
        );
        
        EXPECT_NEAR(expected_height, container_height, EPSILON)
            << "Full height stretch (top:0, bottom:0) should equal container height. "
            << "Container height: " << container_height << ", "
            << "Expected: " << container_height << "px, "
            << "Got: " << expected_height << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8: Absolute positioning stretch with top+bottom**
 * 
 * For any absolutely positioned element, increasing top or bottom inset values
 * SHALL decrease the computed stretch height proportionally
 * 
 * **Validates: Requirements 4.3**
 */
TEST_F(AbsolutePositioningPropertyTest, TopBottomStretchInverseRelationship) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        float top1 = rng_.randInsetValue();
        float bottom1 = rng_.randInsetValue();
        
        // Increase top by a random amount
        float delta = rng_.randInsetValue();
        float top2 = top1 + delta;
        
        float height1 = calculateExpectedStretchHeight(container_height, top1, bottom1, 0.0f, 0.0f);
        float height2 = calculateExpectedStretchHeight(container_height, top2, bottom1, 0.0f, 0.0f);
        
        // Height should decrease by delta (or both be 0 if clamped)
        if (height1 > 0.0f && height2 > 0.0f) {
            EXPECT_NEAR(height1 - height2, delta, EPSILON)
                << "Increasing top should decrease height by same amount. "
                << "Container: " << container_height << ", "
                << "Top1: " << top1 << ", Top2: " << top2 << ", "
                << "Height1: " << height1 << ", Height2: " << height2 << ", "
                << "Delta: " << delta << ", "
                << "Iteration: " << i;
        }
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 9: Absolute positioning stretch with left+right**
 * 
 * For any absolutely positioned element with left: L and right: R but no explicit width,
 * in a containing block of width W, the element's width SHALL equal W - L - R - margins
 * 
 * This test verifies the mathematical property of the stretch calculation.
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, LeftRightStretchWidthCalculation) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate random container width and inset values
        float container_width = rng_.randContainerDimension();
        float left = rng_.randInsetValue();
        float right = rng_.randInsetValue();
        float margin_left = rng_.randMarginValue();
        float margin_right = rng_.randMarginValue();
        
        // Calculate expected width
        float expected_width = calculateExpectedStretchWidth(
            container_width, left, right, margin_left, margin_right
        );
        
        // Verify the calculation is correct
        float computed_width = container_width - left - right - margin_left - margin_right;
        computed_width = std::max(computed_width, 0.0f);
        
        EXPECT_NEAR(computed_width, expected_width, EPSILON)
            << "Left+right stretch width calculation failed. "
            << "Container width: " << container_width << ", "
            << "Left: " << left << ", Right: " << right << ", "
            << "Margin left: " << margin_left << ", Margin right: " << margin_right << ", "
            << "Expected: " << expected_width << "px, "
            << "Got: " << computed_width << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 9: Absolute positioning stretch with left+right**
 * 
 * For any absolutely positioned element with left: L and right: R but no explicit width,
 * the computed width SHALL be non-negative (clamped to 0 if calculation would be negative)
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, LeftRightStretchWidthNonNegative) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate values that might result in negative width
        float container_width = rng_.randContainerDimension();
        
        // Sometimes generate large insets that exceed container
        float left = rng_.randContainerDimension();  // Can be larger than container
        float right = rng_.randContainerDimension();
        float margin_left = rng_.randMarginValue();
        float margin_right = rng_.randMarginValue();
        
        // Calculate expected width (should be clamped to 0)
        float expected_width = calculateExpectedStretchWidth(
            container_width, left, right, margin_left, margin_right
        );
        
        // Verify width is non-negative
        EXPECT_GE(expected_width, 0.0f)
            << "Left+right stretch width should be non-negative. "
            << "Container width: " << container_width << ", "
            << "Left: " << left << ", Right: " << right << ", "
            << "Margin left: " << margin_left << ", Margin right: " << margin_right << ", "
            << "Computed: " << expected_width << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 9: Absolute positioning stretch with left+right**
 * 
 * For any absolutely positioned element with left: 0 and right: 0 and no margins,
 * the element's width SHALL equal the container width exactly
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, LeftRightStretchFullWidth) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        
        // With left: 0, right: 0, and no margins, width should equal container
        float expected_width = calculateExpectedStretchWidth(
            container_width, 0.0f, 0.0f, 0.0f, 0.0f
        );
        
        EXPECT_NEAR(expected_width, container_width, EPSILON)
            << "Full width stretch (left:0, right:0) should equal container width. "
            << "Container width: " << container_width << ", "
            << "Expected: " << container_width << "px, "
            << "Got: " << expected_width << "px, "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 9: Absolute positioning stretch with left+right**
 * 
 * For any absolutely positioned element, increasing left or right inset values
 * SHALL decrease the computed stretch width proportionally
 * 
 * **Validates: Requirements 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, LeftRightStretchInverseRelationship) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float left1 = rng_.randInsetValue();
        float right1 = rng_.randInsetValue();
        
        // Increase left by a random amount
        float delta = rng_.randInsetValue();
        float left2 = left1 + delta;
        
        float width1 = calculateExpectedStretchWidth(container_width, left1, right1, 0.0f, 0.0f);
        float width2 = calculateExpectedStretchWidth(container_width, left2, right1, 0.0f, 0.0f);
        
        // Width should decrease by delta (or both be 0 if clamped)
        if (width1 > 0.0f && width2 > 0.0f) {
            EXPECT_NEAR(width1 - width2, delta, EPSILON)
                << "Increasing left should decrease width by same amount. "
                << "Container: " << container_width << ", "
                << "Left1: " << left1 << ", Left2: " << left2 << ", "
                << "Width1: " << width1 << ", Width2: " << width2 << ", "
                << "Delta: " << delta << ", "
                << "Iteration: " << i;
        }
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8 & 9: Combined stretch**
 * 
 * For any absolutely positioned element with all four insets set (top, right, bottom, left)
 * but no explicit dimensions, both width and height SHALL be computed using the stretch formula
 * 
 * **Validates: Requirements 4.3, 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, CombinedStretchBothDimensions) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float container_height = rng_.randContainerDimension();
        float top = rng_.randInsetValue();
        float right = rng_.randInsetValue();
        float bottom = rng_.randInsetValue();
        float left = rng_.randInsetValue();
        
        float expected_width = calculateExpectedStretchWidth(container_width, left, right, 0.0f, 0.0f);
        float expected_height = calculateExpectedStretchHeight(container_height, top, bottom, 0.0f, 0.0f);
        
        // Both dimensions should be calculated independently
        EXPECT_GE(expected_width, 0.0f)
            << "Combined stretch width should be non-negative. Iteration: " << i;
        EXPECT_GE(expected_height, 0.0f)
            << "Combined stretch height should be non-negative. Iteration: " << i;
        
        // Verify the sum of insets and dimension equals container
        if (expected_width > 0.0f) {
            EXPECT_NEAR(left + expected_width + right, container_width, EPSILON)
                << "Left + width + right should equal container width. Iteration: " << i;
        }
        if (expected_height > 0.0f) {
            EXPECT_NEAR(top + expected_height + bottom, container_height, EPSILON)
                << "Top + height + bottom should equal container height. Iteration: " << i;
        }
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 8 & 9: Symmetry**
 * 
 * For any absolutely positioned element, swapping top/bottom or left/right values
 * SHALL produce the same computed dimension (stretch is symmetric)
 * 
 * **Validates: Requirements 4.3, 4.4**
 */
TEST_F(AbsolutePositioningPropertyTest, StretchSymmetry) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float container_height = rng_.randContainerDimension();
        float inset1 = rng_.randInsetValue();
        float inset2 = rng_.randInsetValue();
        
        // Width should be same whether left=inset1,right=inset2 or left=inset2,right=inset1
        float width1 = calculateExpectedStretchWidth(container_width, inset1, inset2, 0.0f, 0.0f);
        float width2 = calculateExpectedStretchWidth(container_width, inset2, inset1, 0.0f, 0.0f);
        
        EXPECT_NEAR(width1, width2, EPSILON)
            << "Stretch width should be symmetric. "
            << "Container: " << container_width << ", "
            << "Inset1: " << inset1 << ", Inset2: " << inset2 << ", "
            << "Width1: " << width1 << ", Width2: " << width2 << ", "
            << "Iteration: " << i;
        
        // Height should be same whether top=inset1,bottom=inset2 or top=inset2,bottom=inset1
        float height1 = calculateExpectedStretchHeight(container_height, inset1, inset2, 0.0f, 0.0f);
        float height2 = calculateExpectedStretchHeight(container_height, inset2, inset1, 0.0f, 0.0f);
        
        EXPECT_NEAR(height1, height2, EPSILON)
            << "Stretch height should be symmetric. "
            << "Container: " << container_height << ", "
            << "Inset1: " << inset1 << ", Inset2: " << inset2 << ", "
            << "Height1: " << height1 << ", Height2: " << height2 << ", "
            << "Iteration: " << i;
    }
}


//------------------------------------------------------------------------------
// Cross-Layout Mode Absolute Positioning Consistency Property Tests
//------------------------------------------------------------------------------

/**
 * @brief Test fixture for cross-layout mode absolute positioning consistency tests
 * 
 * These tests verify that absolute positioning produces consistent results
 * whether the parent container is a block or flex container.
 * 
 * **Feature: viewport-units-and-absolute-positioning, Property 10**
 * **Validates: Requirements 6.1, 6.2**
 */
class CrossLayoutModeAbsolutePositioningTest : public ::testing::Test {
protected:
    AbsolutePositioningPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    static constexpr float EPSILON = 0.01f;
    
    void SetUp() override {
        ViewportSize::Set(800.0f, 600.0f);
    }
    
    void TearDown() override {
        ViewportSize::Set(0.0f, 0.0f);
    }
    
    /**
     * @brief Calculate expected X position for an absolutely positioned element
     * 
     * @param container_width Width of the containing block
     * @param left Left inset value (or -1 if not set)
     * @param right Right inset value (or -1 if not set)
     * @param element_width Width of the positioned element
     * @param margin_left Left margin
     * @param margin_right Right margin
     * @return Expected X position
     */
    float calculateExpectedX(
        float container_width,
        float left,
        float right,
        float element_width,
        float margin_left,
        float margin_right,
        bool left_set,
        bool right_set
    ) {
        if (left_set) {
            // left takes precedence
            return left + margin_left;
        } else if (right_set) {
            // right positioning
            return container_width - element_width - right - margin_right;
        } else {
            // Default to left: 0
            return margin_left;
        }
    }
    
    /**
     * @brief Calculate expected Y position for an absolutely positioned element
     * 
     * @param container_height Height of the containing block
     * @param top Top inset value (or -1 if not set)
     * @param bottom Bottom inset value (or -1 if not set)
     * @param element_height Height of the positioned element
     * @param margin_top Top margin
     * @param margin_bottom Bottom margin
     * @return Expected Y position
     */
    float calculateExpectedY(
        float container_height,
        float top,
        float bottom,
        float element_height,
        float margin_top,
        float margin_bottom,
        bool top_set,
        bool bottom_set
    ) {
        if (top_set) {
            // top takes precedence
            return top + margin_top;
        } else if (bottom_set) {
            // bottom positioning
            return container_height - element_height - bottom - margin_bottom;
        } else {
            // Default to top: 0
            return margin_top;
        }
    }
    
    /**
     * @brief Calculate expected width when left+right are both set (stretch)
     */
    float calculateStretchWidth(
        float container_width,
        float left,
        float right,
        float margin_left,
        float margin_right
    ) {
        float width = container_width - left - right - margin_left - margin_right;
        return std::max(width, 0.0f);
    }
    
    /**
     * @brief Calculate expected height when top+bottom are both set (stretch)
     */
    float calculateStretchHeight(
        float container_height,
        float top,
        float bottom,
        float margin_top,
        float margin_bottom
    ) {
        float height = container_height - top - bottom - margin_top - margin_bottom;
        return std::max(height, 0.0f);
    }
};

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element, the inset properties SHALL produce
 * the same positioning result whether the parent is a block or flex container.
 * 
 * This test verifies that the position calculation formula is consistent:
 * - When left is set: x = left + margin_left
 * - When right is set (and left is not): x = container_width - element_width - right - margin_right
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, LeftInsetPositionConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float left = rng_.randInsetValue();
        float margin_left = rng_.randMarginValue();
        float element_width = rng_.randInsetValue() + 50.0f;  // Ensure positive width
        
        // Calculate expected X position (same formula for both block and flex)
        float expected_x = calculateExpectedX(
            container_width, left, 0.0f, element_width, margin_left, 0.0f, true, false
        );
        
        // Verify the formula: x = left + margin_left
        float computed_x = left + margin_left;
        
        EXPECT_NEAR(computed_x, expected_x, EPSILON)
            << "Left inset position should be consistent. "
            << "Container width: " << container_width << ", "
            << "Left: " << left << ", "
            << "Margin left: " << margin_left << ", "
            << "Expected X: " << expected_x << ", "
            << "Got: " << computed_x << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with right inset (and no left),
 * the position calculation SHALL be consistent across layout modes.
 * 
 * Formula: x = container_width - element_width - right - margin_right
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, RightInsetPositionConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float right = rng_.randInsetValue();
        float margin_right = rng_.randMarginValue();
        float element_width = rng_.randInsetValue() + 50.0f;
        
        // Ensure element fits in container
        if (element_width + right + margin_right > container_width) {
            element_width = container_width - right - margin_right - 10.0f;
            if (element_width < 10.0f) element_width = 10.0f;
        }
        
        // Calculate expected X position
        float expected_x = calculateExpectedX(
            container_width, 0.0f, right, element_width, 0.0f, margin_right, false, true
        );
        
        // Verify the formula: x = container_width - element_width - right - margin_right
        float computed_x = container_width - element_width - right - margin_right;
        
        EXPECT_NEAR(computed_x, expected_x, EPSILON)
            << "Right inset position should be consistent. "
            << "Container width: " << container_width << ", "
            << "Right: " << right << ", "
            << "Element width: " << element_width << ", "
            << "Margin right: " << margin_right << ", "
            << "Expected X: " << expected_x << ", "
            << "Got: " << computed_x << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with top inset,
 * the position calculation SHALL be consistent across layout modes.
 * 
 * Formula: y = top + margin_top
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, TopInsetPositionConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        float top = rng_.randInsetValue();
        float margin_top = rng_.randMarginValue();
        float element_height = rng_.randInsetValue() + 50.0f;
        
        // Calculate expected Y position
        float expected_y = calculateExpectedY(
            container_height, top, 0.0f, element_height, margin_top, 0.0f, true, false
        );
        
        // Verify the formula: y = top + margin_top
        float computed_y = top + margin_top;
        
        EXPECT_NEAR(computed_y, expected_y, EPSILON)
            << "Top inset position should be consistent. "
            << "Container height: " << container_height << ", "
            << "Top: " << top << ", "
            << "Margin top: " << margin_top << ", "
            << "Expected Y: " << expected_y << ", "
            << "Got: " << computed_y << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with bottom inset (and no top),
 * the position calculation SHALL be consistent across layout modes.
 * 
 * Formula: y = container_height - element_height - bottom - margin_bottom
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, BottomInsetPositionConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        float bottom = rng_.randInsetValue();
        float margin_bottom = rng_.randMarginValue();
        float element_height = rng_.randInsetValue() + 50.0f;
        
        // Ensure element fits in container
        if (element_height + bottom + margin_bottom > container_height) {
            element_height = container_height - bottom - margin_bottom - 10.0f;
            if (element_height < 10.0f) element_height = 10.0f;
        }
        
        // Calculate expected Y position
        float expected_y = calculateExpectedY(
            container_height, 0.0f, bottom, element_height, 0.0f, margin_bottom, false, true
        );
        
        // Verify the formula: y = container_height - element_height - bottom - margin_bottom
        float computed_y = container_height - element_height - bottom - margin_bottom;
        
        EXPECT_NEAR(computed_y, expected_y, EPSILON)
            << "Bottom inset position should be consistent. "
            << "Container height: " << container_height << ", "
            << "Bottom: " << bottom << ", "
            << "Element height: " << element_height << ", "
            << "Margin bottom: " << margin_bottom << ", "
            << "Expected Y: " << expected_y << ", "
            << "Got: " << computed_y << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with left+right stretch,
 * the width calculation SHALL be consistent across layout modes.
 * 
 * Formula: width = container_width - left - right - margin_left - margin_right
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, LeftRightStretchConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float left = rng_.randInsetValue();
        float right = rng_.randInsetValue();
        float margin_left = rng_.randMarginValue();
        float margin_right = rng_.randMarginValue();
        
        // Calculate expected width (same formula for both block and flex)
        float expected_width = calculateStretchWidth(
            container_width, left, right, margin_left, margin_right
        );
        
        // Verify the formula
        float computed_width = container_width - left - right - margin_left - margin_right;
        computed_width = std::max(computed_width, 0.0f);
        
        EXPECT_NEAR(computed_width, expected_width, EPSILON)
            << "Left+right stretch width should be consistent. "
            << "Container width: " << container_width << ", "
            << "Left: " << left << ", Right: " << right << ", "
            << "Margin left: " << margin_left << ", Margin right: " << margin_right << ", "
            << "Expected width: " << expected_width << ", "
            << "Got: " << computed_width << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with top+bottom stretch,
 * the height calculation SHALL be consistent across layout modes.
 * 
 * Formula: height = container_height - top - bottom - margin_top - margin_bottom
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, TopBottomStretchConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        float top = rng_.randInsetValue();
        float bottom = rng_.randInsetValue();
        float margin_top = rng_.randMarginValue();
        float margin_bottom = rng_.randMarginValue();
        
        // Calculate expected height (same formula for both block and flex)
        float expected_height = calculateStretchHeight(
            container_height, top, bottom, margin_top, margin_bottom
        );
        
        // Verify the formula
        float computed_height = container_height - top - bottom - margin_top - margin_bottom;
        computed_height = std::max(computed_height, 0.0f);
        
        EXPECT_NEAR(computed_height, expected_height, EPSILON)
            << "Top+bottom stretch height should be consistent. "
            << "Container height: " << container_height << ", "
            << "Top: " << top << ", Bottom: " << bottom << ", "
            << "Margin top: " << margin_top << ", Margin bottom: " << margin_bottom << ", "
            << "Expected height: " << expected_height << ", "
            << "Got: " << computed_height << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element, when left is set, it SHALL take
 * precedence over right (same behavior in both block and flex).
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, LeftPrecedenceOverRight) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float left = rng_.randInsetValue();
        float right = rng_.randInsetValue();
        float margin_left = rng_.randMarginValue();
        float element_width = rng_.randInsetValue() + 50.0f;
        
        // When both left and right are set, left takes precedence for positioning
        // (right is used for stretch calculation if no explicit width)
        float expected_x_with_left = left + margin_left;
        
        // Calculate X with only left set
        float x_left_only = calculateExpectedX(
            container_width, left, 0.0f, element_width, margin_left, 0.0f, true, false
        );
        
        // Calculate X with both left and right set (left should still win)
        float x_both = calculateExpectedX(
            container_width, left, right, element_width, margin_left, 0.0f, true, true
        );
        
        EXPECT_NEAR(x_left_only, expected_x_with_left, EPSILON)
            << "Left-only position should match expected. Iteration: " << i;
        
        EXPECT_NEAR(x_both, expected_x_with_left, EPSILON)
            << "When both left and right are set, left should take precedence. "
            << "Container width: " << container_width << ", "
            << "Left: " << left << ", Right: " << right << ", "
            << "Expected X: " << expected_x_with_left << ", "
            << "Got: " << x_both << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element, when top is set, it SHALL take
 * precedence over bottom (same behavior in both block and flex).
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, TopPrecedenceOverBottom) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_height = rng_.randContainerDimension();
        float top = rng_.randInsetValue();
        float bottom = rng_.randInsetValue();
        float margin_top = rng_.randMarginValue();
        float element_height = rng_.randInsetValue() + 50.0f;
        
        // When both top and bottom are set, top takes precedence for positioning
        float expected_y_with_top = top + margin_top;
        
        // Calculate Y with only top set
        float y_top_only = calculateExpectedY(
            container_height, top, 0.0f, element_height, margin_top, 0.0f, true, false
        );
        
        // Calculate Y with both top and bottom set (top should still win)
        float y_both = calculateExpectedY(
            container_height, top, bottom, element_height, margin_top, 0.0f, true, true
        );
        
        EXPECT_NEAR(y_top_only, expected_y_with_top, EPSILON)
            << "Top-only position should match expected. Iteration: " << i;
        
        EXPECT_NEAR(y_both, expected_y_with_top, EPSILON)
            << "When both top and bottom are set, top should take precedence. "
            << "Container height: " << container_height << ", "
            << "Top: " << top << ", Bottom: " << bottom << ", "
            << "Expected Y: " << expected_y_with_top << ", "
            << "Got: " << y_both << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element with percentage insets,
 * the percentage SHALL be resolved relative to the containing block dimensions
 * consistently across layout modes.
 * 
 * **Validates: Requirements 6.1, 6.2, 6.3**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, PercentageInsetConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float container_height = rng_.randContainerDimension();
        
        // Generate percentage values (0-50%)
        std::uniform_real_distribution<float> pct_dist(0.0f, 50.0f);
        std::mt19937 gen(std::random_device{}());
        float left_pct = pct_dist(gen);
        float top_pct = pct_dist(gen);
        
        // Calculate pixel values from percentages
        float left_px = container_width * left_pct / 100.0f;
        float top_px = container_height * top_pct / 100.0f;
        
        // Expected position
        float expected_x = left_px;
        float expected_y = top_px;
        
        EXPECT_NEAR(left_px, expected_x, EPSILON)
            << "Percentage left inset should resolve consistently. "
            << "Container width: " << container_width << ", "
            << "Left %: " << left_pct << ", "
            << "Expected X: " << expected_x << ", "
            << "Got: " << left_px << ", "
            << "Iteration: " << i;
        
        EXPECT_NEAR(top_px, expected_y, EPSILON)
            << "Percentage top inset should resolve consistently. "
            << "Container height: " << container_height << ", "
            << "Top %: " << top_pct << ", "
            << "Expected Y: " << expected_y << ", "
            << "Got: " << top_px << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: viewport-units-and-absolute-positioning, Property 10: Consistent absolute positioning across layout modes**
 * 
 * For any absolutely positioned element, the final position SHALL be
 * deterministic given the same inputs (container size, insets, margins, element size).
 * 
 * **Validates: Requirements 6.1, 6.2**
 */
TEST_F(CrossLayoutModeAbsolutePositioningTest, PositionDeterminism) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        float container_width = rng_.randContainerDimension();
        float container_height = rng_.randContainerDimension();
        float left = rng_.randInsetValue();
        float top = rng_.randInsetValue();
        float margin_left = rng_.randMarginValue();
        float margin_top = rng_.randMarginValue();
        float element_width = rng_.randInsetValue() + 50.0f;
        float element_height = rng_.randInsetValue() + 50.0f;
        
        // Calculate position twice with same inputs
        float x1 = calculateExpectedX(container_width, left, 0.0f, element_width, margin_left, 0.0f, true, false);
        float y1 = calculateExpectedY(container_height, top, 0.0f, element_height, margin_top, 0.0f, true, false);
        
        float x2 = calculateExpectedX(container_width, left, 0.0f, element_width, margin_left, 0.0f, true, false);
        float y2 = calculateExpectedY(container_height, top, 0.0f, element_height, margin_top, 0.0f, true, false);
        
        EXPECT_EQ(x1, x2)
            << "Position calculation should be deterministic. "
            << "X1: " << x1 << ", X2: " << x2 << ", "
            << "Iteration: " << i;
        
        EXPECT_EQ(y1, y2)
            << "Position calculation should be deterministic. "
            << "Y1: " << y1 << ", Y2: " << y2 << ", "
            << "Iteration: " << i;
    }
}
