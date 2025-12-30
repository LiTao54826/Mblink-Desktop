# 统一输入管理系统 - 设计文档

## 概述

本文档描述 LightUI 统一输入管理系统的架构设计。该系统参考 Chromium/Blink 的设计，将分散的焦点管理、选区管理、文本编辑等功能统一到一个协调的架构中。

## Blink 架构参考

通过分析 Blink 源码，我们确定了以下关键组件映射关系：

| Blink 组件 | LightUI 组件 | 说明 |
|-----------|-------------|------|
| `blink::EventHandler` | `InputController` | 事件处理入口，协调各子系统 |
| `blink::FocusController` | `FocusController` | 焦点管理，位于 Page 级别 |
| `blink::FrameSelection` | `FrameSelection` | 选区管理，每个 Frame 一个 |
| `blink::SelectionController` | `SelectionController` | 处理鼠标/手势选择操作 |
| `blink::Editor` | `EditContext` | 编辑命令执行 |
| `blink::FrameCaret` | `CaretController` | 光标渲染和闪烁 |
| `blink::Position` | `Position` | DOM 位置表示 |

### Blink 关键设计模式

1. **FocusController 位于 Page 级别**: Blink 的 FocusController 属于 Page，管理跨 Frame 的焦点
2. **FrameSelection 每帧一个**: 每个 LocalFrame 有自己的 FrameSelection
3. **SelectionController 处理用户交互**: 专门处理鼠标拖动、手势等选择操作
4. **Position 使用模板策略**: 支持 DOM Tree 和 Flat Tree 两种遍历策略
5. **事件处理分层**: EventHandler → 各种 Manager (MouseEventManager, KeyboardEventManager 等)

## 设计指南与强制要求

### 强制要求

1. **API 兼容性**: 所有现有的 JavaScript API 必须保持兼容，包括 `element.focus()`、`element.blur()`、`document.activeElement`、Selection API 等
2. **渐进式迁移**: 必须支持新旧系统并存，允许逐步迁移各个元素类型
3. **单一入口**: 所有输入事件必须通过 InputController 统一入口处理，禁止绕过
4. **状态一致性**: 焦点状态、选区状态、编辑状态必须保持同步，禁止出现不一致
5. **事件顺序**: 事件处理必须按照 W3C 标准的顺序触发，禁止乱序

### 设计原则

1. **参考 Blink**: 尽可能参考 Chromium/Blink 的设计，保持概念和 API 一致
2. **职责分离**: 每个组件只负责单一职责，通过接口协作
3. **可测试性**: 所有核心逻辑必须可单元测试，避免依赖 SDL/渲染
4. **最小侵入**: 对现有代码的修改应最小化，优先通过新增代码实现
5. **复用现有代码**: 尽可能复用 `FocusManager`、`SelectionManager` 的现有逻辑

### 注意事项

1. **IME 处理**: IME 输入涉及复杂的状态机，需要特别注意组合状态的处理
2. **光标闪烁**: 光标闪烁需要与事件循环协调，避免性能问题
3. **选区渲染**: 选区高亮需要考虑跨行、跨元素的情况
4. **内存管理**: 使用 weak_ptr 避免循环引用，特别是焦点元素的引用


## 架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              Window                                      │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                        InputController                             │  │
│  │         (参考 Blink EventHandler - 事件处理统一入口)                │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐   │  │
│  │  │ FocusController │  │SelectionControl │  │KeyboardEventMgr  │   │  │
│  │  │ (Page级别焦点)   │  │ (鼠标选择处理)  │  │ (键盘事件处理)    │   │  │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬─────────┘   │  │
│  │           │                    │                    │              │  │
│  └───────────┼────────────────────┼────────────────────┼──────────────┘  │
│              │                    │                    │                 │
│  ┌───────────▼────────────────────▼────────────────────▼──────────────┐  │
│  │                          Document                                   │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │  │
│  │  │ FrameSelection  │  │   EditContext   │  │  CaretController │    │  │
│  │  │ (文档级选区管理) │  │ (编辑命令执行)  │  │  (光标渲染闪烁)  │    │  │
│  │  │                 │  │                 │  │                  │    │  │
│  │  │ - selection     │  │ - composition   │  │ - position       │    │  │
│  │  │ - caret_pos     │  │ - undo_stack    │  │ - blink_state    │    │  │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                    │                                     │
│                                    ▼                                     │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                         Editable 接口                              │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌──────────┐  │  │
│  │  │HTMLInput    │  │HTMLTextArea │  │ContentEdit  │  │Terminal  │  │  │
│  │  │Element      │  │Element      │  │Element      │  │Element   │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └──────────┘  │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### 与 Blink 架构对比

