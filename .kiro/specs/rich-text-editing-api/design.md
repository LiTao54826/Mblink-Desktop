# 富文本编辑 API 设计文档

## 概述

本设计文档描述了 MBink 中富文本编辑相关 API 的实现方案，包括 Selection API、Range API、MutationObserver 和 contenteditable 属性支持。这些 API 是支持第三方代码编辑器（如 CodeMirror）和富文本编辑功能的基础。

## 架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           JavaScript 层                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐  ┌───────────┐ │
│  │  Selection  │  │    Range    │  │ MutationObserver │  │ Clipboard │ │
│  └──────┬──────┘  └──────┬──────┘  └────────┬─────────┘  └─────┬─────┘ │
└─────────┼────────────────┼──────────────────┼──────────────────┼───────┘
          │                │                  │                  │
┌─────────┼────────────────┼──────────────────┼──────────────────┼───────┐
│         │      QuickJS Bindings 层          │                  │       │
│  ┌──────┴──────┐  ┌──────┴──────┐  ┌────────┴─────────┐  ┌─────┴─────┐ │
│  │js_selection │  │  js_range   │  │js_mutation_obs   │  │js_clipboard│ │
│  └──────┬──────┘  └──────┬──────┘  └────────┬─────────┘  └─────┬─────┘ │
└─────────┼────────────────┼──────────────────┼──────────────────┼───────┘
          │                │                  │                  │
┌─────────┼────────────────┼──────────────────┼──────────────────┼───────┐
│         │           C++ 核心层              │                  │       │
│  ┌──────┴──────┐  ┌──────┴──────┐  ┌────────┴─────────┐  ┌─────┴─────┐ │
│  │SelectionMgr │  │    Range    │  │MutationObserver  │  │ClipboardMgr│ │
│  └──────┬──────┘  └──────┬──────┘  └────────┬─────────┘  └─────┬─────┘ │
│         │                │                  │                  │       │
│         └────────────────┴──────────────────┴──────────────────┘       │
│                                    │                                    │
│                          ┌─────────┴─────────┐                         │
│                          │   DOM / Document  │                         │
│                          └───────────────────┘                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 组件和接口

### 1. Range 类

Range 表示文档中的一个片段，包含起始和结束边界。

```cpp
// core/dom/range.h
namespace lightui {

class Range {
public:
    Range(std::shared_ptr<Document> owner_document);
    ~Range();

    // 边界设置
    void SetStart(std::shared_ptr<Node> node, int offset);
    void SetEnd(std::shared_ptr<Node> node, int offset);
    void SetStartBefore(std::shared_ptr<Node> node);
    void SetStartAfter(std::shared_ptr<Node> node);
    void SetEndBefore(std::shared_ptr<Node> node);
    void SetEndAfter(std::shared_ptr<Node> node);

    // 选择操作
    void SelectNode(std::shared_ptr<Node> node);
    void SelectNodeContents(std::shared_ptr<Node> node);
    void Collapse(bool to_start = true);

    // 属性访问
    std::shared_ptr<Node> GetStartContainer() const { return start_container_.lock(); }
    std::shared_ptr<Node> GetEndContainer() const { return end_container_.lock(); }
    int GetStartOffset() const { return start_offset_; }
    int GetEndOffset() const { return end_offset_; }
    bool IsCollapsed() const;
    std::shared_ptr<Node> GetCommonAncestorContainer() const;

    // 克隆和转换
    std::shared_ptr<Range> CloneRange() const;
    std::string ToString() const;

private:
    std::weak_ptr<Document> owner_document_;
    std::weak_ptr<Node> start_container_;
    std::weak_ptr<Node> end_container_;
    int start_offset_ = 0;
    int end_offset_ = 0;

    // 辅助方法
    int GetNodeIndex(std::shared_ptr<Node> node) const;
    std::shared_ptr<Node> FindCommonAncestor(
        std::shared_ptr<Node> node1, 
        std::shared_ptr<Node> node2) const;
};

} // namespace lightui
```

