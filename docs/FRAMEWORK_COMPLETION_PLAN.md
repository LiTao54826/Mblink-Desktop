# MBink 框架完善开发计划

**创建日期**: 2025-11-15  
**当前状态**: 测试通过率 90.5% (105/116)  
**目标**: 修复所有已知问题，达到 99%+ 测试通过率

---

## 📊 当前状态总结

### 测试通过率
| 模块 | 通过/总数 | 通过率 | 状态 |
|------|-----------|--------|------|
| DOM 操作 | 18/19 | 94.7% | 🟡 1个问题 |
| 属性和样式 | 26/26 | 100% | ✅ 完美 |
| 事件系统 | 17/17 | 100% | ✅ 完美 |
| 查询选择器 | 10/20 | 50% | 🔴 10个问题 |
| 定时器 | 9/9 | 100% | ✅ 完美 |
| 表单元素 | 18/18 | 100% | ✅ 完美 |
| HTML 内容 | 7/17 | 41.2% | 🔴 7个问题 |
| **总计** | **105/116** | **90.5%** | 🟡 **18个问题** |

### 已知问题
1. **querySelector 架构问题** - 10个测试失败 (高优先级)
2. **innerHTML/outerHTML 问题** - 7个测试失败 (中优先级)
3. **cloneNode 深克隆 bug** - 1个测试失败 (低优先级)

---

## 🎯 开发计划概览

| 阶段 | 任务 | 工作量 | 优先级 | 预期通过率 |
|------|------|--------|--------|-----------|
| **阶段 0** | 快速修复 | 0.5 天 | 🔴 高 | 91.4% → 92.2% |
| **阶段 1** | innerHTML/outerHTML 修复 | 1-2 天 | 🟡 中 | 92.2% → 97.4% |
| **阶段 2** | DOM 同步机制 | 3-4 天 | 🔴 高 | 97.4% → 99.1% |
| **阶段 3** | 性能优化和测试 | 1 天 | 🟢 低 | 99.1% → 99.1% |
| **总计** | - | **5.5-7.5 天** | - | **99.1%** |

---

## 🚀 阶段 0: 快速修复 (0.5 天)

### 目标
修复最简单的问题，快速提升通过率

### 任务清单

#### 任务 0.1: 修复 outerHTML setter 崩溃 (2 小时)
**问题**: `Cannot set outerHTML without document`

**文件**: `core/dom/element.cpp`

**修复步骤**:
1. 查看 `Element::SetOuterHTML()` 实现
2. 检查 document 引用检查逻辑
3. 确保动态创建的元素也有 document 引用
4. 或者放宽检查条件

**代码位置**:
```cpp
// core/dom/element.cpp - Element::SetOuterHTML()
void Element::SetOuterHTML(const std::string& html) {
    auto doc = GetOwnerDocument();
    if (!doc) {
        // 修复: 不要直接抛出错误，尝试获取父节点的 document
        auto parent = GetParentNode();
        if (parent) {
            doc = parent->GetOwnerDocument();
        }
    }
    
    if (!doc) {
        LOG_ERROR("Cannot set outerHTML without document");
        return;  // 改为 return 而不是抛出异常
    }
    
    // ... 其余逻辑
}
```

**预期效果**: +1 个测试通过 (91.4%)

---

#### 任务 0.2: 修复 cloneNode 深克隆 bug (1 小时)
**问题**: `cloneNode(true)` 只克隆 1 个子节点而不是 2 个

**文件**: `core/dom/element.cpp` 或 `core/dom/node.cpp`

**修复步骤**:
1. 查看 `Element::CloneNode(bool deep)` 实现
2. 检查递归克隆逻辑
3. 确保遍历所有子节点

**代码位置**:
```cpp
// core/dom/element.cpp - Element::CloneNode()
std::shared_ptr<Node> Element::CloneNode(bool deep) {
    auto clone = std::make_shared<Element>(tag_name_);
    
    // 克隆属性
    clone->attributes_ = attributes_;
    
    if (deep) {
        // 修复: 确保遍历所有子节点
        for (const auto& child : child_nodes_) {  // 使用 child_nodes_ 而不是其他
            auto child_clone = child->CloneNode(true);
            clone->AppendChild(child_clone);
        }
    }
    
    return clone;
}
```

**预期效果**: +1 个测试通过 (92.2%)

---

