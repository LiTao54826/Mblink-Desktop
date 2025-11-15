# HTML/CSS 使用示例

> **版本**: 1.0.0  
> **更新时间**: 2025-11-15

本文档提供 MBink 框架中 HTML/CSS 功能的实用示例。

---

## 目录

1. [基础示例](#基础示例)
2. [DOM 操作](#dom-操作)
3. [表单处理](#表单处理)
4. [CSS 选择器](#css-选择器)
5. [实际应用](#实际应用)

---

## 基础示例

### 示例 1: Hello World

最简单的 HTML 解析示例。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Hello World</title></head>
        <body>
            <h1>Hello, MBink!</h1>
        </body>
        </html>
    )";
    
    if (doc.ParseHTML(html)) {
        auto h1 = doc.QuerySelector("h1");
        std::cout << h1->GetTextContent() << std::endl;
        // 输出: Hello, MBink!
    }
    
    return 0;
}
```

### 示例 2: 从文件加载

从 HTML 文件加载内容。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    if (doc.ParseHTMLFromFile("index.html")) {
        std::cout << "文件加载成功！" << std::endl;
        
        // 获取标题
        auto title = doc.QuerySelector("title");
        if (title) {
            std::cout << "页面标题: " << title->GetTextContent() << std::endl;
        }
    } else {
        std::cerr << "文件加载失败！" << std::endl;
        
        // 显示错误
        if (doc.HasErrors()) {
            for (const auto& error : doc.GetErrors()) {
                std::cerr << "错误: " << error << std::endl;
            }
        }
    }
    
    return 0;
}
```

### 示例 3: 错误处理

处理解析错误和警告。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    // 格式不完整的 HTML
    std::string html = "<div><p>Unclosed paragraph<div>Nested</div>";
    
    doc.ParseHTML(html);
    
    // 检查错误
    if (doc.HasErrors()) {
        std::cout << "发现 " << doc.GetErrors().size() << " 个错误：" << std::endl;
        for (const auto& error : doc.GetErrors()) {
            std::cout << "  - " << error << std::endl;
        }
    }
    
    // 检查警告
    if (doc.HasWarnings()) {
        std::cout << "发现 " << doc.GetWarnings().size() << " 个警告：" << std::endl;
        for (const auto& warning : doc.GetWarnings()) {
            std::cout << "  - " << warning << std::endl;
        }
    }
    
    // 清理错误
    doc.ClearErrors();
    
    return 0;
}
```

---

## DOM 操作

### 示例 4: 遍历 DOM 树

遍历和访问 DOM 元素。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

void PrintElement(LexborElement* element, int depth = 0) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "<" << element->GetTagName();
    
    // 打印 ID
    if (!element->GetId().empty()) {
        std::cout << " id=\"" << element->GetId() << "\"";
    }
    
    // 打印 class
    if (!element->GetClassName().empty()) {
        std::cout << " class=\"" << element->GetClassName() << "\"";
    }
    
    std::cout << ">" << std::endl;
}

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <div id="container">
            <h1 class="title">Title</h1>
            <p class="content">Paragraph 1</p>
            <p class="content">Paragraph 2</p>
        </div>
    )";
    
    doc.ParseHTML(html);
    
    // 遍历所有元素
    auto allElements = doc.QuerySelectorAll("*");
    for (auto* element : allElements) {
        PrintElement(element);
    }
    
    return 0;
}
```

### 示例 5: 修改 DOM

修改元素属性和内容。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <div id="container">
            <h1>Old Title</h1>
            <p class="old-class">Old content</p>
        </div>
    )";
    
    doc.ParseHTML(html);
    
    // 修改标题
    auto h1 = doc.QuerySelector("h1");
    h1->SetTextContent("New Title");
    
    // 修改段落
    auto p = doc.QuerySelector("p");
    p->SetTextContent("New content");
    p->RemoveClass("old-class");
    p->AddClass("new-class");
    
    // 修改属性
    auto container = doc.QuerySelector("#container");
    container->SetAttribute("data-version", "2.0");
    
    // 打印结果
    std::cout << "Title: " << h1->GetTextContent() << std::endl;
    std::cout << "Paragraph: " << p->GetTextContent() << std::endl;
    std::cout << "Class: " << p->GetClassName() << std::endl;
    std::cout << "Version: " << container->GetAttribute("data-version") << std::endl;
    
    return 0;
}
```

### 示例 6: 操作 Class

添加、删除和检查 CSS 类。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(<div class="container active">Content</div>)";
    doc.ParseHTML(html);
    
    auto div = doc.QuerySelector("div");
    
    // 检查 class
    if (div->HasClass("active")) {
        std::cout << "元素是激活状态" << std::endl;
    }
    
    // 添加 class
    div->AddClass("highlighted");
    
    // 移除 class
    div->RemoveClass("active");
    
    // 打印最终的 class
    std::cout << "Classes: " << div->GetClassName() << std::endl;
    // 输出: Classes: container highlighted
    
    return 0;
}
```