```
Blink 架构:                          LightUI 架构:
┌─────────────────┐                  ┌─────────────────┐
│      Page       │                  │     Window      │
│ ┌─────────────┐ │                  │ ┌─────────────┐ │
│ │FocusControl │ │                  │ │InputControl │ │
│ └─────────────┘ │                  │ │ ┌─────────┐ │ │
└────────┬────────┘                  │ │ │FocusCtr │ │ │
         │                           │ │ └─────────┘ │ │
┌────────▼────────┐                  │ └─────────────┘ │
│   LocalFrame    │                  └────────┬────────┘
│ ┌─────────────┐ │                           │
│ │EventHandler │ │                  ┌────────▼────────┐
│ │ ┌─────────┐ │ │                  │    Document     │
│ │ │Selection│ │ │                  │ ┌─────────────┐ │
│ │ │Controller│ │ │                  │ │FrameSelect │ │
│ │ └─────────┘ │ │                  │ │ ┌─────────┐ │ │
│ ├─────────────┤ │                  │ │ │EditCtx  │ │ │
│ │FrameSelect │ │                  │ │ └─────────┘ │ │
│ │ ┌─────────┐ │ │                  │ └─────────────┘ │
│ │ │FrameCar │ │ │                  └─────────────────┘
│ │ └─────────┘ │ │
│ └─────────────┘ │
│ ┌─────────────┐ │
│ │   Editor    │ │
│ └─────────────┘ │
└─────────────────┘
```

### 事件流 (参考 Blink EventHandler)

```
SDL Event
    │
    ▼
┌─────────────────┐
│   EventLoop     │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│                    InputController                           │
│  (参考 Blink EventHandler::HandleMousePressEvent 等)         │
│                                                              │
│  1. Hit Testing → 确定目标元素                               │
│  2. FocusController → 处理焦点变化                           │
│  3. SelectionController → 处理选区操作                       │
│  4. EditContext → 处理文本编辑                               │
│  5. DOM Event Dispatch → 分发到元素                          │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────┐
│ DOM Event       │  (分发到元素)
│ Dispatch        │
└─────────────────┘
```


## 组件与接口

### 1. InputController (核心控制器)

```cpp
/**
 * @file input_controller.h
 * @brief 统一输入控制器 - 所有输入事件的统一入口
 * 
 * 参考: Blink 的 EventHandler 类
 * 源码: 参考/blink/renderer/core/input/event_handler.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 包含多个子 Manager (MouseEventManager, KeyboardEventManager 等)
 * 2. 持有 SelectionController 处理选择操作
 * 3. 通过 HitTest 确定事件目标
 */
class InputController {
public:
    InputController(Window* window);
    
    // ========== 事件处理入口 (参考 EventHandler) ==========
    
    /**
     * @brief 处理鼠标按下事件
     * 参考: EventHandler::HandleMousePressEvent
     */
    WebInputEventResult HandleMousePressEvent(const SDL_Event& event);
    
    /**
     * @brief 处理鼠标移动事件
     * 参考: EventHandler::HandleMouseMoveEvent
     */
    WebInputEventResult HandleMouseMoveEvent(const SDL_Event& event);
    
    /**
     * @brief 处理鼠标释放事件
     * 参考: EventHandler::HandleMouseReleaseEvent
     */
    WebInputEventResult HandleMouseReleaseEvent(const SDL_Event& event);
    
    /**
     * @brief 处理键盘事件
     * 参考: EventHandler::KeyEvent
     */
    WebInputEventResult HandleKeyboardEvent(const SDL_Event& event);
    
    /**
     * @brief 处理文本输入事件 (SDL_TEXTINPUT)
     * 参考: EventHandler::HandleTextInputEvent
     */
    WebInputEventResult HandleTextInputEvent(const SDL_Event& event);
    
    /**
     * @brief 处理 IME 组合事件 (SDL_TEXTEDITING)
     */
    WebInputEventResult HandleIMEEvent(const SDL_Event& event);
    
    // ========== Hit Testing (参考 EventHandler::HitTestResultAtLocation) ==========
    
    /**
     * @brief 在指定位置进行 Hit Test
     * 参考: EventHandler::HitTestResultAtLocation
     */
    HitTestResult HitTestAtLocation(const gfx::Point& location);
    
    // ========== 子系统访问 ==========
    
    FocusController* GetFocusController() { return focus_controller_.get(); }
    SelectionController* GetSelectionController() { return selection_controller_.get(); }
    
    // ========== 帧更新 ==========
    
    /**
     * @brief 每帧更新 (光标闪烁等)
     */
    void Update(float delta_time);

private:
    Window* window_;
    
    // 参考 Blink EventHandler 的成员
    std::unique_ptr<FocusController> focus_controller_;
    std::unique_ptr<SelectionController> selection_controller_;
    std::unique_ptr<KeyboardEventManager> keyboard_event_manager_;
    std::unique_ptr<MouseEventManager> mouse_event_manager_;
};

/**
 * @brief 事件处理结果 (参考 Blink WebInputEventResult)
 */
enum class WebInputEventResult {
    kNotHandled,
    kHandledSystem,
    kHandledApplication,
    kHandledSuppressed
};
```

### 2. FocusController (焦点控制器)

