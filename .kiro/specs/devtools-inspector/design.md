# DevTools Inspector Design Document

## Overview

本设计文档描述 MBink 框架的开发者工具（DevTools Inspector）的技术架构和实现方案。DevTools 将作为内嵌面板集成到应用窗口中，提供类似 Chrome DevTools Elements 面板的功能，包括 DOM 树查看、样式分析、盒模型可视化和实时编辑能力。

### 设计目标

1. **轻量级**: 最小化对主应用性能的影响
2. **复用现有架构**: 利用现有的 DOM、渲染和事件系统
3. **实时同步**: DOM 变化实时反映到 Inspector 视图
4. **可扩展**: 为未来添加 Console、Network 等面板预留扩展点

## Architecture

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Window                          │
├─────────────────────────────────────┬───────────────────────────┤
│                                     │                           │
│         Main Application            │      DevTools Panel       │
│         (User Content)              │                           │
│                                     │  ┌─────────────────────┐  │
│                                     │  │   DOM Tree View     │  │
│                                     │  ├─────────────────────┤  │
│                                     │  │   Styles Panel      │  │
│                                     │  │   - Inline Styles   │  │
│                                     │  │   - Computed Styles │  │
│                                     │  │   - Box Model       │  │
│                                     │  └─────────────────────┘  │
│                                     │                           │
└─────────────────────────────────────┴───────────────────────────┘
```

### 模块架构

```
core/devtools/
├── devtools_manager.h/cpp        # DevTools 生命周期管理
├── devtools_panel.h/cpp          # 面板容器和布局
├── inspector/
│   ├── dom_tree_view.h/cpp       # DOM 树视图组件
│   ├── dom_tree_node.h/cpp       # 树节点渲染
│   ├── element_highlighter.h/cpp # 元素高亮覆盖层
│   └── element_picker.h/cpp      # 元素拾取工具
├── styles/
│   ├── styles_panel.h/cpp        # 样式面板容器
│   ├── inline_styles_view.h/cpp  # 内联样式视图
│   ├── computed_styles_view.h/cpp# 计算样式视图
│   └── box_model_view.h/cpp      # 盒模型可视化
├── editor/
│   ├── style_editor.h/cpp        # 样式编辑器
│   └── attribute_editor.h/cpp    # 属性编辑器
├── search/
│   └── element_search.h/cpp      # 元素搜索
└── serializer/
    └── dom_serializer.h/cpp      # DOM 序列化/反序列化
```

### 数据流

```
┌──────────────┐     DOM Changes      ┌──────────────────┐
│   Document   │ ──────────────────▶  │  DevToolsManager │
│   (DOM Tree) │                      │                  │
└──────────────┘                      └────────┬─────────┘
       ▲                                       │
       │                                       ▼
       │ Modifications              ┌──────────────────┐
       │                            │   DOM Observer   │
       │                            │  (MutationObserver)
       │                            └────────┬─────────┘
       │                                     │
       │                                     ▼
┌──────┴───────┐                   ┌──────────────────┐
│ StyleEditor  │ ◀──────────────── │  DOMTreeView     │
│ AttrEditor   │    User Edits     │  StylesPanel     │
└──────────────┘                   └──────────────────┘
```

## Components and Interfaces

### 1. DevToolsManager

DevTools 的核心管理类，负责生命周期和状态管理。

```cpp
namespace lightui {

class DevToolsManager {
public:
    // 单例访问
    static DevToolsManager& GetInstance();
    
    // 生命周期
    void Initialize(Document* document, Window* window);
    void Shutdown();
    
    // 面板控制
    void Toggle();
    void Open();
    void Close();
    bool IsOpen() const;
    
    // 停靠位置
    enum class DockPosition { Bottom, Right };
    void SetDockPosition(DockPosition position);
    DockPosition GetDockPosition() const;
    
    // 面板尺寸
    void SetPanelSize(float size);  // 百分比或像素
    float GetPanelSize() const;
    
    // 元素选择
    void SelectElement(std::shared_ptr<Element> element);
    std::shared_ptr<Element> GetSelectedElement() const;
    
    // 元素拾取模式
    void StartElementPicker();
    void StopElementPicker();
    bool IsPickerActive() const;
    
