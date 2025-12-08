# MBink 增量渲染与局部重绘开发计划

> **版本**: 1.0
> **创建日期**: 2025-12-08
> **目标**: 实现真正的 Virtual DOM diff 和局部重绘功能

## 1. 背景与问题分析

### 1.1 当前架构

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Preact    │ -> │  DOM Tree   │ -> │ Render Tree │ -> │   Canvas    │
│  (VNode)    │    │  (Node)     │    │(RenderObject)│   │   (Skia)    │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
      │                  │                  │                  │
      │   Virtual DOM    │   DOM 操作       │   布局计算        │   绘制
      │     diff         │                  │                  │
      └──────────────────┴──────────────────┴──────────────────┘
                              全量重建/重绘
```

### 1.2 当前问题

| 问题 | 影响 | 代码位置 |
|------|------|----------|
| 节点增删触发 `InvalidateRenderTree()` | 渲染树完全重建 | `window.cpp:73,80` |
| 属性变化只调用 `SetNeedsRepaint()` | 无精确脏区域 | `window.cpp:89` |
| 渲染树无效时走全量路径 | 失去增量优势 | `window.cpp:1233-1236` |
| RenderObject 与 DOM 节点未双向绑定 | 无法增量更新 | - |

### 1.3 目标架构

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Preact    │ -> │  DOM Tree   │ -> │ Render Tree │ -> │   Canvas    │
│  (VNode)    │    │  (Node)     │    │(RenderObject)│   │   (Skia)    │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
      │                  │                  │                  │
      │   Virtual DOM    │   精确变更       │   增量布局        │  局部绘制
      │     diff         │   通知           │                  │
      └──────────────────┴──────────────────┴──────────────────┘
                              增量更新
```

## 2. 开发阶段

### Phase 1: 属性/样式变化的局部重绘 (预计 2-3 天)

**目标**: 属性或样式变化时，只重绘受影响的元素区域

#### 1.1 DOM 节点与 RenderObject 双向绑定

```cpp
// Node 类添加
class Node {
    std::weak_ptr<RenderObject> render_object_;  // 关联的渲染对象
public:
    void SetRenderObject(std::shared_ptr<RenderObject> ro);
    std::shared_ptr<RenderObject> GetRenderObject() const;
};

// RenderObject 类已有 node_ 指针
```

**修改文件**:
- `core/dom/node.h` - 添加 render_object_ 成员
- `core/dom/node.cpp` - 实现 getter/setter
- `core/render/render_tree_builder.cpp` - 构建时建立双向绑定

#### 1.2 精确脏区域标记

```cpp
// 修改 WindowDOMObserver
void OnAttributeChanged(Element* element, ...) override {
    if (window_ && !IsInBatch(element)) {
        // 获取关联的 RenderObject
        auto render_obj = element->GetRenderObject();
        if (render_obj) {
            // 标记该 RenderObject 需要重绘
            render_obj->MarkNeedsPaint();
            // 记录脏矩形
            element->SetDirtyRect(render_obj->GetBoundingRect());
        }
        window_->SetNeedsRepaint();
        // 不调用 InvalidateRenderTree()!
    }
}
```

**修改文件**:
- `core/window/window.cpp` - 修改 `WindowDOMObserver`
- `core/render/render_object.h/cpp` - 添加 `MarkNeedsPaint()`, `GetBoundingRect()`

#### 1.3 增量绘制优化

```cpp
void Window::RenderDocumentIncremental() {
    // 收集所有脏 RenderObject 的边界框
    std::vector<SkRect> dirty_rects;
    CollectDirtyRenderObjects(cached_render_tree_.get(), dirty_rects);
    
    // 合并相邻脏区域
    OptimizeDirtyRects(dirty_rects);
    
    // 只绘制脏区域
    for (const auto& rect : dirty_rects) {
        canvas->save();
        canvas->clipRect(rect);
        PaintRenderObject(cached_render_tree_.get(), canvas, rect);
        canvas->restore();
    }
}
```

**修改文件**:
- `core/window/window.cpp` - 优化 `RenderDocumentIncremental()`

#### 1.4 验收标准

- [ ] 鼠标悬停按钮时，只有按钮区域重绘
- [ ] 修改元素 style 属性时，只有该元素区域重绘
- [ ] 性能测试：100个按钮场景，hover 时 FPS > 60

---

