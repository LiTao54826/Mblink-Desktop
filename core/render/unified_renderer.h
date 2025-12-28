#ifndef LIGHTUI_UNIFIED_RENDERER_H
#define LIGHTUI_UNIFIED_RENDERER_H

#include "core/render/color.h"
#include "core/render/paint.h"
#include "core/render/shapes.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/image/image_loader.h"
#include "core/render/image/image_renderer.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkFont.h"
#include "include/core/SkImage.h"

#include <memory>
#include <string>

namespace lightui {

/**
 * @brief 统一渲染器 - 提供简化的高层渲染 API
 * 
 * UnifiedRenderer 是一个高层渲染接口，内部使用 Shapes、TextRenderer、ImageRenderer 等低层 API。
 * 它提供了状态管理（Paint、Font），使得 JavaScript 绑定更加简单。
 * 
 * 特性：
 * - 内置状态管理（fill/stroke paint, font）
 * - 简化的绘制接口（不需要每次传递 Paint）
 * - 延迟创建辅助对象（按需创建）
 * - 易于 JavaScript 绑定
 * 
 * 使用示例：
 * @code
 * UnifiedRenderer renderer(800, 600);
 * renderer.SetFillColor("#3498db");
 * renderer.FillRect(50, 50, 200, 150);
 * renderer.SetFont("Arial", 24);
 * renderer.DrawText("Hello", 50, 250);
 * renderer.SaveToFile("output.png");
 * @endcode
 */
class UnifiedRenderer {
public:
    // ==================== 构造函数 ====================
    
    /**
     * @brief 创建指定大小的渲染器（自动创建 Surface）
     * @param width 宽度（像素）
     * @param height 高度（像素）
     */
    UnifiedRenderer(int width, int height);
    
    /**
     * @brief 使用现有 Surface 创建渲染器
     * @param surface Skia Surface 对象
     */
    explicit UnifiedRenderer(sk_sp<SkSurface> surface);
    
    /**
     * @brief 使用现有 Canvas 创建渲染器（不拥有 Surface）
     * @param canvas Skia Canvas 对象
     */
    explicit UnifiedRenderer(SkCanvas* canvas);
    
    ~UnifiedRenderer() = default;
    
    // ==================== 状态设置方法 ====================
    
    /**
     * @brief 设置填充颜色
     * @param color 颜色对象
     */
    void SetFillColor(SkColor color);

    /**
     * @brief 设置填充颜色（字符串格式）
     * @param color_str 颜色字符串（支持 #RGB, #RRGGBB, rgb(), rgba()）
     */
    void SetFillColor(const std::string& color_str);

    /**
     * @brief 设置描边颜色
     * @param color 颜色对象
     */
    void SetStrokeColor(SkColor color);

    /**
     * @brief 设置描边颜色（字符串格式）
     * @param color_str 颜色字符串
     */
    void SetStrokeColor(const std::string& color_str);
    
    /**
     * @brief 设置线宽
     * @param width 线宽（像素）
     */
    void SetLineWidth(float width);
    
    /**
     * @brief 设置字体
     * @param family 字体族名称
     * @param size 字体大小（像素）
     * @param bold 是否粗体
     * @param italic 是否斜体
     */
    void SetFont(const std::string& family, float size, bool bold = false, bool italic = false);
    
    /**
     * @brief 设置字体大小
     * @param size 字体大小（像素）
     */
    void SetFontSize(float size);
    
    /**
     * @brief 设置透明度
     * @param alpha 透明度（0.0 - 1.0）
     */
    void SetOpacity(float alpha);
    
    // ==================== 基础图形绘制 ====================
    
    /**
     * @brief 填充矩形
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void FillRect(float x, float y, float width, float height);
    
    /**
     * @brief 描边矩形
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void DrawRect(float x, float y, float width, float height);
    
    /**
     * @brief 填充圆形
     * @param cx 圆心 X 坐标
     * @param cy 圆心 Y 坐标
     * @param radius 半径
     */
    void FillCircle(float cx, float cy, float radius);
    