## 🔧 阶段 1: innerHTML/outerHTML 修复 (1-2 天)

### 目标
修复 HTML 内容操作的所有问题

### 任务清单

#### 任务 1.1: 修复 innerHTML setter JavaScript 错误 (4 小时)
**问题**: `cannot read property 'toLowerCase' of undefined`

**分析**:
- 错误来自 JavaScript 层
- 可能是 HTML 解析过程中的问题
- 需要检查 Lexbor 解析和转换逻辑

**修复步骤**:
1. 在 `Element::SetInnerHTML()` 中添加调试日志
2. 检查 Lexbor 解析是否成功
3. 检查 `ConvertLexborNodeToNode()` 转换逻辑
4. 确保所有节点类型都正确处理

**文件**: `core/dom/element.cpp`

**代码位置**:
```cpp
// core/dom/element.cpp - Element::SetInnerHTML()
void Element::SetInnerHTML(const std::string& html) {
    // 清空现有子节点
    child_nodes_.clear();
    
    if (html.empty()) {
        return;
    }
    
    // 创建临时文档解析 HTML
    lxb_html_document_t* temp_doc = lxb_html_document_create();
    
    // 解析 HTML 片段
    lxb_dom_node_t* fragment = lxb_html_document_parse_fragment(
        temp_doc, 
        lxb_dom_interface_element(body),
        (const lxb_char_t*)html.c_str(), 
        html.length()
    );
    
    if (!fragment) {
        LOG_ERROR("Failed to parse HTML fragment");
        lxb_html_document_destroy(temp_doc);
        return;
    }
    
    // 转换所有子节点
    auto doc = GetOwnerDocument();
    lxb_dom_node_t* child = fragment->first_child;
    while (child) {
        // 添加类型检查
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT || 
            child->type == LXB_DOM_NODE_TYPE_TEXT) {
            std::shared_ptr<Node> new_node = ConvertLexborNodeToNode(child, doc);
            if (new_node) {
                AppendChild(new_node);
            }
        }
        child = child->next;
    }
    
    lxb_html_document_destroy(temp_doc);
}
```

**预期效果**: +3 个测试通过 (94.8%)

---

#### 任务 1.2: 修复 innerHTML getter 序列化问题 (3 小时)
**问题**: `innerHTML` 返回空字符串或不完整的 HTML

**修复步骤**:
1. 检查 `Element::GetInnerHTML()` 实现
2. 确保正确序列化所有子节点
3. 检查标签名、属性、文本内容的序列化

**文件**: `core/dom/element.cpp`

**代码位置**:
```cpp
// core/dom/element.cpp - Element::GetInnerHTML()
std::string Element::GetInnerHTML() const {
    std::string result;
    
    for (const auto& child : child_nodes_) {
        if (auto elem = std::dynamic_pointer_cast<Element>(child)) {
            // 元素节点: 序列化为完整的 HTML
            result += elem->GetOuterHTML();
        } else if (auto text = std::dynamic_pointer_cast<Text>(child)) {
            // 文本节点: 直接添加文本内容
            result += text->GetData();
        }
    }
    
    return result;
}
```

**预期效果**: +2 个测试通过 (96.5%)

---

#### 任务 1.3: 修复 outerHTML getter 序列化问题 (2 小时)
**问题**: `outerHTML` 返回空字符串

**修复步骤**:
1. 检查 `Element::GetOuterHTML()` 实现
2. 确保正确序列化开始标签、属性、子节点、结束标签

**文件**: `core/dom/element.cpp`

**代码位置**:
```cpp
// core/dom/element.cpp - Element::GetOuterHTML()
std::string Element::GetOuterHTML() const {
    std::string result;
    
    // 开始标签
    result += "<" + tag_name_;
    
    // 属性
    for (const auto& [name, value] : attributes_) {
        result += " " + name + "=\"" + value + "\"";
    }
    
    result += ">";
    
    // 子节点 (innerHTML)
    result += GetInnerHTML();
    
    // 结束标签
    result += "</" + tag_name_ + ">";
    
    return result;
}
```

**预期效果**: +1 个测试通过 (97.4%)

---

## 🏗️ 阶段 2: DOM 同步机制 (3-4 天)

### 目标
实现 MBink DOM ↔ Lexbor DOM 双向同步机制，解决 querySelector 问题

### 架构设计

