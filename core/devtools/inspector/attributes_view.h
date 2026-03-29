/**
 * @file attributes_view.h
 * @brief 属性视图组件 - 显示元素的所有属性
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "core/dom/element.h"

class SkCanvas;

namespace mbink {

/**
 * @brief 属性信息
 */
struct AttributeInfo {
    std::string name;
    std::string value;
    bool is_id = false;      // 是否为 id 属性
    bool is_class = false;   // 是否为 class 属性
    bool is_data = false;    // 是否为 data-* 属性
};

/**
 * @brief 属性视图
 */
class AttributesView {
public:
    AttributesView();
    ~AttributesView();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 获取所有属性
     */
    std::vector<AttributeInfo> GetAttributes() const;

    /**
     * @brief 渲染视图
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

private:
    std::weak_ptr<Element> element_;

    void RenderEmptyState(SkCanvas* canvas, float x, float y, float width, float height);
    void RenderAttributeList(SkCanvas* canvas, float x, float y, float width, float height,
                             const std::vector<AttributeInfo>& attributes);
};

} // namespace mbink
