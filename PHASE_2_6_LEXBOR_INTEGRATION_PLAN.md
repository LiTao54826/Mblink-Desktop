# Phase 2.6: Lexbor完整集成 - 实施计划

> **创建日期**: 2025-11-11  
> **参考项目**: Lexbor官方文档 + RmlUi CSS引擎  
> **目标**: 完整集成Lexbor HTML/CSS解析器，为React生态支持打下坚实基础  
> **预计时间**: 2-3周  
> **当前进度**: 25% → 目标100%

---

## 🎯 总体目标

完整集成Lexbor库，实现生产级的HTML/CSS解析和样式计算能力：

1. ✅ **完整的HTML解析** - 支持HTML5标准，完整的文档解析
2. ✅ **CSS样式表解析** - 支持`<style>`标签、外部CSS文件、内联样式
3. ✅ **CSS选择器引擎** - 完整的CSS3选择器支持
4. ✅ **样式计算引擎** - CSS级联、继承、计算值
5. ✅ **性能优化** - 样式缓存、增量更新、批量操作

---

## 📊 当前状态分析

### ✅ 已完成部分 (25%)

| 功能 | 状态 | 文件 |
|------|------|------|
| Lexbor库集成 | ✅ 完成 | `third_party/lexbor/` |
| 基础HTML解析 | ✅ 完成 | `core/dom/element.cpp` (innerHTML/outerHTML) |
| CSS选择器基础 | ✅ 完成 | `core/dom/document.cpp` (querySelector) |
| DOM节点转换 | ✅ 完成 | `core/dom/element.cpp` (ConvertLexborNodeToNode) |

### ⏳ 待完成部分 (75%)

| 功能 | 优先级 | 预计时间 |
|------|--------|---------|
| 完整HTML文档解析 | P0 | 3天 |
| CSS样式表解析 | P0 | 4天 |
| 样式计算引擎 | P0 | 5天 |
| 样式缓存优化 | P1 | 2天 |
| 测试和文档 | P1 | 2天 |

---

## 📋 详细任务清单

### P0: 完整HTML文档解析 (必须完成，3天)

#### Task 1: LexborDocument包装类 ✅

**目标**: 创建完整的Lexbor文档包装类

**需要实现**:
- [ ] `LexborDocument` 类封装 `lxb_html_document_t`
- [ ] 完整的HTML5文档解析 (`ParseHTML`)
- [ ] HTML序列化 (`SerializeHTML`)
- [ ] 错误处理和报告
- [ ] 内存管理（RAII）

**API设计**:
```cpp
class LexborDocument {
public:
    LexborDocument();
    ~LexborDocument();
    
    // 解析HTML文档
    bool ParseHTML(const std::string& html);
    bool ParseHTMLFile(const std::string& file_path);
    
    // 序列化
    std::string SerializeHTML() const;
    std::string SerializeNode(lxb_dom_node_t* node) const;
    
    // 访问DOM树
    lxb_dom_element_t* GetDocumentElement() const;
    lxb_dom_element_t* GetBody() const;
    lxb_dom_element_t* GetHead() const;
    
    // 查询
    lxb_dom_element_t* QuerySelector(const std::string& selector) const;
    std::vector<lxb_dom_element_t*> QuerySelectorAll(const std::string& selector) const;
    
    // 错误处理
    bool HasErrors() const;
    std::vector<std::string> GetErrors() const;
    
private:
    lxb_html_document_t* doc_;
    std::vector<std::string> errors_;
};
```

**参考资料**:
- Lexbor官方文档: https://lexbor.com/docs/lexbor/
- Lexbor示例: `third_party/lexbor/examples/`

**测试用例**:
```cpp
TEST(LexborDocumentTest, ParseSimpleHTML) {
    LexborDocument doc;
    EXPECT_TRUE(doc.ParseHTML("<html><body><h1>Hello</h1></body></html>"));
    EXPECT_FALSE(doc.HasErrors());
}

TEST(LexborDocumentTest, ParseComplexHTML) {
    LexborDocument doc;
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Test</title></head>
        <body>
            <div class="container">
                <p>Paragraph 1</p>
                <p>Paragraph 2</p>
            </div>
        </body>
        </html>
    )";
    EXPECT_TRUE(doc.ParseHTML(html));
}

TEST(LexborDocumentTest, SerializeHTML) {
    LexborDocument doc;
    doc.ParseHTML("<html><body><h1>Hello</h1></body></html>");
    std::string html = doc.SerializeHTML();
    EXPECT_TRUE(html.find("<h1>Hello</h1>") != std::string::npos);
}
```