    // 渲染
    void Render(SkCanvas* canvas, const LayoutBox& bounds);
    
    // 事件处理
    bool HandleEvent(const Event& event);
    
    // 状态持久化
    void SaveState();
    void LoadState();

private:
    Document* document_ = nullptr;
    Window* window_ = nullptr;
    
    bool is_open_ = false;
    DockPosition dock_position_ = DockPosition::Bottom;
    float panel_size_ = 0.3f;  // 30% of window
    
    std::shared_ptr<Element> selected_element_;
    bool picker_active_ = false;
    
    std::unique_ptr<DevToolsPanel> panel_;
    std::unique_ptr<ElementHighlighter> highlighter_;
    std::unique_ptr<ElementPicker> picker_;
};

} // namespace lightui
```

### 2. DOMTreeView

DOM 树视图组件，负责渲染可展开/折叠的树结构。

```cpp
class DOMTreeView {
public:
    DOMTreeView(Document* document);
    
    // 树操作
    void SetRootNode(std::shared_ptr<Node> root);
    void Refresh();
    
    // 节点展开/折叠
    void ExpandNode(std::shared_ptr<Node> node);
    void CollapseNode(std::shared_ptr<Node> node);
    void ExpandAll();
    void CollapseAll();
    bool IsExpanded(std::shared_ptr<Node> node) const;
    
    // 选择
    void SelectNode(std::shared_ptr<Node> node);
    std::shared_ptr<Node> GetSelectedNode() const;
    
    // 滚动到可见
    void ScrollToNode(std::shared_ptr<Node> node);
    
    // 搜索高亮
    void SetSearchResults(const std::vector<std::shared_ptr<Node>>& results);
    void ClearSearchResults();
    
    // 渲染
    void Render(SkCanvas* canvas, const LayoutBox& bounds);
    
    // 事件
    bool HandleMouseEvent(const MouseEvent& event);
    bool HandleKeyboardEvent(const KeyboardEvent& event);
    
    // 回调
    using SelectionCallback = std::function<void(std::shared_ptr<Node>)>;
    void SetOnSelectionChanged(SelectionCallback callback);

private:
    struct TreeNodeState {
        bool expanded = false;
        float y_position = 0;
        float height = 0;
    };
    
    Document* document_;
    std::shared_ptr<Node> root_node_;
    std::shared_ptr<Node> selected_node_;
    std::shared_ptr<Node> hovered_node_;
    
    std::unordered_map<Node*, TreeNodeState> node_states_;
    std::vector<std::shared_ptr<Node>> search_results_;
    
    float scroll_offset_ = 0;
    float content_height_ = 0;
    
    SelectionCallback on_selection_changed_;
    
    // 渲染辅助
    void RenderNode(SkCanvas* canvas, std::shared_ptr<Node> node, 
                    float x, float& y, int depth);
    std::string FormatNodeLabel(std::shared_ptr<Node> node);
};
```

### 3. ElementHighlighter

元素高亮覆盖层，在主应用视图上绘制选中元素的边界。

```cpp
class ElementHighlighter {
public:
    ElementHighlighter();
    
    // 高亮控制
    void SetHighlightedElement(std::shared_ptr<Element> element);
    void SetHoveredElement(std::shared_ptr<Element> element);
    void ClearHighlight();
    void ClearHover();
    
    // 显示选项
    struct HighlightOptions {
        bool show_margin = true;
        bool show_border = true;
        bool show_padding = true;
        bool show_content = true;
        bool show_info_tooltip = true;
    };
    void SetOptions(const HighlightOptions& options);
    
    // 颜色配置
    void SetMarginColor(SkColor color);
    void SetBorderColor(SkColor color);
    void SetPaddingColor(SkColor color);
    void SetContentColor(SkColor color);
    
    // 渲染（在主应用内容之上）
    void Render(SkCanvas* canvas);

private:
    std::weak_ptr<Element> highlighted_element_;
    std::weak_ptr<Element> hovered_element_;
    HighlightOptions options_;
    
    SkColor margin_color_ = SkColorSetARGB(128, 255, 155, 0);   // Orange
    SkColor border_color_ = SkColorSetARGB(128, 255, 255, 0);   // Yellow
    SkColor padding_color_ = SkColorSetARGB(128, 144, 238, 144);// Light green
    SkColor content_color_ = SkColorSetARGB(128, 135, 206, 250);// Light blue
    
