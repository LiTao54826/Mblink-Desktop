/**
 * @file render_context.h
 * @brief 渲染上下文管理
 * 
 * 功能：
 * - 管理渲染状态
 * - 管理变换矩阵 (translate, rotate, scale)
 * - 管理裁剪区域 (clip)
 * - 实现状态栈 (save/restore)
 */

#pragma once

#include <stack>
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkCanvas.h"
#include "core/render/utils/paint.h"

namespace mblink {

/**
 * @brief 渲染状态
 */
struct RenderState {
    SkMatrix matrix;        ///< 变换矩阵
    SkRect clip_rect;       ///< 裁剪矩形
    Paint paint;            ///< 画笔
    float global_alpha;     ///< 全局透明度
    
    RenderState()
        : matrix(SkMatrix::I())
        , clip_rect(SkRect::MakeEmpty())
        , paint()
        , global_alpha(1.0f) {}
};

/**
 * @brief 渲染上下文类
 * 
 * 管理渲染状态，支持状态保存和恢复
 */
class RenderContext {
public:
    /**
     * @brief 构造函数
     * @param canvas Skia 画布指针
     */
    explicit RenderContext(SkCanvas* canvas);
    
    /**
     * @brief 析构函数
     */
    ~RenderContext() = default;
    
    // ========== 状态管理 ==========
    
    /**
     * @brief 保存当前状态
     */
    void Save();
    
    /**
     * @brief 恢复之前保存的状态
     */
    void Restore();
    
    /**
     * @brief 获取状态栈深度
     * @return 状态栈深度
     */
    size_t GetStateStackSize() const;
    
    // ========== 变换操作 ==========
    
    /**
     * @brief 平移变换
     * @param dx X 方向偏移
     * @param dy Y 方向偏移
     */
    void Translate(float dx, float dy);
    
    /**
     * @brief 缩放变换
     * @param sx X 方向缩放
     * @param sy Y 方向缩放
     */
    void Scale(float sx, float sy);
    
    /**
     * @brief 旋转变换
     * @param degrees 旋转角度（度）
     */
    void Rotate(float degrees);
    
    /**
     * @brief 旋转变换（指定中心点）
     * @param degrees 旋转角度（度）
     * @param px 旋转中心 X 坐标
     * @param py 旋转中心 Y 坐标
     */
    void Rotate(float degrees, float px, float py);
    
    /**
     * @brief 设置变换矩阵
     * @param matrix 变换矩阵
     */
    void SetMatrix(const SkMatrix& matrix);
    
    /**
     * @brief 获取当前变换矩阵
     * @return 变换矩阵
     */
    const SkMatrix& GetMatrix() const;
    
    /**
     * @brief 重置变换矩阵为单位矩阵
     */
    void ResetMatrix();
    
    // ========== 裁剪操作 ==========
    
    /**
     * @brief 矩形裁剪
     * @param rect 裁剪矩形
     */
    void ClipRect(const SkRect& rect);
    
    /**
     * @brief 路径裁剪
     * @param path 裁剪路径
     */
    void ClipPath(const SkPath& path);
    
    /**
     * @brief 获取当前裁剪矩形
     * @return 裁剪矩形
     */
    const SkRect& GetClipRect() const;
    
    // ========== 画笔管理 ==========
    
    /**
     * @brief 设置画笔
     * @param paint 画笔
     */
    void SetPaint(const Paint& paint);
    
    /**
     * @brief 获取当前画笔
     * @return 画笔引用
     */
    Paint& GetPaint();
    
    /**
     * @brief 获取当前画笔（常量）
     * @return 画笔常量引用
     */
    const Paint& GetPaint() const;
    
    // ========== 透明度管理 ==========
    
    /**
     * @brief 设置全局透明度
     * @param alpha 透明度 (0.0-1.0)
     */
    void SetGlobalAlpha(float alpha);
    
    /**
     * @brief 获取全局透明度
     * @return 透明度 (0.0-1.0)
     */
    float GetGlobalAlpha() const;
    
    // ========== 画布访问 ==========
    
    /**
     * @brief 获取画布指针
     * @return SkCanvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }
    
    /**
     * @brief 应用当前状态到画布
     */
    void ApplyToCanvas();

private:
    SkCanvas* canvas_;                      ///< Skia 画布指针
    std::stack<RenderState> state_stack_;   ///< 状态栈
    RenderState current_state_;             ///< 当前状态
};

} // namespace mblink