```cpp
/**
 * @file focus_controller.h
 * @brief 焦点控制器 - 管理焦点状态和导航
 * 
 * 参考: Blink 的 FocusController 类
 * 源码: 参考/blink/renderer/core/page/focus_controller.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 位于 Page 级别，管理跨 Frame 焦点
 * 2. 支持 FocusType (键盘、鼠标、脚本等)
 * 3. 使用 FocusChangedObserver 通知焦点变化
 * 4. 支持 is_active_ 和 is_focused_ 两个状态
 * 
 * 可复用现有代码: core/event/input/focus_manager.h
 */
class FocusController : public DOMObserver {
public:
    // ========== 焦点操作 (参考 Blink FocusController) ==========
    
    /**
     * @brief 设置焦点到指定元素
     * 参考: FocusController::SetFocusedElement
     * 
     * @param element 目标元素
     * @param frame 目标 Frame (LightUI 简化为 Document)
     * @param params 焦点参数
     */
    bool SetFocusedElement(std::shared_ptr<Element> element, 
                           Document* document,
                           const FocusParams& params = {});
    
    /**
     * @brief 获取当前焦点元素
     * 注意: Blink 中焦点是 Frame 级别的，这里简化为 Document 级别
     */
    std::shared_ptr<Element> GetFocusedElement() const;
    
    /**
     * @brief 清除焦点
     */
    void ClearFocus();
    
    // ========== Tab 导航 (参考 Blink FocusController::AdvanceFocus) ==========
    
    /**
     * @brief 移动焦点到下一个/上一个可聚焦元素
     * 参考: FocusController::AdvanceFocus
     * 
     * @param type 焦点类型 (Forward, Backward 等)
     */
    bool AdvanceFocus(FocusType type);
    
    // ========== 焦点状态 (参考 Blink) ==========
    
    /**
     * @brief 设置活动状态
     * 参考: FocusController::SetActive
     */
    void SetActive(bool active);
    bool IsActive() const { return is_active_; }
    
    /**
     * @brief 设置焦点状态
     * 参考: FocusController::SetFocused
     */
    void SetFocused(bool focused);
    bool IsFocused() const { return is_focused_; }
    
    // ========== 焦点查询 ==========
    
    bool IsFocusable(std::shared_ptr<Element> element) const;
    
    // ========== 观察者 (参考 Blink FocusChangedObserver) ==========
    
    void RegisterFocusChangedObserver(FocusChangedObserver* observer);
    
    // ========== DOMObserver ==========
    void OnNodeRemoved(Node* node, Node* parent) override;

private:
    std::weak_ptr<Element> focused_element_;
    bool is_active_ = false;
    bool is_focused_ = false;
    std::vector<FocusChangedObserver*> observers_;
};

/**
 * @brief 焦点类型 (参考 Blink mojom::blink::FocusType)
 */
enum class FocusType {
    kNone,
    kMouse,
    kKeyboard,
    kScript,
    kForward,   // Tab
    kBackward,  // Shift+Tab
};

/**
 * @brief 焦点参数 (参考 Blink FocusParams)
 */
struct FocusParams {
    FocusType type = FocusType::kNone;
    bool prevent_scroll = false;
    bool focus_visible = false;
};
```

### 3. SelectionController (选择控制器 - 新增)

```cpp
/**
 * @file selection_controller.h
 * @brief 选择控制器 - 处理鼠标/手势选择操作
 * 
 * 参考: Blink 的 SelectionController 类
 * 源码: 参考/blink/renderer/core/editing/selection_controller.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 专门处理用户交互产生的选择 (鼠标拖动、双击选词等)
 * 2. 与 FrameSelection 分离，FrameSelection 管理状态，SelectionController 处理交互
 * 3. 支持 SelectionState 状态机
 * 
 * 可复用现有代码: core/editing/selection_manager.h 的鼠标处理逻辑
 */
class SelectionController {
public:
    explicit SelectionController(Document& document);
    
    // ========== 鼠标事件处理 (参考 Blink SelectionController) ==========
    
    /**
     * @brief 处理鼠标按下
     * 参考: SelectionController::HandleMousePressEvent
     */
    bool HandleMousePressEvent(const MouseEventWithHitTestResults& event);
    
    /**
     * @brief 处理鼠标拖动
     * 参考: SelectionController::HandleMouseDraggedEvent
     */
    WebInputEventResult HandleMouseDraggedEvent(const MouseEventWithHitTestResults& event);
    
    /**
     * @brief 处理鼠标释放
     * 参考: SelectionController::HandleMouseReleaseEvent
     */
    bool HandleMouseReleaseEvent(const MouseEventWithHitTestResults& event);
    
    // ========== 选择状态 (参考 Blink SelectionState) ==========
    
    enum class SelectionState {
        kHaveNotStartedSelection,
        kPlacedCaret,
        kExtendedSelection
    };
    
    SelectionState GetSelectionState() const { return selection_state_; }
    
    // ========== 选择操作 ==========
    
    /**
     * @brief 更新拖动选择
     * 参考: SelectionController::UpdateSelectionForMouseDrag
     */
    void UpdateSelectionForMouseDrag(const gfx::Point& position);
    
    /**
     * @brief 设置是否可以开始选择
     */
    void SetMouseDownMayStartSelect(bool may_start);
    bool MouseDownMayStartSelect() const { return mouse_down_may_start_select_; }

private:
    // 参考 Blink SelectionController 的私有方法
    bool HandleSingleClick(const MouseEventWithHitTestResults& event);
    bool HandleDoubleClick(const MouseEventWithHitTestResults& event);
    bool HandleTripleClick(const MouseEventWithHitTestResults& event);
    
    /**
     * @brief 选择最近的单词
     * 参考: SelectionController::SelectClosestWordFromHitTestResult
     */
    bool SelectClosestWordFromHitTestResult(const HitTestResult& result);

private:
    Document& document_;
    SelectionState selection_state_ = SelectionState::kHaveNotStartedSelection;
    bool mouse_down_may_start_select_ = false;
    bool mouse_down_was_single_click_in_selection_ = false;
    
    // 选择起始位置 (参考 Blink original_anchor_in_flat_tree_)
    PositionWithAffinity original_anchor_;
};
```