### Phase 2: 文本内容变化的局部重绘 (预计 1-2 天)

**目标**: textContent 变化时，只重绘文本区域

#### 2.1 文本节点变更检测

```cpp
void OnTextContentChanged(Node* node, const std::string& old_text, 
                          const std::string& new_text) override {
    auto render_obj = node->GetRenderObject();
    if (!render_obj) {
        render_obj = node->GetParentNode()->GetRenderObject();
    }
    
    if (render_obj) {
        // 标记需要重新布局（文本可能改变尺寸）
        render_obj->MarkNeedsLayout();
        render_obj->MarkNeedsPaint();
    }
}
```

#### 2.2 增量布局

```cpp
void LayoutDirtySubtree(RenderObject* obj) {
    if (!obj->NeedsLayout()) {
        // 递归检查子节点
        for (auto& child : obj->GetChildren()) {
            LayoutDirtySubtree(child.get());
        }
        return;
    }
    
    // 重新布局该子树
    obj->Layout(available_width, available_height);
    obj->ClearNeedsLayout();
}
```

**修改文件**:
- `core/dom/text.cpp` - 文本变化通知
- `core/layout/native_layout_engine.cpp` - 增量布局支持

#### 2.3 验收标准

- [ ] 修改按钮文字时，只有按钮区域重新布局和绘制
- [ ] 计数器组件更新时，只有数字区域重绘

---

### Phase 3: 节点增删的增量渲染树更新 (预计 3-5 天)

**目标**: DOM 节点增删时，只更新渲染树的对应部分

#### 3.1 渲染树增量更新接口

```cpp
class RenderTreeUpdater {
public:
    // 插入新节点的渲染对象
    void InsertRenderObject(Node* node, Node* parent, Node* reference);
    
    // 移除节点的渲染对象
    void RemoveRenderObject(Node* node);
    
    // 移动节点的渲染对象
    void MoveRenderObject(Node* node, Node* new_parent, Node* reference);
    
private:
    // 为单个节点创建 RenderObject
    std::shared_ptr<RenderObject> CreateRenderObjectForNode(Node* node);
    
    // 更新祖先链的布局
    void InvalidateAncestorLayout(RenderObject* obj);
};
```

#### 3.2 DOM 观察者修改

```cpp
void OnNodeAdded(Node* node, Node* parent) override {
    if (window_ && !IsInBatch(node)) {
        // 增量更新渲染树，而不是完全重建
        render_tree_updater_->InsertRenderObject(node, parent, nullptr);
        
        // 标记父节点需要重新布局
        if (auto parent_ro = parent->GetRenderObject()) {
            parent_ro->MarkNeedsLayout();
        }
        
        window_->SetNeedsRepaint();
        // 不再调用 InvalidateRenderTree()!
    }
}

void OnNodeRemoved(Node* node, Node* parent) override {
    if (window_ && !IsInBatch(node)) {
        // 记录被删除节点的区域（用于清除绘制）
        if (auto ro = node->GetRenderObject()) {
            window_->AddDirtyRect(ro->GetBoundingRect());
        }
        
        // 增量更新渲染树
        render_tree_updater_->RemoveRenderObject(node);
        
        window_->SetNeedsRepaint();
    }
}
```

#### 3.3 布局树增量更新

Taffy 布局引擎需要支持增量更新：

```cpp
class NativeLayoutEngine {
public:
    // 增量添加节点到布局树
    void InsertLayoutNode(RenderObject* obj, RenderObject* parent);
    
    // 从布局树移除节点
    void RemoveLayoutNode(RenderObject* obj);
    
    // 只重新计算受影响的子树
    void ComputeLayoutIncremental(RenderObject* dirty_root);
};
```

**修改文件**:
- `core/render/render_tree_updater.h/cpp` - 新建文件
- `core/window/window.cpp` - 修改 DOM 观察者
- `core/layout/native_layout_engine.h/cpp` - 增量布局支持

#### 3.4 验收标准

- [ ] 列表添加新项时，只创建新项的 RenderObject
- [ ] 列表删除项时，只移除对应的 RenderObject
- [ ] 条件渲染切换时，只更新变化的子树
- [ ] 性能测试：1000项列表添加/删除，响应时间 < 16ms

---

### Phase 4: Preact 深度集成 (预计 2-3 天)

**目标**: 优化 Preact 与渲染系统的集成

#### 4.1 批量更新优化

