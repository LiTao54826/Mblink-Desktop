# Design Document

## Overview

This design document describes the integration of the existing CSS Animation infrastructure into the MBink/LightUI rendering pipeline. The core animation components (CSSAnimation, KeyframesRule, AnimationController, PropertyInterpolation, EasingFunctions, AnimationOptimizer) are already implemented. This design focuses on connecting these components to the CSS parsing, style resolution, and rendering systems.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CSS Source                                   │
│  @keyframes slide { from {...} to {...} }                           │
│  .element { animation: slide 1s ease-in-out; }                      │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      StyleManager                                    │
│  ┌─────────────────┐    ┌─────────────────┐                        │
│  │ ParseCSSString  │───▶│ ExtractKeyframes │                        │
│  └─────────────────┘    └────────┬────────┘                        │
│                                  │                                   │
│                                  ▼                                   │
│                    ┌─────────────────────────┐                      │
│                    │  AnimationController    │                      │
│                    │  RegisterKeyframes()    │                      │
│                    └─────────────────────────┘                      │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      StyleResolver                                   │
│  ┌─────────────────────┐    ┌─────────────────────┐                │
│  │ ParseAnimationProp  │───▶│ ComputedStyle       │                │
│  └─────────────────────┘    │ .animations[]       │                │
│                              └─────────────────────┘                │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Render Loop                                     │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐ │
│  │ Update Time     │───▶│ AnimController  │───▶│ Apply Props     │ │
│  │                 │    │ .Update(time)   │    │ to RenderObject │ │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘ │
│                                                         │           │
│                                                         ▼           │
│                                               ┌─────────────────┐   │
│                                               │ Paint           │   │
│                                               └─────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. StyleManager Extensions

```cpp
// core/lexbor/style_manager.h

class StyleManager {
public:
    // Existing methods...
    
    /**
     * @brief Get the animation controller for this document
     * @return Reference to the AnimationController
     */
    AnimationController& GetAnimationController();
    const AnimationController& GetAnimationController() const;

private:
    /**
     * @brief Extract and register @keyframes rules from CSS text
     * @param css_text The CSS text to parse
     */
    void ExtractAndRegisterKeyframes(const std::string& css_text);
    
    /**
     * @brief Find all @keyframes blocks in CSS text
     * @param css_text The CSS text to search
     * @return Vector of @keyframes rule strings
     */
    std::vector<std::string> FindKeyframesBlocks(const std::string& css_text) const;

    AnimationController animation_controller_;
};
```

### 2. ComputedStyle Extensions

```cpp
// core/render/render_object.h

struct ComputedStyle {
    // Existing fields...
    
    /// CSS Animation configurations
    std::vector<CSSAnimation> animations;
    
    /// Animation play state (can be controlled independently)
    std::string animation_play_state = "running";
};
```

### 3. StyleResolver Extensions

```cpp
// core/render/style_resolver.h

class StyleResolver {
private:
    /**
     * @brief Parse animation-related CSS properties
     * @param style The ComputedStyle to update
     * @param property The property name
     * @param value The property value
     * @return true if the property was handled
     */
    bool ParseAnimationProperty(ComputedStyle& style,
                               const std::string& property,
                               const std::string& value);
};
```

### 4. Animation Application System

```cpp
// core/render/animation_applicator.h (new file)

namespace lightui {

/**
 * @brief Applies animation properties to render objects
 * 
 * This class bridges the AnimationController and RenderObject,
 * applying calculated animation values to element styles.
 */
class AnimationApplicator {
public:
    /**
     * @brief Constructor
     * @param controller Reference to the animation controller
     */
    explicit AnimationApplicator(AnimationController& controller);
    
    /**
     * @brief Start animations for a render object based on its computed style
     * @param object The render object
     */
    void StartAnimationsForObject(RenderObject* object);
    
    /**
     * @brief Apply current animation values to a render object
     * @param object The render object
     */
    void ApplyAnimationValues(RenderObject* object);
    
    /**
     * @brief Stop all animations for a render object
     * @param object The render object
     */
    void StopAnimationsForObject(RenderObject* object);
    
    /**
     * @brief Update animation play state for an object
     * @param object The render object
     * @param paused Whether animations should be paused
     */
    void SetAnimationsPaused(RenderObject* object, bool paused);

private:
    AnimationController& controller_;
    
    /**
     * @brief Apply a single property value to computed style
     * @param style The computed style to modify
     * @param property The property name
     * @param value The property value
     */
    void ApplyPropertyToStyle(ComputedStyle& style,
                             const std::string& property,
                             const std::string& value);
};

} // namespace lightui
```