### 4. FrameSelection (选区管理器)

```cpp
/**
 * @file frame_selection.h
 * @brief 选区管理器 - 管理文本选区和光标位置
 * 
 * 参考: Blink 的 FrameSelection 类
 * 源码: 参考/blink/renderer/core/editing/frame_selection.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 每个 LocalFrame 一个 FrameSelection
 * 2. 包含 SelectionEditor 管理选区状态
 * 3. 包含 FrameCaret 管理光标
 * 4. 包含 LayoutSelection 管理选区渲染
 * 5. 支持 TextGranularity (字符、单词、行、段落等)
 * 
 * 可复用现有代码: core/editing/selection_manager.h
 */
class FrameSelection {
public:
    explicit FrameSelection(Document& document);
    
    // ========== 选区操作 (参考 Blink FrameSelection) ==========
    
    /**
     * @brief 设置选区
     * 参考: FrameSelection::SetSelection
     */
    void SetSelection(const SelectionInDOMTree& selection, 
                      const SetSelectionOptions& options = {});
    
    /**
     * @brief 获取当前选区
     * 参考: FrameSelection::GetSelectionInDOMTree
     */
    const SelectionInDOMTree& GetSelection() const;
    
    /**
     * @brief 清除选区
     */
    void Clear();
    
    // ========== 选区修改 (参考 Blink FrameSelection::Modify) ==========
    
    /**
     * @brief 修改选区
     * 参考: FrameSelection::Modify
     * 
     * @param alteration 修改类型 (Move, Extend)
     * @param direction 方向 (Forward, Backward, Left, Right)
     * @param granularity 粒度 (Character, Word, Line, Paragraph)
     */
    bool Modify(SelectionModifyAlteration alteration,
                SelectionModifyDirection direction,
                TextGranularity granularity);
    
    /**
     * @brief 选中全部内容
     * 参考: FrameSelection::SelectAll
     */
    void SelectAll();
    
    // ========== 选区查询 ==========
    
    /**
     * @brief 检查是否有选区 (非折叠)
     */
    bool HasSelection() const;
    
    /**
     * @brief 获取选中的文本
     * 参考: FrameSelection::SelectedText
     */
    std::string SelectedText() const;
    
    // ========== 光标相关 (委托给 FrameCaret) ==========
    
    /**
     * @brief 获取光标绝对边界
     * 参考: FrameSelection::AbsoluteCaretBounds
     */
    gfx::Rect AbsoluteCaretBounds() const;
    
    /**
     * @brief 设置光标闪烁暂停
     * 参考: FrameSelection::SetCaretBlinkingSuspended
     */
    void SetCaretBlinkingSuspended(bool suspended);
    
    // ========== 焦点相关 ==========
    
    /**
     * @brief 焦点变化时调用
     * 参考: FrameSelection::DidChangeFocus
     */
    void DidChangeFocus();
    
    /**
     * @brief 检查选区是否有焦点
     * 参考: FrameSelection::SelectionHasFocus
     */
    bool SelectionHasFocus() const;
    
    // ========== 渲染相关 ==========
    
    /**
     * @brief 绘制光标
     * 参考: FrameSelection::PaintCaret
     */
    void PaintCaret(SkCanvas* canvas, const gfx::Point& paint_offset);
    
    /**
     * @brief 更新外观
     * 参考: FrameSelection::UpdateAppearance
     */
    void UpdateAppearance();

private:
    Document& document_;
    
    // 参考 Blink FrameSelection 的成员
    std::unique_ptr<SelectionEditor> selection_editor_;
    std::unique_ptr<FrameCaret> frame_caret_;
    std::unique_ptr<LayoutSelection> layout_selection_;
    
    TextGranularity granularity_ = TextGranularity::kCharacter;
    bool focused_ = false;
};

/**
 * @brief 选区修改类型 (参考 Blink SelectionModifyAlteration)
 */
enum class SelectionModifyAlteration {
    kMove,    // 移动光标
    kExtend   // 扩展选区
};

/**
 * @brief 选区修改方向 (参考 Blink SelectionModifyDirection)
 */
enum class SelectionModifyDirection {
    kForward,
    kBackward,
    kLeft,
    kRight
};

/**
 * @brief 文本粒度 (参考 Blink TextGranularity)
 */
enum class TextGranularity {
    kCharacter,
    kWord,
    kSentence,
    kLine,
    kParagraph,
    kDocumentBoundary
};

/**
 * @brief 设置选区选项 (参考 Blink SetSelectionOptions)
 */
struct SetSelectionOptions {
    bool should_close_typing = true;
    bool should_clear_typing_style = true;
    bool do_not_set_focus = false;
};
```

### 5. Position (DOM 位置 - 参考 Blink)

