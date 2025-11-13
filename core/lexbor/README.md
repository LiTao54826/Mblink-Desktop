# Lexbor 模块

## 📋 概述

Lexbor 模块是 MBink 的 HTML/CSS 解析和样式管理层，基于高性能的 Lexbor 库实现。它负责解析 HTML5 文档、CSS3 样式表，并提供完整的样式级联、继承和缓存机制。

## 🎯 主要功能

- **HTML5 解析**: 完整的 HTML5 规范支持
- **CSS3 解析**: CSS3 选择器和属性解析
- **样式管理**: 样式表管理和查询
- **样式级联**: CSS 级联算法实现
- **样式继承**: 自动处理可继承属性
- **样式缓存**: 高性能样式计算缓存
- **选择器匹配**: 高效的 CSS 选择器匹配引擎

## 📁 文件结构

```
lexbor/
├── CMakeLists.txt              # 构建配置
├── lexbor_document.h/cpp       # HTML 文档解析
├── lexbor_stylesheet.h/cpp     # CSS 样式表解析
├── style_manager.h/cpp         # 样式管理器
├── cascade_engine.h/cpp        # 样式级联引擎
└── style_cache.h/cpp           # 样式缓存
```

## 🔌 核心类

### LexborDocument (HTML 解析)

```cpp
class LexborDocument {
public:
    LexborDocument();
    ~LexborDocument();
    
    // HTML 解析
    bool ParseHTML(const std::string& html);
    bool ParseHTMLFile(const std::string& filepath);
    
    // DOM 树转换
    std::shared_ptr<Document> ToDocument();
    
    // 查询
    lxb_dom_element_t* QuerySelector(const std::string& selector);
    std::vector<lxb_dom_element_t*> QuerySelectorAll(
        const std::string& selector);
};
```

### LexborStylesheet (CSS 解析)

```cpp
class LexborStylesheet {
public:
    LexborStylesheet();
    ~LexborStylesheet();
    
    // CSS 解析
    bool ParseCSS(const std::string& css);
    bool ParseCSSFile(const std::string& filepath);
    
    // 规则查询
    std::vector<CSSRule> GetRules() const;
    std::vector<CSSRule> GetMatchingRules(
        std::shared_ptr<Element> element);
};
```

### StyleManager (样式管理)

```cpp
class StyleManager {
public:
    StyleManager();
    ~StyleManager();
    
    // 样式表管理
    void AddStylesheet(std::shared_ptr<LexborStylesheet> stylesheet);
    void RemoveStylesheet(std::shared_ptr<LexborStylesheet> stylesheet);
    void ClearStylesheets();
    
    // 计算样式
    ComputedStyle ComputeStyle(std::shared_ptr<Element> element);
    
    // 样式查询
    std::string GetPropertyValue(std::shared_ptr<Element> element,
                                 const std::string& property);
};
```

### CascadeEngine (样式级联)

```cpp
class CascadeEngine {
public:
    // 级联计算
    ComputedStyle Cascade(std::shared_ptr<Element> element,
                         const std::vector<CSSRule>& rules);
    
    // 继承处理
    void ApplyInheritance(ComputedStyle& style,
                         const ComputedStyle& parent_style);
    
    // 优先级计算
    int CalculateSpecificity(const std::string& selector);
};
```

### StyleCache (样式缓存)

```cpp
class StyleCache {
public:
    // 缓存操作
    void Set(std::shared_ptr<Element> element, 
            const ComputedStyle& style);
    
    std::optional<ComputedStyle> Get(
        std::shared_ptr<Element> element);
    
    void Invalidate(std::shared_ptr<Element> element);
    void Clear();
    
    // 统计信息
    size_t GetHitCount() const;
    size_t GetMissCount() const;
    float GetHitRate() const;
};
```

## 💡 使用示例

### HTML 解析

```cpp
auto lexbor_doc = std::make_shared<LexborDocument>();

// 解析 HTML 字符串
std::string html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Test</title>
    </head>
    <body>
        <div id="container">
            <h1>Hello World</h1>
            <p class="text">Content</p>
        </div>
    </body>
    </html>
)";

if (lexbor_doc->ParseHTML(html)) {
    // 转换为 MBink DOM
    auto doc = lexbor_doc->ToDocument();
}
```

### CSS 解析

```cpp
auto stylesheet = std::make_shared<LexborStylesheet>();

// 解析 CSS
std::string css = R"(
    body {
        margin: 0;
        padding: 0;
        font-family: Arial, sans-serif;
    }
    
    #container {
        width: 800px;
        margin: 0 auto;
    }
    
    .text {
        color: #333;
        font-size: 16px;
    }
)";

if (stylesheet->ParseCSS(css)) {
    // 添加到样式管理器
    style_manager->AddStylesheet(stylesheet);
}
```

### 样式计算

```cpp
auto style_manager = std::make_shared<StyleManager>();

// 添加样式表
style_manager->AddStylesheet(stylesheet);

// 计算元素的最终样式
auto element = doc->QuerySelector("#container");
auto computed_style = style_manager->ComputeStyle(element);

// 获取特定属性值
std::string width = style_manager->GetPropertyValue(element, "width");
std::string color = style_manager->GetPropertyValue(element, "color");
```

