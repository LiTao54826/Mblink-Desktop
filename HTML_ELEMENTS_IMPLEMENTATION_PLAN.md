# MBink HTML元素完整实现计划

**生成时间**: 2025-11-12  
**版本**: 1.0  
**目标**: 实现完整的HTML元素支持，确保React能正常驱动，所有效果与标准浏览器一致

---

## 📊 当前状态

### 已实现的元素类 (2个)
| 元素 | 类名 | 文件 | 完成度 |
|------|------|------|--------|
| `<input>` | HTMLInputElement | core/dom/html_input_element.h/cpp | ⭐⭐⭐⭐⭐ |
| `<textarea>` | HTMLTextAreaElement | core/dom/html_textarea_element.h/cpp | ⭐⭐⭐⭐⭐ |

### 通用Element处理 (~73个标签)
所有其他HTML标签通过基类`Element`处理，支持：
- ✅ 属性管理 (setAttribute, getAttribute)
- ✅ 样式管理 (style, className, classList)
- ✅ 事件监听 (addEventListener, removeEventListener)
- ✅ DOM操作 (appendChild, removeChild, innerHTML)
- ✅ 查询选择器 (querySelector, querySelectorAll)

---

## 🎯 实现优先级分级

### P0 - 关键表单元素 (React必需，1周)

#### 1. HTMLButtonElement
**优先级**: 🔴 最高  
**原因**: React表单、交互必需  
**文件**: `core/dom/html_button_element.h/cpp`

