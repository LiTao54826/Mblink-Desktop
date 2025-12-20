# Layer 系统架构设计

## 目标效果

| 场景 | 当前性能 | 目标性能 | 提升 |
|------|----------|----------|------|
| 窗口 resize (有 shadow) | 2000-3000ms | 30-50ms | 50-100x |
| 滚动 | 卡顿 | 60fps | - |
| transform/opacity 动画 | 每帧重绘 | GPU 合成 | 10-50x |
| 局部更新 (hover 等) | 全量重绘 | 局部重绘 | 5-10x |

---

## 架构概览

```
┌─────────────────────────────────────────────────────────┐
│                      Window                              │
│  ┌─────────────────────────────────────────────────┐    │
│  │              LayerTreeHost                       │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐           │    │
│  │  │ Layer 1 │ │ Layer 2 │ │ Layer 3 │  ...      │    │
│  │  │(root)   │ │(shadow) │ │(scroll) │           │    │
│  │  └────┬────┘ └────┬────┘ └────┬────┘           │    │
│  │       │           │           │                 │    │
│  │       ▼           ▼           ▼                 │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐           │    │
│  │  │SkSurface│ │SkSurface│ │SkSurface│  (缓存)   │    │
│  │  └─────────┘ └─────────┘ └─────────┘           │    │
│  └─────────────────────────────────────────────────┘    │
│                         │                                │
│                         ▼                                │
│              ┌─────────────────┐                        │
│              │   Compositor    │  (合成所有层)           │
│              └────────┬────────┘                        │
│                       ▼                                  │
│              ┌─────────────────┐                        │
│              │  Final Surface  │  (输出到屏幕)          │
│              └─────────────────┘                        │
└─────────────────────────────────────────────────────────┘
```

---

## 核心类设计

### 1. Layer (图层)

```cpp
class Layer {
public:
    // 图层类型
    enum class Type {
        kNormal,        // 普通内容层
        kShadow,        // 阴影层 (box-shadow)
        kTransform,     // 有 transform 的层
        kOpacity,       // 有 opacity 的层
        kScroll,        // 滚动内容层
        kFixed,         // position: fixed
    };

    // 核心属性
    Type type_;
    SkRect bounds_;                    // 层边界
    sk_sp<SkSurface> surface_;         // 离屏缓存
    sk_sp<SkPicture> picture_;         // 绘制命令缓存
    bool needs_repaint_ = true;        // 是否需要重绘
    bool needs_composite_ = true;      // 是否需要重新合成
    
    // 变换属性 (合成时应用，不触发重绘)
    SkMatrix transform_;
    float opacity_ = 1.0f;
    SkRect clip_;
    
    // 关联
    RenderObject* owner_;              // 拥有此层的渲染对象
    Layer* parent_;
    std::vector<Layer*> children_;
    
    // 方法
    void Paint();                      // 绘制到 surface_
    void Invalidate();                 // 标记需要重绘
    void InvalidateRect(SkRect);       // 局部失效
    bool HitTest(float x, float y);    // 命中测试
};
```

### 2. LayerTreeHost (图层树管理器)

```cpp
class LayerTreeHost {
public:
    // 构建图层树
    void BuildLayerTree(RenderObject* root);
    
    // 更新
    void UpdateLayers();               // 重绘脏层
    void Composite(SkCanvas* output);  // 合成到输出
    
    // 优化
    void SetNeedsComposite();          // 标记需要合成
    void SetNeedsRepaint(Layer*);      // 标记层需要重绘
    
private:
    Layer* root_layer_;
    std::vector<Layer*> layers_;       // 所有层 (按 z-order)
    bool needs_composite_ = false;
};
```

### 3. CompositingReasons (分层原因)

```cpp
enum class CompositingReason {
    kNone = 0,
    kBoxShadow = 1 << 0,           // 有 box-shadow
    kTransform3D = 1 << 1,         // 有 3D transform
    kWillChangeTransform = 1 << 2, // will-change: transform
    kWillChangeOpacity = 1 << 3,   // will-change: opacity
    kOpacityAnimation = 1 << 4,    // opacity 动画中
    kTransformAnimation = 1 << 5,  // transform 动画中
    kOverflowScroll = 1 << 6,      // overflow: scroll/auto
    kPositionFixed = 1 << 7,       // position: fixed
    kBackdropFilter = 1 << 8,      // backdrop-filter
    kOverlap = 1 << 9,             // 与已分层元素重叠
};

// 判断是否需要独立层
CompositingReason GetCompositingReasons(RenderObject*);
```