---

## 表单处理

### 示例 7: 读取表单数据

收集表单输入值。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>
#include <map>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <form id="userForm">
            <input type="text" name="username" value="john_doe" />
            <input type="email" name="email" value="john@example.com" />
            <input type="number" name="age" value="25" />
            <input type="checkbox" name="subscribe" checked />
            <select name="country">
                <option value="us">USA</option>
                <option value="uk" selected>UK</option>
                <option value="ca">Canada</option>
            </select>
        </form>
    )";
    
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("#userForm");
    
    // 收集所有输入
    std::map<std::string, std::string> formData;
    
    // 文本输入
    auto textInputs = form->QuerySelectorAll("input[type='text'], input[type='email'], input[type='number']");
    for (auto* input : textInputs) {
        std::string name = input->GetAttribute("name");
        std::string value = input->GetAttribute("value");
        formData[name] = value;
    }
    
    // 复选框
    auto checkboxes = form->QuerySelectorAll("input[type='checkbox']");
    for (auto* checkbox : checkboxes) {
        std::string name = checkbox->GetAttribute("name");
        bool checked = checkbox->GetAttribute("checked") == "";
        formData[name] = checked ? "true" : "false";
    }
    
    // 选择框
    auto selects = form->QuerySelectorAll("select");
    for (auto* select : selects) {
        std::string name = select->GetAttribute("name");
        auto selectedOption = select->QuerySelector("option[selected]");
        if (selectedOption) {
            formData[name] = selectedOption->GetAttribute("value");
        }
    }
    
    // 打印表单数据
    std::cout << "表单数据：" << std::endl;
    for (const auto& [key, value] : formData) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    
    return 0;
}
```

### 示例 8: 表单验证

验证表单输入。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>
#include <regex>

using namespace lightui;

bool ValidateForm(LexborElement* form) {
    bool isValid = true;
    
    // 检查必填字段
    auto requiredFields = form->QuerySelectorAll("[required]");
    for (auto* field : requiredFields) {
        std::string value = field->GetAttribute("value");
        if (value.empty()) {
            std::string name = field->GetAttribute("name");
            std::cerr << "错误: " << name << " 是必填项" << std::endl;
            isValid = false;
        }
    }
    
    // 检查邮箱格式
    auto emailInputs = form->QuerySelectorAll("input[type='email']");
    for (auto* email : emailInputs) {
        std::string value = email->GetAttribute("value");
        if (!value.empty()) {
            std::regex emailPattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
            if (!std::regex_match(value, emailPattern)) {
                std::cerr << "错误: 邮箱格式不正确" << std::endl;
                isValid = false;
            }
        }
    }
    
    // 检查数字范围
    auto numberInputs = form->QuerySelectorAll("input[type='number']");
    for (auto* number : numberInputs) {
        std::string value = number->GetAttribute("value");
        if (!value.empty()) {
            int val = std::stoi(value);
            
            std::string minStr = number->GetAttribute("min");
            if (!minStr.empty() && val < std::stoi(minStr)) {
                std::cerr << "错误: 值小于最小值 " << minStr << std::endl;
                isValid = false;
            }
            
            std::string maxStr = number->GetAttribute("max");
            if (!maxStr.empty() && val > std::stoi(maxStr)) {
                std::cerr << "错误: 值大于最大值 " << maxStr << std::endl;
                isValid = false;
            }
        }
    }
    
    return isValid;
}

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <form id="registrationForm">
            <input type="text" name="username" value="" required />
            <input type="email" name="email" value="invalid-email" required />
            <input type="number" name="age" value="150" min="18" max="120" />
        </form>
    )";
    
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("#registrationForm");
    
    if (ValidateForm(form)) {
        std::cout << "表单验证通过！" << std::endl;
    } else {
        std::cout << "表单验证失败！" << std::endl;
    }
    
    return 0;
}
```