### 样式缓存

```cpp
auto cache = std::make_shared<StyleCache>();

// 计算并缓存样式
auto style = style_manager->ComputeStyle(element);
cache->Set(element, style);

// 从缓存获取
auto cached_style = cache->Get(element);
if (cached_style.has_value()) {
    // 使用缓存的样式
    std::cout << "Cache hit!" << std::endl;
}

// 样式失效时清除缓存
element->GetStyle()->SetProperty("color", "red");
cache->Invalidate(element);
```

## 🔗 依赖关系

### 依赖的模块

- `third_party/lexbor` - Lexbor HTML/CSS 解析库
- `core/dom` - DOM 元素
- `core/utils` - 工具函数

### 被依赖的模块

- `core/render` - 渲染引擎（使用计算后的样式）
- `core/layout` - 布局引擎（使用样式属性）

## 🏗️ 架构说明

Lexbor 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  HTML/CSS Source                        │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Lexbor Module (core/lexbor) ← 当前模块  │
│  HTML Parser + CSS Parser + Cascade     │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  DOM Tree (core/dom)                    │
│  with Computed Styles                   │
└─────────────────────────────────────────┘
```

## 📊 CSS 级联算法

### 优先级顺序（从高到低）

1. **!important 声明**
2. **内联样式** (`style` 属性)
3. **ID 选择器** (`#id`)
4. **类选择器、属性选择器、伪类** (`.class`, `[attr]`, `:hover`)
5. **元素选择器、伪元素** (`div`, `::before`)
6. **继承的样式**
7. **浏览器默认样式**

### 特异性计算

```
选择器                    特异性
#id                      (1, 0, 0)
.class                   (0, 1, 0)
div                      (0, 0, 1)
#id .class div           (1, 1, 1)
```

### 级联示例

```cpp
// CSS 规则
// div { color: blue; }           特异性: (0, 0, 1)
// .text { color: green; }        特异性: (0, 1, 0)
// #main { color: red; }          特异性: (1, 0, 0)

// HTML: <div id="main" class="text">

// 最终 color = red (ID 选择器优先级最高)
```

## 🔧 支持的 CSS 选择器

### 基础选择器
- `*` - 通用选择器
- `div` - 元素选择器
- `.class` - 类选择器
- `#id` - ID 选择器

### 组合选择器
- `div p` - 后代选择器
- `div > p` - 子选择器
- `div + p` - 相邻兄弟选择器
- `div ~ p` - 通用兄弟选择器

### 属性选择器
- `[attr]` - 存在属性
- `[attr=value]` - 精确匹配
- `[attr~=value]` - 包含单词
- `[attr^=value]` - 开头匹配
- `[attr$=value]` - 结尾匹配
- `[attr*=value]` - 包含子串

### 伪类（部分支持）
- `:hover`, `:active`, `:focus`
- `:first-child`, `:last-child`
- `:nth-child(n)`

## ⚠️ 注意事项

1. **编码**: 所有 HTML/CSS 必须使用 UTF-8 编码
2. **性能**: 复杂选择器会影响匹配性能
3. **缓存失效**: 样式变化时需要手动失效缓存
4. **内存**: Lexbor 使用自己的内存管理，注意释放

## 🚀 性能优化

### 选择器优化

```cpp
// ❌ 慢：通用选择器
* { margin: 0; }

// ✅ 快：具体选择器
body, div, p { margin: 0; }

// ❌ 慢：深层后代选择器
html body div div div p { color: red; }

// ✅ 快：类选择器
.text { color: red; }
```

### 缓存策略

```cpp
// 只在必要时失效缓存
void UpdateStyle(std::shared_ptr<Element> element, 
                const std::string& property,
                const std::string& value) {
    element->GetStyle()->SetProperty(property, value);
    
    // 只失效受影响的元素
    cache->Invalidate(element);
    
    // 如果是可继承属性，失效子元素
    if (IsInheritableProperty(property)) {
        InvalidateDescendants(element);
    }
}
```

## 📚 相关文档

- [Lexbor 官方文档](https://lexbor.com/)
- [CSS 级联规范](https://www.w3.org/TR/css-cascade-3/)
- [CSS 选择器规范](https://www.w3.org/TR/selectors-4/)
- [渲染引擎文档](../render/README.md)

## 🐛 调试技巧

### 打印样式规则

```cpp
void PrintMatchingRules(std::shared_ptr<Element> element) {
    auto rules = stylesheet->GetMatchingRules(element);
    
    for (const auto& rule : rules) {
        std::cout << "Selector: " << rule.selector << std::endl;
        std::cout << "Specificity: " << rule.specificity << std::endl;
        
        for (const auto& [prop, value] : rule.declarations) {
            std::cout << "  " << prop << ": " << value << std::endl;
        }
    }
}
```

### 缓存统计

```cpp
void PrintCacheStats() {
    std::cout << "Cache hits: " << cache->GetHitCount() << std::endl;
    std::cout << "Cache misses: " << cache->GetMissCount() << std::endl;
    std::cout << "Hit rate: " << cache->GetHitRate() * 100 << "%" << std::endl;
}
```

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

