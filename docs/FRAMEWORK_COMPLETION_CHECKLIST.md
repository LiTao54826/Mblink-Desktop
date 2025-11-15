# MBink 框架完善任务清单

**创建日期**: 2025-11-15  
**目标**: 修复所有已知问题，达到 99%+ 测试通过率

---

## 📋 阶段 0: 快速修复 (0.5 天)

### ✅ 任务 0.1: 修复 outerHTML setter 崩溃
- [ ] 查看 `core/dom/element.cpp` 中的 `Element::SetOuterHTML()` 实现
- [ ] 定位 "Cannot set outerHTML without document" 错误位置
- [ ] 修改逻辑: 尝试从父节点获取 document
- [ ] 改为返回而不是抛出异常
- [ ] 编译测试: `cmake --build build --config Release`
- [ ] 运行测试: 验证 outerHTML setter 不再崩溃
- [ ] 预期: +1 个测试通过 (91.4%)

**代码修改**:
```cpp
// core/dom/element.cpp - Element::SetOuterHTML()
void Element::SetOuterHTML(const std::string& html) {
    auto doc = GetOwnerDocument();
    if (!doc) {
        auto parent = GetParentNode();
        if (parent) {
            doc = parent->GetOwnerDocument();
        }
    }
    
    if (!doc) {
        LOG_ERROR("Cannot set outerHTML without document");
        return;  // 改为 return
    }
    // ... 其余逻辑
}
```

---

### ✅ 任务 0.2: 修复 cloneNode 深克隆 bug
- [ ] 查看 `core/dom/element.cpp` 或 `core/dom/node.cpp` 中的 `CloneNode()` 实现
- [ ] 检查递归克隆逻辑
- [ ] 确保遍历所有子节点 (使用 `child_nodes_`)
- [ ] 编译测试
- [ ] 运行测试: 验证深克隆包含所有子节点
- [ ] 预期: +1 个测试通过 (92.2%)

**代码修改**:
```cpp
// core/dom/element.cpp - Element::CloneNode()
std::shared_ptr<Node> Element::CloneNode(bool deep) {
    auto clone = std::make_shared<Element>(tag_name_);
    clone->attributes_ = attributes_;
    
    if (deep) {
        for (const auto& child : child_nodes_) {  // 确保遍历所有
            auto child_clone = child->CloneNode(true);
            clone->AppendChild(child_clone);
        }
    }
    
    return clone;
}
```

---

## 📋 阶段 1: innerHTML/outerHTML 修复 (1-2 天)

### ✅ 任务 1.1: 修复 innerHTML setter JavaScript 错误
- [ ] 在 `Element::SetInnerHTML()` 中添加调试日志
- [ ] 检查 Lexbor 解析是否成功
- [ ] 添加节点类型检查 (ELEMENT_NODE, TEXT_NODE)
- [ ] 检查 `ConvertLexborNodeToNode()` 转换逻辑
- [ ] 处理所有可能的节点类型
- [ ] 编译测试
- [ ] 运行测试: 验证 innerHTML setter 不再抛出 JavaScript 错误
- [ ] 预期: +3 个测试通过 (94.8%)

**关键修改点**:
- 文件: `core/dom/element.cpp`
- 函数: `Element::SetInnerHTML()`
- 添加类型检查和错误处理

---

### ✅ 任务 1.2: 修复 innerHTML getter 序列化问题
- [ ] 查看 `Element::GetInnerHTML()` 实现
- [ ] 确保遍历所有子节点
- [ ] 正确序列化元素节点 (调用 `GetOuterHTML()`)
- [ ] 正确序列化文本节点 (调用 `GetData()`)
- [ ] 测试嵌套元素的序列化
- [ ] 编译测试
- [ ] 运行测试: 验证 innerHTML getter 返回正确的 HTML
- [ ] 预期: +2 个测试通过 (96.5%)

**代码修改**:
```cpp
// core/dom/element.cpp - Element::GetInnerHTML()
std::string Element::GetInnerHTML() const {
    std::string result;
    for (const auto& child : child_nodes_) {
        if (auto elem = std::dynamic_pointer_cast<Element>(child)) {
            result += elem->GetOuterHTML();
        } else if (auto text = std::dynamic_pointer_cast<Text>(child)) {
            result += text->GetData();
        }
    }
    return result;
}
```

---