**文件位置**:
- `core/lexbor/lexbor_document.h`
- `core/lexbor/lexbor_document.cpp`
- `tests/test_lexbor_document.cpp`

---

#### Task 2: Document类集成Lexbor ✅

**目标**: 将Lexbor集成到现有的Document类中

**需要实现**:
- [ ] Document类持有LexborDocument实例
- [ ] `LoadHTML()` 方法使用Lexbor解析
- [ ] `SaveHTML()` 方法使用Lexbor序列化
- [ ] 双向同步：Lexbor DOM ↔ MBink DOM
- [ ] 增量更新机制

**API设计**:
```cpp
class Document : public Node {
public:
    // HTML加载和保存
    bool LoadHTML(const std::string& html);
    bool LoadHTMLFile(const std::string& file_path);
    std::string SaveHTML() const;
    
    // 同步Lexbor DOM
    void SyncFromLexbor();  // Lexbor → MBink
    void SyncToLexbor();    // MBink → Lexbor
    
private:
    std::unique_ptr<LexborDocument> lexbor_doc_;
    bool lexbor_dirty_ = false;
};
```

**同步策略**:
1. **解析时**: Lexbor → MBink（一次性转换）
2. **修改时**: 标记dirty，延迟同步
3. **查询时**: 如果dirty，先同步到Lexbor

**测试用例**:
```cpp
TEST(DocumentTest, LoadHTMLWithLexbor) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    std::string html = "<html><body><div id='test'>Hello</div></body></html>";
    EXPECT_TRUE(doc->LoadHTML(html));
    
    auto elem = doc->GetElementById("test");
    EXPECT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetTextContent(), "Hello");
}
```

---

### P0: CSS样式表解析 (必须完成，4天)

#### Task 3: LexborStyleSheet类 ✅

**目标**: 解析和管理CSS样式表

**需要实现**:
- [ ] `LexborStyleSheet` 类封装 `lxb_css_stylesheet_t`
- [ ] CSS解析 (`ParseCSS`)
- [ ] 样式规则管理
- [ ] 选择器解析
- [ ] 属性值解析

**API设计**:
```cpp
class LexborStyleSheet {
public:
    LexborStyleSheet();
    ~LexborStyleSheet();
    
    // 解析CSS
    bool ParseCSS(const std::string& css);
    bool ParseCSSFile(const std::string& file_path);
    
    // 访问规则
    size_t GetRuleCount() const;
    const CSSRule* GetRule(size_t index) const;
    
    // 查询匹配规则
    std::vector<const CSSRule*> GetMatchingRules(Element* element) const;
    
private:
    lxb_css_stylesheet_t* stylesheet_;
    std::vector<std::unique_ptr<CSSRule>> rules_;
};

struct CSSRule {
    std::string selector;           // 选择器字符串
    std::map<std::string, std::string> declarations;  // 属性声明
    int specificity;                // 选择器优先级
};
```

**CSS解析示例**:
```cpp
LexborStyleSheet sheet;
sheet.ParseCSS(R"(
    .container {
        display: flex;
        flex-direction: column;
        padding: 10px;
    }
    
    .container > p {
        color: #333;
        font-size: 14px;
    }
    
    #header {
        background-color: blue;
    }
)");
```

**测试用例**:
```cpp
TEST(LexborStyleSheetTest, ParseSimpleCSS) {
    LexborStyleSheet sheet;
    EXPECT_TRUE(sheet.ParseCSS(".test { color: red; }"));
    EXPECT_EQ(sheet.GetRuleCount(), 1);
}

TEST(LexborStyleSheetTest, ParseComplexCSS) {
    LexborStyleSheet sheet;
    std::string css = R"(
        .container { display: flex; }
        .container > p { color: blue; }
        #header { background: red; }
    )";
    EXPECT_TRUE(sheet.ParseCSS(css));
    EXPECT_EQ(sheet.GetRuleCount(), 3);
}
```

**文件位置**:
- `core/lexbor/lexbor_stylesheet.h`
- `core/lexbor/lexbor_stylesheet.cpp`
- `tests/test_lexbor_stylesheet.cpp`

---

