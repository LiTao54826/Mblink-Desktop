/**
 * @file canvas_gradient.h
 * @brief Canvas渐变对象
 */

#pragma once

#include "include/core/SkColor.h"
#include "include/core/SkShader.h"
#include "include/effects/SkGradientShader.h"
#include <vector>
#include <string>

namespace mbink {

/**
 * @brief Canvas渐变类型
 */
enum class CanvasGradientType {
    LINEAR,
    RADIAL,
    CONIC
};

/**
 * @brief Canvas渐变对象
 * 
 * 对应HTML5 CanvasGradient接口
 */
class CanvasGradient {
public:
    /**
     * @brief 创建线性渐变
     */
    static CanvasGradient* CreateLinear(double x0, double y0, double x1, double y1);
    
    /**
     * @brief 创建径向渐变
     */
    static CanvasGradient* CreateRadial(double x0, double y0, double r0, double x1, double y1, double r1);
    
    /**
     * @brief 创建锥形渐变
     * @param startAngle 起始角度（弧度）
     * @param x 中心X坐标
     * @param y 中心Y坐标
     */
    static CanvasGradient* CreateConic(double startAngle, double x, double y);
    
    /**
     * @brief 添加颜色停止点
     * @param offset 位置(0.0-1.0)
     * @param color CSS颜色字符串
     */
    void AddColorStop(double offset, const std::string& color);
    
    /**
     * @brief 获取Skia Shader
     * @return Skia Shader指针
     */
    sk_sp<SkShader> GetShader() const;
    
    /**
     * @brief 获取渐变类型
     */
    CanvasGradientType GetType() const { return type_; }

private:
    CanvasGradient(CanvasGradientType type);
    
    CanvasGradientType type_;
    
    // 线性渐变参数
    double x0_, y0_, x1_, y1_;
    
    // 径向渐变参数
    double r0_, r1_;
    
    // 锥形渐变参数
    double start_angle_;
    
    // 颜色停止点
    struct ColorStop {
        double offset;
        SkColor color;
    };
    std::vector<ColorStop> color_stops_;
};

} // namespace mbink