```javascript
// Preact 侧 - 使用 requestAnimationFrame 批量更新
function scheduleUpdate(component) {
    if (!pendingUpdates.has(component)) {
        pendingUpdates.add(component);
        
        if (!updateScheduled) {
            updateScheduled = true;
            requestAnimationFrame(flushUpdates);
        }
    }
}

function flushUpdates() {
    // 批量开始
    document.__beginBatch();
    
    for (const component of pendingUpdates) {
        component.__rerender();
    }
    pendingUpdates.clear();
    
    // 批量结束 - 触发一次渲染
    document.__endBatch();
    updateScheduled = false;
}
```

#### 4.2 DOM 操作批量化

```cpp
// C++ 侧 - 暴露批量操作 API
void Document::BeginBatch() {
    batch_depth_++;
}

void Document::EndBatch() {
    batch_depth_--;
    if (batch_depth_ == 0) {
        // 批量结束，触发一次渲染
        FlushPendingUpdates();
    }
}
```

#### 4.3 验收标准

- [ ] useState 连续多次调用，只触发一次渲染
- [ ] 多个组件同时更新，只触发一次渲染
- [ ] 性能测试：100个组件同时更新，FPS > 60

---

## 3. 技术细节

### 3.1 脏标记传播策略

```
节点变化
    │
    ▼
┌─────────────────────────────────────┐
│  标记当前节点脏 (PAINT/LAYOUT)       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│  如果是 LAYOUT 脏:                   │
│  - 向上传播到祖先（标记需要重新布局）   │
│  - 向下传播到子孙（位置可能改变）       │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│  如果是 PAINT 脏:                    │
│  - 不传播，只影响当前节点             │
└─────────────────────────────────────┘
```

### 3.2 脏区域合并策略

```cpp
// 合并策略：如果两个脏区域重叠或距离小于阈值，合并为一个
bool ShouldMerge(const SkRect& a, const SkRect& b) {
    // 扩展区域检测重叠
    SkRect expanded_a = a.makeOutset(MERGE_THRESHOLD, MERGE_THRESHOLD);
    return expanded_a.intersects(b);
}

// 当脏区域数量超过阈值时，直接全量重绘
if (dirty_rects.size() > MAX_DIRTY_RECTS) {
    dirty_rects.clear();
    dirty_rects.push_back(viewport_rect);
}
```

### 3.3 布局缓存策略

```cpp
struct LayoutCache {
    float width;
    float height;
    float x;
    float y;
    bool valid = false;
    
    // 检查是否可以复用
    bool CanReuse(float available_width, float available_height) const {
        return valid && 
               available_width == cached_available_width &&
               available_height == cached_available_height;
    }
};
```

## 4. 测试计划

### 4.1 单元测试

| 测试项 | 文件 | 描述 |
|--------|------|------|
| 脏区域计算 | `test_dirty_region.cpp` | 测试脏区域添加、合并、优化 |
| 增量布局 | `test_incremental_layout.cpp` | 测试局部布局更新 |
| 渲染树更新 | `test_render_tree_update.cpp` | 测试节点增删的渲染树更新 |

### 4.2 集成测试

| 测试项 | 描述 |
|--------|------|
| 计数器组件 | 点击按钮更新数字，验证只有数字区域重绘 |
| 列表组件 | 添加/删除列表项，验证增量更新 |
| 表单组件 | 输入文本，验证增量布局 |

### 4.3 性能测试

| 场景 | 目标 |
|------|------|
| 100个按钮 hover | FPS > 60 |
| 1000项列表滚动 | FPS > 30 |
| 计数器快速点击 | 响应时间 < 16ms |

## 5. 时间线

| 阶段 | 时间 | 交付物 |
|------|------|--------|
| Phase 1 | Day 1-3 | 属性/样式局部重绘 |
| Phase 2 | Day 4-5 | 文本内容局部重绘 |
| Phase 3 | Day 6-10 | 节点增删增量更新 |
| Phase 4 | Day 11-13 | Preact 深度集成 |
| 测试优化 | Day 14-15 | 性能优化与测试 |

**总计**: 约 15 个工作日

## 6. 风险与应对

| 风险 | 可能性 | 影响 | 应对措施 |
|------|--------|------|----------|
| Taffy 不支持增量布局 | 中 | 高 | 实现自定义增量布局包装器 |
| 脏区域计算错误 | 高 | 中 | 添加调试可视化工具 |
| 内存泄漏 | 中 | 高 | 使用 weak_ptr，添加泄漏检测 |
| 兼容性问题 | 低 | 中 | 保留全量渲染回退路径 |

