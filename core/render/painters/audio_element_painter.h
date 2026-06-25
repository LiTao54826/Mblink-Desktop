/**
 * @file audio_element_painter.h
 * @brief Native audio control painter shared by block and inline-block renderers.
 */

#pragma once

#include "core/render/objects/render_object.h"
#include "include/core/SkCanvas.h"

namespace mblink {

class HTMLAudioElement;

void PaintAudioElementControl(
    SkCanvas* canvas,
    HTMLAudioElement* audio,
    const Box& box,
    const ComputedStyle& style);

} // namespace mblink