---

## 工作流程

### 初始渲染

```
1. BuildLayerTree()
   - 遍历 RenderObject 树
   - 根据 CompositingReasons 决定分层
   - 创建 Layer 对象

2. UpdateLayers()
   - 对每个 needs_repaint_ 的层
   - 调用 Paint() 绘制到 surface_

3. Composite()
   - 按 z-order 遍历所有层
   - 应用 transform/opacity
   - drawImage 到最终 surface
```

### 增量更新 (如 hover)

```
1. RenderObject 标记 needs_paint_
2. 找到对应的 Layer
3. Layer.Invalidate() 或 InvalidateRect()
4. 只重绘该层
5. Composite() 合成所有层
```

### Resize

```
1. 根层尺寸变化
2. 重新布局
3. 各层 surface_ 按需 resize
4. 只重绘尺寸变化的层
5. Composite()

关键优化：box-shadow 层的 surface_ 缓存不变，
只是合成位置变化
```

---

## 分阶段实现计划

### Phase 1: 基础 Layer 类 (2天)

```
目标：实现 Layer 基类和 SkSurface 缓存
文件：core/render/layer.h, layer.cpp

- Layer 类基本结构
- SkSurface 创建和管理
- 简单的 Paint/Composite
```

### Phase 2: LayerTreeHost (2天)

```
目标：实现图层树构建和管理
文件：core/render/layer_tree_host.h, layer_tree_host.cpp

- BuildLayerTree 算法
- 层的增删改
- 基本的合成流程
```

### Phase 3: 分层策略 (2天)

```
目标：实现 CompositingReasons 判断
文件：core/render/compositing_reasons.h, cpp

- box-shadow 触发分层
- transform/opacity 触发分层
- overflow:scroll 触发分层
```

### Phase 4: 集成到 Window (2天)

```
目标：替换现有的 Paint 流程
文件：修改 window.cpp, render_object.cpp

- Window 持有 LayerTreeHost
- RenderObject::Paint 改为绘制到 Layer
- 合成输出到屏幕
```

### Phase 5: 脏区域优化 (2天)

```
目标：实现局部重绘
文件：core/render/dirty_region.h (已有，增强)

- 层级别的脏标记
- 矩形级别的脏区域
- 增量合成
```

### Phase 6: 动画优化 (2天)

```
目标：transform/opacity 动画不触发重绘
文件：修改 animation_controller.cpp

- 动画只更新 Layer 的 transform_/opacity_
- 只触发 Composite，不触发 Paint
```

---

## 工期总结

| 阶段 | 工期 | 累计 | 效果 |
|------|------|------|------|
| Phase 1 | 2天 | 2天 | 基础框架 |
| Phase 2 | 2天 | 4天 | 可运行 |
| Phase 3 | 2天 | 6天 | box-shadow 优化生效 |
| Phase 4 | 2天 | 8天 | 完整集成 |
| Phase 5 | 2天 | 10天 | 局部更新 |
| Phase 6 | 2天 | 12天 | 动画优化 |

**总工期：2-3 周**

---

## 风险和注意事项

### 内存开销
- 每个层一个 SkSurface，增加 GPU/CPU 内存
- 需要实现层合并策略，避免层爆炸

### 兼容性
- 需要保证现有功能不受影响
- 建议用 feature flag 控制，可回退

### 调试
- 需要添加层可视化工具
- 显示层边界、重绘区域等

---

## 替代方案：简化版 (1周)

如果完整 Layer 系统工期太长，可以先做简化版：

**只缓存 box-shadow**：
- 不做完整分层
- 只对有 box-shadow 的元素缓存 SkPicture
- 尺寸不变时复用缓存

工期：3-4 天，效果：解决 80% 的 shadow 性能问题
