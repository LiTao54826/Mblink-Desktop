/**
 * @file computed_styles_view.h
 * @brief 计算样式视图
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "core/dom/element.h"

class SkCanvas;

namespace mblink {

// 前向声明
class RenderObject;
struct ComputedStyle;

/**
 * @brief 计算样式属性
 */
struct ComputedStyleProperty {
    std::string name;
    std::string value;
    bool is_default;
    bool is_inherited;
};

/**
 * @brief 计算样式类别
 */
struct ComputedStyleCategory {
    std::string name;
    std::vector<ComputedStyleProperty> properties;
    bool expanded = true;
};

/**
 * @brief 计算样式视图
 */
class ComputedStylesView {
public:
    ComputedStylesView();
    ~ComputedStylesView();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 渲染视图
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief 获取计算样式（按类别分组）
     */
    std::vector<ComputedStyleCategory> GetComputedStyles() const;

private:
    std::weak_ptr<Element> element_;
    std::vector<ComputedStyleCategory> categories_;

    void RefreshStyles();
    void RenderCategory(SkCanvas* canvas, const ComputedStyleCategory& category,
                        float x, float& y, float width);
    
    /**
     * @brief 查找元素对应的 RenderObject
     * @param element 目标元素
     * @return 对应的 RenderObject，未找到返回 nullptr
     */
    std::shared_ptr<RenderObject> FindRenderObject(std::shared_ptr<Element> element) const;
    
    /**
     * @brief 从 ComputedStyle 获取属性值的字符串表示
     * @param style 计算样式
     * @param property_name 属性名
     * @return 属性值字符串
     */
    std::string GetPropertyValue(const ComputedStyle& style, const std::string& property_name) const;
    
    /**
     * @brief 判断属性值是否为默认值
     * @param property_name 属性名
     * @param value 属性值
     * @return 是否为默认值
     */
    bool IsDefaultValue(const std::string& property_name, const std::string& value) const;
};

} // namespace mblink
