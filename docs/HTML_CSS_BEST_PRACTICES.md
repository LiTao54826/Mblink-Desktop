# HTML/CSS 最佳实践指南

> **版本**: 1.0.0  
> **更新时间**: 2025-11-15

本文档提供在 MBink 框架中使用 HTML/CSS 功能的最佳实践和性能优化建议。

---

## 目录

1. [性能优化](#性能优化)
2. [错误处理](#错误处理)
3. [内存管理](#内存管理)
4. [选择器优化](#选择器优化)
5. [表单处理](#表单处理)
6. [安全性](#安全性)
7. [代码组织](#代码组织)

---

## 性能优化

### 1. 重用文档对象

**❌ 不好的做法**:

```cpp
for (const auto& html : htmlList) {
    LexborDocument doc;  // 每次都创建新对象
    doc.ParseHTML(html);
    // 处理...
}
```

**✅ 好的做法**:

```cpp
LexborDocument doc;  // 创建一次
for (const auto& html : htmlList) {
    doc.ParseHTML(html);
    // 处理...
    doc.ClearErrors();  // 清理错误
}
```

**原因**: 创建和销毁文档对象有开销，重用对象可以提高性能。

### 2. 缓存查询结果

**❌ 不好的做法**:

```cpp
for (int i = 0; i < 100; i++) {
    auto form = doc.QuerySelector("#myForm");  // 重复查询
    auto input = form->QuerySelector("input");
    // 处理...
}
```

**✅ 好的做法**:

```cpp
auto form = doc.QuerySelector("#myForm");  // 查询一次
auto input = form->QuerySelector("input");

for (int i = 0; i < 100; i++) {
    // 使用缓存的结果
}
```

**原因**: 查询 DOM 有开销，缓存结果可以避免重复查询。

### 3. 使用具体的选择器

**❌ 不好的做法**:

```cpp
auto elements = doc.QuerySelectorAll("*");  // 选择所有元素
for (auto* elem : elements) {
    if (elem->GetTagName() == "div") {
        // 处理...
    }
}
```

**✅ 好的做法**:

```cpp
auto divs = doc.QuerySelectorAll("div");  // 直接选择 div
for (auto* div : divs) {
    // 处理...
}
```

**原因**: 具体的选择器性能更好，避免不必要的过滤。

### 4. 批量操作

**❌ 不好的做法**:

```cpp
for (int i = 0; i < 100; i++) {
    auto p = doc.QuerySelector("p");
    p->SetAttribute("data-index", std::to_string(i));
}
```

**✅ 好的做法**:

```cpp
auto paragraphs = doc.QuerySelectorAll("p");
for (size_t i = 0; i < paragraphs.size(); i++) {
    paragraphs[i]->SetAttribute("data-index", std::to_string(i));
}
```

**原因**: 批量查询一次比多次单独查询效率高。

### 5. 避免深度嵌套查询

**❌ 不好的做法**:

```cpp
auto elem = doc.QuerySelector("div")
               ->QuerySelector("ul")
               ->QuerySelector("li")
               ->QuerySelector("a");
```

**✅ 好的做法**:

```cpp
auto elem = doc.QuerySelector("div ul li a");
```

**原因**: 单次复杂查询比多次嵌套查询效率高。

---

## 错误处理

### 1. 始终检查解析结果

**❌ 不好的做法**:

```cpp
LexborDocument doc;
doc.ParseHTML(html);
auto element = doc.QuerySelector("#id");
element->GetTextContent();  // 可能崩溃
```

**✅ 好的做法**:

```cpp
LexborDocument doc;
if (!doc.ParseHTML(html)) {
    if (doc.HasErrors()) {
        for (const auto& error : doc.GetErrors()) {
            std::cerr << "解析错误: " << error << std::endl;
        }
    }
    return;
}

auto element = doc.QuerySelector("#id");
if (element) {
    std::cout << element->GetTextContent() << std::endl;
} else {
    std::cerr << "元素未找到" << std::endl;
}
```

**原因**: 解析可能失败，查询可能返回空指针。

### 2. 处理警告

**✅ 好的做法**:

```cpp
doc.ParseHTML(html);

if (doc.HasWarnings()) {
    for (const auto& warning : doc.GetWarnings()) {
        std::cout << "警告: " << warning << std::endl;
    }
}
```

**原因**: 警告可以帮助发现潜在问题。

### 3. 使用异常处理

**✅ 好的做法**:

```cpp
try {
    LexborDocument doc;
    if (!doc.ParseHTMLFromFile(filename)) {
        throw std::runtime_error("文件解析失败");
    }
    
    auto element = doc.QuerySelector("#id");
    if (!element) {
        throw std::runtime_error("元素未找到");
    }
    
    // 处理...
    
} catch (const std::exception& e) {
    std::cerr << "错误: " << e.what() << std::endl;
}
```

**原因**: 异常处理可以统一管理错误。

### 4. 清理错误

**✅ 好的做法**:

```cpp
LexborDocument doc;

for (const auto& html : htmlList) {
    doc.ParseHTML(html);
    
    // 处理...
    
    doc.ClearErrors();  // 清理错误，避免累积
}
```

**原因**: 避免错误累积，保持文档对象干净。

---

## 内存管理

### 1. 注意元素生命周期

**⚠️ 注意**:

```cpp
LexborElement* GetElement() {
    LexborDocument doc;
    doc.ParseHTML("<div>Content</div>");
    return doc.QuerySelector("div");  // 危险！doc 销毁后元素无效
}
```

**✅ 好的做法**:

```cpp
class MyClass {
    LexborDocument doc_;
    
public:
    LexborElement* GetElement() {
        doc_.ParseHTML("<div>Content</div>");
        return doc_.QuerySelector("div");  // 安全，doc_ 仍然存在
    }
};
```

**原因**: 元素依赖于文档对象，文档销毁后元素无效。

### 2. 避免内存泄漏

**✅ 好的做法**:

```cpp
{
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto elements = doc.QuerySelectorAll("div");
    // 使用 elements...
    
}  // doc 自动销毁，elements 也会被清理
```

**原因**: 文档对象会自动管理内存，无需手动释放。

### 3. 大文档处理

**✅ 好的做法**:

```cpp
void ProcessLargeHTML(const std::string& html) {
    LexborDocument doc;
    
    if (!doc.ParseHTML(html)) {
        return;
    }
    
    // 分批处理
    auto elements = doc.QuerySelectorAll("div");
    const size_t batchSize = 100;
    
    for (size_t i = 0; i < elements.size(); i += batchSize) {
        size_t end = std::min(i + batchSize, elements.size());
        
        for (size_t j = i; j < end; j++) {
            // 处理 elements[j]
        }
        
        // 可以在这里做一些清理或进度报告
    }
}
```

**原因**: 分批处理大文档可以避免内存峰值。

---

## 选择器优化

### 1. 选择器优先级

**性能从高到低**:

1. ID 选择器: `#id`
2. 类选择器: `.class`
3. 标签选择器: `div`
4. 属性选择器: `[attr]`
5. 伪类选择器: `:hover`
6. 通用选择器: `*`

**✅ 好的做法**:

```cpp
// 最快
auto element = doc.QuerySelector("#uniqueId");

// 较快
auto elements = doc.QuerySelectorAll(".className");

// 较慢
auto elements = doc.QuerySelectorAll("div[data-type='value']");

// 最慢
auto elements = doc.QuerySelectorAll("*");
```

### 2. 从右到左优化

CSS 选择器从右到左匹配，最右边的选择器应该最具体。

**❌ 不好的做法**:

```cpp
auto elements = doc.QuerySelectorAll("div span");  // 先找所有 span
```

**✅ 好的做法**:

```cpp
auto elements = doc.QuerySelectorAll("div.container span.text");  // 更具体
```

### 3. 避免过度限定

**❌ 不好的做法**:

```cpp
auto element = doc.QuerySelector("div#container");  // ID 已经唯一
```

**✅ 好的做法**:

```cpp
auto element = doc.QuerySelector("#container");
```

### 4. 使用子选择器而非后代选择器

**❌ 较慢**:

```cpp
auto elements = doc.QuerySelectorAll("div a");  // 后代选择器
```

**✅ 较快**:

```cpp
auto elements = doc.QuerySelectorAll("div > a");  // 子选择器
```

**原因**: 子选择器只查找直接子元素，后代选择器查找所有后代。

---

## 表单处理

### 1. 验证输入

**✅ 好的做法**:

```cpp
bool ValidateInput(LexborElement* input) {
    // 检查必填
    if (input->HasAttribute("required")) {
        std::string value = input->GetAttribute("value");
        if (value.empty()) {
            return false;
        }
    }
    
    // 检查长度
    std::string minLength = input->GetAttribute("minlength");
    if (!minLength.empty()) {
        std::string value = input->GetAttribute("value");
        if (value.length() < std::stoul(minLength)) {
            return false;
        }
    }
    
    // 检查模式
    std::string pattern = input->GetAttribute("pattern");
    if (!pattern.empty()) {
        std::string value = input->GetAttribute("value");
        std::regex re(pattern);
        if (!std::regex_match(value, re)) {
            return false;
        }
    }
    
    return true;
}
```

### 2. 收集表单数据

**✅ 好的做法**:

```cpp
std::map<std::string, std::string> CollectFormData(LexborElement* form) {
    std::map<std::string, std::string> data;
    
    // 文本输入
    auto textInputs = form->QuerySelectorAll(
        "input[type='text'], input[type='email'], input[type='number']"
    );
    for (auto* input : textInputs) {
        std::string name = input->GetAttribute("name");
        if (!name.empty()) {
            data[name] = input->GetAttribute("value");
        }
    }
    
    // 复选框
    auto checkboxes = form->QuerySelectorAll("input[type='checkbox']:checked");
    for (auto* checkbox : checkboxes) {
        std::string name = checkbox->GetAttribute("name");
        if (!name.empty()) {
            data[name] = "true";
        }
    }
    
    // 单选按钮
    auto radios = form->QuerySelectorAll("input[type='radio']:checked");
    for (auto* radio : radios) {
        std::string name = radio->GetAttribute("name");
        if (!name.empty()) {
            data[name] = radio->GetAttribute("value");
        }
    }
    
    // 选择框
    auto selects = form->QuerySelectorAll("select");
    for (auto* select : selects) {
        std::string name = select->GetAttribute("name");
        if (!name.empty()) {
            auto selected = select->QuerySelector("option[selected]");
            if (selected) {
                data[name] = selected->GetAttribute("value");
            }
        }
    }
    
    return data;
}
```

### 3. 处理多值字段

**✅ 好的做法**:

```cpp
std::map<std::string, std::vector<std::string>> CollectMultiValueFormData(LexborElement* form) {
    std::map<std::string, std::vector<std::string>> data;
    
    // 多选复选框
    auto checkboxes = form->QuerySelectorAll("input[type='checkbox']:checked");
    for (auto* checkbox : checkboxes) {
        std::string name = checkbox->GetAttribute("name");
        std::string value = checkbox->GetAttribute("value");
        data[name].push_back(value);
    }
    
    // 多选选择框
    auto selects = form->QuerySelectorAll("select[multiple]");
    for (auto* select : selects) {
        std::string name = select->GetAttribute("name");
        auto options = select->QuerySelectorAll("option[selected]");
        for (auto* option : options) {
            data[name].push_back(option->GetAttribute("value"));
        }
    }
    
    return data;
}
```

---

## 安全性

### 1. 防止 XSS 攻击

**❌ 不安全**:

```cpp
std::string userInput = GetUserInput();
std::string html = "<div>" + userInput + "</div>";  // 危险！
doc.ParseHTML(html);
```

**✅ 安全**:

```cpp
std::string EscapeHTML(const std::string& text) {
    std::string escaped;
    for (char c : text) {
        switch (c) {
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c;
        }
    }
    return escaped;
}

std::string userInput = GetUserInput();
std::string safeInput = EscapeHTML(userInput);
std::string html = "<div>" + safeInput + "</div>";
doc.ParseHTML(html);
```

### 2. 验证 URL

**✅ 好的做法**:

```cpp
bool IsValidURL(const std::string& url) {
    // 只允许 http 和 https
    if (url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://") {
        return false;
    }
    
    // 禁止 javascript: 等危险协议
    if (url.find("javascript:") != std::string::npos) {
        return false;
    }
    
    return true;
}

auto links = doc.QuerySelectorAll("a");
for (auto* link : links) {
    std::string href = link->GetAttribute("href");
    if (!IsValidURL(href)) {
        link->RemoveAttribute("href");
    }
}
```

### 3. 清理危险内容

**✅ 好的做法**:

```cpp
void SanitizeHTML(LexborDocument& doc) {
    // 移除 script 标签
    auto scripts = doc.QuerySelectorAll("script");
    // 注意：需要实际的移除 API
    
    // 移除事件处理器
    auto elementsWithEvents = doc.QuerySelectorAll(
        "[onclick], [onload], [onerror], [onmouseover]"
    );
    for (auto* elem : elementsWithEvents) {
        elem->RemoveAttribute("onclick");
        elem->RemoveAttribute("onload");
        elem->RemoveAttribute("onerror");
        elem->RemoveAttribute("onmouseover");
    }
    
    // 移除危险的 iframe
    auto iframes = doc.QuerySelectorAll("iframe");
    // 检查和清理...
}
```

---

## 代码组织

### 1. 封装常用操作

**✅ 好的做法**:

```cpp
class HTMLHelper {
public:
    static std::string GetElementText(LexborDocument& doc, const std::string& selector) {
        auto element = doc.QuerySelector(selector);
        return element ? element->GetTextContent() : "";
    }
    
    static std::vector<std::string> GetAllTexts(LexborDocument& doc, const std::string& selector) {
        std::vector<std::string> texts;
        auto elements = doc.QuerySelectorAll(selector);
        for (auto* elem : elements) {
            texts.push_back(elem->GetTextContent());
        }
        return texts;
    }
    
    static bool ElementExists(LexborDocument& doc, const std::string& selector) {
        return doc.QuerySelector(selector) != nullptr;
    }
};

// 使用
std::string title = HTMLHelper::GetElementText(doc, "h1");
auto paragraphs = HTMLHelper::GetAllTexts(doc, "p");
```

### 2. 使用配置对象

**✅ 好的做法**:

```cpp
struct ParserConfig {
    bool ignoreErrors = false;
    bool collectWarnings = true;
    bool validateHTML = true;
};

class HTMLParser {
    ParserConfig config_;
    LexborDocument doc_;
    
public:
    HTMLParser(const ParserConfig& config) : config_(config) {}
    
    bool Parse(const std::string& html) {
        bool result = doc_.ParseHTML(html);
        
        if (!result && !config_.ignoreErrors) {
            return false;
        }
        
        if (config_.collectWarnings && doc_.HasWarnings()) {
            // 处理警告...
        }
        
        return true;
    }
};
```

### 3. 使用 RAII 模式

**✅ 好的做法**:

```cpp
class ScopedHTMLDocument {
    LexborDocument doc_;
    bool parsed_ = false;
    
public:
    ScopedHTMLDocument(const std::string& html) {
        parsed_ = doc_.ParseHTML(html);
    }
    
    ~ScopedHTMLDocument() {
        // 自动清理
    }
    
    bool IsValid() const { return parsed_; }
    LexborDocument& GetDocument() { return doc_; }
};

// 使用
{
    ScopedHTMLDocument scopedDoc(html);
    if (scopedDoc.IsValid()) {
        auto& doc = scopedDoc.GetDocument();
        // 使用 doc...
    }
}  // 自动清理
```

---

## 性能基准

### 参考性能指标

基于我们的测试结果：

| 操作 | 数据规模 | 性能指标 |
|------|---------|---------|
| HTML 解析 | 5000 元素 | < 40ms |
| querySelector | 1000 元素 | < 10ms |
| querySelectorAll | 1000 元素 | < 50ms |
| 属性访问 | 单个元素 | < 1μs |
| 文本内容获取 | 单个元素 | < 1μs |

### 性能测试

**✅ 好的做法**:

```cpp
#include <chrono>

void BenchmarkParsing() {
    std::string html = GenerateLargeHTML(5000);  // 5000 个元素
    
    auto start = std::chrono::high_resolution_clock::now();
    
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "解析时间: " << duration.count() << "ms" << std::endl;
}
```

---

## 总结

### 核心原则

1. **性能优先**: 重用对象，缓存结果，使用具体选择器
2. **安全第一**: 验证输入，清理危险内容，防止 XSS
3. **错误处理**: 检查返回值，处理异常，记录警告
4. **代码质量**: 封装常用操作，使用 RAII，保持代码清晰

### 检查清单

- [ ] 是否重用了文档对象？
- [ ] 是否缓存了查询结果？
- [ ] 是否检查了空指针？
- [ ] 是否处理了解析错误？
- [ ] 是否验证了用户输入？
- [ ] 是否使用了具体的选择器？
- [ ] 是否清理了错误和警告？
- [ ] 是否进行了性能测试？

---

## 参考资源

- [API 参考文档](HTML_CSS_API_REFERENCE.md)
- [使用示例](HTML_CSS_USAGE_EXAMPLES.md)
- [开发计划](HTML_CSS_COMPLETE_SUPPORT_PLAN.md)

---

**版权所有 © 2025 MBink 项目**

