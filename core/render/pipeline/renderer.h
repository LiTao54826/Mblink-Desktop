/**
 * @file renderer.h
 * @brief Skia 渲染器基类
 *
 * 功能：
 * - 管理 SkCanvas 和 SkSurface
 * - 提供基础绘制接口
 * - 实现 RAII 资源管理
 * - 支持状态保存和恢复
 * - 支持变换和裁剪
 */

#pragma once

#include <memory>
#include <stdexcept>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkColor.h"

namespace mbink {

/**
 * @brief Skia 渲染器基类
 *
 * 管理 Skia 画布和表面，提供基础绘制接口
 */
class Renderer {
public:
    /**
     * @brief 构造函数 - 使用外部 SkCanvas
     * @param canvas Skia 画布指针（不拥有所有权）
     * @throws std::invalid_argument 如果 canvas 为 nullptr
     */
    explicit Renderer(SkCanvas* canvas);

    /**
     * @brief 构造函数 - 使用 SkSurface
     * @param surface Skia 表面智能指针
     * @throws std::invalid_argument 如果 surface 为 nullptr
     */
    explicit Renderer(sk_sp<SkSurface> surface);

    /**
     * @brief 析构函数
     */
    ~Renderer();

    // 禁止拷贝
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // 允许移动
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    // ========== 基础操作 ==========

    /**
     * @brief 清空画布
     * @param color 清空颜色（默认白色）
     */
    void Clear(SkColor color = SK_ColorWHITE);

    /**
     * @brief 刷新渲染命令
     */
    void Flush();

    /**
     * @brief 获取画布指针
     * @return SkCanvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }

    /**
     * @brief 获取表面指针
     * @return SkSurface 智能指针
     */
    sk_sp<SkSurface> GetSurface() const { return surface_; }

    // ========== 状态管理 ==========

    /**
     * @brief 保存当前状态
     * @return 保存的状态层级
     */
    int Save();

    /**
     * @brief 恢复之前保存的状态
     */
    void Restore();

    /**
     * @brief 获取当前保存层级
     * @return 保存层级数
     */
    int GetSaveCount() const;

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

    // ========== 裁剪操作 ==========

    /**
     * @brief 矩形裁剪
     * @param rect 裁剪矩形
     * @param doAntiAlias 是否抗锯齿
     */
    void ClipRect(const SkRect& rect, bool doAntiAlias = false);

    /**
     * @brief 路径裁剪
     * @param path 裁剪路径
     * @param doAntiAlias 是否抗锯齿
     */
    void ClipPath(const SkPath& path, bool doAntiAlias = false);

private:
    sk_sp<SkSurface> surface_;  ///< Skia 表面（如果拥有）
    SkCanvas* canvas_;          ///< Skia 画布指针
    bool owns_surface_;         ///< 是否拥有 surface
};

} // namespace mbink