### 示例 9: 单选按钮组

处理单选按钮组。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <form>
            <input type="radio" name="gender" value="male" />
            <input type="radio" name="gender" value="female" checked />
            <input type="radio" name="gender" value="other" />
        </form>
    )";
    
    doc.ParseHTML(html);
    
    // 查找选中的单选按钮
    auto checkedRadio = doc.QuerySelector("input[name='gender']:checked");
    if (checkedRadio) {
        std::string selectedValue = checkedRadio->GetAttribute("value");
        std::cout << "选中的性别: " << selectedValue << std::endl;
        // 输出: 选中的性别: female
    }
    
    // 获取所有单选按钮
    auto allRadios = doc.QuerySelectorAll("input[name='gender']");
    std::cout << "单选按钮总数: " << allRadios.size() << std::endl;
    
    return 0;
}
```

---

## CSS 选择器

### 示例 10: 复杂选择器

使用复杂的 CSS 选择器。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <div class="container">
            <ul class="menu">
                <li><a href="/home">Home</a></li>
                <li><a href="/about">About</a></li>
                <li class="active"><a href="/contact">Contact</a></li>
            </ul>
            <div class="content">
                <p>First paragraph</p>
                <p class="highlight">Second paragraph</p>
                <p>Third paragraph</p>
            </div>
        </div>
    )";
    
    doc.ParseHTML(html);
    
    // 后代选择器
    auto menuLinks = doc.QuerySelectorAll(".menu a");
    std::cout << "菜单链接数: " << menuLinks.size() << std::endl;
    
    // 子选择器
    auto directChildren = doc.QuerySelectorAll(".container > div");
    std::cout << "直接子 div 数: " << directChildren.size() << std::endl;
    
    // 相邻兄弟选择器
    auto nextP = doc.QuerySelector(".highlight + p");
    if (nextP) {
        std::cout << "高亮段落后的段落: " << nextP->GetTextContent() << std::endl;
    }
    
    // 属性选择器
    auto homeLink = doc.QuerySelector("a[href='/home']");
    if (homeLink) {
        std::cout << "首页链接: " << homeLink->GetTextContent() << std::endl;
    }
    
    // 伪类选择器
    auto firstLi = doc.QuerySelector("li:first-child");
    if (firstLi) {
        std::cout << "第一个列表项: " << firstLi->GetTextContent() << std::endl;
    }
    
    return 0;
}
```

### 示例 11: 属性选择器

