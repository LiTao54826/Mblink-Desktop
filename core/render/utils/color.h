/**
 * @file color.h
 * @brief 颜色管理模块
 * 
 * 功能：
 * - 颜色解析和转换
 * - 支持 RGB, RGBA 格式
 * - 支持 HEX 格式 (#RRGGBB, #RRGGBBAA)
 * - 支持命名颜色 (red, blue, etc.)
 */

#pragma once

#include <string>
#include <unordered_map>
#include "include/core/SkColor.h"

namespace mblink {

/**
 * @brief 颜色工具类
 */
class Color {
public:
    /**
     * @brief 从 RGB 创建颜色
     * @param r 红色分量 (0-255)
     * @param g 绿色分量 (0-255)
     * @param b 蓝色分量 (0-255)
     * @return SkColor
     */
    static SkColor FromRGB(int r, int g, int b);
    
    /**
     * @brief 从 RGBA 创建颜色
     * @param r 红色分量 (0-255)
     * @param g 绿色分量 (0-255)
     * @param b 蓝色分量 (0-255)
     * @param a 透明度 (0-255)
     * @return SkColor
     */
    static SkColor FromRGBA(int r, int g, int b, int a);
    
    /**
     * @brief 从 HEX 字符串创建颜色
     * @param hex HEX 字符串 (#RGB, #RRGGBB, #RRGGBBAA)
     * @return SkColor
     */
    static SkColor FromHex(const std::string& hex);
    
    /**
     * @brief 从命名颜色创建颜色
     * @param name 颜色名称 (red, blue, green, etc.)
     * @return SkColor
     */
    static SkColor FromName(const std::string& name);
    
    /**
     * @brief 从 CSS 颜色字符串解析颜色
     * @param str CSS 颜色字符串 (支持 rgb(), rgba(), #hex, 命名颜色)
     * @return SkColor
     */
    static SkColor Parse(const std::string& str);
    
    /**
     * @brief 获取红色分量
     * @param color SkColor
     * @return 红色分量 (0-255)
     */
    static int GetRed(SkColor color);
    
    /**
     * @brief 获取绿色分量
     * @param color SkColor
     * @return 绿色分量 (0-255)
     */
    static int GetGreen(SkColor color);
    
    /**
     * @brief 获取蓝色分量
     * @param color SkColor
     * @return 蓝色分量 (0-255)
     */
    static int GetBlue(SkColor color);
    
    /**
     * @brief 获取透明度
     * @param color SkColor
     * @return 透明度 (0-255)
     */
    static int GetAlpha(SkColor color);
    
    /**
     * @brief 转换为 HEX 字符串
     * @param color SkColor
     * @param includeAlpha 是否包含透明度
     * @return HEX 字符串
     */
    static std::string ToHex(SkColor color, bool includeAlpha = false);

private:
    /**
     * @brief 初始化命名颜色映射表
     */
    static void InitNamedColors();
    
    /**
     * @brief 解析 rgb() 或 rgba() 字符串
     */
    static SkColor ParseRgbString(const std::string& str);
    
    /**
     * @brief 解析 hsl() 或 hsla() 字符串
     */
    static SkColor ParseHslString(const std::string& str);
    
    /**
     * @brief HSL 转 RGB
     */
    static SkColor HslToRgb(float h, float s, float l, float a = 1.0f);
    
    /**
     * @brief 命名颜色映射表
     */
    static std::unordered_map<std::string, SkColor> named_colors_;
    static bool named_colors_initialized_;
};

} // namespace mblink
