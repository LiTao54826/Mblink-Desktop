/**
 * @file form_element_painter.cpp
 * @brief 表单元素绘制器实现
 * 
 * 从 render_object.cpp 提取的表单元素绘制逻辑
 */

#include "form_element_painter.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/element.h"
#include "core/render/input/input_paint_model.h"
#include "core/render/objects/render_object.h"
#include "core/render/utils/color.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>

namespace mbink {

FormElementPainter::FormElementPainter(SkCanvas* canvas)
    : canvas_(canvas) {
}

void FormElementPainter::PaintInputElement(HTMLInputElement* input, 
                                           const Box& box, 
                                           const FormElementPaintParams& params) {
    if (!input) return;

    InputType type = input->GetInputType();

    // 处理文本类型的 input
    if (type == InputType::Text || type == InputType::Password ||
        type == InputType::Email || type == InputType::Tel ||
        type == InputType::Url || type == InputType::Search ||
        type == InputType::Number) {
        PaintTextInput(input, box, params, type == InputType::Password);
    }
    // 处理 checkbox 和 radio 类型
    else if (type == InputType::Checkbox) {
        PaintCheckbox(box, input->GetChecked());
    }
    else if (type == InputType::Radio) {
        PaintRadio(box, input->GetChecked());
    }
}

void FormElementPainter::PaintTextAreaElement(HTMLTextAreaElement* textarea, 
                                              const Box& box, 
                                              const FormElementPaintParams& params) {
    if (!textarea) return;

    std::string value = textarea->GetValue();
    bool is_placeholder = false;

    if (value.empty()) {
        // 显示 placeholder
        value = textarea->GetPlaceholder();
        is_placeholder = true;
    }

    if (!value.empty()) {
        // 创建字体
        SkFont font = CreateFont(params);

        // 获取字体度量信息
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);
        float line_height = -font_metrics.fAscent + font_metrics.fDescent + font_metrics.fLeading;

        // 创建文本渲染器
        TextRenderer text_renderer(canvas_);

        // 设置文本颜色
        mbink::Paint text_paint;
        text_paint.SetColor(GetTextColor(params, is_placeholder));

        // 绘制多行文本
        float text_x = box.content_x;
        float text_y = box.content_y - font_metrics.fAscent;

        text_renderer.DrawMultilineText(value, text_x, text_y, box.content_width, line_height, font, text_paint);

        // 如果有焦点且不是 placeholder，绘制光标
        if (!is_placeholder && params.has_focus) {
            int cursor_pos = textarea->GetSelectionStart();
            PaintTextAreaCursor(text_x, text_y, font, font_metrics, line_height, 
                               textarea->GetValue(), cursor_pos);
        }
    }
}

void FormElementPainter::PaintCheckbox(const Box& box, bool checked) {
    float cx = box.content_x + box.content_width / 2;
    float cy = box.content_y + box.content_height / 2;

    // 绘制 checkbox 方框边框
    SkPaint border_paint;
    border_paint.setColor(kControlBorderColor);
    border_paint.setStrokeWidth(1);
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setAntiAlias(true);

    float size = std::min(box.content_width, box.content_height);
    float half = size / 2;
    SkRect checkbox_rect = SkRect::MakeXYWH(cx - half, cy - half, size, size);

    // 背景
    SkPaint bg_paint;
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setStyle(SkPaint::kFill_Style);
    canvas_->drawRoundRect(checkbox_rect, kCheckboxCornerRadius, kCheckboxCornerRadius, bg_paint);

    // 边框
    canvas_->drawRoundRect(checkbox_rect, kCheckboxCornerRadius, kCheckboxCornerRadius, border_paint);

    // 如果选中，绘制勾选标记
    if (checked) {
        SkPaint check_paint;
        check_paint.setColor(SK_ColorBLACK);
        check_paint.setStrokeWidth(2);
        check_paint.setStyle(SkPaint::kStroke_Style);
        check_paint.setAntiAlias(true);

        SkPath check_path;
        check_path.moveTo(cx - 4, cy);
        check_path.lineTo(cx - 1, cy + 3);
        check_path.lineTo(cx + 4, cy - 3);
        canvas_->drawPath(check_path, check_paint);
    }
}