#### Task 4: StyleManager类 ✅

**目标**: 管理所有样式表和样式规则

**需要实现**:
- [ ] 管理多个样式表（用户样式、内联样式、默认样式）
- [ ] 样式表优先级管理
- [ ] `<style>`标签解析
- [ ] 外部CSS文件加载
- [ ] 内联样式解析

**API设计**:
```cpp
class StyleManager {
public:
    // 添加样式表
    void AddStyleSheet(std::shared_ptr<LexborStyleSheet> sheet, int priority = 0);
    void RemoveStyleSheet(std::shared_ptr<LexborStyleSheet> sheet);
    
    // 解析样式
    void ParseStyleElement(Element* style_element);
    void ParseInlineStyle(Element* element, const std::string& style);
    
    // 加载外部CSS
    bool LoadCSSFile(const std::string& file_path);
    
    // 获取匹配规则
    std::vector<const CSSRule*> GetMatchingRules(Element* element) const;
    
    // 计算最终样式
    std::map<std::string, std::string> ComputeStyle(Element* element) const;
    
private:
    struct StyleSheetEntry {
        std::shared_ptr<LexborStyleSheet> sheet;
        int priority;
    };
    
    std::vector<StyleSheetEntry> stylesheets_;
};
```

**使用示例**:
```cpp
auto doc = std::make_shared<Document>();
doc->LoadHTML(R"(
    <html>
    <head>
        <style>
            .container { display: flex; }
        </style>
    </head>
    <body>
        <div class="container" style="color: red;">
            Content
        </div>
    </body>
    </html>
)");

auto& style_mgr = doc->GetStyleManager();
auto div = doc->QuerySelector(".container");
auto computed_style = style_mgr.ComputeStyle(div.get());

// computed_style = {
//     "display": "flex",
//     "color": "red"
// }
```

**文件位置**:
- `core/lexbor/style_manager.h`
- `core/lexbor/style_manager.cpp`
- `tests/test_style_manager.cpp`

---

### P0: 样式计算引擎 (必须完成，5天)

#### Task 5: CSS级联和继承 ✅

**目标**: 实现CSS级联、继承和计算值

**需要实现**:
- [ ] 选择器优先级计算（Specificity）
- [ ] CSS级联规则（Cascade）
- [ ] 属性继承（Inheritance）
- [ ] 计算值（Computed Values）
- [ ] 使用值（Used Values）

**优先级计算**:
```cpp
struct Specificity {
    int inline_style;   // 内联样式
    int id_count;       // ID选择器数量
    int class_count;    // 类/属性/伪类选择器数量
    int element_count;  // 元素/伪元素选择器数量
    
    int Compare(const Specificity& other) const {
        if (inline_style != other.inline_style) return inline_style - other.inline_style;
        if (id_count != other.id_count) return id_count - other.id_count;
        if (class_count != other.class_count) return class_count - other.class_count;
        return element_count - other.element_count;
    }
};

Specificity CalculateSpecificity(const std::string& selector);
```

**级联规则**:
```cpp
class CascadeEngine {
public:
    // 计算元素的最终样式
    std::map<std::string, std::string> ComputeStyle(
        Element* element,
        const std::vector<const CSSRule*>& matching_rules
    ) const;
    
private:
    // 应用级联规则
    std::string ApplyCascade(
        const std::string& property,
        const std::vector<std::pair<std::string, Specificity>>& values
    ) const;
    
    // 应用继承
    std::string ApplyInheritance(
        Element* element,
        const std::string& property
    ) const;
    
    // 计算值
    std::string ComputeValue(
        Element* element,
        const std::string& property,
        const std::string& specified_value
    ) const;
};
```

**继承属性列表**:
```cpp
const std::set<std::string> INHERITED_PROPERTIES = {
    "color",
    "font-family",
    "font-size",
    "font-weight",
    "line-height",
    "text-align",
    "visibility",
    // ... 更多
};
```

**测试用例**:
```cpp
TEST(CascadeEngineTest, Specificity) {
    auto spec1 = CalculateSpecificity("#id");
    auto spec2 = CalculateSpecificity(".class");
    EXPECT_GT(spec1.Compare(spec2), 0);  // ID > Class
}

TEST(CascadeEngineTest, Cascade) {
    // CSS: .test { color: red; }
    //      #test { color: blue; }
    // HTML: <div id="test" class="test">
    // 结果: color = blue (ID优先级更高)
}

TEST(CascadeEngineTest, Inheritance) {
    // CSS: body { color: red; }
    // HTML: <body><div><p>Text</p></div></body>
    // 结果: p的color继承为red
}
```

