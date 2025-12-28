/**
 * @file form_element_painter.h
 * @brief 表单元素绘制器 - 负责渲染表单控件
 * 
 * 功能：
 * - 渲染文本输入框 (text, password, email, tel, url, search, number)
 * - 渲染多行文本框 (textarea)
 * - 渲染复选框 (checkbox)
 * - 渲染单选按钮 (radio)
 * - 渲染文本选中高亮
 * - 渲染输入光标
 * 
 * 从 render_object.cpp 提取的表单元素绘制逻辑
 */

#pragma once

#include "box_renderer.h"
#include "core/render/text/text_renderer.h"
#include "core/render/utils/paint.h"
#include "core/render/text/font_manager.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include <string>
#include <memory>

namespace lightui {

// 前向声明
class HTMLInputElement;
class HTMLTextAreaElement;
class Element;
struct ComputedStyle;

/**
 * @brief 表单元素绘制参数
 * 
 * 封装表单元素绘制所需的样式和状态信息
 */
struct FormElementPaintParams {
    // 字体信息
    std::string font_family = "sans-serif";
    float font_size = 14.0f;
    
    // 文本颜色
    std::string text_color;
    
    // 是否有焦点
    bool has_focus = false;
};

/**
 * @brief 表单元素绘制器
 * 
 * 负责渲染表单控件，包括：
 * - 文本输入框 (input type=text/password/email/tel/url/search/number)
 * - 多行文本框 (textarea)
 * - 复选框 (checkbox)
 * - 单选按钮 (radio)
 */
class FormElementPainter {
public:
    /// 复选框/单选按钮边框颜色
    static constexpr SkColor kControlBorderColor = SkColorSetRGB(118, 118, 118);
    
    /// 占位符文本颜色
    static constexpr SkColor kPlaceholderColor = SkColorSetRGB(150, 150, 150);
    
    /// 选中高亮颜色（半透明蓝色）
    static constexpr SkColor kSelectionColor = SkColorSetARGB(128, 51, 153, 255);
    
    /// 光标宽度
    static constexpr float kCursorWidth = 1.5f;
    
    /// 复选框圆角半径
    static constexpr float kCheckboxCornerRadius = 2.0f;

    /**
     * @brief 构造函数
     * @param canvas Skia 画布
     */
    explicit FormElementPainter(SkCanvas* canvas);

    /**
     * @brief 绘制输入框元素
     * 
     * 根据输入类型绘制相应的控件：
     * - text/password/email/tel/url/search/number: 文本输入框
     * - checkbox: 复选框
     * - radio: 单选按钮
     * 
     * @param input 输入框元素
     * @param box 盒模型定义
     * @param params 绘制参数
     */
    void PaintInputElement(HTMLInputElement* input, 
                           const Box& box, 
                           const FormElementPaintParams& params);

    /**
     * @brief 绘制多行文本框元素
     * @param textarea 多行文本框元素
     * @param box 盒模型定义
     * @param params 绘制参数
     */
    void PaintTextAreaElement(HTMLTextAreaElement* textarea, 
                              const Box& box, 
                              const FormElementPaintParams& params);

    /**
     * @brief 绘制复选框
     * @param box 盒模型定义
     * @param checked 是否选中
     */
    void PaintCheckbox(const Box& box, bool checked);

    /**
     * @brief 绘制单选按钮
     * @param box 盒模型定义
     * @param checked 是否选中
     */
    void PaintRadio(const Box& box, bool checked);

    /**
     * @brief 绘制文本输入框内容
     * 
     * 包括文本、选中高亮和光标
     * 
     * @param input 输入框元素
     * @param box 盒模型定义
     * @param params 绘制参数
     * @param is_password 是否为密码类型
     */
    void PaintTextInput(HTMLInputElement* input, 
                        const Box& box, 
                        const FormElementPaintParams& params,
                        bool is_password);

private:
    /**
     * @brief 绘制文本选中高亮
     * @param text_x 文本起始 X 坐标
     * @param box 盒模型定义
     * @param font 字体
     * @param value 文本值
     * @param sel_start 选中起始位置
     * @param sel_end 选中结束位置
     * @param is_password 是否为密码类型
     */
    void PaintSelectionHighlight(float text_x, 
                                 const Box& box, 
                                 const SkFont& font,
                                 const std::string& value,
                                 int sel_start, 
                                 int sel_end,
                                 bool is_password);

    /**
     * @brief 绘制输入光标
     * @param text_x 文本起始 X 坐标
     * @param box 盒模型定义
     * @param font 字体
     * @param font_metrics 字体度量
     * @param value 文本值
     * @param cursor_pos 光标位置
     * @param is_password 是否为密码类型
     */
    void PaintCursor(float text_x, 
                     const Box& box, 
                     const SkFont& font,
                     const SkFontMetrics& font_metrics,
                     const std::string& value,
                     int cursor_pos,
                     bool is_password);

    /**
     * @brief 绘制多行文本框光标
     * @param text_x 文本起始 X 坐标
     * @param text_y 文本起始 Y 坐标
     * @param font 字体
     * @param font_metrics 字体度量
     * @param line_height 行高
     * @param value 文本值
     * @param cursor_pos 光标位置
     */
    void PaintTextAreaCursor(float text_x, 
                             float text_y, 
                             const SkFont& font,
                             const SkFontMetrics& font_metrics,
                             float line_height,
                             const std::string& value,
                             int cursor_pos);

    /**
     * @brief 检查光标是否应该可见（基于闪烁周期）
     * @return 如果光标应该可见返回 true
     */
    bool IsCursorVisible() const;

    /**
     * @brief 创建字体
     * @param params 绘制参数
     * @return Skia 字体对象
     */
    SkFont CreateFont(const FormElementPaintParams& params);

    /**
     * @brief 获取文本颜色
     * @param params 绘制参数
     * @param is_placeholder 是否为占位符文本
     * @return 文本颜色
     */
    SkColor GetTextColor(const FormElementPaintParams& params, bool is_placeholder);

    SkCanvas* canvas_;
};

} // namespace lightui