void FormElementPainter::PaintRadio(const Box& box, bool checked) {
    float cx = box.content_x + box.content_width / 2;
    float cy = box.content_y + box.content_height / 2;
    float radius = std::min(box.content_width, box.content_height) / 2;

    // 背景
    SkPaint bg_paint;
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setStyle(SkPaint::kFill_Style);
    bg_paint.setAntiAlias(true);
    canvas_->drawCircle(cx, cy, radius, bg_paint);

    // 边框
    SkPaint border_paint;
    border_paint.setColor(kControlBorderColor);
    border_paint.setStrokeWidth(1);
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setAntiAlias(true);
    canvas_->drawCircle(cx, cy, radius, border_paint);

    // 如果选中，绘制内圆点
    if (checked) {
        SkPaint dot_paint;
        dot_paint.setColor(SK_ColorBLACK);
        dot_paint.setStyle(SkPaint::kFill_Style);
        dot_paint.setAntiAlias(true);

        float inner_radius = radius / 2;
        canvas_->drawCircle(cx, cy, inner_radius, dot_paint);
    }
}

void FormElementPainter::PaintTextInput(HTMLInputElement* input,
                                        const Box& box,
                                        const FormElementPaintParams& params,
                                        bool is_password) {
    InputPaintModel model = InputPaintModel::FromInputElement(input);
    if (model.display_text.empty()) {
        return;
    }

    SkFont font = CreateFont(params);

    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);

    float text_x = box.content_x;
    float text_y = box.content_y + (box.content_height - font_metrics.fDescent + font_metrics.fAscent) / 2 - font_metrics.fAscent;

    PaintInputTextLayer(model, text_x, text_y, font, params);

    if (!params.has_focus) {
        return;
    }

    PaintInputSelectionLayer(model, text_x, box, font, is_password);

    if (IsCursorVisible()) {
        PaintInputCaretLayer(model, text_x, box, font, font_metrics, is_password);
    }
}

void FormElementPainter::PaintInputTextLayer(const InputPaintModel& model,
                                             float text_x,
                                             float text_y,
                                             const SkFont& font,
                                             const FormElementPaintParams& params) {
    TextRenderer text_renderer(canvas_);

    mbink::Paint text_paint;
    text_paint.SetColor(GetTextColor(params, model.is_placeholder));

    text_renderer.DrawTextWithEmoji(model.display_text, text_x, text_y, font, text_paint);
}

void FormElementPainter::PaintInputSelectionLayer(const InputPaintModel& model,
                                                  float text_x,
                                                  const Box& box,
                                                  const SkFont& font,
                                                  bool is_password) {
    if (!model.HasSelection() || model.is_placeholder) {
        return;
    }

    PaintSelectionHighlight(text_x, box, font, model.value,
                            model.selection_start, model.selection_end,
                            is_password);
}

void FormElementPainter::PaintInputCaretLayer(const InputPaintModel& model,
                                              float text_x,
                                              const Box& box,
                                              const SkFont& font,
                                              const SkFontMetrics& font_metrics,
                                              bool is_password) {
    PaintCursor(text_x, box, font, font_metrics, model.value,
                model.caret_position, is_password);
}

void FormElementPainter::PaintSelectionHighlight(float text_x,
                                                 const Box& box,
                                                 const SkFont& font,
                                                 const std::string& value,
                                                 int sel_start,
                                                 int sel_end,
                                                 bool is_password) {
    int start_char = std::min(sel_start, sel_end);
    int end_char = std::max(sel_start, sel_end);

    // 使用 UTF-8 工具计算字节位置
    size_t start_byte = utf8::CharPosToBytePos(value, start_char);
    size_t end_byte = utf8::CharPosToBytePos(value, end_char);

    std::string text_before_sel = value.substr(0, start_byte);
    std::string selected_text = value.substr(start_byte, end_byte - start_byte);

    // 如果是密码类型，使用星号
    if (is_password) {
        text_before_sel = std::string(start_char, '*');
        selected_text = std::string(end_char - start_char, '*');
    }

    float sel_start_x = text_x + MeasureInputTextWidth(text_before_sel, font);
    float sel_width = MeasureInputTextWidth(selected_text, font);

    // 绘制选中背景
    SkPaint sel_paint;
    sel_paint.setColor(kSelectionColor);
    sel_paint.setStyle(SkPaint::kFill_Style);

    canvas_->drawRect(SkRect::MakeXYWH(sel_start_x, box.content_y, sel_width, box.content_height), sel_paint);
}