**文件位置**:
- `core/lexbor/cascade_engine.h`
- `core/lexbor/cascade_engine.cpp`
- `tests/test_cascade_engine.cpp`

---

#### Task 6: 样式缓存系统 ✅

**目标**: 优化样式计算性能

**需要实现**:
- [ ] 计算样式缓存
- [ ] 选择器匹配缓存
- [ ] 增量更新机制
- [ ] 脏标记系统

**API设计**:
```cpp
class StyleCache {
public:
    // 获取缓存的样式
    const std::map<std::string, std::string>* GetCachedStyle(Element* element) const;

    // 设置缓存
    void SetCachedStyle(Element* element, const std::map<std::string, std::string>& style);

    // 失效缓存
    void InvalidateElement(Element* element);
    void InvalidateSubtree(Element* element);
    void InvalidateAll();

    // 统计信息
    size_t GetCacheSize() const;
    double GetHitRate() const;

private:
    std::unordered_map<Element*, std::map<std::string, std::string>> cache_;
    size_t hits_ = 0;
    size_t misses_ = 0;
};
```

**缓存策略**:
1. **首次计算**: 计算并缓存
2. **DOM修改**: 失效相关元素缓存
3. **样式表修改**: 失效所有缓存
4. **属性修改**: 只失效该元素缓存

**性能目标**:
- 缓存命中率 > 90%
- 样式计算时间 < 1ms（缓存命中）
- 样式计算时间 < 10ms（缓存未命中）

**文件位置**:
- `core/lexbor/style_cache.h`
- `core/lexbor/style_cache.cpp`
- `tests/test_style_cache.cpp`

---

### P1: 性能优化和测试 (重要，4天)

#### Task 7: 性能优化 ✅

**目标**: 优化Lexbor集成的性能

**优化项**:
- [ ] 批量DOM操作优化
- [ ] 选择器匹配优化（Bloom Filter）
- [ ] 内存池管理
- [ ] 多线程样式计算（可选）

**批量操作API**:
```cpp
class Document {
public:
    // 批量操作模式
    void BeginBatch();
    void EndBatch();

    // 在批量模式下，所有DOM修改不会立即触发样式重计算
    // EndBatch时一次性重计算
};

// 使用示例
doc->BeginBatch();
for (int i = 0; i < 1000; ++i) {
    auto elem = doc->CreateElement("div");
    elem->SetAttribute("class", "item");
    container->AppendChild(elem);
}
doc->EndBatch();  // 一次性计算所有样式
```

**性能基准**:
```cpp
BENCHMARK(ParseHTML) {
    LexborDocument doc;
    std::string html = GenerateLargeHTML(10000);  // 10000个元素

    auto start = std::chrono::high_resolution_clock::now();
    doc.ParseHTML(html);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100);  // < 100ms
}

BENCHMARK(QuerySelectorAll) {
    // 测试复杂选择器性能
    auto elements = doc->QuerySelectorAll(".container > .item:nth-child(odd)");
    // 目标: < 10ms for 10000 elements
}

BENCHMARK(StyleComputation) {
    // 测试样式计算性能
    auto style = style_mgr.ComputeStyle(element);
    // 目标: < 1ms (cached), < 10ms (uncached)
}
```

**文件位置**:
- `tests/benchmark_lexbor.cpp`

---

#### Task 8: 完整测试覆盖 ✅

**目标**: 100%测试覆盖率

**测试类型**:

**1. 单元测试** (60个测试用例)
```cpp
// LexborDocument测试
TEST(LexborDocumentTest, ParseHTML)
TEST(LexborDocumentTest, SerializeHTML)
TEST(LexborDocumentTest, QuerySelector)
TEST(LexborDocumentTest, ErrorHandling)

// LexborStyleSheet测试
TEST(LexborStyleSheetTest, ParseCSS)
TEST(LexborStyleSheetTest, GetMatchingRules)
TEST(LexborStyleSheetTest, Specificity)

// CascadeEngine测试
TEST(CascadeEngineTest, Cascade)
TEST(CascadeEngineTest, Inheritance)
TEST(CascadeEngineTest, ComputedValues)

// StyleCache测试
TEST(StyleCacheTest, CacheHit)
TEST(StyleCacheTest, CacheMiss)
TEST(StyleCacheTest, Invalidation)
```