### 2. Selection 类

Selection 管理用户的文本选择状态。

```cpp
// core/dom/selection.h
namespace lightui {

class Selection {
public:
    Selection(std::shared_ptr<Document> document);
    ~Selection();

    // 锚点和焦点
    std::shared_ptr<Node> GetAnchorNode() const { return anchor_node_.lock(); }
    std::shared_ptr<Node> GetFocusNode() const { return focus_node_.lock(); }
    int GetAnchorOffset() const { return anchor_offset_; }
    int GetFocusOffset() const { return focus_offset_; }
    bool IsCollapsed() const;
    int GetRangeCount() const { return ranges_.empty() ? 0 : 1; }

    // 选择操作
    void Collapse(std::shared_ptr<Node> node, int offset);
    void Extend(std::shared_ptr<Node> node, int offset);
    void SelectAllChildren(std::shared_ptr<Node> node);
    void RemoveAllRanges();
    void AddRange(std::shared_ptr<Range> range);
    std::shared_ptr<Range> GetRangeAt(int index) const;

    // 转换
    std::string ToString() const;

    // 内部更新（由事件系统调用）
    void UpdateFromUserAction(
        std::shared_ptr<Node> anchor, int anchor_offset,
        std::shared_ptr<Node> focus, int focus_offset);

private:
    std::weak_ptr<Document> document_;
    std::weak_ptr<Node> anchor_node_;
    std::weak_ptr<Node> focus_node_;
    int anchor_offset_ = 0;
    int focus_offset_ = 0;
    std::vector<std::shared_ptr<Range>> ranges_;

    void UpdateRangeFromSelection();
};

} // namespace lightui
```

### 3. SelectionManager 类

管理选择状态和光标渲染。

```cpp
// core/event/selection_manager.h
namespace lightui {

class SelectionManager {
public:
    SelectionManager();
    ~SelectionManager();

    // 获取当前选择
    std::shared_ptr<Selection> GetSelection(std::shared_ptr<Document> document);

    // 处理用户输入
    void HandleMouseDown(std::shared_ptr<Element> target, int x, int y);
    void HandleMouseMove(std::shared_ptr<Element> target, int x, int y, bool is_dragging);
    void HandleMouseUp(std::shared_ptr<Element> target, int x, int y);

    // 键盘选择
    void HandleShiftArrow(std::shared_ptr<Document> document, 
                          const std::string& direction);

    // 光标位置计算
    struct CaretPosition {
        std::shared_ptr<Node> node;
        int offset;
        float x, y;  // 屏幕坐标
    };
    CaretPosition HitTestToCaretPosition(
        std::shared_ptr<Element> element, int x, int y);

    // 光标渲染
    void RenderCaret(SkCanvas* canvas, std::shared_ptr<Document> document);

private:
    std::map<Document*, std::shared_ptr<Selection>> selections_;
    bool is_selecting_ = false;
    CaretPosition selection_start_;
};

} // namespace lightui
```

### 4. MutationObserver 类

监听 DOM 变化。

```cpp
// core/dom/mutation_observer.h
namespace lightui {

// 变化记录
struct MutationRecord {
    std::string type;  // "childList", "attributes", "characterData"
    std::weak_ptr<Node> target;
    std::vector<std::shared_ptr<Node>> added_nodes;
    std::vector<std::shared_ptr<Node>> removed_nodes;
    std::weak_ptr<Node> previous_sibling;
    std::weak_ptr<Node> next_sibling;
    std::string attribute_name;
    std::string attribute_namespace;
    std::string old_value;
};

// 观察选项
struct MutationObserverInit {
    bool child_list = false;
    bool attributes = false;
    bool character_data = false;
    bool subtree = false;
    bool attribute_old_value = false;
    bool character_data_old_value = false;
    std::vector<std::string> attribute_filter;
};

class MutationObserver : public DOMObserver {
public:
    using Callback = std::function<void(
        const std::vector<MutationRecord>&, 
        MutationObserver*)>;

    MutationObserver(Callback callback);
    ~MutationObserver();

    // 观察控制
    void Observe(std::shared_ptr<Node> target, const MutationObserverInit& options);
    void Disconnect();
    std::vector<MutationRecord> TakeRecords();

    // DOMObserver 接口实现
    void OnNodeAdded(Node* node, Node* parent) override;
    void OnNodeRemoved(Node* node, Node* parent) override;
    void OnAttributeChanged(Element* element, 
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override;
    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override;

private:
    Callback callback_;
    std::vector<MutationRecord> pending_records_;
    
    struct ObservationTarget {
        std::weak_ptr<Node> target;
        MutationObserverInit options;
    };
    std::vector<ObservationTarget> targets_;

    bool is_scheduled_ = false;
    void ScheduleCallback();
    void FlushRecords();
    bool ShouldObserve(Node* node, const std::string& type) const;
};

} // namespace lightui
```

