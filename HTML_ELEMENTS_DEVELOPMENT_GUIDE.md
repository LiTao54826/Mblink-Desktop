# HTML元素开发指南

**版本**: 1.0  
**生效日期**: 2025-11-12  
**强制执行**: 所有HTML元素实现必须遵循本指南

---

## 🎯 核心原则

### 1. 浏览器标准一致性 (Browser Standard Compliance)
- ✅ **必须**: 所有行为与Chrome/Firefox/Safari一致
- ✅ **必须**: 遵循WHATWG HTML Living Standard
- ✅ **必须**: 实现所有标准IDL属性和方法
- ❌ **禁止**: 添加非标准属性或行为

### 2. React兼容性 (React Compatibility)
- ✅ **必须**: 支持React Virtual DOM操作
- ✅ **必须**: 属性变更触发重新渲染
- ✅ **必须**: 事件系统符合React合成事件
- ✅ **必须**: 支持受控组件和非受控组件

### 3. 性能要求 (Performance Requirements)
- ✅ 元素创建 < 1ms
- ✅ 属性更新 < 0.1ms
- ✅ 样式计算 < 5ms
- ✅ 渲染帧率 >= 60fps

---

## 📁 文件结构规范

### 头文件模板 (*.h)

```cpp
/**
 * @file html_button_element.h
 * @brief HTML Button元素类
 * 
 * 标准参考:
 * - WHATWG HTML: https://html.spec.whatwg.org/multipage/form-elements.html#the-button-element
 * - MDN Web Docs: https://developer.mozilla.org/en-US/docs/Web/HTML/Element/button
 * - Web IDL: https://html.spec.whatwg.org/multipage/form-elements.html#htmlbuttonelement
 * 
 * 实现状态:
 * - [x] 基础IDL属性
 * - [x] 表单关联
 * - [x] 验证API
 * - [x] 默认样式
 * - [x] 伪类状态
 * - [x] 事件处理
 * 
 * 测试覆盖: 15个单元测试
 * 
 * @author MBink Team
 * @date 2025-11-12
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

namespace lightui {

// 前向声明
class HTMLFormElement;

/**
 * @brief HTML Button元素类
 * 
 * 实现WHATWG HTMLButtonElement接口
 * 参考: https://html.spec.whatwg.org/multipage/form-elements.html#htmlbuttonelement
 */
class HTMLButtonElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLButtonElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLButtonElement() override = default;
    
    // ========== IDL属性 (按标准顺序) ==========
    
    /**
     * @brief 获取disabled属性
     * @return true表示禁用
     * 
     * 标准: https://html.spec.whatwg.org/multipage/form-elements.html#dom-button-disabled
     */
    bool GetDisabled() const { return disabled_; }
    
    /**
     * @brief 设置disabled属性
     * @param disabled 是否禁用
     * 
     * 副作用:
     * - 更新:disabled伪类
     * - 触发属性变更事件
     * - 如果禁用，移除焦点
     */
    void SetDisabled(bool disabled);
    
    // ... 更多IDL属性 ...
    
    // ========== 重写方法 ==========
    
    /**
     * @brief 重写SetAttribute以处理特殊属性
     */
    void SetAttribute(const std::string& name, const std::string& value) override;
    
    /**
     * @brief 应用默认样式
     */
    void ApplyDefaultStyle() override;
    
    /**
     * @brief 处理点击事件
     */
    void HandleClick() override;
    
private:
    // ========== 私有成员变量 (按字母顺序) ==========
    
    bool disabled_ = false;
    std::string form_action_;
    std::string form_enctype_;
    std::string form_method_;
    bool form_no_validate_ = false;
    std::string form_target_;
    std::string name_;
    std::string type_ = "submit";  // submit | reset | button
    std::string value_;
    
    // 验证相关
    std::string custom_validity_;
    
    // ========== 私有辅助方法 ==========
    
    /**
     * @brief 更新伪类状态
     */
    void UpdatePseudoClasses();
    
    /**
     * @brief 查找关联的表单
     */
    std::shared_ptr<HTMLFormElement> FindForm() const;
};

} // namespace lightui
```

### 实现文件模板 (*.cpp)