**2. 集成测试** (20个测试用例)
```cpp
TEST(LexborIntegrationTest, FullHTMLParsing) {
    auto doc = std::make_shared<Document>();
    doc->LoadHTML(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                .container { display: flex; }
                .item { color: red; }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="item">Item 1</div>
                <div class="item">Item 2</div>
            </div>
        </body>
        </html>
    )");

    auto items = doc->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 2);

    auto style = doc->GetStyleManager().ComputeStyle(items[0].get());
    EXPECT_EQ(style["color"], "red");
}
```

**3. 性能测试** (10个基准测试)
- HTML解析性能
- CSS解析性能
- 选择器匹配性能
- 样式计算性能
- 缓存性能

**4. 兼容性测试** (15个测试用例)
- HTML5标准兼容性
- CSS3选择器兼容性
- 边界情况处理
- 错误恢复

**测试覆盖率目标**:
- 行覆盖率 > 95%
- 分支覆盖率 > 90%
- 函数覆盖率 > 95%

**文件位置**:
- `tests/test_lexbor_document.cpp`
- `tests/test_lexbor_stylesheet.cpp`
- `tests/test_cascade_engine.cpp`
- `tests/test_style_cache.cpp`
- `tests/test_lexbor_integration.cpp`
- `tests/benchmark_lexbor.cpp`

---

#### Task 9: 文档和示例 ✅

**目标**: 完整的文档和示例代码

**文档内容**:

**1. API文档** (`docs/LEXBOR_API.md`)
- LexborDocument API参考
- LexborStyleSheet API参考
- StyleManager API参考
- CascadeEngine API参考
- 使用指南

**2. 集成指南** (`docs/LEXBOR_INTEGRATION.md`)
- Lexbor集成架构
- HTML解析流程
- CSS样式计算流程
- 性能优化建议
- 常见问题解答

**3. 示例代码** (`examples/lexbor_example.cpp`)
```cpp
#include "core/dom/document.h"
#include "core/lexbor/style_manager.h"

int main() {
    // 1. 创建文档并加载HTML
    auto doc = std::make_shared<Document>();
    doc->Initialize();

    doc->LoadHTML(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body { font-family: Arial; }
                .container {
                    display: flex;
                    padding: 20px;
                }
                .item {
                    color: #333;
                    font-size: 14px;
                }
                #header {
                    background-color: blue;
                    color: white;
                }
            </style>
        </head>
        <body>
            <div id="header">Header</div>
            <div class="container">
                <div class="item">Item 1</div>
                <div class="item">Item 2</div>
            </div>
        </body>
        </html>
    )");

    // 2. 查询元素
    auto header = doc->GetElementById("header");
    auto items = doc->QuerySelectorAll(".item");

    // 3. 计算样式
    auto& style_mgr = doc->GetStyleManager();
    auto header_style = style_mgr.ComputeStyle(header.get());

    std::cout << "Header background: " << header_style["background-color"] << std::endl;
    std::cout << "Header color: " << header_style["color"] << std::endl;

    // 4. 遍历元素
    for (const auto& item : items) {
        auto item_style = style_mgr.ComputeStyle(item.get());
        std::cout << "Item color: " << item_style["color"] << std::endl;
        std::cout << "Item font-size: " << item_style["font-size"] << std::endl;
    }

    // 5. 修改样式
    header->SetAttribute("style", "background-color: red;");
    auto new_style = style_mgr.ComputeStyle(header.get());
    std::cout << "New header background: " << new_style["background-color"] << std::endl;

    return 0;
}
```

**文件位置**:
- `docs/LEXBOR_API.md`
- `docs/LEXBOR_INTEGRATION.md`
- `examples/lexbor_example.cpp`

---

## 📈 进度跟踪

### 周计划

**第1周** (5天):
- Day 1-2: Task 1 - LexborDocument包装类
- Day 3: Task 2 - Document类集成Lexbor
- Day 4-5: Task 3 - LexborStyleSheet类

**第2周** (5天):
- Day 1-2: Task 4 - StyleManager类
- Day 3-4: Task 5 - CSS级联和继承
- Day 5: Task 6 - 样式缓存系统

