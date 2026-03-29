/**
 * @file attribute_editor.h
 * @brief 属性编辑器
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include "core/dom/element.h"

namespace mbink {

/**
 * @brief 属性编辑器
 */
class AttributeEditor {
public:
    AttributeEditor();
    ~AttributeEditor();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 设置属性
     */
    bool SetAttribute(const std::string& name, const std::string& value);

    /**
     * @brief 添加属性
     */
    bool AddAttribute(const std::string& name, const std::string& value);

    /**
     * @brief 删除属性
     */
    bool RemoveAttribute(const std::string& name);

    /**
     * @brief 获取属性值
     */
    std::string GetAttribute(const std::string& name) const;

    /**
     * @brief 验证属性名是否有效
     */
    static bool ValidateAttributeName(const std::string& name);

    /**
     * @brief 属性变更回调
     */
    using AttributeChangeCallback = std::function<void(const std::string& name, const std::string& value)>;
    void SetOnAttributeChanged(AttributeChangeCallback callback) { on_attribute_changed_ = callback; }

private:
    std::weak_ptr<Element> element_;
    AttributeChangeCallback on_attribute_changed_;
};

} // namespace mbink