```cpp
/**
 * @file position.h
 * @brief DOM 树中的位置
 * 
 * 参考: Blink 的 Position 类
 * 源码: 参考/blink/renderer/core/editing/position.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 支持多种锚点类型 (OffsetInAnchor, BeforeAnchor, AfterAnchor, AfterChildren)
 * 2. 使用模板支持 DOM Tree 和 Flat Tree
 * 3. 提供丰富的位置计算方法
 */

/**
 * @brief 位置锚点类型 (参考 Blink PositionAnchorType)
 */
enum class PositionAnchorType {
    kOffsetInAnchor,  // 节点内偏移
    kBeforeAnchor,    // 节点之前
    kAfterAnchor,     // 节点之后
    kAfterChildren    // 子节点之后
};

/**
 * @brief DOM 树中的位置 (简化版，不使用模板)
 */
class Position {
public:
    Position() = default;
    
    // 创建偏移位置
    Position(Node* anchor_node, int offset);
    
    // 创建 before/after 位置
    Position(Node* anchor_node, PositionAnchorType type);
    
    // ========== 静态工厂方法 (参考 Blink) ==========
    
    static Position BeforeNode(Node& node);
    static Position AfterNode(Node& node);
    static Position FirstPositionInNode(Node& node);
    static Position LastPositionInNode(Node& node);
    
    // ========== 查询方法 ==========
    
    Node* AnchorNode() const { return anchor_node_; }
    PositionAnchorType AnchorType() const { return anchor_type_; }
    
    bool IsNull() const { return !anchor_node_; }
    bool IsNotNull() const { return anchor_node_ != nullptr; }
    
    /**
     * @brief 计算容器节点
     * 参考: Position::ComputeContainerNode
     */
    Node* ComputeContainerNode() const;
    
    /**
     * @brief 计算容器内偏移
     * 参考: Position::ComputeOffsetInContainerNode
     */
    int ComputeOffsetInContainerNode() const;
    
    // ========== 比较 ==========
    
    bool operator==(const Position& other) const;
    bool operator<(const Position& other) const;

private:
    Node* anchor_node_ = nullptr;
    int offset_ = 0;
    PositionAnchorType anchor_type_ = PositionAnchorType::kOffsetInAnchor;
};

/**
 * @brief DOM 树中的选区 (参考 Blink SelectionInDOMTree)
 */
struct SelectionInDOMTree {
    Position anchor;  // 选区起点 (用户开始选择的位置)
    Position focus;   // 选区终点 (用户结束选择的位置)
    
    bool IsCollapsed() const { return anchor == focus; }
    bool IsNone() const { return anchor.IsNull() && focus.IsNull(); }
    
    // 获取规范化的起点和终点
    Position Start() const { return anchor < focus ? anchor : focus; }
    Position End() const { return anchor < focus ? focus : anchor; }
};
```

### 6. EditContext (编辑上下文)

```cpp
/**
 * @file edit_context.h
 * @brief 编辑上下文 - 管理文本编辑操作
 * 
 * 参考: Blink 的 Editor 类
 * 源码: 参考/blink/renderer/core/editing/editor.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 执行编辑命令 (InsertText, Delete 等)
 * 2. 管理撤销/重做栈 (UndoStack)
 * 3. 处理剪贴板操作
 * 4. 支持 CompositeEditCommand 组合命令
 * 
 * 可复用现有代码: core/editing/contenteditable_controller.h
 */
class EditContext {
public:
    explicit EditContext(Document& document);
    
    // ========== 编辑能力查询 (参考 Blink Editor) ==========
    
    bool CanEdit() const;
    bool CanEditRichly() const;
    bool CanCut() const;
    bool CanCopy() const;
    bool CanPaste() const;
    bool CanDelete() const;
    
    // ========== 文本编辑 (参考 Blink Editor) ==========
    
    /**
     * @brief 插入文本
     * 参考: Editor::InsertText
     */
    bool InsertText(const std::string& text, KeyboardEvent* triggering_event = nullptr);
    
    /**
     * @brief 删除选中内容或光标前/后的字符
     * 参考: Editor::DeleteSelectionWithSmartDelete
     */
    void DeleteSelection(DeleteMode mode, DeleteDirection direction);
    
    /**
     * @brief 插入换行
     * 参考: Editor::InsertLineBreak
     */
    bool InsertLineBreak();
    
    /**
     * @brief 插入段落分隔符
     * 参考: Editor::InsertParagraphSeparator
     */
    bool InsertParagraphSeparator();
    
    // ========== IME 支持 ==========
    
    void StartComposition();
    void UpdateComposition(const std::string& text, int cursor);
    void CommitComposition(const std::string& text);
    void CancelComposition();
    bool IsComposing() const { return is_composing_; }
    
    // ========== 剪贴板 (参考 Blink Editor) ==========
    
    void Cut();
    void Copy();
    void Paste();
    
    // ========== 撤销/重做 (参考 Blink Editor + UndoStack) ==========
    
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    
    // ========== 命令执行 (参考 Blink Editor::ExecuteCommand) ==========
    
    /**
     * @brief 执行编辑命令
     * 参考: Editor::ExecuteCommand
     */
    bool ExecuteCommand(const std::string& command_name);
    bool ExecuteCommand(const std::string& command_name, const std::string& value);

private:
    Document& document_;
    
    // IME 状态
    bool is_composing_ = false;
    std::string composition_text_;
    int composition_cursor_ = 0;
    
    // 撤销栈 (参考 Blink UndoStack)
    std::unique_ptr<UndoStack> undo_stack_;
};

enum class DeleteMode {
    kSimple,
    kSmart  // 智能删除 (处理空格等)
};

enum class DeleteDirection {
    kBackward,  // Backspace
    kForward    // Delete
};
```