### ✅ 任务 1.3: 修复 outerHTML getter 序列化问题
- [ ] 查看 `Element::GetOuterHTML()` 实现
- [ ] 确保正确序列化开始标签
- [ ] 确保正确序列化属性
- [ ] 确保正确序列化子节点 (调用 `GetInnerHTML()`)
- [ ] 确保正确序列化结束标签
- [ ] 处理自闭合标签 (如 `<img>`, `<br>`)
- [ ] 编译测试
- [ ] 运行测试: 验证 outerHTML getter 返回正确的 HTML
- [ ] 预期: +1 个测试通过 (97.4%)

**代码修改**:
```cpp
// core/dom/element.cpp - Element::GetOuterHTML()
std::string Element::GetOuterHTML() const {
    std::string result = "<" + tag_name_;
    
    for (const auto& [name, value] : attributes_) {
        result += " " + name + "=\"" + value + "\"";
    }
    
    result += ">" + GetInnerHTML() + "</" + tag_name_ + ">";
    return result;
}
```

---

## 📋 阶段 2: DOM 同步机制 (3-4 天)

### ✅ 任务 2.1: Element 类添加 Lexbor 引用
- [ ] 修改 `core/dom/element.h`
  - [ ] 添加 `lxb_dom_element_t* lexbor_element_` 成员
  - [ ] 添加 `bool lexbor_dirty_` 标志
  - [ ] 添加 `SetLexborElement()` 方法
  - [ ] 添加 `GetLexborElement()` 方法
  - [ ] 添加 `SyncToLexbor()` 方法
  - [ ] 添加 `MarkLexborDirty()` 方法
- [ ] 修改 `core/dom/element.cpp`
  - [ ] 实现 `SyncToLexbor()` 方法
  - [ ] 在析构函数中清理 Lexbor 节点
- [ ] 编译测试
- [ ] 预期: 编译通过，无功能变化

---

### ✅ 任务 2.2: Document::CreateElement 同步创建
- [ ] 修改 `core/dom/document.cpp` 中的 `CreateElement()`
- [ ] 在创建 MBink Element 后，同时创建 Lexbor 节点
- [ ] 使用 `lxb_dom_document_create_element()` 创建
- [ ] 调用 `element->SetLexborElement()` 关联
- [ ] 编译测试
- [ ] 运行测试: 验证动态创建的元素有 Lexbor 表示
- [ ] 预期: 编译通过，为后续 querySelector 做准备

