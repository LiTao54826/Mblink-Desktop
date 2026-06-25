/**
 * @file repaint_reason.h
 * @brief Repaint reason definitions.
 */

#pragma once

namespace mblink {

enum class RepaintReason {
    Unknown,
    Initial,
    Resize,
    DOMMutation,
    PseudoClass,
    Focus,
    KeyboardInput,
    MouseHover,
    MouseButton,
    WheelScroll,
    Animation,
    Terminal,
    DevTools,
    API,
    Layout
};

const char* RepaintReasonName(RepaintReason reason);
bool RepaintReasonMayAffectLayout(RepaintReason reason);

} // namespace mblink