### 7. FrameCaret (光标控制器)

```cpp
/**
 * @file frame_caret.h
 * @brief 光标控制器 - 管理光标渲染和闪烁
 * 
 * 参考: Blink 的 FrameCaret 类
 * 源码: 参考/blink/renderer/core/editing/frame_caret.h
 * 
 * 关键设计点 (来自 Blink):
 * 1. 使用定时器控制闪烁
 * 2. 支持暂停闪烁 (鼠标按下时)
 * 3. 使用 CaretDisplayItemClient 进行渲染
 * 4. 支持 CaretShape (Bar, Block, Underscore)
 */
class FrameCaret {
public:
    explicit FrameCaret(FrameSelection& selection);
    
    // ========== 光标状态 (参考 Blink FrameCaret) ==========
    
    /**
     * @brief 检查光标是否活动
     * 参考: FrameCaret::IsActive
     */
    bool IsActive() const;
    
    /**
     * @brief 设置光标启用状态
     * 参考: FrameCaret::SetCaretEnabled
     */
    void SetCaretEnabled(bool enabled);
    
    /**
     * @brief 暂停光标闪烁
     * 参考: FrameCaret::SetCaretBlinkingSuspended
     */
    void SetCaretBlinkingSuspended(bool suspended);
    bool IsCaretBlinkingSuspended() const { return caret_blinking_suspended_; }
    
    /**
     * @brief 开始闪烁
     * 参考: FrameCaret::StartBlinkCaret
     */
    void StartBlinkCaret();
    
    /**
     * @brief 停止闪烁
     * 参考: FrameCaret::StopCaretBlinkTimer
     */
    void StopCaretBlinkTimer();
    
    // ========== 光标渲染 ==========
    
    /**
     * @brief 获取光标绝对边界
     * 参考: FrameCaret::AbsoluteCaretBounds
     */
    gfx::Rect AbsoluteCaretBounds() const;
    
    /**
     * @brief 绘制光标
     * 参考: FrameCaret::PaintCaret
     */
    void PaintCaret(SkCanvas* canvas, const gfx::Point& paint_offset) const;
    
    /**
     * @brief 获取光标形状
     * 参考: FrameCaret::GetCaretShape
     */
    CaretShape GetCaretShape() const;
    
    // ========== 更新 ==========
    
    /**
     * @brief 更新样式和布局
     * 参考: FrameCaret::UpdateStyleAndLayoutIfNeeded
     */
    void UpdateStyleAndLayoutIfNeeded();
    
    /**
     * @brief 定时器触发
     */
    void CaretBlinkTimerFired();

private:
    FrameSelection& selection_;
    
    bool caret_enabled_ = true;
    bool caret_visible_ = true;  // 当前是否显示 (闪烁状态)
    bool caret_blinking_suspended_ = false;
    
    // 闪烁定时器 (使用事件循环的定时任务)
    float blink_timer_ = 0.0f;
    
    static constexpr float BLINK_INTERVAL = 500.0f;  // 毫秒
};

/**
 * @brief 光标形状 (参考 Blink CaretShape)
 */
enum class CaretShape {
    kBar,        // 竖线 (默认)
    kBlock,      // 方块
    kUnderscore  // 下划线
};
```

### 8. Editable 接口

```cpp
/**
 * @file editable.h
 * @brief 可编辑接口 - 所有可编辑元素必须实现
 * 
 * 这是 InputController 与可编辑元素交互的统一接口
 * 
 * 注意: Blink 没有直接对应的接口，而是通过 HTMLInputElement、
 * HTMLTextAreaElement 等各自实现编辑功能。我们抽象出这个接口
 * 以简化统一处理。
 */
class Editable {
public:
    virtual ~Editable() = default;
    
    // ========== 内容访问 ==========
    
    virtual std::string GetTextContent() const = 0;
    virtual void SetTextContent(const std::string& text) = 0;
    
    // ========== 选区支持 ==========
    
    virtual int GetSelectionStart() const = 0;
    virtual int GetSelectionEnd() const = 0;
    virtual void SetSelectionRange(int start, int end) = 0;
    
    // ========== 编辑操作 ==========
    
    virtual void InsertTextAtCursor(const std::string& text) = 0;
    virtual void DeleteContent(DeleteDirection direction) = 0;
    virtual void ReplaceSelectedText(const std::string& text) = 0;
    
    // ========== 光标位置 ==========
    
    /**
     * @brief 获取光标在屏幕上的位置 (用于渲染)
     */
    virtual CaretRect GetCaretRect() const = 0;
    
    /**
     * @brief 将屏幕坐标转换为文本偏移量
     */
    virtual int HitTestToOffset(float x, float y) const = 0;
    
    // ========== 状态查询 ==========
    
    virtual bool IsEditable() const = 0;
    virtual bool IsMultiLine() const = 0;
    
    // ========== 事件通知 ==========
    
    virtual void OnContentChanged() = 0;
    virtual void OnValueCommitted() = 0;
};

/**
 * @brief 光标矩形
 */
struct CaretRect {
    float x, y;
    float width, height;
    
    bool IsValid() const { return width > 0 && height > 0; }
};
```


