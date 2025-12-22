# 设计文档

## 概述

Layer 分层系统是对现有 OverlayManager 的重构和增强，实现完整的 CSS Stacking Context 支持。系统将元素按 z-index 分为三个层级（Base、Overlay、Modal），并提供统一的绘制、Hit Testing 和事件处理接口。

核心目标：
- 保持现有绘制功能不变
- 新增 Layer 级别的 Hit Testing
- 新增滚动事件隔离
- 集成到现有事件系统

## 架构

```
┌─────────────────────────────────────────────────────────┐
│                      Window                              │
│  ┌───────────────────────────────────────────────────┐  │
│  │                  LayerManager                      │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 2: Modal (z >= 1000)                 │  │  │
│  │  │  - LayerItem[]                              │  │  │
│  │  │  - HitTest() → 优先级最高                   │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 1: Overlay (z 100-999)               │  │  │
│  │  │  - LayerItem[]                              │  │  │
│  │  │  - HitTest() → 次优先级                     │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Layer 0: Base (z < 100)                    │  │  │
│  │  │  - 完整渲染树                               │  │  │
│  │  │  - HitTest() → 最低优先级                   │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
│                                                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │                   EventLoop                        │  │
│  │  - 使用 LayerManager::HitTest() 进行点击测试      │  │
│  │  - 使用 LayerManager::HandleWheel() 处理滚动      │  │
│  │  - 更新 hover chain                               │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 组件和接口

### LayerLevel 枚举

```cpp
enum class LayerLevel {
    Base = 0,      // z-index < 100: 普通元素
    Overlay = 1,   // z-index 100-999: dropdown, tooltip, popover
    Modal = 2,     // z-index >= 1000: modal, dialog
    Count = 3
};
```

### LayerItem 结构

```cpp
struct LayerItem {
    std::shared_ptr<RenderObject> render_obj;  // 渲染对象
    SkMatrix transform;                         // 收集时的变换矩阵
    int z_index;                                // z-index 值
    float abs_x, abs_y;                         // 绝对坐标
    float width, height;                        // 尺寸
};
```

### Layer 类接口

```cpp
class Layer {
public:
    Layer(int z_min, int z_max = -1);
    
    // 属性
    int GetZIndexMin() const;
    int GetZIndexMax() const;
    bool ContainsZIndex(int z_index) const;
    bool IsEmpty() const;
    size_t GetItemCount() const;
    
    // 元素管理
    void AddItem(std::shared_ptr<RenderObject> obj, const SkMatrix& transform, int z_index);
    void Clear();
    
    // 核心功能
    void Paint(SkCanvas* canvas);
    bool HitTest(float x, float y, HitTestResult& result);
    bool HandleWheel(float x, float y, float delta_x, float delta_y);
};
```

### LayerManager 类接口

```cpp
class LayerManager {
public:
    static LayerManager& Instance();
    
    // 帧管理
    void BeginFrame();
    
    // 元素收集
    bool ShouldCollect(const RenderObject* render_obj) const;
    void Collect(std::shared_ptr<RenderObject> render_obj, const SkMatrix& transform, int z_index);
    
    // 核心功能
    void PaintLayers(SkCanvas* canvas);
    bool HitTest(float x, float y, HitTestResult& result);
    bool HandleWheel(float x, float y, float delta_x, float delta_y);
    
    // 状态查询
    bool HasOverlays() const;
    bool IsPaintingLayers() const;
    Layer* GetLayer(LayerLevel level);
    
    // 配置
    void SetOverlayThreshold(int threshold);
    void SetModalThreshold(int threshold);
};
```

### HitTesting 类修改

```cpp
class HitTesting {
public:
    // 新增：使用 LayerManager 的 Hit Testing
    HitTestResult HitTestWithLayers(std::shared_ptr<Document> document, float x, float y);
    
    // 保留原有接口用于 Base 层
    HitTestResult HitTest(std::shared_ptr<Document> document, float x, float y);
    HitTestResult HitTestRenderObject(...);
};
```

## 数据模型

### Layer 数据流

```
RenderObject::Paint()
    │
    ├─ ShouldCollect() == true?
    │       │
    │       └─ Collect() → LayerItem → Layer[level]
    │
    └─ ShouldCollect() == false?
            │
            └─ 正常绘制到 Base 层
```

### Hit Testing 数据流

```
EventLoop::HandleMouseEvent()
    │
    └─ LayerManager::HitTest(x, y)
            │
            ├─ Layer[Modal]::HitTest() → 命中? 返回
            │
            ├─ Layer[Overlay]::HitTest() → 命中? 返回
            │
            └─ HitTesting::HitTestRenderObject() → Base 层
```

### 滚动事件数据流

```
EventLoop::HandleMouseWheelEvent()
    │
    └─ LayerManager::HandleWheel(x, y, dx, dy)
            │
            ├─ Layer[Modal]::HandleWheel() → 处理? 返回 true
            │
            ├─ Layer[Overlay]::HandleWheel() → 处理? 返回 true
            │
            └─ 返回 false → 传递到 Base 层