使用各种属性选择器。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <div>
            <a href="https://example.com">HTTPS Link</a>
            <a href="http://example.com">HTTP Link</a>
            <img src="photo.jpg" alt="Photo" />
            <img src="icon.png" alt="Icon" />
            <input type="text" class="form-control" />
            <input type="email" class="form-control email-input" />
        </div>
    )";
    
    doc.ParseHTML(html);
    
    // 前缀匹配 (^=)
    auto httpsLinks = doc.QuerySelectorAll("a[href^='https']");
    std::cout << "HTTPS 链接数: " << httpsLinks.size() << std::endl;
    
    // 后缀匹配 ($=)
    auto jpgImages = doc.QuerySelectorAll("img[src$='.jpg']");
    std::cout << "JPG 图片数: " << jpgImages.size() << std::endl;
    
    // 子串匹配 (*=)
    auto exampleLinks = doc.QuerySelectorAll("a[href*='example']");
    std::cout << "包含 'example' 的链接数: " << exampleLinks.size() << std::endl;
    
    // 包含单词 (~=)
    auto emailInputs = doc.QuerySelectorAll("input[class~='email-input']");
    std::cout << "邮箱输入框数: " << emailInputs.size() << std::endl;
    
    return 0;
}
```

### 示例 12: 伪类选择器

使用结构伪类。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <table>
            <tr><td>Row 1</td></tr>
            <tr><td>Row 2</td></tr>
            <tr><td>Row 3</td></tr>
            <tr><td>Row 4</td></tr>
            <tr><td>Row 5</td></tr>
        </table>
    )";
    
    doc.ParseHTML(html);
    
    // 第一个子元素
    auto firstRow = doc.QuerySelector("tr:first-child");
    std::cout << "第一行: " << firstRow->GetTextContent() << std::endl;
    
    // 最后一个子元素
    auto lastRow = doc.QuerySelector("tr:last-child");
    std::cout << "最后一行: " << lastRow->GetTextContent() << std::endl;
    
    // 第 n 个子元素
    auto thirdRow = doc.QuerySelector("tr:nth-child(3)");
    std::cout << "第三行: " << thirdRow->GetTextContent() << std::endl;
    
    // 奇数行
    auto oddRows = doc.QuerySelectorAll("tr:nth-child(odd)");
    std::cout << "奇数行数: " << oddRows.size() << std::endl;
    
    // 偶数行
    auto evenRows = doc.QuerySelectorAll("tr:nth-child(even)");
    std::cout << "偶数行数: " << evenRows.size() << std::endl;
    
    return 0;
}
```

---

## 实际应用

### 示例 13: 网页爬虫

提取网页中的特定信息。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>
#include <vector>

using namespace lightui;

struct Article {
    std::string title;
    std::string author;
    std::string date;
    std::string content;
};

std::vector<Article> ExtractArticles(const std::string& html) {
    std::vector<Article> articles;
    
    LexborDocument doc;
    if (!doc.ParseHTML(html)) {
        return articles;
    }
    
    auto articleElements = doc.QuerySelectorAll("article");
    for (auto* articleElem : articleElements) {
        Article article;
        
        // 提取标题
        auto titleElem = articleElem->QuerySelector("h2.title");
        if (titleElem) {
            article.title = titleElem->GetTextContent();
        }
        
        // 提取作者
        auto authorElem = articleElem->QuerySelector(".author");
        if (authorElem) {
            article.author = authorElem->GetTextContent();
        }
        
        // 提取日期
        auto dateElem = articleElem->QuerySelector("time");
        if (dateElem) {
            article.date = dateElem->GetAttribute("datetime");
        }
        
        // 提取内容
        auto contentElem = articleElem->QuerySelector(".content");
        if (contentElem) {
            article.content = contentElem->GetTextContent();
        }
        
        articles.push_back(article);
    }
    
    return articles;
}

int main() {
    std::string html = R"(
        <div class="articles">
            <article>
                <h2 class="title">First Article</h2>
                <div class="meta">
                    <span class="author">John Doe</span>
                    <time datetime="2025-11-15">Nov 15, 2025</time>
                </div>
                <div class="content">This is the first article content.</div>
            </article>
            <article>
                <h2 class="title">Second Article</h2>
                <div class="meta">
                    <span class="author">Jane Smith</span>
                    <time datetime="2025-11-14">Nov 14, 2025</time>
                </div>
                <div class="content">This is the second article content.</div>
            </article>
        </div>
    )";
    
    auto articles = ExtractArticles(html);
    
    std::cout << "找到 " << articles.size() << " 篇文章：" << std::endl;
    for (const auto& article : articles) {
        std::cout << "\n标题: " << article.title << std::endl;
        std::cout << "作者: " << article.author << std::endl;
        std::cout << "日期: " << article.date << std::endl;
        std::cout << "内容: " << article.content << std::endl;
    }
    
    return 0;
}
```

### 示例 14: HTML 清理

清理和规范化 HTML 内容。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>

using namespace lightui;

void CleanHTML(LexborDocument& doc) {
    // 移除所有 script 标签
    auto scripts = doc.QuerySelectorAll("script");
    for (auto* script : scripts) {
        // 注意：实际的移除需要 DOM 操作 API
        std::cout << "发现 script 标签" << std::endl;
    }
    
    // 移除所有 style 标签
    auto styles = doc.QuerySelectorAll("style");
    for (auto* style : styles) {
        std::cout << "发现 style 标签" << std::endl;
    }
    
    // 移除所有内联样式
    auto elementsWithStyle = doc.QuerySelectorAll("[style]");
    for (auto* elem : elementsWithStyle) {
        elem->RemoveAttribute("style");
    }
    
    // 移除所有事件处理器
    auto elementsWithEvents = doc.QuerySelectorAll("[onclick], [onload], [onerror]");
    for (auto* elem : elementsWithEvents) {
        elem->RemoveAttribute("onclick");
        elem->RemoveAttribute("onload");
        elem->RemoveAttribute("onerror");
    }
}

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <div>
            <p style="color: red;" onclick="alert('click')">Paragraph</p>
            <script>console.log('script');</script>
            <style>.class { color: blue; }</style>
        </div>
    )";
    
    doc.ParseHTML(html);
    
    std::cout << "清理前：" << std::endl;
    auto p = doc.QuerySelector("p");
    std::cout << "style 属性: " << p->GetAttribute("style") << std::endl;
    std::cout << "onclick 属性: " << p->GetAttribute("onclick") << std::endl;
    
    CleanHTML(doc);
    
    std::cout << "\n清理后：" << std::endl;
    std::cout << "style 属性: " << p->GetAttribute("style") << std::endl;
    std::cout << "onclick 属性: " << p->GetAttribute("onclick") << std::endl;
    
    return 0;
}
```