**标准接口** (参考: https://html.spec.whatwg.org/multipage/form-elements.html#the-button-element):
```cpp
class HTMLButtonElement : public Element {
public:
    // 构造函数
    HTMLButtonElement();
    
    // ========== IDL属性 ==========
    
    // type: "submit" | "reset" | "button"
    std::string GetType() const;
    void SetType(const std::string& type);
    
    // disabled: 是否禁用
    bool GetDisabled() const;
    void SetDisabled(bool disabled);
    
    // form: 关联的表单元素
    std::shared_ptr<HTMLFormElement> GetForm() const;
    
    // name: 表单提交时的名称
    std::string GetName() const;
    void SetName(const std::string& name);
    
    // value: 表单提交时的值
    std::string GetValue() const;
    void SetValue(const std::string& value);
    
    // ========== 状态管理 ==========
    
    // 验证状态
    bool CheckValidity();
    bool ReportValidity();
    
    // ========== 渲染相关 ==========
    
    // 默认样式
    void ApplyDefaultStyle() override;
    
    // 伪类状态
    bool IsHovered() const;
    bool IsPressed() const;
    bool IsFocused() const;
    
private:
    std::string type_ = "submit";  // submit, reset, button
    bool disabled_ = false;
    std::string name_;
    std::string value_;
};
```

**默认CSS样式**:
```css
button {
    display: inline-block;
    padding: 2px 6px;
    border: 2px outset buttonface;
    background-color: buttonface;
    color: buttontext;
    cursor: pointer;
    font: inherit;
}

button:hover {
    background-color: buttonhighlight;
}

button:active {
    border-style: inset;
}

button:disabled {
    color: graytext;
    cursor: default;
}
```

**测试用例**:
```cpp
TEST(HTMLButtonElement, BasicCreation) {
    auto button = std::make_shared<HTMLButtonElement>();
    EXPECT_EQ(button->GetTagName(), "button");
    EXPECT_EQ(button->GetType(), "submit");
}

TEST(HTMLButtonElement, TypeAttribute) {
    auto button = std::make_shared<HTMLButtonElement>();
    button->SetType("button");
    EXPECT_EQ(button->GetType(), "button");
}

TEST(HTMLButtonElement, DisabledState) {
    auto button = std::make_shared<HTMLButtonElement>();
    button->SetDisabled(true);
    EXPECT_TRUE(button->GetDisabled());
    EXPECT_TRUE(button->HasPseudoClass(":disabled"));
}
```

---

#### 2. HTMLSelectElement
**优先级**: 🔴 最高  
**原因**: 下拉选择，React表单必需  
**文件**: `core/dom/html_select_element.h/cpp`

**标准接口**:
```cpp
class HTMLSelectElement : public Element {
public:
    HTMLSelectElement();
    
    // ========== IDL属性 ==========
    
    // multiple: 是否多选
    bool GetMultiple() const;
    void SetMultiple(bool multiple);
    
    // size: 显示的选项数量
    int GetSize() const;
    void SetSize(int size);
    
    // selectedIndex: 当前选中的索引
    int GetSelectedIndex() const;
    void SetSelectedIndex(int index);
    
    // value: 当前选中的值
    std::string GetValue() const;
    void SetValue(const std::string& value);
    
    // options: 所有option元素的集合
    std::vector<std::shared_ptr<HTMLOptionElement>> GetOptions() const;
    
    // selectedOptions: 所有选中的option元素
    std::vector<std::shared_ptr<HTMLOptionElement>> GetSelectedOptions() const;
    
    // ========== 方法 ==========
    
    // 添加option
    void Add(std::shared_ptr<HTMLOptionElement> option, 
             std::shared_ptr<Element> before = nullptr);
    
    // 移除option
    void Remove(int index);
    
    // ========== 事件 ==========
    
    // change事件: 选择改变时触发
    void OnSelectionChange();
    
private:
    bool multiple_ = false;
    int size_ = 0;  // 0表示默认
    int selected_index_ = -1;
};
```

**默认CSS样式**:
```css
select {
    display: inline-block;
    padding: 1px 2px;
    border: 1px solid #767676;
    background-color: white;
    color: black;
    cursor: pointer;
}

select:focus {
    outline: 2px solid Highlight;
    outline-offset: -1px;
}

select[multiple] {
    padding: 0;
}
```

---

#### 3. HTMLOptionElement
**优先级**: 🔴 最高  
**原因**: 配合select使用  
**文件**: `core/dom/html_option_element.h/cpp`

**标准接口**:
```cpp
class HTMLOptionElement : public Element {
public:
    HTMLOptionElement();
    
    // selected: 是否选中
    bool GetSelected() const;
    void SetSelected(bool selected);
    
    // value: 选项值
    std::string GetValue() const;
    void SetValue(const std::string& value);
    
    // text: 选项文本
    std::string GetText() const;
    void SetText(const std::string& text);
    
    // disabled: 是否禁用
    bool GetDisabled() const;
    void SetDisabled(bool disabled);
    
    // index: 在select中的索引
    int GetIndex() const;
    
private:
    bool selected_ = false;
    bool disabled_ = false;
};
```

---

#### 4. HTMLFormElement
**优先级**: 🟡 高  
**原因**: 表单提交、验证  
**文件**: `core/dom/html_form_element.h/cpp`

**标准接口**:
```cpp
class HTMLFormElement : public Element {
public:
    HTMLFormElement();
    
    // action: 提交URL
    std::string GetAction() const;
    void SetAction(const std::string& action);
    
    // method: GET | POST
    std::string GetMethod() const;
    void SetMethod(const std::string& method);
    
    // elements: 所有表单控件
    std::vector<std::shared_ptr<Element>> GetElements() const;
    
    // 提交表单
    void Submit();
    
    // 重置表单
    void Reset();
    
    // 验证表单
    bool CheckValidity();
    bool ReportValidity();
    
private:
    std::string action_;
    std::string method_ = "get";
};
```

---

### P1 - 常用交互元素 (React常用，1周)

#### 5. HTMLAnchorElement (`<a>`)
**优先级**: 🟡 高  
**文件**: `core/dom/html_anchor_element.h/cpp`

```cpp
class HTMLAnchorElement : public Element {
public:
    // href: 链接地址
    std::string GetHref() const;
    void SetHref(const std::string& href);
    
    // target: _blank | _self | _parent | _top
    std::string GetTarget() const;
    void SetTarget(const std::string& target);
    
    // download: 下载文件名
    std::string GetDownload() const;
    void SetDownload(const std::string& download);
    
    // rel: 关系类型
    std::string GetRel() const;
    void SetRel(const std::string& rel);
    
    // 点击处理
    void HandleClick() override;
};
```

**默认样式**:
```css
a {
    color: #0000EE;
    text-decoration: underline;
    cursor: pointer;
}

a:visited {
    color: #551A8B;
}

a:hover {
    text-decoration: underline;
}

a:active {
    color: #FF0000;
}
```

---

#### 6. HTMLLabelElement (`<label>`)
```cpp
class HTMLLabelElement : public Element {
public:
    // htmlFor: 关联的表单控件ID
    std::string GetHtmlFor() const;
    void SetHtmlFor(const std::string& for_id);
    
    // control: 关联的表单控件
    std::shared_ptr<Element> GetControl() const;
    
    // 点击时聚焦到关联控件
    void HandleClick() override;
};
```

---

#### 7. HTMLImageElement (`<img>`)
**优先级**: 🟡 高  
**文件**: `core/dom/html_image_element.h/cpp`

```cpp
class HTMLImageElement : public Element {
public:
    // src: 图片URL
    std::string GetSrc() const;
    void SetSrc(const std::string& src);
    
    // alt: 替代文本
    std::string GetAlt() const;
    void SetAlt(const std::string& alt);
    
    // width, height: 尺寸
    int GetWidth() const;
    void SetWidth(int width);
    int GetHeight() const;
    void SetHeight(int height);
    
    // 加载状态
    bool IsComplete() const;
    
    // 图片数据
    sk_sp<SkImage> GetImage() const;
    
    // 加载图片
    void LoadImage();
    
private:
    std::string src_;
    std::string alt_;
    int width_ = 0;
    int height_ = 0;
    sk_sp<SkImage> image_;
    bool complete_ = false;
};
```

---

### P2 - 媒体和高级元素 (2周)

#### 8. HTMLCanvasElement (`<canvas>`)
#### 9. HTMLVideoElement (`<video>`)
#### 10. HTMLAudioElement (`<audio>`)
#### 11. HTMLIFrameElement (`<iframe>`)
#### 12. HTMLTableElement (`<table>`)
#### 13. HTMLTableRowElement (`<tr>`)
#### 14. HTMLTableCellElement (`<td>`, `<th>`)

---

## 📋 实施计划

### 第1周: P0关键表单元素

**Day 1-2: HTMLButtonElement**
- [ ] 创建头文件和实现文件
- [ ] 实现所有IDL属性
- [ ] 实现默认样式
- [ ] 实现伪类状态 (:hover, :active, :disabled)
- [ ] 编写单元测试 (10+个测试用例)
- [ ] 集成到Element工厂方法

**Day 3-4: HTMLSelectElement + HTMLOptionElement**
- [ ] 创建两个元素类
- [ ] 实现选择逻辑
- [ ] 实现change事件
- [ ] 实现默认样式
- [ ] 编写单元测试

**Day 5: HTMLFormElement**
- [ ] 创建表单元素类
- [ ] 实现表单控件收集
- [ ] 实现验证逻辑
- [ ] 编写单元测试

---

### 第2周: P1常用交互元素

**Day 1-2: HTMLAnchorElement**
- [ ] 实现链接导航
- [ ] 实现target处理
- [ ] 实现伪类 (:visited, :hover, :active)
- [ ] 编写测试

**Day 3: HTMLLabelElement**
- [ ] 实现控件关联
- [ ] 实现点击聚焦
- [ ] 编写测试

**Day 4-5: HTMLImageElement**
- [ ] 实现图片加载
- [ ] 集成Skia图片渲染
- [ ] 实现尺寸计算
- [ ] 编写测试

---

## 🔧 技术规范

### 1. 元素工厂方法修改

**文件**: `core/dom/element.cpp`

```cpp
std::shared_ptr<Element> Element::CreateFromLexbor(lxb_dom_node_t* lexbor_node) {
    // ... 现有代码 ...
    
    std::shared_ptr<Element> new_elem;
    
    // 根据标签名创建特定类型的元素
    if (tag_name == "input") {
        new_elem = std::make_shared<HTMLInputElement>();
    } else if (tag_name == "textarea") {
        new_elem = std::make_shared<HTMLTextAreaElement>();
    } else if (tag_name == "button") {
        new_elem = std::make_shared<HTMLButtonElement>();
    } else if (tag_name == "select") {
        new_elem = std::make_shared<HTMLSelectElement>();
    } else if (tag_name == "option") {
        new_elem = std::make_shared<HTMLOptionElement>();
    } else if (tag_name == "form") {
        new_elem = std::make_shared<HTMLFormElement>();
    } else if (tag_name == "a") {
        new_elem = std::make_shared<HTMLAnchorElement>();
    } else if (tag_name == "label") {
        new_elem = std::make_shared<HTMLLabelElement>();
    } else if (tag_name == "img") {
        new_elem = std::make_shared<HTMLImageElement>();
    } else {
        new_elem = std::make_shared<Element>(tag_name);
    }
    
    // ... 现有代码 ...
}
```

### 2. 默认样式系统

**文件**: `core/render/user_agent_stylesheet.h/cpp`

```cpp
class UserAgentStylesheet {
public:
    static std::string GetDefaultStyles() {
        return R"(
            /* 表单元素 */
            button {
                display: inline-block;
                padding: 2px 6px;
                border: 2px outset buttonface;
                background-color: buttonface;
                color: buttontext;
                cursor: pointer;
            }
            
            select {
                display: inline-block;
                padding: 1px 2px;
                border: 1px solid #767676;
                background-color: white;
                cursor: pointer;
            }
            
            /* 链接 */
            a {
                color: #0000EE;
                text-decoration: underline;
                cursor: pointer;
            }
            
            a:visited {
                color: #551A8B;
            }
            
            /* ... 更多默认样式 ... */
        )";
    }
};
```

---

## ✅ 验收标准

### 功能完整性
- [ ] 所有P0元素实现完成
- [ ] 所有IDL属性符合标准
- [ ] 所有默认样式正确
- [ ] 所有伪类状态正确

### React兼容性
- [ ] 可以通过React创建元素
- [ ] 属性变更触发重新渲染
- [ ] 事件处理符合React规范
- [ ] Virtual DOM diff正常工作

### 浏览器一致性
- [ ] 渲染效果与Chrome一致
- [ ] 交互行为与Firefox一致
- [ ] CSS样式与标准浏览器一致

### 测试覆盖
- [ ] 每个元素至少10个单元测试
- [ ] 集成测试覆盖React场景
- [ ] 性能测试达标

---

## 📊 进度跟踪

| 元素 | 状态 | 完成度 | 负责人 | 预计完成 |
|------|------|--------|--------|----------|
| HTMLButtonElement | 🔴 未开始 | 0% | - | Week 1 |
| HTMLSelectElement | 🔴 未开始 | 0% | - | Week 1 |
| HTMLOptionElement | 🔴 未开始 | 0% | - | Week 1 |
| HTMLFormElement | 🔴 未开始 | 0% | - | Week 1 |
| HTMLAnchorElement | 🔴 未开始 | 0% | - | Week 2 |
| HTMLLabelElement | 🔴 未开始 | 0% | - | Week 2 |
| HTMLImageElement | 🔴 未开始 | 0% | - | Week 2 |