```cpp
/**
 * @file html_button_element.cpp
 * @brief HTML Button元素实现
 */

#include "html_button_element.h"
#include "html_form_element.h"
#include "event.h"
#include <algorithm>

namespace lightui {

// ========== 构造函数 ==========

HTMLButtonElement::HTMLButtonElement()
    : Element("button")
    , disabled_(false)
    , form_no_validate_(false)
    , type_("submit") {
    
    // 应用默认样式
    ApplyDefaultStyle();
}

// ========== IDL属性实现 ==========

void HTMLButtonElement::SetDisabled(bool disabled) {
    if (disabled_ == disabled) {
        return;  // 无变化，直接返回
    }
    
    disabled_ = disabled;
    
    // 更新属性
    if (disabled) {
        SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
    
    // 更新伪类
    UpdatePseudoClasses();
    
    // 如果禁用，移除焦点
    if (disabled && IsFocused()) {
        Blur();
    }
    
    // 触发属性变更事件 (用于React)
    // TODO: 实现MutationObserver通知
}

// ========== 重写方法 ==========

void HTMLButtonElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法
    Element::SetAttribute(name, value);
    
    // 处理特殊属性
    if (name == "disabled") {
        SetDisabled(!value.empty());
    } else if (name == "type") {
        SetType(value);
    } else if (name == "name") {
        SetName(value);
    } else if (name == "value") {
        SetValue(value);
    }
}

void HTMLButtonElement::ApplyDefaultStyle() {
    // 应用浏览器默认样式
    // 参考: Chromium User Agent Stylesheet
    
    SetStyle("display", "inline-block");
    SetStyle("padding", "1px 6px");
    SetStyle("border-width", "2px");
    SetStyle("border-style", "outset");
    SetStyle("border-color", "buttonborder");
    SetStyle("background-color", "buttonface");
    SetStyle("color", "buttontext");
    SetStyle("cursor", "pointer");
    SetStyle("text-align", "center");
}

void HTMLButtonElement::HandleClick() {
    // 如果禁用，不处理点击
    if (disabled_) {
        return;
    }
    
    // 根据type执行不同操作
    if (type_ == "submit") {
        // 提交表单
        auto form = FindForm();
        if (form) {
            form->Submit();
        }
    } else if (type_ == "reset") {
        // 重置表单
        auto form = FindForm();
        if (form) {
            form->Reset();
        }
    }
    // type == "button" 不执行默认操作
    
    // 调用基类方法触发事件
    Element::HandleClick();
}

// ========== 私有辅助方法 ==========

void HTMLButtonElement::UpdatePseudoClasses() {
    SetPseudoClass(":disabled", disabled_);
    SetPseudoClass(":enabled", !disabled_);
}

std::shared_ptr<HTMLFormElement> HTMLButtonElement::FindForm() const {
    // 向上查找最近的form元素
    auto parent = GetParentElement();
    while (parent) {
        if (parent->GetTagName() == "form") {
            return std::dynamic_pointer_cast<HTMLFormElement>(parent);
        }
        parent = parent->GetParentElement();
    }
    return nullptr;
}

} // namespace lightui
```

---

## 🧪 测试文件模板