### 5. ContentEditableHandler 类

处理可编辑元素的输入。

```cpp
// core/event/contenteditable_handler.h
namespace lightui {

class ContentEditableHandler {
public:
    ContentEditableHandler(SelectionManager* selection_manager);
    ~ContentEditableHandler();

    // 检查元素是否可编辑
    bool IsEditable(std::shared_ptr<Element> element) const;
    bool IsContentEditable(std::shared_ptr<Element> element) const;

    // 输入处理
    void HandleTextInput(std::shared_ptr<Element> target, const std::string& text);
    void HandleKeyDown(std::shared_ptr<Element> target, const KeyEvent& event);
    
    // 编辑操作
    void InsertText(std::shared_ptr<Document> document, const std::string& text);
    void DeleteSelection(std::shared_ptr<Document> document);
    void DeleteCharacter(std::shared_ptr<Document> document, bool forward);
    void InsertLineBreak(std::shared_ptr<Document> document);

    // execCommand 支持
    bool ExecCommand(std::shared_ptr<Document> document, 
                    const std::string& command,
                    const std::string& value = "");
    bool QueryCommandState(std::shared_ptr<Document> document,
                          const std::string& command);
    bool QueryCommandEnabled(std::shared_ptr<Document> document,
                            const std::string& command);

private:
    SelectionManager* selection_manager_;

    // 事件分发
    bool DispatchBeforeInputEvent(std::shared_ptr<Element> target,
                                  const std::string& input_type,
                                  const std::string& data);
    void DispatchInputEvent(std::shared_ptr<Element> target,
                           const std::string& input_type,
                           const std::string& data);

    // 格式化命令
    void ApplyBold(std::shared_ptr<Document> document);
    void ApplyItalic(std::shared_ptr<Document> document);
    void ApplyUnderline(std::shared_ptr<Document> document);
};

} // namespace lightui
```

### 6. ClipboardManager 类

管理剪贴板操作。

```cpp
// core/event/clipboard_manager.h
namespace lightui {

class ClipboardManager {
public:
    ClipboardManager();
    ~ClipboardManager();

    // 剪贴板操作
    bool Copy(std::shared_ptr<Document> document);
    bool Cut(std::shared_ptr<Document> document);
    bool Paste(std::shared_ptr<Document> document);

    // 直接访问
    std::string GetText() const;
    void SetText(const std::string& text);

    // 事件处理
    void HandleCopyEvent(std::shared_ptr<Element> target);
    void HandleCutEvent(std::shared_ptr<Element> target);
    void HandlePasteEvent(std::shared_ptr<Element> target);

private:
    SelectionManager* selection_manager_;
    ContentEditableHandler* editable_handler_;

    // 事件分发
    bool DispatchClipboardEvent(std::shared_ptr<Element> target,
                               const std::string& type);
};

} // namespace lightui
```

## 数据模型

### Range 边界表示

```
文档结构:
<div>Hello <b>World</b>!</div>

节点树:
div
├── Text("Hello ")
├── b
│   └── Text("World")
└── Text("!")

Range 示例 (选中 "lo Wor"):
- startContainer: Text("Hello ")
- startOffset: 3
- endContainer: Text("World")
- endOffset: 3
```

