/**
 * @file html_canvas_element.h
 * @brief HTML Canvas元素类
 * 
 * 功能：
 * - 实现<canvas>元素
 * - 支持width, height属性
 * - 提供2D渲染上下文
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

class CanvasRenderingContext2D;

/**
 * @brief HTML Canvas元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/canvas.html#the-canvas-element
 * 
 * IDL定义：
 * interface HTMLCanvasElement : HTMLElement {
 *   attribute unsigned long width;
 *   attribute unsigned long height;
 *   RenderingContext? getContext(DOMString contextId, optional any options = null);
 *   USVString toDataURL(optional DOMString type = "image/png", optional any quality);
 * };
 */
class HTMLCanvasElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLCanvasElement();

    /**
     * @brief 析构函数
     */
    ~HTMLCanvasElement() override;

    // ========== IDL属性 ==========

    /**
     * @brief 获取width属性（画布宽度）
     * @return width属性值，默认300
     */
    unsigned long GetWidth() const { return width_; }

    /**
     * @brief 设置width属性
     * @param width 画布宽度
     */
    void SetWidth(unsigned long width);

    /**
     * @brief 获取height属性（画布高度）
     * @return height属性值，默认150
     */
    unsigned long GetHeight() const { return height_; }

    /**
     * @brief 设置height属性
     * @param height 画布高度
     */
    void SetHeight(unsigned long height);

    // ========== IDL方法 ==========

    /**
     * @brief 获取渲染上下文
     * @param context_id 上下文类型（"2d", "webgl"等）
     * @return 渲染上下文指针，不支持的类型返回nullptr
     */
    void* GetContext(const std::string& context_id);

    /**
     * @brief 获取2D渲染上下文（类型安全版本）
     * @return 2D渲染上下文指针，如果未创建则返回nullptr
     */
    CanvasRenderingContext2D* GetContext2D() const { return context_2d_.get(); }

    /**
     * @brief 将画布内容导出为Data URL
     * @param type MIME类型（默认"image/png"）
     * @param quality 图片质量（0.0-1.0，仅对有损格式有效）
     * @return Data URL字符串
     */
    std::string ToDataURL(const std::string& type = "image/png", double quality = 1.0);

    // ========== 重写方法 ==========

    /**
     * @brief 设置属性
     * @param name 属性名
     * @param value 属性值
     */
    void SetAttribute(const std::string& name, const std::string& value) override;

    /**
     * @brief 移除属性
     * @param name 属性名
     */
    void RemoveAttribute(const std::string& name) override;

private:
    /**
     * @brief 解析宽度/高度属性值
     * @param value 属性值字符串
     * @param default_value 默认值
     * @return 解析后的数值，解析失败返回默认值
     */
    unsigned long ParseDimension(const std::string& value, unsigned long default_value) const;

    /**
     * @brief 调整画布大小（重新创建surface）
     */
    void ResizeCanvas();

private:
    unsigned long width_;   ///< 画布宽度（默认300）
    unsigned long height_;  ///< 画布高度（默认150）
    
    /// 2D渲染上下文（延迟创建）
    std::unique_ptr<CanvasRenderingContext2D> context_2d_;
};

} // namespace lightui