    /**
     * @brief 描边圆形
     * @param cx 圆心 X 坐标
     * @param cy 圆心 Y 坐标
     * @param radius 半径
     */
    void DrawCircle(float cx, float cy, float radius);
    
    /**
     * @brief 绘制线条
     * @param x1 起点 X 坐标
     * @param y1 起点 Y 坐标
     * @param x2 终点 X 坐标
     * @param y2 终点 Y 坐标
     */
    void DrawLine(float x1, float y1, float x2, float y2);
    
    /**
     * @brief 填充圆角矩形
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param radius 圆角半径
     */
    void FillRoundRect(float x, float y, float width, float height, float radius);
    
    /**
     * @brief 描边圆角矩形
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     * @param radius 圆角半径
     */
    void DrawRoundRect(float x, float y, float width, float height, float radius);
    
    // ==================== 文本绘制 ====================
    
    /**
     * @brief 绘制文本
     * @param text 文本内容
     * @param x X 坐标
     * @param y Y 坐标（基线位置）
     */
    void DrawText(const std::string& text, float x, float y);
    
    /**
     * @brief 测量文本尺寸
     * @param text 文本内容
     * @return 文本度量信息
     */
    TextMetrics MeasureText(const std::string& text);
    
    // ==================== 图片绘制 ====================
    
    /**
     * @brief 从文件加载图片
     * @param path 图片文件路径
     * @return 图片对象（失败返回 nullptr）
     */
    sk_sp<SkImage> LoadImage(const std::string& path);
    
    /**
     * @brief 绘制图片（原始大小）
     * @param image 图片对象
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     */
    void DrawImage(sk_sp<SkImage> image, float x, float y);
    
    /**
     * @brief 绘制图片（指定大小）
     * @param image 图片对象
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void DrawImage(sk_sp<SkImage> image, float x, float y, float width, float height);
    
    // ==================== 渲染控制 ====================
    
    /**
     * @brief 清空画布
     * @param color 清空颜色（默认白色）
     */
    void Clear(SkColor color = SK_ColorWHITE);
    
    /**
     * @brief 刷新渲染（提交到 GPU）
     */
    void Flush();
    
    /**
     * @brief 保存为 PNG 文件
     * @param path 文件路径
     * @return 是否成功
     */
    bool SaveToFile(const std::string& path);
    
    // ==================== 状态管理 ====================
    
    /**
     * @brief 保存当前状态（变换、裁剪等）
     */
    void Save();
    
    /**
     * @brief 恢复之前保存的状态
     */
    void Restore();
    
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
    
    // ==================== 获取器 ====================
    
    /**
     * @brief 获取底层 Canvas 对象
     * @return Canvas 指针
     */
    SkCanvas* GetCanvas() const { return canvas_; }
    
    /**
     * @brief 获取底层 Surface 对象
     * @return Surface 智能指针
     */
    sk_sp<SkSurface> GetSurface() const { return surface_; }

private:
    // 核心对象
    sk_sp<SkSurface> surface_;      ///< Skia Surface（可能为空）
    SkCanvas* canvas_;               ///< Skia Canvas（必须有效）
    bool owns_surface_;              ///< 是否拥有 Surface
    
    // 状态管理
    Paint fill_paint_;               ///< 填充画笔
    Paint stroke_paint_;             ///< 描边画笔
    SkFont current_font_;            ///< 当前字体
    std::shared_ptr<FontManager> font_manager_;  ///< 字体管理器
    
    // 辅助对象（延迟创建）
    std::unique_ptr<Shapes> shapes_;                    ///< 图形绘制器
    std::unique_ptr<TextRenderer> text_renderer_;       ///< 文本渲染器
    std::unique_ptr<ImageRenderer> image_renderer_;     ///< 图片渲染器
    
    // 辅助方法
    void EnsureShapes();          ///< 确保 Shapes 已创建
    void EnsureTextRenderer();    ///< 确保 TextRenderer 已创建
    void EnsureImageRenderer();   ///< 确保 ImageRenderer 已创建
};

} // namespace lightui

#endif // LIGHTUI_UNIFIED_RENDERER_H