### Selection 状态

```
Selection 状态:
┌─────────────────────────────────────┐
│  anchorNode: Text("Hello ")         │
│  anchorOffset: 3                    │
│  focusNode: Text("World")           │
│  focusOffset: 3                     │
│  isCollapsed: false                 │
│  rangeCount: 1                      │
└─────────────────────────────────────┘
```

### MutationRecord 结构

```
MutationRecord 示例:
{
  type: "childList",
  target: <div>,
  addedNodes: [<span>],
  removedNodes: [],
  previousSibling: Text("Hello"),
  nextSibling: null,
  attributeName: null,
  oldValue: null
}
```


## 正确性属性 (Correctness Properties)

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Selection 状态一致性
*For any* Selection 对象，其 anchorNode/anchorOffset 和 focusNode/focusOffset 属性应始终反映当前选择的实际起始和结束位置，且 isCollapsed 属性应在 anchor 和 focus 位置相同时返回 true。
**Validates: Requirements 1.2, 1.3, 1.4, 1.5, 1.6**

### Property 2: Selection collapse 操作正确性
*For any* Selection 对象和有效的 (node, offset) 位置，调用 collapse(node, offset) 后，Selection 应处于折叠状态（isCollapsed=true），且 anchorNode/focusNode 都指向指定节点，anchorOffset/focusOffset 都等于指定偏移量。
**Validates: Requirements 1.7**

### Property 3: Selection extend 操作保持锚点
*For any* Selection 对象，调用 extend(node, offset) 后，anchorNode 和 anchorOffset 应保持不变，而 focusNode 和 focusOffset 应更新为指定值。
**Validates: Requirements 1.8**

### Property 4: Selection toString 返回选中文本
*For any* Selection 对象，toString() 应返回从 anchor 到 focus 之间的所有文本内容的字符串表示。
**Validates: Requirements 1.13**

### Property 5: Range 边界一致性
*For any* Range 对象，设置 start/end 边界后，对应的 startContainer/startOffset 和 endContainer/endOffset 属性应返回设置的值，且 collapsed 属性应在 start 和 end 位置相同时返回 true。
**Validates: Requirements 2.2, 2.3, 2.14, 2.15, 2.16, 2.17, 2.18**

### Property 6: Range 相对定位正确性
*For any* Range 对象和目标节点，setStartBefore/setStartAfter/setEndBefore/setEndAfter 应将边界设置到节点的正确相对位置（父节点中的索引位置）。
**Validates: Requirements 2.4, 2.5, 2.6, 2.7**

### Property 7: Range selectNode 完整选择
*For any* Range 对象和目标节点，selectNode(node) 后，Range 应完整包含该节点（startContainer 和 endContainer 为父节点，offset 为节点在父节点中的索引和索引+1）。
**Validates: Requirements 2.8**

### Property 8: Range selectNodeContents 内容选择
*For any* Range 对象和目标节点，selectNodeContents(node) 后，Range 应选中节点的所有内容（startContainer=endContainer=node，startOffset=0，endOffset=子节点数或文本长度）。
**Validates: Requirements 2.9**

### Property 9: Range collapse 位置正确性
*For any* Range 对象，collapse(true) 应将 end 移动到 start 位置，collapse(false) 应将 start 移动到 end 位置，结果 Range 应处于折叠状态。
**Validates: Requirements 2.10**

### Property 10: Range cloneRange 独立副本
*For any* Range 对象，cloneRange() 应返回一个具有相同边界的新 Range 对象，且修改原 Range 不应影响克隆的 Range。
**Validates: Requirements 2.11**

### Property 11: Range toString 文本提取
*For any* Range 对象，toString() 应返回 Range 边界内的所有文本内容。
**Validates: Requirements 2.12**