## 数据模型

### 焦点状态

```cpp
struct FocusState {
    std::weak_ptr<Element> focused_element;  // 当前焦点元素
    bool focus_visible;                       // 是否显示焦点指示器
    std::vector<std::weak_ptr<Element>> focus_chain;  // 焦点链 (从元素到根)
};
```

### 选区状态

```cpp
struct SelectionState {
    Position anchor;           // 选区锚点
    Position focus;            // 选区焦点
    SelectionDirection direction;  // 选区方向
    
    enum class SelectionDirection {
        NONE,
        FORWARD,
        BACKWARD
    };
};
```

### IME 组合状态

```cpp
struct CompositionState {
    bool is_composing;         // 是否正在组合
    std::string text;          // 组合文本
    int cursor;                // 组合光标位置
    int selection_start;       // 组合选区起始
    int selection_end;         // 组合选区结束
};
```

## 正确性属性

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 焦点唯一性
*For any* 时刻，系统中最多只有一个元素拥有焦点
**Validates: Requirements 1.1, 1.2**

### Property 2: 焦点转移一致性
*For any* 可聚焦元素，调用 focus() 后该元素应成为焦点元素，且旧焦点元素应收到 blur 事件
**Validates: Requirements 1.3, 1.4**

### Property 3: Tab 导航顺序
*For any* 具有 tabindex 的元素集合，Tab 导航应按 tabindex 升序访问，相同 tabindex 按 DOM 顺序
**Validates: Requirements 1.5**

### Property 4: 焦点元素移除
*For any* 焦点元素被从 DOM 移除时，焦点应被清除且触发 blur 事件
**Validates: Requirements 1.6**

### Property 5: 点击定位准确性
*For any* 可编辑元素中的点击，光标应定位到最接近点击位置的字符边界
**Validates: Requirements 2.1**

### Property 6: 拖动选区连续性
*For any* 拖动选择操作，选区应从起点连续扩展到当前鼠标位置
**Validates: Requirements 2.2**

### Property 7: 选区扩展正确性
*For any* Shift+方向键操作，选区应正确扩展或收缩一个字符/行
**Validates: Requirements 2.3**

### Property 8: 双击选词边界
*For any* 双击操作，应选中完整的单词（按 Unicode 单词边界）
**Validates: Requirements 2.5**

### Property 9: 文本插入位置
*For any* 文本插入操作，文本应插入到光标位置，光标应移动到插入文本之后
**Validates: Requirements 3.1**

### Property 10: 删除操作正确性
*For any* 删除操作，应删除选中内容或光标前/后的字符，光标位置应正确更新
**Validates: Requirements 3.2, 3.3**

### Property 11: 剪贴板往返一致性
*For any* 文本，复制后粘贴应得到相同的文本
**Validates: Requirements 3.4, 3.5**

### Property 12: IME 组合状态机
*For any* IME 输入序列，组合状态应正确转换：未组合 → 组合中 → 已确认/已取消
**Validates: Requirements 3.6**

### Property 13: 光标可见性
*For any* 可编辑元素获得焦点时，光标应可见；失去焦点时，光标应隐藏
**Validates: Requirements 5.1, 5.2**

### Property 14: 选区与光标互斥
*For any* 时刻，如果存在非折叠选区，则光标应隐藏
**Validates: Requirements 5.5**

### Property 15: API 兼容性
*For any* JavaScript API 调用，新系统的行为应与旧系统一致
**Validates: Requirements 7.1, 7.2, 7.3, 7.4, 7.5, 7.6**

### Property 16: 渐进式迁移隔离
*For any* 未迁移的元素，其行为应不受新系统影响
**Validates: Requirements 8.2, 8.4**


## 错误处理

### 焦点错误

1. **焦点元素被销毁**: 通过 DOMObserver 监听节点移除，自动清除焦点
2. **焦点设置失败**: SetFocusedElement 返回 false，不改变当前状态
3. **循环焦点**: Tab 导航到达末尾时循环到开头

### 选区错误

1. **无效位置**: Position 验证节点是否在 DOM 中，无效时返回空选区
2. **跨元素选区**: 支持跨元素选区，但编辑操作限制在单个可编辑元素内
3. **选区越界**: 自动裁剪到有效范围

### 编辑错误

1. **只读元素**: 检查 IsEditable()，只读时忽略编辑操作
2. **IME 中断**: 组合过程中焦点变化时自动取消组合
3. **剪贴板失败**: 剪贴板操作失败时静默忽略

## 测试策略

### 单元测试

1. **FocusController 测试**
   - 焦点设置和清除
   - Tab 导航顺序
   - 焦点元素移除处理

2. **FrameSelection 测试**
   - 选区设置和查询
   - 光标移动
   - 选区扩展

3. **EditContext 测试**
   - 文本插入和删除
   - IME 组合状态机
   - 撤销/重做