## 7. 文件修改清单

### 7.1 需要修改的文件

```
core/dom/
├── node.h              # 添加 render_object_ 成员
├── node.cpp            # 实现 RenderObject 绑定
├── element.cpp         # 优化脏标记逻辑
└── text.cpp            # 文本变化通知

core/render/
├── render_object.h     # 添加增量更新接口
├── render_object.cpp   # 实现脏标记和边界框
├── render_tree_builder.cpp  # 双向绑定
└── dirty_region_collector.cpp  # 优化脏区域收集

core/window/
└── window.cpp          # 修改 DOM 观察者，优化渲染流程

core/layout/
└── native_layout_engine.cpp  # 增量布局支持

js/preact/
├── preact.js           # 批量更新优化
└── hooks.js            # setState 批量化
```

### 7.2 需要新建的文件

```
core/render/
├── render_tree_updater.h    # 渲染树增量更新器头文件
└── render_tree_updater.cpp  # 渲染树增量更新器实现

tests/
├── test_dirty_region.cpp         # 脏区域单元测试
├── test_incremental_layout.cpp   # 增量布局测试
└── test_render_tree_update.cpp   # 渲染树更新测试
```

## 8. 核心代码示例

### 8.1 RenderObject 脏标记系统

```cpp
// render_object.h
class RenderObject {
public:
    enum class DirtyFlag : uint32_t {
        NONE = 0,
        NEEDS_LAYOUT = 1 << 0,    // 需要重新布局
        NEEDS_PAINT = 1 << 1,     // 需要重新绘制
        CHILDREN_NEED_LAYOUT = 1 << 2,  // 子节点需要布局
    };

    void MarkNeedsLayout();
    void MarkNeedsPaint();
    void ClearDirtyFlags();

    bool NeedsLayout() const;
    bool NeedsPaint() const;

    // 获取边界框（用于脏区域计算）
    SkRect GetBoundingRect() const;

private:
    uint32_t dirty_flags_ = 0;
    SkRect cached_bounds_;
};
```

### 8.2 渲染树增量更新器

```cpp
// render_tree_updater.h
class RenderTreeUpdater {
public:
    explicit RenderTreeUpdater(std::shared_ptr<RenderObject> root);

    // 为新 DOM 节点创建 RenderObject 并插入树中
    void OnNodeInserted(Node* node, Node* parent);

    // 移除 DOM 节点对应的 RenderObject
    void OnNodeRemoved(Node* node);

    // 属性变化时更新 RenderObject
    void OnAttributeChanged(Element* element, const std::string& name);

    // 样式变化时更新 RenderObject
    void OnStyleChanged(Element* element);

private:
    std::shared_ptr<RenderObject> root_;

    // 创建单个节点的 RenderObject
    std::shared_ptr<RenderObject> CreateRenderObject(Node* node);

    // 查找父 RenderObject
    RenderObject* FindParentRenderObject(Node* node);

    // 查找参考位置
    RenderObject* FindReferenceRenderObject(Node* node);
};
```

### 8.3 增量布局接口

```cpp
// native_layout_engine.h 新增接口
class NativeLayoutEngine {
public:
    // 现有接口
    void BuildLayoutTree(std::shared_ptr<RenderObject> root);
    void ComputeLayout(float width, float height);
    void GetLayoutInfo(std::shared_ptr<RenderObject> render_tree);

    // 新增增量接口
    void InsertNode(RenderObject* obj);
    void RemoveNode(RenderObject* obj);
    void ComputeLayoutIncremental(RenderObject* dirty_subtree_root);

private:
    // Taffy 节点映射
    std::unordered_map<RenderObject*, TaffyNodeId> node_map_;
};
```

### 8.4 Window 渲染优化