### 5. Transform Interpolation Enhancement

```cpp
// core/render/property_interpolation.h

class PropertyInterpolation {
public:
    // Existing methods...

private:
    /**
     * @brief Decomposed transform components for interpolation
     */
    struct DecomposedTransform {
        float translate_x = 0.0f;
        float translate_y = 0.0f;
        float rotate = 0.0f;      // radians
        float scale_x = 1.0f;
        float scale_y = 1.0f;
        float skew_x = 0.0f;      // radians
        float skew_y = 0.0f;      // radians
    };
    
    /**
     * @brief Decompose a transform string into components
     * @param transform_str The transform CSS string
     * @return Decomposed transform, or nullopt if parsing fails
     */
    static std::optional<DecomposedTransform> DecomposeTransform(
        const std::string& transform_str);
    
    /**
     * @brief Interpolate between two decomposed transforms
     * @param from Start transform
     * @param to End transform
     * @param factor Interpolation factor (0.0 to 1.0)
     * @return Interpolated transform
     */
    static DecomposedTransform InterpolateDecomposed(
        const DecomposedTransform& from,
        const DecomposedTransform& to,
        float factor);
    
    /**
     * @brief Compose a decomposed transform back to CSS string
     * @param decomposed The decomposed transform
     * @return CSS transform string
     */
    static std::string ComposeTransform(const DecomposedTransform& decomposed);
};
```

### 6. Render Loop Integration

```cpp
// Integration point in main render loop

void RenderLoop::Frame() {
    // Get current time in seconds
    double current_time = GetHighResolutionTime();
    
    // Update all animations
    style_manager_->GetAnimationController().Update(current_time);
    
    // Apply animation values to render objects
    ApplyAnimationsToRenderTree(render_tree_root_);
    
    // Continue with layout and paint...
    Layout();
    Paint();
}

void RenderLoop::ApplyAnimationsToRenderTree(RenderObject* root) {
    if (!root) return;
    
    // Apply animations to this object
    animation_applicator_->ApplyAnimationValues(root);
    
    // Recurse to children
    for (auto& child : root->GetChildren()) {
        ApplyAnimationsToRenderTree(child.get());
    }
}
```

## Data Models

### Animation State in RenderObject

```cpp
// Additional state tracked per RenderObject for animations

struct AnimationState {
    /// Names of currently active animations
    std::vector<std::string> active_animation_names;
    
    /// Whether animations have been started for this object
    bool animations_initialized = false;
    
    /// Cached original style (before animation modifications)
    std::optional<ComputedStyle> base_style;
};
```

### @keyframes Extraction Pattern

```cpp
// Regex pattern for finding @keyframes blocks
// Pattern: @keyframes <name> { ... }

const std::regex KEYFRAMES_PATTERN(
    R"(@keyframes\s+([a-zA-Z0-9_-]+)\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\})",
    std::regex::ECMAScript
);
```



## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: @keyframes registration consistency
*For any* valid CSS text containing @keyframes rules, parsing through StyleManager should result in all @keyframes being registered in the AnimationController with correct names and keyframe data.
**Validates: Requirements 1.1, 1.3, 1.4**

### Property 2: @keyframes cascade override
*For any* CSS text containing multiple @keyframes rules with the same name, only the last defined rule should be retained in the AnimationController.
**Validates: Requirements 1.2**

### Property 3: Animation shorthand parsing completeness
*For any* valid animation shorthand property value, parsing should correctly extract all components (name, duration, timing-function, delay, iteration-count, direction, fill-mode, play-state) into the ComputedStyle.
**Validates: Requirements 2.1, 2.2**

### Property 4: Multi-animation preservation
*For any* comma-separated list of animations, all animation configurations should be stored in ComputedStyle.animations with the correct count and order.
**Validates: Requirements 2.4**

