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
#include "core/render/utils/color.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <chrono>

namespace lightui {

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
        lightui::Paint text_paint;
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
    std::string value = input->GetValue();
    std::string display_text = value;
    bool is_placeholder = false;

    // 如果是密码类型，显示为星号
    if (is_password && !value.empty()) {
        display_text = std::string(value.length(), '*');
    }

    // 如果值为空，显示 placeholder
    if (value.empty()) {
        display_text = input->GetPlaceholder();
        is_placeholder = true;
    }

    if (!display_text.empty()) {
        // 创建字体
        SkFont font = CreateFont(params);

        // 获取字体度量信息
        SkFontMetrics font_metrics;
        font.getMetrics(&font_metrics);

        // 计算文本位置（左对齐，垂直居中）
        float text_x = box.content_x;
        float text_y = box.content_y + (box.content_height - font_metrics.fDescent + font_metrics.fAscent) / 2 - font_metrics.fAscent;

        // 创建文本渲染器
        TextRenderer text_renderer(canvas_);

        // 设置文本颜色
        lightui::Paint text_paint;
        text_paint.SetColor(GetTextColor(params, is_placeholder));

        // 绘制文本
        text_renderer.DrawText(display_text, text_x, text_y, font, text_paint);

        // 如果有焦点，绘制选中高亮和光标
        if (params.has_focus) {
            int sel_start = input->GetSelectionStart();
            int sel_end = input->GetSelectionEnd();

            // 绘制选中区域高亮
            if (sel_start != sel_end && !is_placeholder) {
                PaintSelectionHighlight(text_x, box, font, value, sel_start, sel_end, is_password);
            }

            // 绘制光标
            if (IsCursorVisible()) {
                PaintCursor(text_x, box, font, font_metrics, value, sel_end, is_password);
            }
        }
    }
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

    float sel_start_x = text_x;
    if (start_char > 0) {
        sel_start_x += font.measureText(text_before_sel.c_str(), text_before_sel.length(), SkTextEncoding::kUTF8);
    }
    float sel_width = font.measureText(selected_text.c_str(), selected_text.length(), SkTextEncoding::kUTF8);

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
    float cursor_x = text_x;
    if (cursor_pos > 0) {
        cursor_x += font.measureText(
            text_before_cursor.c_str(),
            text_before_cursor.length(),
            SkTextEncoding::kUTF8
        );
    }

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
    float cursor_x = text_x;
    if (!current_line_before_cursor.empty()) {
        cursor_x += font.measureText(
            current_line_before_cursor.c_str(),
            current_line_before_cursor.length(),
            SkTextEncoding::kUTF8
        );
    }

    // 绘制光标
    SkPaint cursor_paint;
    cursor_paint.setColor(SK_ColorBLACK);
    cursor_paint.setStrokeWidth(1);
    cursor_paint.setAntiAlias(true);

    canvas_->drawLine(cursor_x, cursor_y + font_metrics.fAscent,
                     cursor_x, cursor_y + font_metrics.fDescent, cursor_paint);
}

bool FormElementPainter::IsCursorVisible() const {
    // 基于时间的光标闪烁：每 500 毫秒切换一次
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return (ms / 500) % 2 == 0;
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
        return lightui::Color::Parse(params.text_color);
    }
    
    return SK_ColorBLACK;
}

} // namespace lightui