### Property 12: Range commonAncestorContainer 正确性
*For any* Range 对象，commonAncestorContainer 应返回同时包含 startContainer 和 endContainer 的最深节点。
**Validates: Requirements 2.13**

### Property 13: MutationObserver 过滤正确性
*For any* MutationObserver，当 observe() 指定特定选项时，只有匹配选项的变化才会被报告：childList 报告子节点变化，attributes 报告属性变化，characterData 报告文本变化，attributeFilter 限制报告的属性名称。
**Validates: Requirements 3.3, 3.4, 3.5, 3.9**

### Property 14: MutationObserver subtree 递归观察
*For any* MutationObserver，当 subtree=true 时，目标节点的所有后代节点的变化都应被报告。
**Validates: Requirements 3.6**

### Property 15: MutationObserver oldValue 记录
*For any* MutationObserver，当 attributeOldValue=true 或 characterDataOldValue=true 时，MutationRecord 的 oldValue 应包含变化前的值。
**Validates: Requirements 3.7, 3.8**

### Property 16: MutationObserver disconnect 停止观察
*For any* MutationObserver，调用 disconnect() 后，不应再报告任何变化。
**Validates: Requirements 3.11**

### Property 17: MutationObserver takeRecords 清空队列
*For any* MutationObserver，takeRecords() 应返回所有待处理的 MutationRecord 并清空队列，后续回调不应包含这些记录。
**Validates: Requirements 3.12**

### Property 18: contenteditable 继承正确性
*For any* 元素，isContentEditable 应正确反映元素的可编辑状态：contenteditable="true" 返回 true，contenteditable="false" 返回 false，无属性时继承父元素状态。
**Validates: Requirements 4.1, 4.2, 4.3, 4.13**

### Property 19: 文本插入位置正确性
*For any* contenteditable 元素和光标位置，插入文本后，文本应出现在光标位置，光标应移动到插入文本之后。
**Validates: Requirements 4.5**

### Property 20: Backspace 删除正确性
*For any* contenteditable 元素，Backspace 应删除选中文本（如有）或光标前一个字符，光标位置应相应更新。
**Validates: Requirements 4.6**

### Property 21: Delete 删除正确性
*For any* contenteditable 元素，Delete 应删除选中文本（如有）或光标后一个字符，光标位置应保持不变。
**Validates: Requirements 4.7**

### Property 22: beforeinput 事件可取消性
*For any* contenteditable 元素的输入操作，如果 beforeinput 事件被取消（preventDefault），则输入操作不应执行，DOM 不应改变。
**Validates: Requirements 6.1, 6.3**

### Property 23: input 事件数据正确性
*For any* contenteditable 元素的输入操作，input 事件的 inputType 应正确反映操作类型，data 属性应包含插入的文本（如适用）。
**Validates: Requirements 6.2, 6.4, 6.5**

### Property 24: execCommand 格式切换
*For any* Selection 和格式命令（bold/italic/underline），execCommand 应切换选中文本的格式状态，queryCommandState 应返回当前格式状态。
**Validates: Requirements 7.1, 7.2, 7.3, 7.7**

### Property 25: execCommand insertText 正确性
*For any* Selection 和文本，execCommand('insertText', false, text) 应在光标位置插入指定文本。
**Validates: Requirements 7.4**

### Property 26: Clipboard 事件结构
*For any* 剪贴板操作，copy/cut/paste 事件应是可取消的，并提供对剪贴板数据的访问。
**Validates: Requirements 5.5, 5.6, 5.7**

## 错误处理

### Range 错误处理

| 错误场景 | 处理方式 |
|---------|---------|
| setStart/setEnd 的 offset 超出范围 | 抛出 IndexSizeError DOMException |
| setStart/setEnd 的 node 为 null | 抛出 TypeError |
| setStartBefore/After 的 node 无父节点 | 抛出 InvalidNodeTypeError DOMException |
| Range 边界指向已删除的节点 | Range 自动调整或失效 |

### Selection 错误处理

