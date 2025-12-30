/**
 * @file text_style.h
 * @brief 共享文本样式定义
 *
 * 提供终端和日志视图共享的样式结构和颜色调色板。
 */

#pragma once

#include "include/core/SkColor.h"

#include <array>
#include <cstdint>

namespace lightui {

/**
 * @brief 文本样式
 *
 * 紧凑的样式结构，用于存储每个字符的显示属性。
 * 总大小：4 bytes
 */
struct TextStyle {
    uint8_t fg_color = 7;   ///< 前景色索引 (0-255)
    uint8_t bg_color = 0;   ///< 背景色索引 (0-255)
    uint8_t flags = 0;      ///< 样式标志

    /// 样式标志位
    static constexpr uint8_t BOLD          = 0x01;  ///< 粗体
    static constexpr uint8_t ITALIC        = 0x02;  ///< 斜体
    static constexpr uint8_t UNDERLINE     = 0x04;  ///< 下划线
    static constexpr uint8_t INVERSE       = 0x08;  ///< 反色
    static constexpr uint8_t STRIKETHROUGH = 0x10;  ///< 删除线
    static constexpr uint8_t DIM           = 0x20;  ///< 暗淡
    static constexpr uint8_t BLINK         = 0x40;  ///< 闪烁（通常忽略）
    static constexpr uint8_t HIDDEN        = 0x80;  ///< 隐藏

    /**
     * @brief 检查是否为默认样式
     * @return 是默认样式返回 true
     */
    bool IsDefault() const {
        return fg_color == 7 && bg_color == 0 && flags == 0;
    }

    /**
     * @brief 重置为默认样式
     */
    void Reset() {
        fg_color = 7;
        bg_color = 0;
        flags = 0;
    }

    // 样式标志操作
    bool IsBold() const { return flags & BOLD; }
    bool IsItalic() const { return flags & ITALIC; }
    bool IsUnderline() const { return flags & UNDERLINE; }
    bool IsInverse() const { return flags & INVERSE; }
    bool IsStrikethrough() const { return flags & STRIKETHROUGH; }
    bool IsDim() const { return flags & DIM; }
    bool IsHidden() const { return flags & HIDDEN; }

    void SetBold(bool v) { v ? (flags |= BOLD) : (flags &= ~BOLD); }
    void SetItalic(bool v) { v ? (flags |= ITALIC) : (flags &= ~ITALIC); }
    void SetUnderline(bool v) { v ? (flags |= UNDERLINE) : (flags &= ~UNDERLINE); }
    void SetInverse(bool v) { v ? (flags |= INVERSE) : (flags &= ~INVERSE); }
    void SetStrikethrough(bool v) { v ? (flags |= STRIKETHROUGH) : (flags &= ~STRIKETHROUGH); }
    void SetDim(bool v) { v ? (flags |= DIM) : (flags &= ~DIM); }
    void SetHidden(bool v) { v ? (flags |= HIDDEN) : (flags &= ~HIDDEN); }
};

/**
 * @brief 256 色调色板
 *
 * 标准 ANSI 256 色调色板：
 * - 0-7: 标准颜色
 * - 8-15: 高亮颜色
 * - 16-231: 216 色立方体 (6x6x6)
 * - 232-255: 24 级灰度
 */
struct ColorPalette {
    std::array<SkColor, 256> colors;

    /**
     * @brief 获取默认调色板
     * @return 默认 256 色调色板
     */
    static ColorPalette Default();

    /**
     * @brief 获取 Solarized Dark 调色板
     * @return Solarized Dark 调色板
     */
    static ColorPalette SolarizedDark();

    /**
     * @brief 获取 Monokai 调色板
     * @return Monokai 调色板
     */
    static ColorPalette Monokai();

    /**
     * @brief 解析颜色索引
     * @param index 颜色索引 (0-255)
     * @return Skia 颜色值
     */
    SkColor Resolve(uint8_t index) const {
        return colors[index];
    }

