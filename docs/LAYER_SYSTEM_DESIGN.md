# Layer 分层系统设计文档

## 1. 问题背景

当前的 OverlayManager 只解决了**绘制顺序**问题，但存在以下缺陷：

| 问题 | 现象 | 原因 |
|------|------|------|
| 点击穿透 | 点击 dropdown 选项无响应 | Hit Testing 不考虑 overlay |
| 滚动穿透 | dropdown 内滚动触发页面滚动 | 滚动事件路由到错误元素 |
| 事件错误 | hover 等事件发送到下层元素 | 事件系统不知道 overlay 存在 |

## 2. 设计目标

实现完整的 CSS Stacking Context，支持：
- ✅ 正确的绘制顺序（已有）
- 🔲 正确的 Hit Testing
- 🔲 正确的事件路由
- 🔲 滚动隔离
- 🔲 多层级支持

## 3. 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                      Window                              │
│  ┌───────────────────────────────────────────────────┐  │
│  │                  LayerManager                      │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 2: Modal (z >= 1000)                 │  │  │
│  │  │  - RenderObjects[]                          │  │  │
│  │  │  - HitTest() → 优先级最高                   │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 1: Overlay (z 100-999)               │  │  │
│  │  │  - RenderObjects[]                          │  │  │
│  │  │  - HitTest() → 次优先级                     │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 0: Base (z < 100)                    │  │  │
│  │  │  - 完整渲染树                               │  │  │
│  │  │  - HitTest() → 最低优先级                   │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 4. 核心类设计

### 4.1 Layer 类

```cpp
// core/render/layer.h
class Layer {
public:
    int GetZIndexMin() const;
    int GetZIndexMax() const;
    
    // 添加/移除元素
    void AddRenderObject(std::shared_ptr<RenderObject> obj, const SkMatrix& transform);
    void RemoveRenderObject(RenderObject* obj);
    void Clear();
    
    // 绘制
    void Paint(SkCanvas* canvas);
    
    // Hit Testing - 返回命中的元素
    HitTestResult HitTest(float x, float y);
    
    // 滚动处理
    bool HandleWheel(float x, float y, float delta_x, float delta_y);
    
private:
    int z_index_min_;
    int z_index_max_;
    std::vector<LayerItem> items_;  // 按 z-index 排序
};
```

### 4.2 LayerManager 类

```cpp
// core/render/layer_manager.h
class LayerManager {
public:
    static LayerManager& Instance();
    
    // 帧管理
    void BeginFrame();
    void EndFrame();
    
    // 元素收集（在 Paint 过程中调用）
    bool ShouldCollect(const RenderObject* obj) const;
    void Collect(std::shared_ptr<RenderObject> obj, const SkMatrix& transform, int z_index);
    
    // 绘制所有 Layer
    void PaintLayers(SkCanvas* canvas);
    
    // Hit Testing - 从最高 Layer 开始
    HitTestResult HitTest(float x, float y);
    
    // 滚动处理 - 从最高 Layer 开始
    bool HandleWheel(float x, float y, float delta_x, float delta_y);
    
    // 获取特定 Layer
    Layer* GetLayer(int level);
    
private:
    std::vector<std::unique_ptr<Layer>> layers_;
    // Layer 0: z < 100 (base)
    // Layer 1: z 100-999 (overlay)
    // Layer 2: z >= 1000 (modal)
};
```

## 5. 实现计划

### Phase 1: 重构 OverlayManager → LayerManager (1-2h)

**目标**: 保持现有功能，重构为 Layer 架构

1. 创建 `Layer` 类
2. 创建 `LayerManager` 类（替代 OverlayManager）
3. 迁移现有的收集和绘制逻辑
4. 更新 `render_object.cpp` 和 `window.cpp` 的调用

**验证**: 绘制功能不变，dropdown 仍然显示在最上层

### Phase 2: 实现 Layer Hit Testing (1-2h)

**目标**: 点击 overlay 元素能正确响应

1. 在 `Layer` 中实现 `HitTest()`
2. 在 `LayerManager` 中实现从高到低的 Hit Testing
3. 修改 `HitTesting::HitTestRenderObject()` 调用 LayerManager
4. 修改 `EventLoop` 使用新的 Hit Testing

**验证**: 点击 dropdown 选项能正确选中

### Phase 3: 实现滚动隔离 (1h)

**目标**: overlay 内滚动不影响页面

1. 在 `Layer` 中实现 `HandleWheel()`
2. 在 `LayerManager` 中实现滚动事件路由
3. 修改 `EventLoop::HandleMouseWheelEventForDOM()`

**验证**: 在 dropdown 上滚动不会滚动页面

### Phase 4: 完善事件系统 (1h)

**目标**: hover、focus 等事件正确路由

1. 修改 `UpdateHoverChain` 使用 LayerManager
2. 确保 mouseenter/mouseleave 正确触发

**验证**: hover 效果在 dropdown 选项上正常工作

## 6. 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `core/render/layer.h` | 新增 | Layer 类定义 |
| `core/render/layer.cpp` | 新增 | Layer 类实现 |
| `core/render/layer_manager.h` | 新增 | LayerManager 类定义 |
| `core/render/layer_manager.cpp` | 新增 | LayerManager 类实现 |
| `core/render/overlay_manager.h` | 删除 | 被 LayerManager 替代 |
| `core/render/overlay_manager.cpp` | 删除 | 被 LayerManager 替代 |
| `core/render/render_object.cpp` | 修改 | 使用 LayerManager |
| `core/window/window.cpp` | 修改 | 使用 LayerManager |
| `core/event/hit_testing.cpp` | 修改 | 集成 LayerManager |
| `core/event/event_loop.cpp` | 修改 | 使用 LayerManager 处理事件 |
| `core/render/CMakeLists.txt` | 修改 | 更新文件列表 |

## 7. 测试用例

```javascript
// test_layer_system.js
// 1. 点击测试：点击 dropdown 选项应该选中
// 2. 滚动测试：在 dropdown 上滚动不应该滚动页面
// 3. Hover 测试：鼠标移到选项上应该高亮
// 4. 多层测试：打开 Modal，Modal 应该在 dropdown 之上
```

## 8. 预计工时

| Phase | 工时 | 累计 |
|-------|------|------|
| Phase 1 | 1-2h | 1-2h |
| Phase 2 | 1-2h | 2-4h |
| Phase 3 | 1h | 3-5h |
| Phase 4 | 1h | 4-6h |

**总计**: 4-6 小时
