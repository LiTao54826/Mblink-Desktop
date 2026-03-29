/**
 * @file paint.h
 * @brief 画笔管理模块
 * 
 * 功能：
 * - 封装 SkPaint
 * - 管理颜色、线宽、样式
 * - 支持填充和描边模式
 * - 支持抗锯齿
 */

#pragma once

#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"

namespace mbink {

/**
 * @brief 画笔样式
 */
enum class PaintStyle {
    FILL,       ///< 填充
    STROKE,     ///< 描边
    FILL_STROKE ///< 填充+描边
};

/**
 * @brief 线条端点样式
 */
enum class StrokeCap {
    BUTT,   ///< 平头
    ROUND,  ///< 圆头
    SQUARE  ///< 方头
};

/**
 * @brief 线条连接样式
 */
enum class StrokeJoin {
    MITER,  ///< 尖角
    ROUND,  ///< 圆角
    BEVEL   ///< 斜角
};

/**
 * @brief 画笔类
 * 
 * 封装 Skia SkPaint，提供便捷的画笔配置接口
 */
class Paint {
public:
    /**
     * @brief 构造函数
     */
    Paint();
    
    /**
     * @brief 析构函数
     */
    ~Paint() = default;
    
    // ========== 颜色设置 ==========
    
    /**
     * @brief 设置颜色
     * @param color 颜色值
     */
    void SetColor(SkColor color);
    
    /**
     * @brief 获取颜色
     * @return 颜色值
     */
    SkColor GetColor() const;
    
    /**
     * @brief 设置透明度
     * @param alpha 透明度 (0-255)
     */
    void SetAlpha(int alpha);
    
    /**
     * @brief 获取透明度
     * @return 透明度 (0-255)
     */
    int GetAlpha() const;
    
    // ========== 样式设置 ==========
    
    /**
     * @brief 设置画笔样式
     * @param style 画笔样式
     */
    void SetStyle(PaintStyle style);
    
    /**
     * @brief 获取画笔样式
     * @return 画笔样式
     */
    PaintStyle GetStyle() const;
    
    // ========== 描边设置 ==========
    
    /**
     * @brief 设置线宽
     * @param width 线宽
     */
    void SetStrokeWidth(float width);
    
    /**
     * @brief 获取线宽
     * @return 线宽
     */
    float GetStrokeWidth() const;
    
    /**
     * @brief 设置线条端点样式
     * @param cap 端点样式
     */
    void SetStrokeCap(StrokeCap cap);
    
    /**
     * @brief 获取线条端点样式
     * @return 端点样式
     */
    StrokeCap GetStrokeCap() const;
    
    /**
     * @brief 设置线条连接样式
     * @param join 连接样式
     */
    void SetStrokeJoin(StrokeJoin join);
    
    /**
     * @brief 获取线条连接样式
     * @return 连接样式
     */
    StrokeJoin GetStrokeJoin() const;
    
    /**
     * @brief 设置斜接限制
     * @param miter 斜接限制
     */
    void SetStrokeMiter(float miter);
    
    /**
     * @brief 获取斜接限制
     * @return 斜接限制
     */
    float GetStrokeMiter() const;
    
    // ========== 抗锯齿设置 ==========
    
    /**
     * @brief 设置抗锯齿
     * @param antiAlias 是否抗锯齿
     */
    void SetAntiAlias(bool antiAlias);
    
    /**
     * @brief 获取抗锯齿状态
     * @return 是否抗锯齿
     */
    bool IsAntiAlias() const;
    
    // ========== 其他设置 ==========
    
    /**
     * @brief 设置抖动
     * @param dither 是否抖动
     */
    void SetDither(bool dither);
    
    /**
     * @brief 获取抖动状态
     * @return 是否抖动
     */
    bool IsDither() const;
    
    /**
     * @brief 重置画笔为默认状态
     */
    void Reset();
    
    /**
     * @brief 获取底层 SkPaint 对象
     * @return SkPaint 引用
     */
    SkPaint& GetSkPaint() { return paint_; }
    
    /**
     * @brief 获取底层 SkPaint 对象（常量）
     * @return SkPaint 常量引用
     */
    const SkPaint& GetSkPaint() const { return paint_; }

private:
    SkPaint paint_;  ///< Skia 画笔对象
};

} // namespace mbink