| 错误场景 | 处理方式 |
|---------|---------|
| collapse 的 node 为 null | 清除选择 |
| getRangeAt 的 index 超出范围 | 抛出 IndexSizeError DOMException |
| addRange 的 range 为 null | 忽略操作 |
| 操作不可编辑区域 | 操作被忽略 |

### MutationObserver 错误处理

| 错误场景 | 处理方式 |
|---------|---------|
| observe 的 target 为 null | 抛出 TypeError |
| observe 的 options 无效（全为 false） | 抛出 TypeError |
| 回调函数抛出异常 | 捕获异常，继续处理其他观察者 |

### ContentEditable 错误处理

| 错误场景 | 处理方式 |
|---------|---------|
| 在非可编辑元素中输入 | 忽略输入 |
| execCommand 在无焦点时调用 | 返回 false |
| 无效的 execCommand 命令 | 返回 false |

## 测试策略

### 单元测试

使用 Google Test 框架进行 C++ 单元测试：

1. **Range 类测试**
   - 边界设置和获取
   - 相对定位方法
   - collapse 和 cloneRange
   - toString 文本提取

2. **Selection 类测试**
   - 属性访问
   - collapse/extend 操作
   - Range 管理

3. **MutationObserver 测试**
   - 各种观察选项
   - 回调触发时机
   - disconnect 和 takeRecords

4. **ContentEditableHandler 测试**
   - 可编辑性判断
   - 文本插入和删除
   - execCommand 实现

### 属性测试 (Property-Based Testing)

使用 RapidCheck 库进行属性测试：

```cpp
// 示例：Range 边界一致性测试
// **Feature: rich-text-editing-api, Property 5: Range 边界一致性**
RC_GTEST_PROP(RangeTest, BoundaryConsistency, 
              (std::shared_ptr<Node> node, int offset)) {
    auto range = document->CreateRange();
    range->SetStart(node, offset);
    
    RC_ASSERT(range->GetStartContainer() == node);
    RC_ASSERT(range->GetStartOffset() == offset);
}

// 示例：Selection collapse 测试
// **Feature: rich-text-editing-api, Property 2: Selection collapse 操作正确性**
RC_GTEST_PROP(SelectionTest, CollapseCorrectness,
              (std::shared_ptr<Node> node, int offset)) {
    auto selection = document->GetSelection();
    selection->Collapse(node, offset);
    
    RC_ASSERT(selection->IsCollapsed());
    RC_ASSERT(selection->GetAnchorNode() == node);
    RC_ASSERT(selection->GetFocusNode() == node);
    RC_ASSERT(selection->GetAnchorOffset() == offset);
    RC_ASSERT(selection->GetFocusOffset() == offset);
}

// 示例：Range cloneRange 独立性测试
// **Feature: rich-text-editing-api, Property 10: Range cloneRange 独立副本**
RC_GTEST_PROP(RangeTest, CloneIndependence,
              (std::shared_ptr<Node> node1, int offset1,
               std::shared_ptr<Node> node2, int offset2)) {
    auto range = document->CreateRange();
    range->SetStart(node1, offset1);
    range->SetEnd(node1, offset1 + 5);
    
    auto cloned = range->CloneRange();
    
    // 修改原 Range
    range->SetStart(node2, offset2);
    
    // 克隆的 Range 应保持不变
    RC_ASSERT(cloned->GetStartContainer() == node1);
    RC_ASSERT(cloned->GetStartOffset() == offset1);
}
```

### 集成测试

1. **JavaScript API 测试**
   - 通过 QuickJS 绑定测试完整 API
   - 验证 JavaScript 和 C++ 层的交互

2. **事件流测试**
   - 测试 beforeinput/input 事件序列
   - 测试剪贴板事件流程

3. **渲染测试**
   - 测试光标渲染
   - 测试选择高亮渲染

### 测试配置

- 属性测试最少运行 100 次迭代
- 每个属性测试必须标注对应的正确性属性编号
- 测试标注格式：`**Feature: rich-text-editing-api, Property {number}: {property_text}**`