### Property 5: Animation progress calculation
*For any* running animation with duration D at time T (where T >= start_time), the progress should equal ((T - start_time - delay) mod D) / D, clamped to [0, 1].
**Validates: Requirements 3.2**

### Property 6: Property interpolation correctness
*For any* two numeric property values V1 and V2 with the same unit, interpolating at factor F should produce V1 + (V2 - V1) * F.
**Validates: Requirements 3.2, 3.3**

### Property 7: Transform decomposition round-trip
*For any* valid transform string containing translate, rotate, scale, or skew functions, decomposing and then recomposing should produce a functionally equivalent transform.
**Validates: Requirements 4.1, 4.3**

### Property 8: Transform interpolation linearity
*For any* two decomposed transforms T1 and T2, interpolating at factor F should produce component values that are linear interpolations: T1.component + (T2.component - T1.component) * F.
**Validates: Requirements 4.2**

### Property 9: Animation event timing
*For any* animation with delay D and duration T, animationstart should fire at time D, and animationend should fire at time D + T * iteration_count.
**Validates: Requirements 5.1, 5.2**

### Property 10: Animation event properties
*For any* dispatched animation event, the event object should contain non-null animationName matching the animation's name, and elapsedTime >= 0.
**Validates: Requirements 5.4**

### Property 11: Fill-mode forwards behavior
*For any* animation with fill-mode "forwards" or "both", after the animation ends, the element's computed style should match the final keyframe values.
**Validates: Requirements 6.1, 6.3**

### Property 12: Fill-mode backwards behavior
*For any* animation with fill-mode "backwards" or "both" and delay > 0, during the delay period, the element's computed style should match the first keyframe values.
**Validates: Requirements 6.2, 6.3**

### Property 13: Pause state preservation
*For any* animation that is paused at progress P, the progress should remain P regardless of elapsed time until resumed.
**Validates: Requirements 7.1**

### Property 14: Resume position continuity
*For any* animation paused at progress P and then resumed, the animation should continue from progress P, not restart from 0.
**Validates: Requirements 7.2**

## Error Handling

### CSS Parsing Errors

| Error | Handling |
|-------|----------|
| Invalid @keyframes syntax | Log warning, skip the rule |
| Invalid animation property value | Use default values |
| Unknown animation name | Animation not started, no error |
| Invalid timing function | Fall back to "ease" |
| Negative duration | Treat as 0 |
| Negative delay | Allow (starts in the past) |

### Runtime Errors

| Error | Handling |
|-------|----------|
| RenderObject destroyed during animation | AnimationController removes animation |
| Transform decomposition fails | Use step interpolation (0.5 threshold) |
| Property interpolation fails | Use target value |
| Event dispatch fails | Log error, continue animation |

## Testing Strategy

### Dual Testing Approach

This feature requires both unit tests and property-based tests:

- **Unit tests** verify specific examples, edge cases, and integration points
- **Property-based tests** verify universal properties that should hold across all inputs

### Real Rendering Layer Integration

**CRITICAL**: All tests MUST connect to the real rendering layer, NOT use mocks. This ensures:
1. Tests validate actual behavior, not mocked behavior
2. Tests can be used directly after development without modification
3. Integration issues are caught early

**Test Infrastructure**:
- Use `DOMTestBase` from `tests/test_utils/test_helpers.h` for DOM creation
- Use real `Document`, `Element`, `RenderObject` instances
- Use real `StyleManager`, `AnimationController`, `StyleResolver`
- Create real render trees with `RenderBlock`, `RenderInline`, etc.

**Example Test Setup**:
```cpp
class AnimationIntegrationTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        // Get real StyleManager from document
        style_manager_ = doc_->GetStyleManager();
    }
    
    StyleManager* style_manager_;
};
```

### Property-Based Testing Library

