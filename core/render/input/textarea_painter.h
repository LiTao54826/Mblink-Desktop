#pragma once

class SkCanvas;

namespace mblink {

class HTMLTextAreaElement;
class RenderObject;
struct Box;

namespace textarea_painter {

void Paint(SkCanvas* canvas,
           HTMLTextAreaElement* textarea,
           RenderObject* render_object,
           const Box& box,
           bool cursor_visible);

}  // namespace textarea_painter
}  // namespace mblink