```cpp
/**
 * @file test_html_button_element.cpp
 * @brief HTML Button元素单元测试
 */

#include <gtest/gtest.h>
#include "core/dom/html_button_element.h"
#include "core/dom/html_form_element.h"
#include "core/dom/document.h"

using namespace lightui;

// ========== 基础功能测试 ==========

TEST(HTMLButtonElement, Construction) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    EXPECT_EQ(button->GetTagName(), "button");
    EXPECT_EQ(button->GetType(), "submit");
    EXPECT_FALSE(button->GetDisabled());
}

TEST(HTMLButtonElement, TypeAttribute) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    // 测试所有有效类型
    button->SetType("submit");
    EXPECT_EQ(button->GetType(), "submit");
    
    button->SetType("reset");
    EXPECT_EQ(button->GetType(), "reset");
    
    button->SetType("button");
    EXPECT_EQ(button->GetType(), "button");
    
    // 测试无效类型（应该回退到submit）
    button->SetType("invalid");
    EXPECT_EQ(button->GetType(), "submit");
}

TEST(HTMLButtonElement, DisabledState) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    // 初始状态
    EXPECT_FALSE(button->GetDisabled());
    EXPECT_FALSE(button->HasPseudoClass(":disabled"));
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));
    
    // 设置禁用
    button->SetDisabled(true);
    EXPECT_TRUE(button->GetDisabled());
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));
    EXPECT_FALSE(button->HasPseudoClass(":enabled"));
    EXPECT_EQ(button->GetAttribute("disabled"), "");
    
    // 取消禁用
    button->SetDisabled(false);
    EXPECT_FALSE(button->GetDisabled());
    EXPECT_FALSE(button->HasPseudoClass(":disabled"));
    EXPECT_TRUE(button->HasPseudoClass(":enabled"));
    EXPECT_FALSE(button->HasAttribute("disabled"));
}

// ========== 表单关联测试 ==========

TEST(HTMLButtonElement, FormAssociation) {
    auto doc = std::make_shared<Document>();
    auto form = std::make_shared<HTMLFormElement>();
    auto button = std::make_shared<HTMLButtonElement>();
    
    // 添加button到form
    form->AppendChild(button);
    
    // 验证关联
    auto associated_form = button->GetForm();
    EXPECT_NE(associated_form, nullptr);
    EXPECT_EQ(associated_form, form);
}

// ========== 事件处理测试 ==========

TEST(HTMLButtonElement, ClickEvent) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    bool clicked = false;
    button->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        clicked = true;
    });
    
    // 模拟点击
    button->HandleClick();
    EXPECT_TRUE(clicked);
}

TEST(HTMLButtonElement, DisabledNoClick) {
    auto button = std::make_shared<HTMLButtonElement>();
    button->SetDisabled(true);
    
    bool clicked = false;
    button->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        clicked = true;
    });
    
    // 禁用状态下点击不应触发事件
    button->HandleClick();
    EXPECT_FALSE(clicked);
}

// ========== 默认样式测试 ==========

TEST(HTMLButtonElement, DefaultStyles) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    EXPECT_EQ(button->GetStyle("display"), "inline-block");
    EXPECT_EQ(button->GetStyle("cursor"), "pointer");
    EXPECT_EQ(button->GetStyle("text-align"), "center");
}

// ========== React兼容性测试 ==========

TEST(HTMLButtonElement, ReactAttributeUpdate) {
    auto button = std::make_shared<HTMLButtonElement>();
    
    // 模拟React更新属性
    button->SetAttribute("disabled", "true");
    EXPECT_TRUE(button->GetDisabled());
    
    button->SetAttribute("type", "reset");
    EXPECT_EQ(button->GetType(), "reset");
}

// ========== 性能测试 ==========

TEST(HTMLButtonElement, PerformanceCreation) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto button = std::make_shared<HTMLButtonElement>();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 平均创建时间应该 < 1ms
    EXPECT_LT(duration.count() / 1000.0, 1000.0);
}
```

---

## ✅ 开发检查清单

### 实现前
- [ ] 阅读WHATWG标准文档
- [ ] 查看MDN参考文档
- [ ] 研究Chromium/WebKit实现
- [ ] 确定所有IDL属性和方法
- [ ] 确定默认CSS样式

### 实现中
- [ ] 创建头文件和实现文件
- [ ] 实现所有IDL属性
- [ ] 实现所有IDL方法
- [ ] 应用默认样式
- [ ] 实现伪类状态
- [ ] 处理特殊属性
- [ ] 实现事件处理
- [ ] 添加详细注释

### 实现后
- [ ] 编写至少10个单元测试
- [ ] 所有测试通过
- [ ] 性能测试达标
- [ ] 代码审查通过
- [ ] 更新文档
- [ ] 集成到Element工厂

---

## 🚫 常见错误

### 1. 忘记更新伪类状态
```cpp
// ❌ 错误
void SetDisabled(bool disabled) {
    disabled_ = disabled;
}

// ✅ 正确
void SetDisabled(bool disabled) {
    disabled_ = disabled;
    UpdatePseudoClasses();  // 更新:disabled和:enabled
}
```

### 2. 忘记处理属性同步
```cpp
// ❌ 错误
void SetDisabled(bool disabled) {
    disabled_ = disabled;
}

// ✅ 正确
void SetDisabled(bool disabled) {
    disabled_ = disabled;
    if (disabled) {
        SetAttribute("disabled", "");
    } else {
        RemoveAttribute("disabled");
    }
}
```

### 3. 忘记应用默认样式
```cpp
// ❌ 错误
HTMLButtonElement::HTMLButtonElement()
    : Element("button") {
}

// ✅ 正确
HTMLButtonElement::HTMLButtonElement()
    : Element("button") {
    ApplyDefaultStyle();
}
```

---

## 📚 参考资源

- [WHATWG HTML Standard](https://html.spec.whatwg.org/)
- [MDN Web Docs](https://developer.mozilla.org/en-US/docs/Web/HTML)
- [Chromium Source](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/html/)
- [Web Platform Tests](https://github.com/web-platform-tests/wpt)