```
┌─────────────────────────────────────────┐
│          MBink DOM (主 DOM)              │
│  - 所有 DOM 操作                         │
│  - 事件系统                              │
│  - 样式管理                              │
└─────────────────────────────────────────┘
              ↕ (双向同步)
┌─────────────────────────────────────────┐
│        Lexbor DOM (辅助 DOM)             │
│  - HTML 解析                             │
│  - CSS 选择器查询                        │
└─────────────────────────────────────────┘
```

### 任务清单

#### 任务 2.1: Element 类添加 Lexbor 引用 (0.5 天)

**文件**: `core/dom/element.h`, `core/dom/element.cpp`

**修改内容**:
```cpp
// core/dom/element.h
class Element : public Node {
private:
    lxb_dom_element_t* lexbor_element_ = nullptr;  // Lexbor 表示
    bool lexbor_dirty_ = false;                     // 是否需要同步
    
public:
    // 同步方法
    void SetLexborElement(lxb_dom_element_t* elem) { lexbor_element_ = elem; }
    lxb_dom_element_t* GetLexborElement() const { return lexbor_element_; }
    
    void SyncToLexbor();      // 同步到 Lexbor
    void UpdateLexbor();      // 更新 Lexbor 属性
    void MarkLexborDirty() { lexbor_dirty_ = true; }
    
    // 析构时清理 Lexbor 节点
    ~Element();
};
```

---

#### 任务 2.2: Document::CreateElement 同步创建 (0.5 天)

**文件**: `core/dom/document.cpp`

**修改内容**:
```cpp
// core/dom/document.cpp
std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    auto element = CreateElementByTagName(tag_name);
    
    // 同时创建 Lexbor 节点
    if (lexbor_doc_) {
        lxb_dom_document_t* lexbor_native = 
            lxb_dom_interface_document(lexbor_doc_->GetNativeDocument());
        
        lxb_dom_element_t* lexbor_elem = lxb_dom_document_create_element(
            lexbor_native,
            (const lxb_char_t*)tag_name.c_str(), 
            tag_name.length(),
            nullptr
        );
        
        if (lexbor_elem) {
            element->SetLexborElement(lexbor_elem);
        }
    }
    
    return element;
}
```

---

#### 任务 2.3: AppendChild/RemoveChild 同步 (1 天)

**文件**: `core/dom/node.cpp`, `core/dom/element.cpp`

**修改内容**:
```cpp
// core/dom/node.cpp
void Node::AppendChild(std::shared_ptr<Node> child) {
    // MBink DOM 操作
    child_nodes_.push_back(child);
    child->parent_node_ = weak_from_this();
    
    // 同步到 Lexbor
    auto this_elem = std::dynamic_pointer_cast<Element>(shared_from_this());
    auto child_elem = std::dynamic_pointer_cast<Element>(child);
    
    if (this_elem && child_elem) {
        lxb_dom_element_t* this_lexbor = this_elem->GetLexborElement();
        lxb_dom_element_t* child_lexbor = child_elem->GetLexborElement();
        
        if (this_lexbor && child_lexbor) {
            lxb_dom_node_insert_child(
                lxb_dom_interface_node(this_lexbor),
                lxb_dom_interface_node(child_lexbor)
            );
        }
    }
}

void Node::RemoveChild(std::shared_ptr<Node> child) {
    // MBink DOM 操作
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), child);
    if (it != child_nodes_.end()) {
        child_nodes_.erase(it);
        child->parent_node_.reset();
        
        // 同步到 Lexbor
        auto child_elem = std::dynamic_pointer_cast<Element>(child);
        if (child_elem) {
            lxb_dom_element_t* child_lexbor = child_elem->GetLexborElement();
            if (child_lexbor) {
                lxb_dom_node_remove(lxb_dom_interface_node(child_lexbor));
            }
        }
    }
}
```

---

#### 任务 2.4: SetAttribute 同步 (0.5 天)

**文件**: `core/dom/element.cpp`