**Library**: [rapidcheck](https://github.com/emil-e/rapidcheck) for C++

**Configuration**: Each property test should run a minimum of 100 iterations.

### Test Categories

#### 1. @keyframes Parsing Tests

```cpp
// Property test: @keyframes registration
// **Feature: css-animation-integration, Property 1: @keyframes registration consistency**
// Uses REAL StyleManager, not mocks
class KeyframesParsingPropertyTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        // Use real StyleManager from document
        style_manager_ = doc_->GetStyleManager();
    }
    StyleManager* style_manager_;
};

RC_GTEST_PROP(KeyframesParsing, RegistrationConsistency, ()) {
    auto keyframes_css = *rc::gen::keyframesCss();
    
    // Use REAL StyleManager
    style_manager_->ParseCSSString(keyframes_css);
    
    auto expected_names = extractKeyframeNames(keyframes_css);
    for (const auto& name : expected_names) {
        auto* rule = style_manager_->GetAnimationController().GetKeyframes(name);
        RC_ASSERT(rule != nullptr);
        RC_ASSERT(rule->name == name);
    }
}
```

#### 2. Animation Property Parsing Tests

```cpp
// Property test: Animation shorthand parsing
// **Feature: css-animation-integration, Property 3: Animation shorthand parsing completeness**
// Uses REAL StyleResolver and ComputedStyle
class AnimationParsingPropertyTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
    }
};

RC_GTEST_PROP(AnimationParsing, ShorthandCompleteness, ()) {
    auto anim = *rc::gen::animationShorthand();
    
    // Create REAL element and apply style
    auto div = doc_->CreateElement("div");
    div->SetStyle("animation", anim.css_string);
    doc_->GetBody()->AppendChild(div);
    
    // Build render tree and resolve styles
    doc_->BuildRenderTree();
    auto render_obj = div->GetRenderObject();
    
    RC_ASSERT(render_obj != nullptr);
    const auto& computed = render_obj->GetComputedStyle();
    RC_ASSERT(computed.animations.size() >= 1);
    RC_ASSERT(computed.animations[0].name == anim.expected_name);
    RC_ASSERT(std::abs(computed.animations[0].duration - anim.expected_duration) < 0.001f);
}
```

#### 3. Transform Interpolation Tests

```cpp
// Property test: Transform round-trip
// **Feature: css-animation-integration, Property 7: Transform decomposition round-trip**
RC_GTEST_PROP(TransformInterpolation, RoundTrip, ()) {
    auto transform = *rc::gen::validTransformString();
    
    auto decomposed = PropertyInterpolation::DecomposeTransform(transform);
    RC_PRE(decomposed.has_value());
    
    auto recomposed = PropertyInterpolation::ComposeTransform(*decomposed);
    auto redecomposed = PropertyInterpolation::DecomposeTransform(recomposed);
    
    RC_ASSERT(redecomposed.has_value());
    RC_ASSERT(decomposedEqual(*decomposed, *redecomposed, 0.001f));
}
```

#### 4. Animation Event Tests

```cpp
// Property test: Event timing
// **Feature: css-animation-integration, Property 9: Animation event timing**
// Uses REAL AnimationController and RenderObject
class AnimationEventsPropertyTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        style_manager_ = doc_->GetStyleManager();
    }
    StyleManager* style_manager_;
};

RC_GTEST_PROP(AnimationEvents, Timing, ()) {
    auto config = *rc::gen::animationConfig();
    
    // Create REAL element with animation
    auto div = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(div);
    doc_->BuildRenderTree();
    
    auto render_obj = div->GetRenderObject();
    auto& controller = style_manager_->GetAnimationController();
    
    // Track events
    double start_event_time = -1;
    double end_event_time = -1;
    
    div->AddEventListener("animationstart", [&](Event* e) {
        start_event_time = controller.GetCurrentTime();
    });
    div->AddEventListener("animationend", [&](Event* e) {
        end_event_time = controller.GetCurrentTime();
    });
    
    // Start animation and simulate time
    controller.StartAnimation(render_obj.get(), config);
    
    // Simulate animation lifecycle
    controller.Update(config.delay + 0.001);  // Just after delay
    RC_ASSERT(std::abs(start_event_time - config.delay) < 0.01);
    
    controller.Update(config.delay + config.duration * config.iteration_count + 0.001);
    RC_ASSERT(std::abs(end_event_time - (config.delay + config.duration * config.iteration_count)) < 0.01);
}
```

#### 5. Fill-Mode Tests

```cpp
// Property test: Forwards fill
// **Feature: css-animation-integration, Property 11: Fill-mode forwards behavior**
// Uses REAL RenderObject and ComputedStyle
class FillModePropertyTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        style_manager_ = doc_->GetStyleManager();
    }
    StyleManager* style_manager_;
};

RC_GTEST_PROP(FillMode, Forwards, ()) {
    auto keyframes = *rc::gen::keyframesWithProperties();
    auto config = *rc::gen::animationConfigWithFillMode("forwards");
    
    // Register REAL keyframes
    style_manager_->ParseCSSString(keyframes.css_string);
    
    // Create REAL element
    auto div = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(div);
    doc_->BuildRenderTree();
    
    auto render_obj = div->GetRenderObject();
    auto& controller = style_manager_->GetAnimationController();
    
    // Start and complete animation
    controller.StartAnimation(render_obj.get(), config);
    controller.Update(config.delay + config.duration * config.iteration_count + 1.0);
    
    // Verify final style matches last keyframe
    const auto& final_style = render_obj->GetComputedStyle();
    auto expected = keyframes.keyframes.back().properties;
    
    for (const auto& [prop, value] : expected) {
        RC_ASSERT(getStyleProperty(final_style, prop) == value);
    }
}
```

### Unit Test Examples

```cpp
// Unit test: Specific @keyframes parsing
// Uses REAL StyleManager
class KeyframesUnitTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        style_manager_ = doc_->GetStyleManager();
    }
    StyleManager* style_manager_;
};

TEST_F(KeyframesUnitTest, FromToKeywords) {
    std::string css = R"(
        @keyframes slide {
            from { transform: translateX(0); }
            to { transform: translateX(100px); }
        }
    )";
    
    // Use REAL StyleManager to parse
    style_manager_->ParseCSSString(css);
    
    auto* rule = style_manager_->GetAnimationController().GetKeyframes("slide");
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->name, "slide");
    EXPECT_EQ(rule->keyframes.size(), 2);
    EXPECT_FLOAT_EQ(rule->keyframes[0].offset, 0.0f);
    EXPECT_FLOAT_EQ(rule->keyframes[1].offset, 1.0f);
}

// Unit test: Animation event properties
// Uses REAL Element and event system
TEST_F(KeyframesUnitTest, EventProperties) {
    // Setup CSS with keyframes
    std::string css = R"(
        @keyframes slide {
            from { opacity: 0; }
            to { opacity: 1; }
        }
    )";
    style_manager_->ParseCSSString(css);
    
    // Create REAL element
    auto div = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(div);
    doc_->BuildRenderTree();
    
    // Capture event
    AnimationEvent* captured_event = nullptr;
    div->AddEventListener("animationstart", [&](Event* e) {
        captured_event = static_cast<AnimationEvent*>(e);
    });
    
    // Start animation
    auto render_obj = div->GetRenderObject();
    CSSAnimation anim;
    anim.name = "slide";
    anim.duration = 1.0f;
    
    auto& controller = style_manager_->GetAnimationController();
    controller.StartAnimation(render_obj.get(), anim);
    controller.Update(0.001);  // Trigger start event
    
    ASSERT_NE(captured_event, nullptr);
    EXPECT_EQ(captured_event->GetAnimationName(), "slide");
    EXPECT_GE(captured_event->GetElapsedTime(), 0.0f);
}
```

### Integration Tests

```javascript
// JavaScript integration test
describe('CSS Animation Integration', () => {
    it('should animate element with @keyframes', async () => {
        // Add CSS with @keyframes
        const style = document.createElement('style');
        style.textContent = `
            @keyframes fadeIn {
                from { opacity: 0; }
                to { opacity: 1; }
            }
            .animated { animation: fadeIn 1s ease-in-out; }
        `;
        document.head.appendChild(style);
        
        // Create and animate element
        const div = document.createElement('div');
        div.className = 'animated';
        document.body.appendChild(div);
        
        // Wait for animation
        await new Promise(resolve => {
            div.addEventListener('animationend', resolve);
        });
        
        // Verify final state
        expect(getComputedStyle(div).opacity).toBe('1');
    });
});
```
