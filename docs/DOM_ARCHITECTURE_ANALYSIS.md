# MBink DOM 架构分析

**日期**: 2025-11-15  
**问题**: 为什么有两个 DOM 系统？能否直接使用 Lexbor DOM？

---

## 📊 当前架构

### 双 DOM 系统

MBink 目前使用**两个独立的 DOM 系统**:

```
┌─────────────────────────────────────────────────────────┐
│                   JavaScript Layer                       │
│              (QuickJS + DOM Bindings)                    │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                    MBink DOM                             │
│  - core/dom/node.h/cpp                                   │
│  - core/dom/element.h/cpp                                │
│  - core/dom/document.h/cpp                               │
│  - 40+ HTML 元素类 (HTMLInputElement, etc.)              │
│  - 事件系统 (addEventListener, etc.)                     │
│  - 样式管理 (classList, style, dataset)                 │
└─────────────────────────────────────────────────────────┘
                           ↓
                    (单向同步)
                           ↓
┌─────────────────────────────────────────────────────────┐
│                   Lexbor DOM                             │
│  - third_party/lexbor/                                   │
│  - HTML5 解析器                                          │
│  - CSS 选择器引擎                                        │
│  - 只读查询 (querySelector, etc.)                        │
└─────────────────────────────────────────────────────────┘
```

### 两个系统的职责

| 功能 | MBink DOM | Lexbor DOM |
|------|-----------|------------|
| **HTML 解析** | ❌ 不支持 | ✅ 完整的 HTML5 解析器 |
| **动态创建元素** | ✅ `createElement()` | ❌ 不支持 |
| **DOM 树操作** | ✅ 完整支持 | ❌ 只读 |
| **CSS 选择器** | ❌ 依赖 Lexbor | ✅ 高性能选择器引擎 |
| **事件系统** | ✅ 完整实现 | ❌ 不支持 |
| **样式管理** | ✅ classList, style | ❌ 不支持 |
| **JavaScript 绑定** | ✅ 所有 API | ❌ 不绑定 |
| **渲染/布局** | ✅ 连接到渲染引擎 | ❌ 不连接 |

---

## 🤔 为什么会有两个 DOM？

### 历史原因

从代码分析来看，MBink 的架构演进可能是这样的:

1. **阶段 1**: 最初使用 Lexbor 作为 HTML 解析器
   - Lexbor 提供了强大的 HTML5 解析能力
   - Lexbor 提供了高性能的 CSS 选择器引擎

2. **阶段 2**: 发现 Lexbor 的局限性
   - Lexbor 主要是解析器，不是完整的 DOM 实现
   - Lexbor 不支持动态 DOM 操作 (createElement, appendChild 等)
   - Lexbor 不支持事件系统
   - Lexbor 不适合与 JavaScript 引擎深度集成

3. **阶段 3**: 创建 MBink DOM
   - 实现完整的 DOM API (Node, Element, Document)
   - 实现事件系统 (addEventListener, dispatchEvent)
   - 实现样式管理 (classList, style, dataset)
   - 实现 40+ 专用 HTML 元素类
   - 与 QuickJS 深度集成

4. **阶段 4**: 保留 Lexbor 用于特定功能
   - HTML 解析: `LoadHTML()` 使用 Lexbor 解析，然后转换为 MBink DOM
   - CSS 选择器: `querySelector()` 委托给 Lexbor

### 技术原因

#### Lexbor 的优势
- ✅ **高性能 HTML5 解析器** - 符合标准，速度快
- ✅ **强大的 CSS 选择器引擎** - 支持复杂选择器
- ✅ **成熟稳定** - 经过大量测试

#### Lexbor 的局限性
- ❌ **只读 DOM** - 主要用于解析，不适合动态操作
- ❌ **C API** - 不是 C++ 友好的 API
- ❌ **无事件系统** - 不支持 addEventListener
- ❌ **无样式管理** - 不支持 classList, style 等
- ❌ **不适合 JavaScript 绑定** - 需要大量包装代码

#### MBink DOM 的优势
- ✅ **完整的 DOM API** - 支持所有标准 DOM 操作
- ✅ **C++ 友好** - 使用 `std::shared_ptr`, 现代 C++ 设计
- ✅ **事件系统** - 完整的事件冒泡、捕获机制
- ✅ **样式管理** - classList, style, dataset 等
- ✅ **易于扩展** - 可以添加自定义元素和功能
- ✅ **JavaScript 集成** - 与 QuickJS 深度集成