```cpp
// window.cpp
void Window::RenderDocumentIncremental() {
    auto canvas = GetCanvas();
    if (!canvas) return;

    // Step 1: 检查渲染树状态
    if (!render_tree_valid_) {
        // 渲染树结构变化，需要增量更新
        render_tree_updater_->ProcessPendingUpdates();
        render_tree_valid_ = true;
    }

    // Step 2: 收集脏 RenderObject
    std::vector<RenderObject*> dirty_objects;
    CollectDirtyRenderObjects(cached_render_tree_.get(), dirty_objects);

    if (dirty_objects.empty()) {
        return;  // 无需重绘
    }

    // Step 3: 增量布局
    for (auto* obj : dirty_objects) {
        if (obj->NeedsLayout()) {
            layout_engine_->ComputeLayoutIncremental(obj);
        }
    }

    // Step 4: 计算脏区域
    DirtyRegion dirty_region;
    for (auto* obj : dirty_objects) {
        dirty_region.AddRect(obj->GetBoundingRect());
    }
    dirty_region.Optimize();

    // Step 5: 局部绘制
    for (const auto& rect : dirty_region.GetRegions()) {
        canvas->save();
        canvas->clipRect(rect);

        // 清除脏区域
        SkPaint clear_paint;
        clear_paint.setColor(SK_ColorWHITE);
        canvas->drawRect(rect, clear_paint);

        // 绘制与脏区域相交的 RenderObject
        PaintIntersecting(cached_render_tree_.get(), canvas, rect);

        canvas->restore();
    }

    // Step 6: 清除脏标记
    for (auto* obj : dirty_objects) {
        obj->ClearDirtyFlags();
    }

    needs_repaint_ = false;
}
```

### 8.5 Preact 批量更新

```javascript
// preact.js 修改
var pendingUpdates = new Set();
var updateScheduled = false;

function scheduleUpdate(component) {
    pendingUpdates.add(component);

    if (!updateScheduled) {
        updateScheduled = true;
        // 使用微任务批量更新
        Promise.resolve().then(flushUpdates);
    }
}

function flushUpdates() {
    // 通知 C++ 开始批量操作
    if (typeof document.__beginBatch === 'function') {
        document.__beginBatch();
    }

    // 执行所有待更新组件
    var updates = Array.from(pendingUpdates);
    pendingUpdates.clear();
    updateScheduled = false;

    for (var i = 0; i < updates.length; i++) {
        var component = updates[i];
        if (component.__rerender) {
            component.__rerender();
        }
    }

    // 通知 C++ 结束批量操作
    if (typeof document.__endBatch === 'function') {
        document.__endBatch();
    }
}

// 修改 hooks.js 中的 setState
function useState(initialValue) {
    // ... 现有代码 ...

    const setState = (newValue) => {
        // ... 计算新值 ...

        if (hookState.value !== nextValue) {
            hookState.value = nextValue;
            // 调度更新而非立即重渲染
            scheduleUpdate(component);
        }
    };

    return [hookState.value, setState];
}
```

## 9. 调试工具

### 9.1 脏区域可视化

```cpp
// 调试模式下绘制脏区域边框
#ifdef DEBUG_DIRTY_REGIONS
void Window::DrawDirtyRegionOverlay(SkCanvas* canvas,
                                    const DirtyRegion& dirty_region) {
    SkPaint debug_paint;
    debug_paint.setStyle(SkPaint::kStroke_Style);
    debug_paint.setColor(SK_ColorRED);
    debug_paint.setStrokeWidth(2.0f);

    for (const auto& rect : dirty_region.GetRegions()) {
        canvas->drawRect(rect, debug_paint);
    }
}
#endif
```

### 9.2 性能统计

```cpp
struct RenderStats {
    int full_paints = 0;        // 全量绘制次数
    int partial_paints = 0;     // 局部绘制次数
    int dirty_rects = 0;        // 脏区域数量
    float dirty_area_ratio = 0; // 脏区域占比
    double layout_time_ms = 0;  // 布局时间
    double paint_time_ms = 0;   // 绘制时间

    void Log() const {
        printf("[RenderStats] Full: %d, Partial: %d, DirtyRects: %d, "
               "DirtyRatio: %.2f%%, Layout: %.2fms, Paint: %.2fms\n",
               full_paints, partial_paints, dirty_rects,
               dirty_area_ratio * 100, layout_time_ms, paint_time_ms);
    }
};
```

## 10. 参考资料

- [Chromium Rendering Pipeline](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/life_of_a_pixel.md)
- [React Fiber Architecture](https://github.com/acdlite/react-fiber-architecture)
- [Flutter Rendering](https://flutter.dev/docs/resources/architectural-overview#rendering)
- [Taffy Layout Engine](https://github.com/DioxusLabs/taffy)
- [Skia Documentation](https://skia.org/docs/)