    void RenderBoxModel(SkCanvas* canvas, std::shared_ptr<Element> element,
                        SkColor color, bool is_hover);
    void RenderInfoTooltip(SkCanvas* canvas, std::shared_ptr<Element> element);
};
```

### 4. StylesPanel

样式面板，包含内联样式、计算样式和盒模型视图。

```cpp
class StylesPanel {
public:
    StylesPanel();
    
    // 设置目标元素
    void SetElement(std::shared_ptr<Element> element);
    
    // 子面板切换
    enum class Tab { InlineStyles, ComputedStyles, BoxModel };
    void SetActiveTab(Tab tab);
    Tab GetActiveTab() const;
    
    // 渲染
    void Render(SkCanvas* canvas, const LayoutBox& bounds);
    
    // 事件
    bool HandleEvent(const Event& event);
    
    // 样式编辑回调
    using StyleChangeCallback = std::function<void(
        const std::string& property, const std::string& value)>;
    void SetOnStyleChanged(StyleChangeCallback callback);

private:
    std::weak_ptr<Element> element_;
    Tab active_tab_ = Tab::InlineStyles;
    
    std::unique_ptr<InlineStylesView> inline_styles_view_;
    std::unique_ptr<ComputedStylesView> computed_styles_view_;
    std::unique_ptr<BoxModelView> box_model_view_;
    
    StyleChangeCallback on_style_changed_;
};
```

### 5. BoxModelView

盒模型可视化组件。

```cpp
class BoxModelView {
public:
    BoxModelView();
    
    // 设置元素
    void SetElement(std::shared_ptr<Element> element);
    
    // 获取盒模型数据
    struct BoxModelData {
        // Content
        float content_width = 0;
        float content_height = 0;
        
        // Padding
        float padding_top = 0;
        float padding_right = 0;
        float padding_bottom = 0;
        float padding_left = 0;
        
        // Border
        float border_top = 0;
        float border_right = 0;
        float border_bottom = 0;
        float border_left = 0;
        
        // Margin
        float margin_top = 0;
        float margin_right = 0;
        float margin_bottom = 0;
        float margin_left = 0;
    };
    BoxModelData GetBoxModelData() const;
    
    // 渲染
    void Render(SkCanvas* canvas, const LayoutBox& bounds);

private:
    std::weak_ptr<Element> element_;
    
    void RenderBoxDiagram(SkCanvas* canvas, const LayoutBox& bounds,
                          const BoxModelData& data);
};
```

### 6. DOMSerializer

DOM 序列化器，支持格式化输出和往返一致性。

```cpp
class DOMSerializer {
public:
    // 序列化选项
    struct SerializeOptions {
        int indent_size = 2;           // 缩进空格数
        bool include_text_nodes = true;
        bool pretty_print = true;
        bool escape_special_chars = true;
    };
    
    // 序列化
    static std::string Serialize(std::shared_ptr<Node> node,
                                 const SerializeOptions& options = {});
    
    // 反序列化（解析 HTML）
    static std::shared_ptr<Document> Deserialize(const std::string& html);
    
    // 往返测试辅助
    static bool IsEquivalent(std::shared_ptr<Node> a, std::shared_ptr<Node> b);

private:
    static void SerializeNode(std::shared_ptr<Node> node,
                              std::string& output,
                              int depth,
                              const SerializeOptions& options);
    
    static std::string EscapeHTML(const std::string& text);
    static std::string FormatAttributes(std::shared_ptr<Element> element);
};
```

### 7. ElementSearch

元素搜索组件。

```cpp
class ElementSearch {
public:
    ElementSearch(Document* document);
    
    // 搜索
    struct SearchResult {
        std::vector<std::shared_ptr<Element>> elements;
        int current_index = -1;
    };
    
    SearchResult Search(const std::string& query);
    
    // 导航
    void NextResult();
    void PreviousResult();
    std::shared_ptr<Element> GetCurrentResult() const;
    