**修改内容**:
```cpp
// core/dom/element.cpp
void Element::SetAttribute(const std::string& name, const std::string& value) {
    // MBink DOM 操作
    attributes_[name] = value;
    
    // 特殊属性处理
    if (name == "id") {
        SetId(value);
    } else if (name == "class") {
        SetClassName(value);
    }
    
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

---

#### 任务 2.5: QuerySelector 使用 Lexbor (0.5 天)

**文件**: `core/dom/element.cpp`, `core/dom/document.cpp`

**修改内容**:
```cpp
// core/dom/element.cpp
std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    // 确保 Lexbor 表示是最新的
    if (lexbor_dirty_) {
        SyncToLexbor();
        lexbor_dirty_ = false;
    }
    
    if (!lexbor_element_) {
        return nullptr;
    }
    
    // 使用 Lexbor 查询
    auto doc = GetOwnerDocument();
    if (!doc || !doc->GetLexborDocument()) {
        return nullptr;
    }
    
    LexborElement lexbor_wrapper(lexbor_element_, doc->GetLexborDocument());
    LexborElement* result = lexbor_wrapper.QuerySelector(selector);
    
    if (!result) {
        return nullptr;
    }
    
    // 在 MBink DOM 中查找对应的元素
    return FindMBinkElement(result->GetNativeElement());
}
```

---

#### 任务 2.6: 延迟同步优化 (0.5 天)

**目标**: 避免每次 DOM 操作都同步，只在需要查询时才同步

**策略**:
```cpp
// 在 DOM 操作时标记为 dirty
void Element::AppendChild(std::shared_ptr<Node> child) {
    Node::AppendChild(child);
    MarkLexborDirty();  // 标记需要同步
}

// 在查询时才真正同步
std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    if (lexbor_dirty_) {
        SyncToLexbor();  // 延迟同步
        lexbor_dirty_ = false;
    }
    // ... 查询逻辑
}
```

---

#### 任务 2.7: 测试和调试 (1 天)

**测试内容**:
1. 动态创建元素后使用 querySelector
2. 修改属性后使用 querySelector
3. 添加/删除子节点后使用 querySelector
4. 性能测试 - 确保同步不影响性能

**预期效果**: +10 个测试通过 (99.1%)

---

## 🎨 阶段 3: 性能优化和测试 (1 天)

### 任务清单

#### 任务 3.1: 性能优化 (0.5 天)
- 批量 DOM 操作优化
- 选择器缓存
- 延迟同步优化

#### 任务 3.2: 全面测试 (0.5 天)
- 运行所有测试
- 修复边界情况
- 性能基准测试

---

## 📊 预期成果

### 测试通过率提升
| 阶段 | 通过率 | 提升 | 失败测试 |
|------|--------|------|---------|
| 当前 | 90.5% | - | 11 个 |
| 阶段 0 | 92.2% | +1.7% | 9 个 |
| 阶段 1 | 97.4% | +5.2% | 3 个 |
| 阶段 2 | 99.1% | +1.7% | 1 个 |
| 阶段 3 | **99.1%** | 0% | **1 个** |

### 各模块最终通过率
| 模块 | 当前 | 最终 | 提升 |
|------|------|------|------|
| DOM 操作 | 94.7% | **100%** | +5.3% |
| 属性和样式 | 100% | **100%** | 0% |
| 事件系统 | 100% | **100%** | 0% |
| 查询选择器 | 50% | **95%** | +45% |
| 定时器 | 100% | **100%** | 0% |
| 表单元素 | 100% | **100%** | 0% |
| HTML 内容 | 41.2% | **94.1%** | +52.9% |

---

## 📅 时间表

| 日期 | 阶段 | 任务 | 状态 |
|------|------|------|------|
| Day 1 上午 | 阶段 0 | 快速修复 | ⏳ 待开始 |
| Day 1 下午 - Day 2 | 阶段 1 | innerHTML/outerHTML | ⏳ 待开始 |
| Day 3 - Day 6 | 阶段 2 | DOM 同步机制 | ⏳ 待开始 |
| Day 7 | 阶段 3 | 优化和测试 | ⏳ 待开始 |

**总工作量**: 5.5 - 7.5 天

---

## ✅ 验收标准

### 必须达成
- ✅ 测试通过率 ≥ 99%
- ✅ 所有核心 API 测试通过
- ✅ querySelector 支持动态创建的元素
- ✅ innerHTML/outerHTML 正常工作
- ✅ 无崩溃和严重 bug

### 可选目标
- 🎯 创建 Preact Hello World 示例
- 🎯 运行 Preact 示例验证
- 🎯 性能基准测试
- 🎯 完善文档和示例

---

**创建者**: MBink Team  
**最后更新**: 2025-11-15