**第3周** (4天):
- Day 1: Task 7 - 性能优化
- Day 2-3: Task 8 - 完整测试覆盖
- Day 4: Task 9 - 文档和示例

### 里程碑

| 里程碑 | 日期 | 描述 |
|--------|------|------|
| M1: HTML解析完成 | Day 3 | LexborDocument + Document集成 |
| M2: CSS解析完成 | Day 7 | LexborStyleSheet + StyleManager |
| M3: 样式计算完成 | Day 11 | CascadeEngine + StyleCache |
| M4: 测试和文档完成 | Day 14 | 100%测试覆盖 + 完整文档 |

---

## 🎯 验收标准

### 功能验收

- [ ] 能够解析完整的HTML5文档
- [ ] 能够解析CSS样式表（内联、`<style>`、外部文件）
- [ ] 能够正确计算元素的最终样式
- [ ] 支持CSS级联、继承、优先级
- [ ] 支持完整的CSS3选择器
- [ ] 性能达标（见性能目标）

### 性能验收

| 指标 | 目标 | 测试方法 |
|------|------|---------|
| HTML解析 | < 100ms (10000元素) | benchmark_lexbor |
| CSS解析 | < 50ms (1000规则) | benchmark_lexbor |
| 选择器匹配 | < 10ms (10000元素) | benchmark_lexbor |
| 样式计算 | < 1ms (缓存命中) | benchmark_lexbor |
| 样式计算 | < 10ms (缓存未命中) | benchmark_lexbor |
| 缓存命中率 | > 90% | 统计数据 |

### 质量验收

- [ ] 单元测试覆盖率 > 95%
- [ ] 集成测试通过率 100%
- [ ] 性能测试通过率 100%
- [ ] 无内存泄漏
- [ ] 无已知严重Bug

### 文档验收

- [ ] API文档完整
- [ ] 集成指南完整
- [ ] 示例代码可运行
- [ ] 注释覆盖率 > 80%

---

## 🔗 依赖关系

### 前置依赖

- ✅ Phase 2.5完成（DOM API、事件系统）
- ✅ Lexbor库已集成
- ✅ 基础HTML解析已实现

### 后续依赖

- Phase 3: React生态支持（依赖完整的样式计算）
- Phase 4: 高级CSS特性（依赖样式引擎）

---

## 📚 参考资料

### Lexbor官方资料
- 官方文档: https://lexbor.com/docs/lexbor/
- GitHub仓库: https://github.com/lexbor/lexbor
- API参考: https://lexbor.com/api/
- 示例代码: `third_party/lexbor/examples/`

### CSS规范
- CSS Cascading and Inheritance: https://www.w3.org/TR/css-cascade-3/
- CSS Selectors Level 3: https://www.w3.org/TR/selectors-3/
- CSS Values and Units: https://www.w3.org/TR/css-values-3/

### 参考实现
- RmlUi CSS引擎: `ReferenceProject/RmlUi/Source/Core/StyleSheet*`
- Servo CSS引擎: https://github.com/servo/servo
- WebKit CSS引擎: https://webkit.org/

---

## 🐛 风险和应对

### 技术风险

**1. Lexbor API复杂度**
- **风险**: Lexbor C API较底层，封装复杂
- **应对**: 参考官方示例，逐步封装，充分测试

**2. 性能问题**
- **风险**: 样式计算可能成为性能瓶颈
- **应对**: 实现缓存系统，性能基准测试，持续优化

**3. 内存管理**
- **风险**: Lexbor使用C内存管理，可能泄漏
- **应对**: RAII封装，智能指针，内存检测工具

### 进度风险

**1. 时间估算不准**
- **应对**: 每日进度跟踪，及时调整计划

**2. 测试覆盖不足**
- **应对**: TDD开发，边开发边测试

---

## ✅ 完成标志

Phase 2.6完成后，MBink将具备：

1. ✅ **生产级HTML解析** - 完整的HTML5支持
2. ✅ **完整的CSS引擎** - 样式表解析、级联、继承
3. ✅ **高性能样式计算** - 缓存优化、增量更新
4. ✅ **100%测试覆盖** - 单元测试、集成测试、性能测试
5. ✅ **完整文档** - API文档、集成指南、示例代码

**为Phase 3 React生态支持打下坚实基础！** 🚀

---

**创建日期**: 2025-11-11
**最后更新**: 2025-11-11
**维护者**: MBink Team