4. **CaretController 测试**
   - 光标闪烁计时
   - 可见性状态

### 属性测试

使用 [rapid-check](https://github.com/emil-e/rapidcheck) 或类似的 C++ 属性测试库：

1. **焦点属性测试**: 生成随机的焦点操作序列，验证焦点唯一性
2. **选区属性测试**: 生成随机的选区操作，验证选区一致性
3. **编辑属性测试**: 生成随机的编辑操作，验证内容正确性
4. **API 兼容性测试**: 对比新旧系统的 API 行为

### 集成测试

1. **端到端焦点测试**: 模拟用户点击和 Tab 导航
2. **端到端编辑测试**: 模拟用户输入和编辑操作
3. **IME 集成测试**: 模拟 IME 输入序列

## 迁移计划

### 阶段 1: 基础架构 (复用现有代码)

1. 创建 `core/input/` 目录结构
2. 实现 `Position` 类 (参考 Blink Position)
3. 实现 `FocusController` (重构自 `FocusManager`)
   - 复用 `focus_manager.cpp` 的焦点设置、Tab 导航逻辑
   - 添加 `FocusType`、`FocusParams` 支持
   - 添加 `FocusChangedObserver` 机制
4. 保持与现有代码并存

### 阶段 2: 选区管理 (拆分现有代码)

1. 实现 `FrameSelection` (重构自 `SelectionManager`)
   - 复用选区状态管理逻辑
   - 添加 `TextGranularity` 支持
   - 添加 `SetSelectionOptions` 支持
2. 实现 `SelectionController` (新增)
   - 从 `SelectionManager` 提取鼠标选择逻辑
   - 实现双击选词、三击选行
   - 实现拖动选择
3. 实现 `FrameCaret` (从 `SelectionManager` 提取)
   - 复用光标闪烁逻辑
   - 复用光标渲染逻辑

### 阶段 3: 编辑上下文 (复用现有代码)

1. 实现 `EditContext` (重构自 `ContentEditableController`)
   - 复用文本插入、删除逻辑
   - 添加撤销/重做栈
   - 添加命令执行机制
2. 实现 `Editable` 接口
3. 迁移 `HTMLInputElement` 到新系统

### 阶段 4: 统一入口

1. 实现 `InputController`
   - 整合所有子系统
   - 实现事件分发流程
   - 实现 Hit Testing
2. 实现 `KeyboardEventManager`
3. 修改 `Window` 使用 `InputController`

### 阶段 5: 全面迁移

1. 迁移 `HTMLTextAreaElement`
2. 迁移 `contentEditable` 支持
3. 迁移 `Terminal` 元素
4. 验证所有 JavaScript API 兼容性

### 阶段 6: 清理和优化

1. 删除旧的 `FocusManager`
2. 删除旧的 `SelectionManager`
3. 删除元素中的冗余编辑代码
4. 性能优化
5. 更新文档

## 文件结构

```
core/
├── input/                          # 新的输入管理目录
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── input_controller.h          # 参考 Blink EventHandler
│   ├── input_controller.cpp
│   ├── focus_controller.h          # 参考 Blink FocusController
│   ├── focus_controller.cpp        # 可复用 focus_manager.cpp 逻辑
│   ├── selection_controller.h      # 参考 Blink SelectionController (新增)
│   ├── selection_controller.cpp
│   ├── frame_selection.h           # 参考 Blink FrameSelection
│   ├── frame_selection.cpp         # 可复用 selection_manager.cpp 逻辑
│   ├── edit_context.h              # 参考 Blink Editor
│   ├── edit_context.cpp            # 可复用 contenteditable_controller.cpp 逻辑
│   ├── frame_caret.h               # 参考 Blink FrameCaret
│   ├── frame_caret.cpp
│   ├── position.h                  # 参考 Blink Position
│   ├── position.cpp
│   ├── editable.h                  # 可编辑接口定义
│   └── keyboard_event_manager.h    # 参考 Blink KeyboardEventManager
│   └── keyboard_event_manager.cpp
```

### 与现有代码的关系

| 新组件 | 现有代码 | 关系 |
|-------|---------|------|
| `FocusController` | `core/event/input/focus_manager.h` | 重构，复用大部分逻辑 |
| `FrameSelection` | `core/editing/selection_manager.h` | 重构，复用选区管理逻辑 |
| `SelectionController` | `core/editing/selection_manager.h` | 新增，提取鼠标选择逻辑 |
| `EditContext` | `core/editing/contenteditable_controller.h` | 重构，复用编辑命令逻辑 |
| `InputController` | `core/event/input/input_handler.h` | 新增，作为统一入口 |
| `FrameCaret` | `core/editing/selection_manager.h` | 新增，提取光标渲染逻辑 |

### 迁移策略

1. **阶段 1**: 创建新的 `core/input/` 目录，实现基础框架
2. **阶段 2**: 将 `FocusManager` 重构为 `FocusController`，保持 API 兼容
3. **阶段 3**: 将 `SelectionManager` 拆分为 `FrameSelection` + `SelectionController`
4. **阶段 4**: 创建 `InputController` 作为统一入口
5. **阶段 5**: 逐步迁移各元素到新系统
6. **阶段 6**: 删除旧代码