    /**
     * @brief 设置标准颜色 (0-15)
     * @param index 颜色索引
     * @param color 颜色值
     */
    void SetStandardColor(int index, SkColor color) {
        if (index >= 0 && index < 16) {
            colors[index] = color;
        }
    }
};

// === ColorPalette 实现 ===

inline ColorPalette ColorPalette::Default() {
    ColorPalette palette;

    // 标准颜色 (0-7)
    palette.colors[0] = SkColorSetRGB(0, 0, 0);       // Black
    palette.colors[1] = SkColorSetRGB(205, 49, 49);   // Red
    palette.colors[2] = SkColorSetRGB(13, 188, 121);  // Green
    palette.colors[3] = SkColorSetRGB(229, 229, 16);  // Yellow
    palette.colors[4] = SkColorSetRGB(36, 114, 200);  // Blue
    palette.colors[5] = SkColorSetRGB(188, 63, 188);  // Magenta
    palette.colors[6] = SkColorSetRGB(17, 168, 205);  // Cyan
    palette.colors[7] = SkColorSetRGB(229, 229, 229); // White

    // 高亮颜色 (8-15)
    palette.colors[8] = SkColorSetRGB(102, 102, 102);  // Bright Black
    palette.colors[9] = SkColorSetRGB(241, 76, 76);    // Bright Red
    palette.colors[10] = SkColorSetRGB(35, 209, 139);  // Bright Green
    palette.colors[11] = SkColorSetRGB(245, 245, 67);  // Bright Yellow
    palette.colors[12] = SkColorSetRGB(59, 142, 234);  // Bright Blue
    palette.colors[13] = SkColorSetRGB(214, 112, 214); // Bright Magenta
    palette.colors[14] = SkColorSetRGB(41, 184, 219);  // Bright Cyan
    palette.colors[15] = SkColorSetRGB(255, 255, 255); // Bright White

    // 216 色立方体 (16-231)
    // 6x6x6 RGB 立方体，每个分量 0, 95, 135, 175, 215, 255
    const int levels[] = {0, 95, 135, 175, 215, 255};
    int index = 16;
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                palette.colors[index++] = SkColorSetRGB(levels[r], levels[g], levels[b]);
            }
        }
    }

    // 24 级灰度 (232-255)
    // 从 8 到 238，步长 10
    for (int i = 0; i < 24; ++i) {
        int gray = 8 + i * 10;
        palette.colors[232 + i] = SkColorSetRGB(gray, gray, gray);
    }

    return palette;
}

inline ColorPalette ColorPalette::SolarizedDark() {
    ColorPalette palette = Default();

    // Solarized Dark 主题颜色
    palette.colors[0] = SkColorSetRGB(7, 54, 66);      // base02
    palette.colors[1] = SkColorSetRGB(220, 50, 47);    // red
    palette.colors[2] = SkColorSetRGB(133, 153, 0);    // green
    palette.colors[3] = SkColorSetRGB(181, 137, 0);    // yellow
    palette.colors[4] = SkColorSetRGB(38, 139, 210);   // blue
    palette.colors[5] = SkColorSetRGB(211, 54, 130);   // magenta
    palette.colors[6] = SkColorSetRGB(42, 161, 152);   // cyan
    palette.colors[7] = SkColorSetRGB(238, 232, 213);  // base2

    palette.colors[8] = SkColorSetRGB(0, 43, 54);      // base03
    palette.colors[9] = SkColorSetRGB(203, 75, 22);    // orange
    palette.colors[10] = SkColorSetRGB(88, 110, 117);  // base01
    palette.colors[11] = SkColorSetRGB(101, 123, 131); // base00
    palette.colors[12] = SkColorSetRGB(131, 148, 150); // base0
    palette.colors[13] = SkColorSetRGB(108, 113, 196); // violet
    palette.colors[14] = SkColorSetRGB(147, 161, 161); // base1
    palette.colors[15] = SkColorSetRGB(253, 246, 227); // base3

    return palette;
}

inline ColorPalette ColorPalette::Monokai() {
    ColorPalette palette = Default();

    // Monokai 主题颜色
    palette.colors[0] = SkColorSetRGB(39, 40, 34);     // Background
    palette.colors[1] = SkColorSetRGB(249, 38, 114);   // Red/Pink
    palette.colors[2] = SkColorSetRGB(166, 226, 46);   // Green
    palette.colors[3] = SkColorSetRGB(244, 191, 117);  // Yellow
    palette.colors[4] = SkColorSetRGB(102, 217, 239);  // Blue/Cyan
    palette.colors[5] = SkColorSetRGB(174, 129, 255);  // Purple
    palette.colors[6] = SkColorSetRGB(161, 239, 228);  // Cyan
    palette.colors[7] = SkColorSetRGB(248, 248, 242);  // Foreground

    palette.colors[8] = SkColorSetRGB(117, 113, 94);   // Comment
    palette.colors[9] = SkColorSetRGB(249, 38, 114);   // Bright Red
    palette.colors[10] = SkColorSetRGB(166, 226, 46);  // Bright Green
    palette.colors[11] = SkColorSetRGB(230, 219, 116); // Bright Yellow
    palette.colors[12] = SkColorSetRGB(102, 217, 239); // Bright Blue
    palette.colors[13] = SkColorSetRGB(174, 129, 255); // Bright Purple
    palette.colors[14] = SkColorSetRGB(161, 239, 228); // Bright Cyan
    palette.colors[15] = SkColorSetRGB(248, 248, 242); // Bright White

    return palette;
}

}  // namespace lightui