void FormElementPainter::PaintCursor(float text_x,
                                     const Box& box,
                                     const SkFont& font,
                                     const SkFontMetrics& font_metrics,
                                     const std::string& value,
                                     int cursor_pos,
                                     bool is_password) {
    // 计算光标位置 - 使用 UTF-8 字符位置转换为字节位置
    size_t cursor_byte_pos = utf8::CharPosToBytePos(value, cursor_pos);
    std::string text_before_cursor = value.substr(0, cursor_byte_pos);

    // 如果是密码类型，使用星号计算宽度
    if (is_password) {
        text_before_cursor = std::string(cursor_pos, '*');
    }

    // 测量光标前的文本宽度
    float cursor_x = text_x + MeasureInputTextWidth(text_before_cursor, font);

    // 计算光标的 Y 坐标（基于字体度量，垂直居中）
    float font_height = font_metrics.fDescent - font_metrics.fAscent;
    float cursor_y_top = box.content_y + (box.content_height - font_height) / 2;
    float cursor_y_bottom = cursor_y_top + font_height;

    // 绘制光标
    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(kCursorWidth);
    cursor_paint.setAntiAlias(true);

    canvas_->drawLine(cursor_x, cursor_y_top, cursor_x, cursor_y_bottom, cursor_paint);
}

float FormElementPainter::MeasureInputTextWidth(const std::string& text,
                                                const SkFont& font) const {
    if (text.empty()) {
        return 0.0f;
    }

    TextRenderer text_renderer(nullptr);
    return text_renderer.MeasureTextWidthWithEmoji(text, font);
}

void FormElementPainter::PaintTextAreaCursor(float text_x,
                                             float text_y, 
                                             const SkFont& font,
                                             const SkFontMetrics& font_metrics,
                                             float line_height,
                                             const std::string& value,
                                             int cursor_pos) {
    if (!IsCursorVisible()) return;

    std::string text_before_cursor = value.substr(0, cursor_pos);

    // 找到最后一个换行符的位置
    size_t last_newline = text_before_cursor.rfind('\n');
    std::string current_line_before_cursor;
    float cursor_y = text_y;

    if (last_newline != std::string::npos) {
        // 光标在某一行中
        current_line_before_cursor = text_before_cursor.substr(last_newline + 1);
        // 计算光标所在行（简化：每个 \n 增加一行）
        int line_count = std::count(text_before_cursor.begin(), text_before_cursor.end(), '\n');
        cursor_y += line_count * line_height;
    } else {
        // 光标在第一行
        current_line_before_cursor = text_before_cursor;
    }

    // 测量光标前的文本宽度
    float cursor_x = text_x + MeasureInputTextWidth(current_line_before_cursor, font);

    // 绘制光标
    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(1);
    cursor_paint.setAntiAlias(true);

    canvas_->drawLine(cursor_x, cursor_y + font_metrics.fAscent,
                     cursor_x, cursor_y + font_metrics.fDescent, cursor_paint);
}

bool FormElementPainter::IsCursorVisible() const {
    // 使用 RenderObject 的全局光标状态，避免重复的系统时间调用
    // 光标闪烁由 EventLoop 统一管理，每 500ms 切换一次
    return RenderObject::IsCursorVisible();
}

SkFont FormElementPainter::CreateFont(const FormElementPaintParams& params) {
    FontDescriptor desc;
    desc.family = params.font_family;
    desc.size = params.font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;

    return FontManager::GetInstance().LoadFont(desc);
}

SkColor FormElementPainter::GetTextColor(const FormElementPaintParams& params, bool is_placeholder) {
    if (is_placeholder) {
        return kPlaceholderColor;
    }
    
    if (!params.text_color.empty()) {
        return mbink::Color::Parse(params.text_color);
    }
    
    return SK_ColorBLACK;
}

} // namespace mbink
