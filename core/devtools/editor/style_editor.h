/**
 * @file style_editor.h
 * @brief 样式编辑器
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include "core/dom/element.h"

namespace lightui {

/**
 * @brief 样式编辑器
 */
class StyleEditor {
public:
    StyleEditor();
    ~StyleEditor();

    /**
     * @brief 设置目标元素
     */
    void SetElement(std::shared_ptr<Element> element);

    /**
     * @brief 设置样式属性
     * @return 是否成功（无效值返回 false）
     */
    bool SetStyleProperty(const std::string& property, const std::string& value);

    /**
     * @brief 添加样式属性
     */
    bool AddStyleProperty(const std::string& property, const std::string& value);

    /**
     * @brief 删除样式属性
     */
    bool RemoveStyleProperty(const std::string& property);

    /**
     * @brief 获取样式属性值
     */
    std::string GetStyleProperty(const std::string& property) const;

    /**
     * @brief 验证样式值是否有效
     */
    static bool ValidateStyleValue(const std::string& property, const std::string& value);

    /**
     * @brief 样式变更回调
     */
    using StyleChangeCallback = std::function<void(const std::string& property, const std::string& value)>;
    void SetOnStyleChanged(StyleChangeCallback callback) { on_style_changed_ = callback; }

private:
    std::weak_ptr<Element> element_;
    StyleChangeCallback on_style_changed_;
};

} // namespace lightui