**代码修改**:
```cpp
// core/dom/document.cpp
std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    auto element = CreateElementByTagName(tag_name);
    
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

### ✅ 任务 2.3: AppendChild 同步
- [ ] 修改 `core/dom/node.cpp` 中的 `AppendChild()`
- [ ] 在 MBink DOM 操作后，同步到 Lexbor
- [ ] 使用 `lxb_dom_node_insert_child()` 插入
- [ ] 处理文本节点的同步
- [ ] 编译测试
- [ ] 运行测试: 验证 appendChild 后 Lexbor DOM 也更新
- [ ] 预期: 编译通过，DOM 树同步正常

---

### ✅ 任务 2.4: RemoveChild 同步
- [ ] 修改 `core/dom/node.cpp` 中的 `RemoveChild()`
- [ ] 在 MBink DOM 操作后，同步到 Lexbor
- [ ] 使用 `lxb_dom_node_remove()` 移除
- [ ] 编译测试
- [ ] 运行测试: 验证 removeChild 后 Lexbor DOM 也更新
- [ ] 预期: 编译通过，DOM 树同步正常

---

### ✅ 任务 2.5: SetAttribute 同步
- [ ] 修改 `core/dom/element.cpp` 中的 `SetAttribute()`
- [ ] 在 MBink DOM 操作后，同步到 Lexbor
- [ ] 使用 `lxb_dom_element_set_attribute()` 设置
- [ ] 编译测试
- [ ] 运行测试: 验证 setAttribute 后 Lexbor DOM 也更新
- [ ] 预期: 编译通过，属性同步正常

---

### ✅ 任务 2.6: QuerySelector 使用 Lexbor
- [ ] 修改 `core/dom/element.cpp` 中的 `QuerySelector()`
- [ ] 在查询前检查 `lexbor_dirty_` 标志
- [ ] 如果 dirty，调用 `SyncToLexbor()`
- [ ] 使用 Lexbor 的 CSS 选择器引擎查询
- [ ] 将 Lexbor 结果转换回 MBink Element
- [ ] 编译测试
- [ ] 运行测试: 验证动态元素可以被 querySelector 查询
- [ ] 预期: +10 个测试通过 (99.1%)

**代码修改**:
```cpp
// core/dom/element.cpp
std::shared_ptr<Element> Element::QuerySelector(const std::string& selector) {
    if (lexbor_dirty_) {
        SyncToLexbor();
        lexbor_dirty_ = false;
    }
    
    if (!lexbor_element_) {
        return nullptr;
    }
    
    // 使用 Lexbor 查询
    auto doc = GetOwnerDocument();
    LexborElement lexbor_wrapper(lexbor_element_, doc->GetLexborDocument());
    LexborElement* result = lexbor_wrapper.QuerySelector(selector);
    
    if (!result) {
        return nullptr;
    }
    
    return FindMBinkElement(result->GetNativeElement());
}
```

---

### ✅ 任务 2.7: 延迟同步优化
- [ ] 在所有 DOM 修改操作中调用 `MarkLexborDirty()`
- [ ] 只在 querySelector 时才真正同步
- [ ] 添加性能日志，监控同步频率
- [ ] 编译测试
- [ ] 性能测试: 确保同步不影响性能
- [ ] 预期: 性能无明显下降

---

### ✅ 任务 2.8: 测试和调试
- [ ] 测试场景 1: 动态创建元素后 querySelector
- [ ] 测试场景 2: 修改属性后 querySelector
- [ ] 测试场景 3: 添加子节点后 querySelector
- [ ] 测试场景 4: 删除子节点后 querySelector
- [ ] 测试场景 5: 复杂选择器查询
- [ ] 修复发现的 bug
- [ ] 运行完整测试套件
- [ ] 预期: 所有 querySelector 测试通过

---

## 📋 阶段 3: 性能优化和测试 (1 天)

### ✅ 任务 3.1: 性能优化
- [ ] 批量 DOM 操作优化
  - [ ] 实现 DocumentFragment 支持
  - [ ] 批量插入时只同步一次
- [ ] 选择器缓存
  - [ ] 缓存常用选择器结果
  - [ ] 在 DOM 修改时清除缓存
- [ ] 延迟同步优化
  - [ ] 确保只在必要时同步
  - [ ] 添加性能监控
- [ ] 运行性能基准测试
- [ ] 预期: 性能无明显下降

---

### ✅ 任务 3.2: 全面测试
- [ ] 运行所有测试: `.\build\bin\Release\comprehensive_test_app.exe`
- [ ] 统计测试通过率
- [ ] 修复边界情况
- [ ] 测试内存泄漏
- [ ] 测试多线程安全性 (如果适用)
- [ ] 创建测试报告
- [ ] 预期: 99.1% 测试通过率

---

### ✅ 任务 3.3: 文档更新
- [ ] 更新 `KNOWN_ISSUES.md` - 标记已修复的问题
- [ ] 更新 `BINDINGS_COMPLETION_SUMMARY.md` - 更新最终通过率
- [ ] 创建 `FRAMEWORK_COMPLETION_REPORT.md` - 完成报告
- [ ] 更新 `README.md` - 更新项目状态
- [ ] 添加使用示例和最佳实践

---

## 📊 进度跟踪

### 总体进度
- [ ] 阶段 0: 快速修复 (0/2 任务完成)
- [ ] 阶段 1: innerHTML/outerHTML 修复 (0/3 任务完成)
- [ ] 阶段 2: DOM 同步机制 (0/8 任务完成)
- [ ] 阶段 3: 性能优化和测试 (0/3 任务完成)

**总计**: 0/16 任务完成

### 测试通过率进度
- [x] 当前: 90.5% (105/116)
- [ ] 阶段 0 完成: 92.2% (107/116)
- [ ] 阶段 1 完成: 97.4% (113/116)
- [ ] 阶段 2 完成: 99.1% (115/116)
- [ ] 最终目标: 99.1% (115/116)

---

## ✅ 验收标准

### 必须达成
- [ ] 测试通过率 ≥ 99%
- [ ] 所有核心 API 测试通过
- [ ] querySelector 支持动态创建的元素
- [ ] innerHTML/outerHTML 正常工作
- [ ] 无崩溃和严重 bug
- [ ] 性能无明显下降

### 可选目标
- [ ] 创建 Preact Hello World 示例
- [ ] 运行 Preact 示例验证
- [ ] 性能基准测试报告
- [ ] 完善文档和示例

---

## 📝 注意事项

1. **每完成一个任务，立即测试** - 不要积累问题
2. **提交 Git** - 每个阶段完成后提交代码
3. **记录问题** - 遇到问题及时记录到 KNOWN_ISSUES.md
4. **性能监控** - 确保修复不影响性能
5. **代码审查** - 重要修改需要仔细审查

---

**创建者**: MBink Team  
**最后更新**: 2025-11-15