    // 搜索类型
    enum class SearchType {
        CSSSelector,  // CSS 选择器搜索
        Text          // 文本搜索（tag, id, class）
    };
    void SetSearchType(SearchType type);

private:
    Document* document_;
    SearchType search_type_ = SearchType::CSSSelector;
    SearchResult current_result_;
    
    std::vector<std::shared_ptr<Element>> SearchBySelector(
        const std::string& selector);
    std::vector<std::shared_ptr<Element>> SearchByText(
        const std::string& text);
};
```

## Data Models

### ComputedStyleInfo

用于显示计算样式的数据结构。

```cpp
struct ComputedStyleInfo {
    // 样式属性
    struct Property {
        std::string name;
        std::string value;
        bool is_default;      // 是否为默认值
        bool is_inherited;    // 是否继承自父元素
        std::string source;   // 来源（inline, stylesheet, inherited）
    };
    
    // 按类别分组
    struct Category {
        std::string name;
        std::vector<Property> properties;
    };
    
    std::vector<Category> categories;
    
    // 预定义类别
    static const std::vector<std::string> CATEGORY_NAMES;
    // = {"Layout", "Typography", "Colors", "Background", "Border", 
    //    "Spacing", "Position", "Effects", "Other"}
};
```

### DevToolsState

DevTools 状态持久化数据。

```cpp
struct DevToolsState {
    bool is_open = false;
    DevToolsManager::DockPosition dock_position = 
        DevToolsManager::DockPosition::Bottom;
    float panel_size = 0.3f;
    
    // 展开的节点 ID 列表
    std::vector<std::string> expanded_node_ids;
    
    // 上次选中的元素路径
    std::string selected_element_path;
    
    // 活动的样式面板 Tab
    StylesPanel::Tab active_styles_tab = StylesPanel::Tab::InlineStyles;
    