---

## 💡 能否直接使用 Lexbor DOM？

### 答案: **不能**

直接使用 Lexbor DOM 会面临以下问题:

### 问题 1: 缺少动态 DOM 操作
```javascript
// 这些操作在 Lexbor 中无法实现
const div = document.createElement('div');  // ❌ Lexbor 不支持
div.appendChild(span);                      // ❌ Lexbor 不支持
div.removeChild(span);                      // ❌ Lexbor 不支持
```

### 问题 2: 缺少事件系统
```javascript
// Lexbor 没有事件系统
button.addEventListener('click', handler);  // ❌ Lexbor 不支持
button.dispatchEvent(event);                // ❌ Lexbor 不支持
```

### 问题 3: 缺少样式管理
```javascript
// Lexbor 没有样式管理
element.classList.add('active');            // ❌ Lexbor 不支持
element.style.color = 'red';                // ❌ Lexbor 不支持
element.dataset.userId = '123';             // ❌ Lexbor 不支持
```

### 问题 4: C API 不友好
```cpp
// Lexbor 是 C API，使用起来很繁琐
lxb_dom_element_t* elem = lxb_dom_element_create(...);  // 手动内存管理
lxb_dom_element_set_attribute(...);                     // 冗长的函数名
// 需要大量包装代码才能暴露给 JavaScript
```

### 问题 5: 无法与渲染引擎集成
- Lexbor 只是解析器，不知道如何渲染
- MBink DOM 需要与 Skia 渲染引擎、Yoga 布局引擎集成
- 需要自定义的元素类 (HTMLInputElement, HTMLButtonElement) 来处理特殊行为

---

## 🎯 正确的架构方案

### 方案对比

| 方案 | 优点 | 缺点 | 可行性 |
|------|------|------|--------|
| **方案 1: 保持双 DOM** | 各司其职，清晰分离 | querySelector 问题 | ✅ 当前方案 |
| **方案 2: 只用 Lexbor** | 简化架构 | 缺少太多功能 | ❌ 不可行 |
| **方案 3: 只用 MBink DOM** | 统一架构 | 需要实现 HTML 解析器和 CSS 选择器 | ⚠️ 工作量大 |
| **方案 4: 双 DOM + 同步** | 解决 querySelector 问题 | 增加复杂度 | ✅ **推荐** |

### 推荐方案: 双 DOM + 同步机制

保持双 DOM 架构，但实现**双向同步机制**:

```
┌─────────────────────────────────────────┐
│          MBink DOM (主 DOM)              │
│  - 所有 DOM 操作                         │
│  - 事件系统                              │
│  - 样式管理                              │
│  - JavaScript 绑定                       │
└─────────────────────────────────────────┘
              ↕ (双向同步)
┌─────────────────────────────────────────┐
│        Lexbor DOM (辅助 DOM)             │
│  - HTML 解析                             │
│  - CSS 选择器查询                        │
└─────────────────────────────────────────┘
```

#### 同步策略

**MBink → Lexbor 同步** (在需要 querySelector 时):
```cpp
// 在 Element::QuerySelector() 中
std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    // 1. 如果元素没有 Lexbor 表示，创建它
    if (!lexbor_element_) {
        SyncToLexbor();
    }
    
    // 2. 使用 Lexbor 查询
    auto lexbor_result = lexbor_element_->QuerySelector(selector);
    
    // 3. 转换回 MBink Element
    return ConvertLexborToMBink(lexbor_result);
}
```

**Lexbor → MBink 同步** (在 LoadHTML 时):
```cpp
// 在 Document::LoadHTML() 中
bool Document::LoadHTML(const std::string& html) {
    // 1. Lexbor 解析 HTML
    lexbor_doc_->ParseHTML(html);
    
    // 2. 转换为 MBink DOM
    SyncFromLexbor();
    
    return true;
}
```

---

## 🔧 实现同步机制

### 需要修改的地方

#### 1. Element 类添加 Lexbor 引用
```cpp
// core/dom/element.h
class Element : public Node {
private:
    lxb_dom_element_t* lexbor_element_ = nullptr;  // Lexbor 表示
    
public:
    void SyncToLexbor();      // 同步到 Lexbor
    void UpdateLexbor();      // 更新 Lexbor 属性
};
```