### 示例 15: 表格数据提取

从 HTML 表格提取数据。

```cpp
#include "core/lexbor/lexbor_document.h"
#include <iostream>
#include <vector>

using namespace lightui;

int main() {
    LexborDocument doc;
    
    std::string html = R"(
        <table id="users">
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Email</th>
                    <th>Age</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>John Doe</td>
                    <td>john@example.com</td>
                    <td>30</td>
                </tr>
                <tr>
                    <td>Jane Smith</td>
                    <td>jane@example.com</td>
                    <td>25</td>
                </tr>
            </tbody>
        </table>
    )";
    
    doc.ParseHTML(html);
    
    auto table = doc.QuerySelector("#users");
    
    // 提取表头
    auto headers = table->QuerySelectorAll("thead th");
    std::cout << "表头: ";
    for (auto* th : headers) {
        std::cout << th->GetTextContent() << "\t";
    }
    std::cout << std::endl;
    
    // 提取数据行
    auto rows = table->QuerySelectorAll("tbody tr");
    for (auto* row : rows) {
        auto cells = row->QuerySelectorAll("td");
        for (auto* cell : cells) {
            std::cout << cell->GetTextContent() << "\t";
        }
        std::cout << std::endl;
    }
    
    return 0;
}
```

---

## 最佳实践

### 1. 错误处理

始终检查解析结果和错误：

```cpp
LexborDocument doc;
if (!doc.ParseHTML(html)) {
    if (doc.HasErrors()) {
        for (const auto& error : doc.GetErrors()) {
            std::cerr << "错误: " << error << std::endl;
        }
    }
    return;
}
```

### 2. 空指针检查

查询结果可能为空：

```cpp
auto element = doc.QuerySelector("#id");
if (element) {
    // 使用 element
} else {
    std::cerr << "元素未找到" << std::endl;
}
```

### 3. 使用具体选择器

具体的选择器性能更好：

```cpp
// 好
auto element = doc.QuerySelector("#specificId");

// 避免
auto elements = doc.QuerySelectorAll("*");
```

### 4. 缓存查询结果

避免重复查询：

```cpp
// 好
auto form = doc.QuerySelector("#form");
auto inputs = form->QuerySelectorAll("input");

// 避免
for (int i = 0; i < 100; i++) {
    auto form = doc.QuerySelector("#form");  // 重复查询
}
```

### 5. 清理资源

文档对象会自动清理，但可以手动清理错误：

```cpp
doc.ClearErrors();
```

---

## 参考资源

- [API 参考文档](HTML_CSS_API_REFERENCE.md)
- [开发计划](HTML_CSS_COMPLETE_SUPPORT_PLAN.md)
- [进度报告](HTML_CSS_PROGRESS_REPORT.md)

---

**版权所有 © 2025 MBink 项目**