    // 序列化/反序列化
    std::string ToJSON() const;
    static DevToolsState FromJSON(const std::string& json);
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

Based on the prework analysis, the following correctness properties have been identified:

### Property 1: DOM Tree Completeness
*For any* DOM document, when DevTools is opened, the DOM tree view SHALL display all nodes from the document root, and the count of displayed nodes SHALL equal the count of nodes in the actual DOM tree.
**Validates: Requirements 1.1**

### Property 2: Expand Indicator Consistency
*For any* node in the DOM tree view, the node SHALL display an expand/collapse indicator if and only if the node has one or more child nodes.
**Validates: Requirements 1.2**

### Property 3: DOM Update Synchronization
*For any* DOM modification (appendChild, removeChild, setAttribute, etc.), the DOM tree view SHALL reflect the change, and the tree structure SHALL match the actual DOM structure after the update.
**Validates: Requirements 1.5**

### Property 4: Single Selection Highlight
*For any* sequence of element selections, exactly one element SHALL be highlighted at any time, and the highlighted element SHALL be the most recently selected element.
**Validates: Requirements 2.2, 2.4**

### Property 5: Attribute Display Completeness
*For any* selected element with N attributes, the attributes panel SHALL display exactly N attributes, and each displayed attribute SHALL have a name and value matching the element's actual attributes.
**Validates: Requirements 3.1, 3.3, 3.4**

### Property 6: Inline Style Display Completeness
*For any* selected element with inline styles, all style properties defined in the element's style attribute SHALL be displayed with their correct property names and values.
**Validates: Requirements 4.1, 4.2**

### Property 7: Computed Style Completeness
*For any* selected element, the computed styles panel SHALL display all CSS properties, and each property value SHALL match the value returned by StyleResolver::ResolveStyle().
**Validates: Requirements 5.1, 5.2**

### Property 8: Computed Style Categorization
*For any* displayed computed style property, the property SHALL be assigned to exactly one category, and all properties in the same category SHALL be grouped together.
**Validates: Requirements 5.4**

### Property 9: Box Model Accuracy
*For any* selected element, the box model view SHALL display margin, border, padding, and content values that match the element's computed layout box dimensions.
**Validates: Requirements 6.1, 6.2, 6.3, 6.4, 6.5**

### Property 10: Style Edit Round-Trip
*For any* valid CSS property-value pair edited in the Inspector, setting the style and then reading it back SHALL return the same value (or an equivalent normalized value).
**Validates: Requirements 7.1, 7.2, 7.3**

### Property 11: Invalid Style Rejection
*For any* invalid CSS property-value pair, the Inspector SHALL reject the change and the element's style SHALL remain unchanged from its previous valid state.
**Validates: Requirements 7.4**

### Property 12: Attribute Edit Round-Trip
*For any* attribute name-value pair edited in the Inspector, setting the attribute and then reading it back SHALL return the same value.
**Validates: Requirements 8.1, 8.2, 8.3**

### Property 13: CSS Selector Search Accuracy
*For any* valid CSS selector query, the search results SHALL contain exactly the elements that match the selector according to QuerySelectorAll().
**Validates: Requirements 9.1**

### Property 14: Text Search Coverage
*For any* text search query, the search results SHALL include all elements where the query string appears in the tag name, id attribute, or any class name.
**Validates: Requirements 9.2**

### Property 15: Search Result Count Accuracy
*For any* search query with N matching elements, the displayed result count SHALL equal N.
**Validates: Requirements 9.3**

### Property 16: Panel Size Persistence
*For any* panel resize operation, closing and reopening DevTools SHALL restore the panel to the same size (within 1 pixel tolerance).
**Validates: Requirements 11.3**

### Property 17: State Restoration
*For any* DevTools session, reopening DevTools SHALL restore the previously selected element (if it still exists in the DOM) and the previous panel state.
**Validates: Requirements 12.3**

### Property 18: DOM Serialization Round-Trip
*For any* DOM subtree, serializing to HTML and then parsing back SHALL produce a DOM structure that is equivalent to the original (same node types, tag names, attributes, and text content).
**Validates: Requirements 13.1, 13.2, 13.3, 13.4**

### Property 19: Serialization Attribute Preservation
*For any* element with attributes, the serialized HTML string SHALL contain all attribute names and values, and parsing the string SHALL restore all attributes.
**Validates: Requirements 13.2**

## Error Handling

### 错误类型

```cpp
enum class DevToolsError {
    None,
    InvalidSelector,      // 无效的 CSS 选择器
    InvalidStyleValue,    // 无效的样式值
    InvalidAttributeName, // 无效的属性名
    ElementNotFound,      // 元素不存在（已被删除）
    SerializationError,   // 序列化失败
    ParseError            // 解析失败
};
```

### 错误处理策略

1. **无效样式值**: 显示错误指示器，保留原值，不应用更改
2. **元素被删除**: 清除选择状态，显示提示信息
3. **无效选择器**: 显示语法错误提示，不执行搜索
4. **序列化错误**: 记录日志，返回空字符串

## Testing Strategy

### 双重测试方法

本项目采用单元测试和属性测试相结合的方式：

- **单元测试**: 验证具体示例和边界情况
- **属性测试**: 验证在所有有效输入上都应成立的通用属性

### 属性测试框架

使用 [RapidCheck](https://github.com/emil-e/rapidcheck) 作为 C++ 属性测试库。

### 属性测试要求

- 每个属性测试必须运行至少 100 次迭代
- 每个属性测试必须使用注释标注对应的正确性属性
- 注释格式: `// **Feature: devtools-inspector, Property {number}: {property_text}**`

### 测试文件结构

```
tests/devtools/
├── test_dom_tree_view.cpp        # DOM 树视图测试
├── test_element_highlighter.cpp  # 高亮覆盖层测试
├── test_styles_panel.cpp         # 样式面板测试
├── test_box_model_view.cpp       # 盒模型测试
├── test_element_search.cpp       # 搜索功能测试
├── test_dom_serializer.cpp       # 序列化测试
├── test_style_editor.cpp         # 样式编辑测试
└── test_devtools_properties.cpp  # 属性测试集合
```

### 关键测试场景

1. **DOM 树同步测试**
   - 创建随机 DOM 树，验证树视图完整性
   - 执行随机 DOM 操作，验证视图同步

2. **样式编辑往返测试**
   - 生成随机有效样式值，编辑后读取验证一致性
   - 生成随机无效样式值，验证拒绝行为

3. **序列化往返测试**
   - 生成随机 DOM 树，序列化后解析，验证等价性

4. **搜索准确性测试**
   - 生成随机 DOM 树和选择器，验证搜索结果与 QuerySelectorAll 一致
