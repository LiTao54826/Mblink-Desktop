/**
 * @file text_renderer.h
 * @brief 文本渲染器
 *
 * 功能：
 * - 文本绘制
 * - 文本测量
 * - 文本排版
 * - 文本样式（粗体、斜体、下划线、删除线）
 */

#pragma once

#include <string>
#include <vector>
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkTextBlob.h"
#include "core/render/utils/paint.h"
#include "text/font_manager.h"

namespace mbink {

/**
 * @brief 文本对齐方式
 */
enum class TextAlign {
    LEFT,       ///< 左对齐
    CENTER,     ///< 居中对齐
    RIGHT,      ///< 右对齐
    JUSTIFY     ///< 两端对齐
};

/**
 * @brief 文本装饰
 */
enum class TextDecoration {
    NONE = 0,
    UNDERLINE = 1 << 0,     ///< 下划线
    LINE_THROUGH = 1 << 1,  ///< 删除线
    OVERLINE = 1 << 2       ///< 上划线
};

/**
 * @brief 文本测量结果
 */
struct TextMetrics {
    float width;            ///< 文本宽度
    float height;           ///< 文本高度
    float ascent;           ///< 上升高度
    float descent;          ///< 下降高度
    float leading;          ///< 行间距

    TextMetrics()
        : width(0), height(0), ascent(0), descent(0), leading(0) {}
};

/**
 * @brief 文本渲染器类
 */
class TextRenderer {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布指针
     */
    explicit TextRenderer(SkCanvas* canvas);

    /**
     * @brief 析构函数
     */
    ~TextRenderer() = default;

    // ========== 文本绘制 ==========

    /**
     * @brief 绘制文本
     * @param text 文本内容
     * @param x X 坐标
     * @param y Y 坐标（基线位置）
     * @param font 字体
     * @param paint 画笔
     */
    void DrawText(const std::string& text, float x, float y, const SkFont& font, const Paint& paint);

    /**
     * @brief 绘制文本（使用字体描述符）
     * @param text 文本内容
     * @param x X 坐标
     * @param y Y 坐标
     * @param descriptor 字体描述符
     * @param paint 画笔
     */
    void DrawText(const std::string& text, float x, float y, const FontDescriptor& descriptor, const Paint& paint);

    /**
     * @brief 绘制支持emoji的文本
     *
     * 此方法会自动检测文本中的emoji字符，并使用emoji字体进行渲染。
     * 对于普通文本使用指定的字体，对于emoji使用系统emoji字体。
     *
     * @param text 文本内容（可包含emoji）
     * @param x X 坐标
     * @param y Y 坐标（基线位置）
     * @param font 主字体（用于普通文本）
     * @param paint 画笔
     */
    void DrawTextWithEmoji(const std::string& text, float x, float y, const SkFont& font, const Paint& paint);

    /**
     * @brief 绘制多行文本
     * @param text 文本内容
     * @param x X 坐标
     * @param y Y 坐标
     * @param max_width 最大宽度
     * @param line_height 行高
     * @param font 字体
     * @param paint 画笔
     */
    void DrawMultilineText(const std::string& text, float x, float y, float max_width,
                          float line_height, const SkFont& font, const Paint& paint);

    // ========== 文本测量 ==========

    /**
     * @brief 测量文本
     * @param text 文本内容
     * @param font 字体
     * @return 文本测量结果
     */
    TextMetrics MeasureText(const std::string& text, const SkFont& font);

    /**
     * @brief 测量文本宽度
     * @param text 文本内容
     * @param font 字体
     * @return 文本宽度
     */
    float MeasureTextWidth(const std::string& text, const SkFont& font);

    /**
     * @brief 测量文本高度
     * @param font 字体
     * @return 文本高度
     */
    float MeasureTextHeight(const SkFont& font);

    /**
     * @brief 测量包含emoji的文本宽度
     * @param text 文本内容（可包含emoji）
     * @param font 主字体
     * @return 文本宽度
     */
    float MeasureTextWidthWithEmoji(const std::string& text, const SkFont& font);

    /**
     * @brief 静态方法：测量包含emoji/CJK的文本宽度
     * @param text 文本内容
     * @param font 主字体
     * @return 文本宽度
     */
    static float MeasureMixedTextWidth(const std::string& text, const SkFont& font);

    /**
     * @brief 测量文本的最小内容宽度（最长单词的宽度）
     * @param text 文本内容
     * @param font 字体
     * @return 最小内容宽度
     */
    float MeasureMinContentWidth(const std::string& text, const SkFont& font);

    // ========== 文本装饰 ==========

    /**
     * @brief 绘制下划线
     * @param x 起始 X 坐标
     * @param y 基线 Y 坐标
     * @param width 宽度
     * @param paint 画笔
     */
    void DrawUnderline(float x, float y, float width, const Paint& paint);

    /**
     * @brief 绘制删除线
     * @param x 起始 X 坐标
     * @param y 基线 Y 坐标
     * @param width 宽度
     * @param font 字体
     * @param paint 画笔
     */
    void DrawLineThrough(float x, float y, float width, const SkFont& font, const Paint& paint);

    // ========== 文本换行 ==========

    /**
     * @brief 将文本分割为多行
     * @param text 文本内容
     * @param max_width 最大宽度
     * @param font 字体
     * @return 文本行数组
     */
    std::vector<std::string> WrapText(const std::string& text, float max_width, const SkFont& font);

    // ========== 画布访问 ==========

    /**
     * @brief 获取画布指针
     * @return SkCanvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }

private:
    SkCanvas* canvas_;  ///< Skia 画布指针
};

} // namespace mbink