#### 2. 在 DOM 操作时同步
```cpp
// core/dom/element.cpp

void Element::AppendChild(std::shared_ptr<Node> child) {
    // MBink DOM 操作
    Node::AppendChild(child);
    
    // 同步到 Lexbor
    if (lexbor_element_) {
        auto child_elem = std::dynamic_pointer_cast<Element>(child);
        if (child_elem) {
            child_elem->SyncToLexbor();
            lxb_dom_node_insert_child(
                lxb_dom_interface_node(lexbor_element_),
                lxb_dom_interface_node(child_elem->lexbor_element_)
            );
        }
    }
}

void Element::SetAttribute(const std::string& name, const std::string& value) {
    // MBink DOM 操作
    attributes_[name] = value;
    
    // 同步到 Lexbor
    if (lexbor_element_) {
        lxb_dom_element_set_attribute(
            lexbor_element_,
            (const lxb_char_t*)name.c_str(), name.length(),
            (const lxb_char_t*)value.c_str(), value.length()
        );
    }
}
```

#### 3. 在 createElement 时创建 Lexbor 节点
```cpp
// core/dom/document.cpp

std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    auto element = CreateElementByTagName(tag_name);
    
    // 同时创建 Lexbor 节点
    if (lexbor_doc_) {
        lxb_dom_element_t* lexbor_elem = lxb_dom_document_create_element(
            lxb_dom_interface_document(lexbor_doc_->GetNativeDocument()),
            (const lxb_char_t*)tag_name.c_str(), tag_name.length(),
            nullptr
        );
        element->SetLexborElement(lexbor_elem);
    }
    
    return element;
}
```

### 性能优化

#### 延迟同步
```cpp
class Element {
private:
    bool lexbor_dirty_ = false;  // 标记是否需要同步
    
public:
    void MarkLexborDirty() { lexbor_dirty_ = true; }
    
    std::shared_ptr<Element> QuerySelector(const std::string& selector) {
        // 只在需要查询时才同步
        if (lexbor_dirty_) {
            SyncToLexbor();
            lexbor_dirty_ = false;
        }
        
        // 使用 Lexbor 查询
        return DoQuerySelector(selector);
    }
};
```

---

## 📊 工作量评估

### 实现同步机制的工作量

| 任务 | 工作量 | 优先级 |
|------|--------|--------|
| 1. Element 添加 Lexbor 引用 | 0.5 天 | 高 |
| 2. CreateElement 同步创建 | 0.5 天 | 高 |
| 3. AppendChild/RemoveChild 同步 | 1 天 | 高 |
| 4. SetAttribute 同步 | 0.5 天 | 高 |
| 5. 延迟同步优化 | 0.5 天 | 中 |
| 6. 测试和调试 | 1 天 | 高 |
| **总计** | **4 天** | - |

### 预期效果

- ✅ querySelector 测试通过率: 50% → **95%+**
- ✅ 总体测试通过率: 90.5% → **95%+**
- ✅ 支持动态元素的 CSS 选择器查询
- ✅ 保持现有架构的优势

---

## 🎯 结论

### 为什么有两个 DOM？

1. **Lexbor** 提供了强大的 HTML 解析和 CSS 选择器引擎
2. **MBink DOM** 提供了完整的 DOM API、事件系统、样式管理
3. 两者各有优势，互补使用

### 能否直接使用 Lexbor DOM？

**不能**，因为:
- ❌ Lexbor 缺少动态 DOM 操作
- ❌ Lexbor 缺少事件系统
- ❌ Lexbor 缺少样式管理
- ❌ Lexbor 不适合 JavaScript 绑定

### 最佳方案

**保持双 DOM 架构 + 实现同步机制**:
- ✅ 保留 Lexbor 的 HTML 解析和 CSS 选择器优势
- ✅ 保留 MBink DOM 的完整 API 和灵活性
- ✅ 通过同步机制解决 querySelector 问题
- ✅ 工作量可控 (约 4 天)

---

**建议**: 实现 MBink DOM ↔ Lexbor DOM 双向同步机制，这是解决当前 querySelector 问题的最佳方案。