```

## 正确性属性

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. 
Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: BeginFrame 清除所有 Layer
*For any* LayerManager 状态，调用 BeginFrame() 后，所有 Layer 的 ItemCount 应该为 0
**Validates: Requirements 1.1**

### Property 2: 元素收集到正确的 Layer
*For any* RenderObject 的 z-index 和 position 组合，如果 z-index >= 100 且 position 为 positioned，则元素应该被收集到对应的 Layer（Overlay 或 Modal）
**Validates: Requirements 1.2**

### Property 3: Layer 绘制顺序
*For any* 包含多个 Layer 元素的 LayerManager，PaintLayers 应该按 Base -> Overlay -> Modal 的顺序调用各 Layer 的 Paint
**Validates: Requirements 1.3**

### Property 4: Layer 内 z-index 排序
*For any* 包含多个元素的 Layer，Paint 时应该按 z-index 升序绘制，HitTest 时应该按 z-index 降序测试
**Validates: Requirements 1.4, 2.4, 5.2**

### Property 5: Layer 优先级 Hit Testing
*For any* 在多个 Layer 都有重叠元素的情况下，HitTest 应该返回最高层 Layer 中的元素
**Validates: Requirements 2.1, 2.2, 5.1**

### Property 6: Hit Testing 穿透
*For any* 高层 Layer 没有命中元素的点击位置，HitTest 应该返回低层 Layer 中的元素
**Validates: Requirements 2.3**

### Property 7: pointer-events: none 跳过
*For any* 设置了 pointer-events: none 的元素，HitTest 应该跳过该元素并继续测试其他元素
**Validates: Requirements 2.5**

### Property 8: 滚动事件路由
*For any* 在 Overlay 层可滚动元素上的滚动操作，HandleWheel 应该返回 true 并更新该元素的滚动位置
**Validates: Requirements 3.1, 3.4**

### Property 9: 滚动事件阻止
*For any* HandleWheel 返回 true 的情况，滚动事件不应该传递到 Base 层
**Validates: Requirements 3.2**

### Property 10: 滚动事件穿透
*For any* Overlay 层没有可滚动元素命中的情况，HandleWheel 应该返回 false
**Validates: Requirements 3.3**

### Property 11: HasOverlays 状态查询
*For any* LayerManager 状态，HasOverlays() 应该返回 Overlay 层或 Modal 层是否有元素
**Validates: Requirements 5.3**

### Property 12: 兼容性 - 元素收集
*For any* 相同的 RenderObject 输入，LayerManager::ShouldCollect() 应该与 OverlayManager::ShouldDeferPaint() 返回相同结果
**Validates: Requirements 6.1, 6.2**

## 错误处理

| 错误场景 | 处理方式 |
|---------|---------|
| render_obj 为 nullptr | AddItem/Collect 直接返回，不添加 |
| canvas 为 nullptr | Paint 直接返回，不绘制 |
| Layer 为空 | HitTest/HandleWheel 返回 false |
| z-index 超出范围 | 使用默认 Layer（Base） |
| 变换矩阵无效 | 使用单位矩阵 |

## 测试策略

### 单元测试

使用 Google Test 框架进行单元测试：

1. **Layer 类测试**
   - AddItem 正确添加元素
   - Clear 清除所有元素
   - ContainsZIndex 正确判断范围
   - Paint 按 z-index 顺序绘制
   - HitTest 从高到低测试

2. **LayerManager 类测试**
   - BeginFrame 清除所有 Layer
   - ShouldCollect 正确判断
   - Collect 分发到正确 Layer
   - PaintLayers 按层级顺序
   - HitTest 按优先级返回
   - HandleWheel 正确路由

### 属性测试

使用 RapidCheck 库进行属性测试，每个属性测试运行至少 100 次迭代：

1. **Property 1-4**: Layer 基础功能属性
2. **Property 5-7**: Hit Testing 属性
3. **Property 8-10**: 滚动事件属性
4. **Property 11-12**: 状态查询和兼容性属性

每个属性测试必须使用以下格式标注：
```cpp
// **Feature: layer-system, Property {number}: {property_text}**
// **Validates: Requirements X.Y**
```

### 集成测试

1. **dropdown 点击测试**: 验证点击 dropdown 选项能正确选中
2. **滚动隔离测试**: 验证在 dropdown 上滚动不会滚动页面
3. **hover 测试**: 验证鼠标移到选项上能正确高亮
4. **多层测试**: 验证 Modal 在 dropdown 之上

## 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `core/render/layer.h` | 已存在 | Layer 类定义 |
| `core/render/layer.cpp` | 已存在 | Layer 类实现 |
| `core/render/layer_manager.h` | 已存在 | LayerManager 类定义 |
| `core/render/layer_manager.cpp` | 新增 | LayerManager 类实现 |
| `core/render/overlay_manager.h` | 保留 | 暂时保留，后续删除 |
| `core/render/overlay_manager.cpp` | 保留 | 暂时保留，后续删除 |
| `core/event/hit_testing.h` | 修改 | 新增 HitTestWithLayers |
| `core/event/hit_testing.cpp` | 修改 | 集成 LayerManager |
| `core/event/event_loop.cpp` | 修改 | 使用 LayerManager |
| `core/render/CMakeLists.txt` | 修改 | 添加 layer_manager.cpp |
| `tests/property/layer_system_test.cpp` | 新增 | 属性测试 |
