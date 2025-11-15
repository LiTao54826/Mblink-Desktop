# HTML/CSS API 参考文档

> **版本**: 1.0.0  
> **更新时间**: 2025-11-15

本文档提供 MBink 框架中 HTML/CSS 解析和操作的完整 API 参考。

---

## 目录

1. [LexborDocument 类](#lexbordocument-类)
2. [LexborElement 类](#lexborelement-类)
3. [HTML5 解析](#html5-解析)
4. [CSS3 选择器](#css3-选择器)
5. [表单元素](#表单元素)
6. [错误处理](#错误处理)
7. [性能优化](#性能优化)

---

## LexborDocument 类

`LexborDocument` 是 Lexbor HTML 文档的 C++ 包装类，提供了完整的 HTML 解析和 DOM 操作功能。

### 构造和析构

```cpp
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// 创建文档
LexborDocument doc;

// 文档会在析构时自动清理资源
```

### HTML 解析

#### ParseHTML()

解析 HTML 字符串。

```cpp
bool ParseHTML(const std::string& html);
```

**参数**:
- `html`: HTML 字符串

**返回值**:
- `true`: 解析成功
- `false`: 解析失败

**示例**:

```cpp
LexborDocument doc;
std::string html = R"(
    <!DOCTYPE html>
    <html>
    <head><title>Hello World</title></head>
    <body>
        <h1 id="title">Hello, MBink!</h1>
        <p class="content">This is a paragraph.</p>
    </body>
    </html>
)";

if (doc.ParseHTML(html)) {
    std::cout << "HTML 解析成功！" << std::endl;
} else {
    std::cout << "HTML 解析失败！" << std::endl;
}
```

#### ParseHTMLFromFile()

从文件解析 HTML。

```cpp
bool ParseHTMLFromFile(const std::string& filename);
```

**参数**:
- `filename`: HTML 文件路径

**返回值**:
- `true`: 解析成功
- `false`: 解析失败

**示例**:

```cpp
LexborDocument doc;
if (doc.ParseHTMLFromFile("index.html")) {
    std::cout << "文件解析成功！" << std::endl;
}
```

### DOM 查询

#### QuerySelector()

使用 CSS 选择器查找第一个匹配的元素。

```cpp
LexborElement* QuerySelector(const std::string& selector);
```

**参数**:
- `selector`: CSS 选择器字符串

**返回值**:
- 匹配的元素指针，如果没有找到返回 `nullptr`

**示例**:

```cpp
// 按 ID 查找
auto title = doc.QuerySelector("#title");
if (title) {
    std::cout << "Title: " << title->GetTextContent() << std::endl;
}

// 按类名查找
auto content = doc.QuerySelector(".content");

// 按标签名查找
auto firstParagraph = doc.QuerySelector("p");

// 复杂选择器
auto link = doc.QuerySelector("div.container > a[href^='https']");
```

#### QuerySelectorAll()

使用 CSS 选择器查找所有匹配的元素。

```cpp
std::vector<LexborElement*> QuerySelectorAll(const std::string& selector);
```

**参数**:
- `selector`: CSS 选择器字符串

**返回值**:
- 匹配的元素列表

**示例**:

```cpp
// 查找所有段落
auto paragraphs = doc.QuerySelectorAll("p");
for (auto* p : paragraphs) {
    std::cout << p->GetTextContent() << std::endl;
}

// 查找所有链接
auto links = doc.QuerySelectorAll("a");

// 查找所有选中的复选框
auto checkedBoxes = doc.QuerySelectorAll("input[type='checkbox']:checked");
```

### 文档信息

#### GetDocumentMode()

获取文档模式。

```cpp
std::string GetDocumentMode() const;
```

**返回值**:
- `"quirks"`: 怪异模式
- `"no-quirks"`: 标准模式
- `"limited-quirks"`: 有限怪异模式

**示例**:

```cpp
std::string mode = doc.GetDocumentMode();
if (mode == "quirks") {
    std::cout << "文档处于怪异模式" << std::endl;
}
```

#### IsQuirksMode()

检查是否为怪异模式。

```cpp
bool IsQuirksMode() const;
```

**返回值**:
- `true`: 怪异模式
- `false`: 标准模式

#### GetDoctype()

获取 DOCTYPE 声明。

```cpp
std::string GetDoctype() const;
```

**返回值**:
- DOCTYPE 名称（如 "html"）

**示例**:

```cpp
std::string doctype = doc.GetDoctype();
std::cout << "DOCTYPE: " << doctype << std::endl;
```

### 错误处理

#### GetErrors()

获取所有解析错误。

```cpp
const std::vector<std::string>& GetErrors() const;
```

**返回值**:
- 错误消息列表

**示例**:

```cpp
if (doc.HasErrors()) {
    auto errors = doc.GetErrors();
    for (const auto& error : errors) {
        std::cerr << "错误: " << error << std::endl;
    }
}
```

#### HasErrors()

检查是否有错误。

```cpp
bool HasErrors() const;
```

#### ClearErrors()

清空错误列表。

```cpp
void ClearErrors();
```

#### GetWarnings()

获取所有警告。

```cpp
const std::vector<std::string>& GetWarnings() const;
```

#### HasWarnings()

检查是否有警告。

```cpp
bool HasWarnings() const;
```

### DOM 树操作

#### GetParentElement()

获取父元素。

```cpp
LexborElement* GetParentElement();
```

#### GetChildren()

获取所有子元素。

```cpp
std::vector<LexborElement*> GetChildren();
```

#### GetFirstChild()

获取第一个子元素。

```cpp
LexborElement* GetFirstChild();
```

#### GetLastChild()

获取最后一个子元素。

```cpp
LexborElement* GetLastChild();
```

#### AppendChild()

添加子元素。

```cpp
void AppendChild(LexborElement* child);
```

#### RemoveChild()

移除子元素。

```cpp
void RemoveChild(LexborElement* child);
```

---

## LexborElement 类

`LexborElement` 表示 HTML 元素，提供了元素属性和内容的访问接口。

### 基本属性

#### GetTagName()

获取标签名（小写）。

```cpp
std::string GetTagName() const;
```

**示例**:

```cpp
auto element = doc.QuerySelector("div");
std::cout << "Tag: " << element->GetTagName() << std::endl;  // 输出: div
```

#### GetId() / SetId()

获取或设置元素 ID。

```cpp
std::string GetId() const;
void SetId(const std::string& id);
```

**示例**:

```cpp
auto element = doc.QuerySelector("#myDiv");
std::cout << "ID: " << element->GetId() << std::endl;

element->SetId("newId");
```

#### GetClassName() / SetClassName()

获取或设置 class 属性。

```cpp
std::string GetClassName() const;
void SetClassName(const std::string& class_name);
```

### 属性操作

#### GetAttribute()

获取属性值。

```cpp
std::string GetAttribute(const std::string& name) const;
```

**参数**:
- `name`: 属性名

**返回值**:
- 属性值，如果不存在返回空字符串

**示例**:

```cpp
auto link = doc.QuerySelector("a");
std::string href = link->GetAttribute("href");
std::string target = link->GetAttribute("target");
```

#### SetAttribute()

设置属性。

```cpp
void SetAttribute(const std::string& name, const std::string& value);
```

**示例**:

```cpp
auto link = doc.QuerySelector("a");
link->SetAttribute("href", "https://example.com");
link->SetAttribute("target", "_blank");
```

#### HasAttribute()

检查是否有某个属性。

```cpp
bool HasAttribute(const std::string& name) const;
```

**示例**:

```cpp
auto input = doc.QuerySelector("input");
if (input->HasAttribute("required")) {
    std::cout << "此字段为必填项" << std::endl;
}
```

#### RemoveAttribute()

移除属性。

```cpp
void RemoveAttribute(const std::string& name);
```

### 内容操作

#### GetTextContent() / SetTextContent()

获取或设置文本内容。

```cpp
std::string GetTextContent() const;
void SetTextContent(const std::string& text);
```

**示例**:

```cpp
auto paragraph = doc.QuerySelector("p");
std::cout << "Text: " << paragraph->GetTextContent() << std::endl;

paragraph->SetTextContent("New content");
```

#### GetInnerHTML() / SetInnerHTML()

获取或设置内部 HTML。

```cpp
std::string GetInnerHTML() const;
void SetInnerHTML(const std::string& html);
```

**示例**:

```cpp
auto div = doc.QuerySelector("div");
std::cout << "HTML: " << div->GetInnerHTML() << std::endl;

div->SetInnerHTML("<p>New <strong>content</strong></p>");
```

### Class 操作

#### HasClass()

检查是否有某个 class。

```cpp
bool HasClass(const std::string& class_name) const;
```

#### AddClass()

添加 class。

```cpp
void AddClass(const std::string& class_name);
```

#### RemoveClass()

移除 class。

```cpp
void RemoveClass(const std::string& class_name);
```

**示例**:

```cpp
auto element = doc.QuerySelector("div");

if (!element->HasClass("active")) {
    element->AddClass("active");
}

element->RemoveClass("inactive");
```

### 子元素查询

#### QuerySelector()

在当前元素的子树中查找第一个匹配的元素。

```cpp
LexborElement* QuerySelector(const std::string& selector);
```

#### QuerySelectorAll()

在当前元素的子树中查找所有匹配的元素。

```cpp
std::vector<LexborElement*> QuerySelectorAll(const std::string& selector);
```

**示例**:

```cpp
auto form = doc.QuerySelector("#myForm");

// 查找表单中的所有输入框
auto inputs = form->QuerySelectorAll("input");

// 查找表单中的提交按钮
auto submitBtn = form->QuerySelector("button[type='submit']");
```

---

## HTML5 解析

### 支持的特性

#### DOCTYPE 处理

支持所有标准 DOCTYPE：

```html
<!-- HTML5 -->
<!DOCTYPE html>

<!-- HTML4 Strict -->
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">

<!-- XHTML 1.0 -->
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
```

#### HTML 实体

支持 200+ 种 HTML 实体：

```html
<p>&lt;div&gt; &amp; &copy; &reg; &#169; &#x00A9;</p>
```

```cpp
auto p = doc.QuerySelector("p");
std::cout << p->GetTextContent() << std::endl;
// 输出: <div> & © ® © ©
```

#### 特殊元素

正确处理特殊元素：

```html
<script>
    console.log("JavaScript code");
</script>

<style>
    .class { color: red; }
</style>

<template>
    <div>Template content</div>
</template>

<svg>
    <circle cx="50" cy="50" r="40" />
</svg>
```

#### 自闭合标签

自动处理自闭合标签：

```html
<img src="image.jpg" />
<br />
<hr />
<input type="text" />
<meta charset="UTF-8" />
```

#### 布尔属性

正确处理布尔属性：

```html
<input type="checkbox" checked />
<input type="text" disabled />
<input type="text" readonly />
<option selected>Option</option>
```

```cpp
auto checkbox = doc.QuerySelector("input[type='checkbox']");
if (checkbox->GetAttribute("checked") == "") {
    std::cout << "复选框已选中" << std::endl;
}
```

### 容错机制

#### 格式错误的 HTML

```cpp
std::string badHTML = "<div><p>Unclosed paragraph<div>Nested div</div>";
doc.ParseHTML(badHTML);  // 仍然可以解析

if (doc.HasWarnings()) {
    auto warnings = doc.GetWarnings();
    for (const auto& warning : warnings) {
        std::cout << "警告: " << warning << std::endl;
    }
}
```

#### 无效嵌套

```html
<!-- 无效嵌套会被自动修正 -->
<p><div>Invalid nesting</div></p>
```

#### 深度嵌套

支持深度嵌套（测试通过 100 层）：

```html
<div><div><div>...</div></div></div>
```

---

## CSS3 选择器

### 基础选择器

#### 类型选择器

```cpp
auto divs = doc.QuerySelectorAll("div");
auto paragraphs = doc.QuerySelectorAll("p");
```

#### 类选择器

```cpp
auto elements = doc.QuerySelectorAll(".className");
auto multiple = doc.QuerySelectorAll(".class1.class2");
```

#### ID 选择器

```cpp
auto element = doc.QuerySelector("#elementId");
```

#### 通用选择器

```cpp
auto allElements = doc.QuerySelectorAll("*");
```

### 组合选择器

#### 后代选择器

```cpp
auto links = doc.QuerySelectorAll("div a");  // div 内的所有 a
```

#### 子选择器

```cpp
auto directChildren = doc.QuerySelectorAll("ul > li");  // ul 的直接子 li
```

#### 相邻兄弟选择器

```cpp
auto next = doc.QuerySelectorAll("h1 + p");  // h1 后的第一个 p
```

#### 通用兄弟选择器

```cpp
auto siblings = doc.QuerySelectorAll("h1 ~ p");  // h1 后的所有 p
```

### 属性选择器

```cpp
// 存在属性
auto withAttr = doc.QuerySelectorAll("[href]");

// 精确匹配
auto exact = doc.QuerySelectorAll("[type='text']");

// 包含单词
auto word = doc.QuerySelectorAll("[class~='active']");

// 前缀匹配
auto prefix = doc.QuerySelectorAll("[href^='https']");

// 后缀匹配
auto suffix = doc.QuerySelectorAll("[src$='.jpg']");

// 子串匹配
auto substring = doc.QuerySelectorAll("[href*='example']");

// 连字符匹配
auto hyphen = doc.QuerySelectorAll("[lang|='en']");
```

### 伪类

#### 结构伪类

```cpp
auto firstChild = doc.QuerySelectorAll("li:first-child");
auto lastChild = doc.QuerySelectorAll("li:last-child");
auto nthChild = doc.QuerySelectorAll("li:nth-child(2)");
auto oddRows = doc.QuerySelectorAll("tr:nth-child(odd)");
auto evenRows = doc.QuerySelectorAll("tr:nth-child(even)");
```

#### 表单伪类

```cpp
auto enabled = doc.QuerySelectorAll("input:enabled");
auto disabled = doc.QuerySelectorAll("input:disabled");
auto checked = doc.QuerySelectorAll("input:checked");
```

#### 其他伪类

```cpp
auto root = doc.QuerySelector(":root");
auto empty = doc.QuerySelectorAll("div:empty");
auto notDiv = doc.QuerySelectorAll(":not(div)");
```

### 伪元素

```cpp
// 注意：伪元素在 CSS 中使用，但在 DOM 查询中不可用
// 这些主要用于样式应用
```

---

## 表单元素

### 表单基本操作

```cpp
auto form = doc.QuerySelector("#myForm");

// 获取表单属性
std::string action = form->GetAttribute("action");
std::string method = form->GetAttribute("method");

// 查找表单中的所有输入
auto inputs = form->QuerySelectorAll("input");
```

### Input 类型

```cpp
// 文本输入
auto textInput = doc.QuerySelector("input[type='text']");
std::string value = textInput->GetAttribute("value");

// 数字输入
auto numberInput = doc.QuerySelector("input[type='number']");
std::string min = numberInput->GetAttribute("min");
std::string max = numberInput->GetAttribute("max");

// 复选框
auto checkbox = doc.QuerySelector("input[type='checkbox']");
bool isChecked = checkbox->GetAttribute("checked") == "";

// 单选按钮
auto radios = doc.QuerySelectorAll("input[name='gender']");
for (auto* radio : radios) {
    if (radio->GetAttribute("checked") == "") {
        std::string selectedValue = radio->GetAttribute("value");
        break;
    }
}
```

### Select 元素

```cpp
auto select = doc.QuerySelector("select");

// 获取所有选项
auto options = select->QuerySelectorAll("option");

// 获取选中的选项
auto selectedOptions = select->QuerySelectorAll("option[selected]");
for (auto* option : selectedOptions) {
    std::string value = option->GetAttribute("value");
    std::string text = option->GetTextContent();
}
```

### Textarea 元素

```cpp
auto textarea = doc.QuerySelector("textarea");
std::string content = textarea->GetTextContent();
```

### 表单验证

```cpp
auto input = doc.QuerySelector("input");

// 检查验证属性
bool isRequired = input->HasAttribute("required");
std::string pattern = input->GetAttribute("pattern");
std::string minLength = input->GetAttribute("minlength");
std::string maxLength = input->GetAttribute("maxlength");
```

---

## 错误处理

### 解析错误

```cpp
LexborDocument doc;
if (!doc.ParseHTML(html)) {
    if (doc.HasErrors()) {
        auto errors = doc.GetErrors();
        for (const auto& error : errors) {
            std::cerr << "解析错误: " << error << std::endl;
        }
    }
}
```

### 警告处理

```cpp
doc.ParseHTML(html);
if (doc.HasWarnings()) {
    auto warnings = doc.GetWarnings();
    for (const auto& warning : warnings) {
        std::cout << "警告: " << warning << std::endl;
    }
}
```

### 清理错误

```cpp
doc.ClearErrors();
```

---

## 性能优化

### 性能特点

- **快速解析**: 5000 元素文档仅需 38ms
- **高效查询**: querySelector 在 1000 元素中查询 < 10ms
- **低内存占用**: 优化的内存管理

### 最佳实践

#### 1. 重用文档对象

```cpp
LexborDocument doc;  // 创建一次
for (const auto& html : htmlList) {
    doc.ParseHTML(html);
    // 处理...
    doc.ClearErrors();  // 清理错误
}
```

#### 2. 使用具体选择器

```cpp
// 好：具体的选择器
auto element = doc.QuerySelector("#specificId");

// 避免：过于宽泛的选择器
auto elements = doc.QuerySelectorAll("*");
```

#### 3. 缓存查询结果

```cpp
// 好：缓存结果
auto form = doc.QuerySelector("#myForm");
auto inputs = form->QuerySelectorAll("input");

// 避免：重复查询
for (int i = 0; i < 100; i++) {
    auto form = doc.QuerySelector("#myForm");  // 重复查询
}
```

---

## 完整示例

### 示例 1: 解析和查询

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Example</title></head>
        <body>
            <div id="container">
                <h1 class="title">Hello World</h1>
                <p class="content">This is a paragraph.</p>
                <a href="https://example.com">Link</a>
            </div>
        </body>
        </html>
    )";
    
    if (doc.ParseHTML(html)) {
        // 查询元素
        auto title = doc.QuerySelector(".title");
        std::cout << "Title: " << title->GetTextContent() << std::endl;
        
        // 查询所有段落
        auto paragraphs = doc.QuerySelectorAll("p");
        for (auto* p : paragraphs) {
            std::cout << "Paragraph: " << p->GetTextContent() << std::endl;
        }
        
        // 查询链接
        auto link = doc.QuerySelector("a[href^='https']");
        if (link) {
            std::cout << "Link: " << link->GetAttribute("href") << std::endl;
        }
    }
    
    return 0;
}
```

### 示例 2: 表单处理

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <form id="userForm">
            <input type="text" name="username" value="john" required />
            <input type="email" name="email" value="john@example.com" />
            <input type="checkbox" name="subscribe" checked />
            <select name="country">
                <option value="us">USA</option>
                <option value="uk" selected>UK</option>
            </select>
        </form>
    )";
    
    if (doc.ParseHTML(html)) {
        auto form = doc.QuerySelector("#userForm");
        
        // 收集表单数据
        auto inputs = form->QuerySelectorAll("input, select");
        for (auto* input : inputs) {
            std::string name = input->GetAttribute("name");
            std::string value = input->GetAttribute("value");
            std::cout << name << ": " << value << std::endl;
        }
        
        // 检查必填字段
        auto required = form->QuerySelectorAll("[required]");
        std::cout << "必填字段数量: " << required.size() << std::endl;
    }
    
    return 0;
}
```

---

## 参考资源

- [HTML5 规范](https://html.spec.whatwg.org/)
- [CSS 选择器规范](https://www.w3.org/TR/selectors-4/)
- [Lexbor 文档](https://lexbor.com/)
- [MBink 项目文档](../README.md)

---

**版权所有 © 2025 MBink 项目**

