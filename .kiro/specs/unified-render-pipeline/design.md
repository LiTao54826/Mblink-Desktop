# 统一渲染管线重构 - 设计文档

## 1. 背景与目标

### 1.1 当前问题

当前渲染系统存在以下问题：

1. **架构分裂**：存在两套独立的渲染管线
   - `RenderPipelineLegacy` (V1)：负责 DOM 同步、样式、布局、绘制
   - `RenderPipelineV2`：负责层树构建、光栅化、合成

2. **冗余适配层**：`WindowCompositorAdapter` 作为 V2 的包装器，增加了不必要的间接层

3. **Window 类臃肿**：Window 类直接持有大量渲染相关组件，职责不清晰

4. **代码重复**：两套管线有重复的脏标记、状态管理逻辑

### 1.2 目标

将 V1 和 V2 **合并**为一个真正统一的 `RenderPipeline`：

- **单一入口**：所有渲染逻辑通过一个管线处理
- **完整流程**：DOM同步 → 样式 → 布局 → 层树 → 光栅化 → 合成
- **删除冗余**：移除 `WindowCompositorAdapter`、旧版管线文件
- **简化 Window**：Window 只持有一个 `RenderPipeline` 实例

---

## 2. 现有代码分析

### 2.1 RenderPipelineLegacy (V1) 职责

```
文件：core/render/render_pipeline_legacy.h/cpp

职责：
├── SetDirtyTracker()      - 设置脏节点追踪器
├── SetRenderTree()        - 设置渲染树
├── SetLayoutEngine()      - 设置布局引擎
├── SetDocument()          - 设置文档
├── SetSynchronizer()      - 设置渲染树同步器
├── SetViewportSize()      - 设置视口尺寸
├── SetPaintCallback()     - 设置绘制回调
│
├── MarkNeedsStyleRecalc() - 标记需要样式重算
├── MarkNeedsLayout()      - 标记需要布局
├── MarkNeedsPaint()       - 标记需要绘制
├── NeedsUpdate()          - 检查是否需要更新
│
└── ProcessFrame()         - 处理一帧
    ├── DoRenderTreeSync() - 渲染树同步
    ├── DoStyleRecalc()    - 样式重算
    ├── DoLayout()         - 布局计算
    └── DoPaint()          - 绘制

依赖组件：
- DirtyNodeTracker        - 脏节点追踪
- RenderTreeSynchronizer  - 渲染树同步
- NativeLayoutEngine      - 布局引擎
- Document                - 文档对象
```

### 2.2 RenderPipelineV2 职责

```
文件：core/compositor/render_pipeline_v2.h/cpp

职责：
├── Initialize()           - 初始化管线
├── Resize()               - 调整视口大小
├── Shutdown()             - 关闭管线
│
├── Render()               - 渲染一帧（到屏幕）
├── RenderToCanvas()       - 渲染到 Canvas
├── MarkNeedsRender()      - 标记需要渲染
├── InvalidateLayerTree()  - 使层树无效
├── ForceRasterize()       - 强制重新光栅化
│
├── HandleScroll()         - 处理滚动
├── ScrollTo()             - 滚动到指定位置
│
├── BeginAnimationFrame()  - 开始动画帧
├── UpdateAnimationProperty() - 更新动画属性
├── EndAnimationFrame()    - 结束动画帧
├── OnAnimationStart()     - 动画开始回调
├── OnAnimationEnd()       - 动画结束回调
│
├── MarkDirty()            - 标记对象为脏
├── MarkDirtyRegion()      - 标记区域为脏
│
└── 内部流程
    ├── BuildLayerTree()   - 构建层树
    ├── RasterizeDirtyLayers() - 光栅化脏层
    └── CompositeLayers()  - 合成层

依赖组件：
- LayerTreeBuilder        - 层树构建器
- Rasterizer              - 光栅化器
- Compositor              - 合成器
- AnimationLayerBridge    - 动画层桥接
- ScrollLayerManager      - 滚动层管理器
- PropertyTrees           - 属性树
- PropertyTreeBuilder     - 属性树构建器
- PaintArtifactCompositor - 绘制产物合成器
```

### 2.3 WindowCompositorAdapter 职责

```
文件：core/compositor/window_compositor_adapter.h/cpp

职责：
- 包装 RenderPipelineV2
- 提供 Window 友好的接口
- 管理属性树系统的连接

问题：
- 纯粹的适配层，没有实际逻辑
- 增加了不必要的间接调用
- 合并后可以删除
```

### 2.4 Window 类中的渲染相关成员

```cpp
// 当前 Window 类持有的渲染相关成员（需要简化）

// 渲染树
std::shared_ptr<RenderObject> cached_render_tree_;
std::shared_ptr<RenderTreeBuilder> render_tree_builder_;

// 动画
std::unique_ptr<AnimationTimeline> animation_timeline_;
std::unique_ptr<AnimationController> animation_controller_;
std::unique_ptr<AnimationApplicator> animation_applicator_;

// 布局
std::unique_ptr<LayoutEngine> layout_engine_;

// 旧版管线
std::unique_ptr<RenderPipelineLegacy> render_pipeline_;
std::shared_ptr<RenderTreeSynchronizer> render_tree_synchronizer_;

// 新版管线适配器
std::unique_ptr<WindowCompositorAdapter> compositor_adapter_;

// 脏区域
std::vector<SkRect> dirty_rects_;
bool needs_repaint_;
bool render_tree_valid_;
```

---

## 3. 统一管线设计

### 3.1 新 RenderPipeline 类设计

```cpp
/**
 * @file render_pipeline.h
 * @brief 统一渲染管线
 *
 * 整合 V1 和 V2 的所有功能，提供完整的渲染流程：
 * DOM同步 → 样式计算 → 布局 → 层树构建 → 光栅化 → 合成
 */

namespace lightui {

/**
 * @brief 渲染阶段枚举
 */
enum class RenderStage {
    Idle,           // 空闲
    DOMSync,        // DOM 同步
    StyleRecalc,    // 样式重算
    Layout,         // 布局计算
    LayerTreeBuild, // 层树构建
    Rasterize,      // 光栅化
    Composite       // 合成
};

/**
 * @brief 渲染管线配置
 */
struct RenderPipelineConfig {
    // 功能开关
    bool enable_gpu_compositing = true;      // GPU 合成
    bool enable_layer_promotion = true;      // 层提升
    bool enable_incremental_rasterize = true;// 增量光栅化
    bool enable_scroll_optimization = true;  // 滚动优化
    bool enable_animation_optimization = true;// 动画优化
    bool enable_frame_skip = true;           // 帧跳过
    bool enable_property_trees = true;       // 属性树系统
    
    // 调试选项
    bool show_layer_borders = false;         // 显示层边界
    bool show_dirty_regions = false;         // 显示脏区域
    bool enable_stats = true;                // 启用统计
};

/**
 * @brief 帧统计信息
 */
struct FrameStats {
    // 各阶段耗时（毫秒）
    double dom_sync_time = 0.0;
    double style_time = 0.0;
    double layout_time = 0.0;
    double layer_tree_time = 0.0;
    double rasterize_time = 0.0;
    double composite_time = 0.0;
    double total_time = 0.0;
    
    // 计数
    int dirty_nodes = 0;
    int layers_built = 0;
    int layers_rasterized = 0;
    int layers_composited = 0;
    
    // 状态
    bool frame_skipped = false;
    bool using_gpu = false;
    
    void Reset();
};

/**
 * @brief 统一渲染管线
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    // ========== 初始化 ==========
    
    /**
     * @brief 初始化渲染管线
     * @param width 视口宽度
     * @param height 视口高度
     * @param config 配置选项
     */
    bool Initialize(int width, int height, 
                    const RenderPipelineConfig& config = {});
    
    void Shutdown();
    void Resize(int width, int height);
    bool IsInitialized() const;

    // ========== 配置 ==========
    
    void SetDocument(std::shared_ptr<Document> doc);
    void SetConfig(const RenderPipelineConfig& config);
    void SetDpiScale(float scale);
    float GetDpiScale() const;

    // ========== 主渲染入口 ==========
    
    /**
     * @brief 处理一帧（主入口）
     * @param canvas 目标画布
     * @return true 如果渲染成功
     *
     * 完整流程：
     * 1. DOM 同步（如果有变化）
     * 2. 样式重算（如果需要）
     * 3. 布局计算（如果需要）
     * 4. 层树构建/更新
     * 5. 光栅化脏层
     * 6. 合成到画布
     */
    bool ProcessFrame(SkCanvas* canvas);
    
    /**
     * @brief 检查是否需要更新
     */
    bool NeedsUpdate() const;

    // ========== 脏标记 ==========
    
    void MarkNeedsDOMSync();
    void MarkNeedsStyleRecalc();
    void MarkNeedsLayout();
    void MarkNeedsPaint();
    void MarkNeedsLayerTreeRebuild();
    void ForceFullUpdate();
    
    void MarkDirty(RenderObject* object);
    void MarkDirtyRegion(const SkRect& region);

    // ========== 滚动处理 ==========
    
    bool HandleScroll(RenderObject* container, float dx, float dy);
    bool ScrollTo(RenderObject* container, float x, float y);

    // ========== 动画处理 ==========
    
    void BeginAnimationFrame();
    AnimationUpdateType UpdateAnimationProperty(
        RenderObject* object,
        const std::string& property,
        const std::string& value);
    bool EndAnimationFrame();
    
    void OnAnimationStart(RenderObject* object,
                          const std::string& name,
                          const std::vector<std::string>& properties);
    void OnAnimationEnd(RenderObject* object, const std::string& name);

    // ========== 属性树直接更新 ==========
    
    bool DirectlyUpdateTransform(RenderObject* obj, const SkM44& matrix);
    bool DirectlyUpdateOpacity(RenderObject* obj, float opacity);
    bool DirectlyUpdateScrollOffset(RenderObject* obj, const SkPoint& offset);

    // ========== 状态查询 ==========
    
    RenderStage GetCurrentStage() const;
    const FrameStats& GetLastFrameStats() const;
    
    // ========== 组件访问（调试用）==========
    
    RenderObject* GetRenderTree() const;
    CompositorLayer* GetRootLayer() const;
    PropertyTrees* GetPropertyTrees() const;
    PaintArtifactCompositor* GetPaintArtifactCompositor() const;
    bool IsUsingPropertyTreeSystem() const;

private:
    // ========== 渲染阶段实现 ==========
    
    void DoDOMSync();
    void DoStyleRecalc();
    void DoLayout();
    void DoLayerTreeBuild();
    void DoRasterize();
    void DoComposite(SkCanvas* canvas);
    
    // ========== 辅助方法 ==========
    
    void UpdateLayerTreeBounds(CompositorLayer* layer);
    void RegisterScrollableElements(RenderObject* root);
    bool CheckRenderObjectNeedsPaint(RenderObject* obj);

private:
    // 状态
    bool initialized_ = false;
    RenderStage current_stage_ = RenderStage::Idle;
    RenderPipelineConfig config_;
    
    // 脏标记
    bool needs_dom_sync_ = false;
    bool needs_style_recalc_ = false;
    bool needs_layout_ = false;
    bool needs_paint_ = false;
    bool needs_layer_tree_rebuild_ = true;
    bool needs_render_ = true;
    
    // 视口
    int viewport_width_ = 0;
    int viewport_height_ = 0;
    float dpi_scale_ = 1.0f;
    
    // 文档和渲染树
    std::weak_ptr<Document> document_;
    std::shared_ptr<RenderObject> render_tree_;
    
    // ========== 来自 V1 的组件 ==========
    std::unique_ptr<RenderTreeBuilder> render_tree_builder_;
    std::shared_ptr<RenderTreeSynchronizer> synchronizer_;
    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    
    // ========== 来自 V2 的组件 ==========
    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;
    std::unique_ptr<AnimationLayerBridge> animation_bridge_;
    std::unique_ptr<ScrollLayerManager> scroll_manager_;
    
    // 属性树系统
    std::unique_ptr<PropertyTrees> property_trees_;
    std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
    std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;
    
    // 层树
    std::shared_ptr<CompositorLayer> root_layer_;
    
    // 统计
    FrameStats last_frame_stats_;
    FrameStats current_frame_stats_;
    double frame_start_time_ = 0.0;
};

} // namespace lightui
```

### 3.2 渲染流程图

```
ProcessFrame(canvas)
    │
    ├─► [1] DOM 同步阶段
    │   └─► DoDOMSync()
    │         ├─ 检查 DirtyNodeTracker
    │         ├─ 调用 RenderTreeSynchronizer
    │         └─ 如果有变化 → 标记 needs_layout_
    │
    ├─► [2] 样式重算阶段
    │   └─► DoStyleRecalc()
    │         ├─ 遍历脏节点
    │         ├─ 重新解析样式
    │         └─ 如果有变化 → 标记 needs_layout_
    │
    ├─► [3] 布局阶段
    │   └─► DoLayout()
    │         ├─ 调用 NativeLayoutEngine
    │         ├─ 计算所有节点的位置和尺寸
    │         └─ 如果有变化 → 标记 needs_paint_
    │
    ├─► [4] 层树构建阶段
    │   └─► DoLayerTreeBuild()
    │         ├─ 调用 LayerTreeBuilder
    │         ├─ 构建 PropertyTrees
    │         ├─ 注册滚动容器
    │         └─ 如果是首次或需要重建 → 完整构建
    │             否则 → 只更新边界
    │
    ├─► [5] 光栅化阶段
    │   └─► DoRasterize()
    │         ├─ 调用 Rasterizer
    │         └─ 只光栅化脏层
    │
    └─► [6] 合成阶段
        └─► DoComposite(canvas)
              ├─ 检查帧跳过
              ├─ 调用 Compositor
              └─ 合成所有层到 canvas
```

### 3.3 脏标记传播规则

```
MarkNeedsDOMSync()
    └─► needs_dom_sync_ = true

MarkNeedsStyleRecalc()
    └─► needs_style_recalc_ = true
        └─► needs_layout_ = true
            └─► needs_paint_ = true

MarkNeedsLayout()
    └─► needs_layout_ = true
        └─► needs_paint_ = true

MarkNeedsPaint()
    └─► needs_paint_ = true

MarkNeedsLayerTreeRebuild()
    └─► needs_layer_tree_rebuild_ = true
        └─► needs_render_ = true
```

---

## 4. Window 类简化

### 4.1 简化后的 Window 渲染相关成员

```cpp
class Window {
private:
    // 文档
    std::shared_ptr<Document> document_;
    
    // 统一渲染管线（唯一的渲染入口）
    std::unique_ptr<RenderPipeline> render_pipeline_;
    
    // 动画系统（保留，因为动画逻辑独立于渲染管线）
    std::unique_ptr<AnimationTimeline> animation_timeline_;
    std::unique_ptr<AnimationController> animation_controller_;
    std::unique_ptr<AnimationApplicator> animation_applicator_;
    
    // 简单状态
    bool needs_repaint_ = true;
};
```

### 4.2 简化后的 Window::Render()

```cpp
void Window::Render() {
    if (!document_ || !surface_) {
        return;
    }
    
    // 检查是否需要渲染
    bool has_active_animations = CheckActiveAnimations();
    if (!needs_repaint_ && !has_active_animations && 
        !render_pipeline_->NeedsUpdate()) {
        return;
    }
    
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }
    
    // 初始化管线（如果需要）
    if (!render_pipeline_->IsInitialized()) {
        InitializeRenderPipeline();
    }
    
    // 更新动画
    UpdateAnimations(GetCurrentTime());
    
    // 清除背景
    canvas->clear(GetBackgroundColor());
    
    // 应用 DPI 缩放
    canvas->save();
    canvas->scale(dpi_scale_, dpi_scale_);
    
    // 处理一帧（所有渲染逻辑都在这里）
    render_pipeline_->ProcessFrame(canvas);
    
    canvas->restore();
    
    // 更新状态
    needs_repaint_ = has_active_animations;
}
```

---

## 5. 文件变更计划

### 5.1 新建文件

| 文件 | 说明 |
|------|------|
| `core/render/render_pipeline.h` | 统一管线头文件（重写） |
| `core/render/render_pipeline.cpp` | 统一管线实现（重写） |

### 5.2 删除文件

| 文件 | 原因 |
|------|------|
| `core/render/render_pipeline_legacy.h` | 功能合并到新管线 |
| `core/render/render_pipeline_legacy.cpp` | 功能合并到新管线 |
| `core/compositor/render_pipeline_v2.h` | 功能合并到新管线 |
| `core/compositor/render_pipeline_v2.cpp` | 功能合并到新管线 |
| `core/compositor/window_compositor_adapter.h` | 适配层，不再需要 |
| `core/compositor/window_compositor_adapter.cpp` | 适配层，不再需要 |

### 5.3 修改文件

| 文件 | 修改内容 |
|------|----------|
| `core/window/window.h` | 移除旧管线成员，使用新管线 |
| `core/window/window.cpp` | 简化 Render()，使用新管线 |
| `core/render/CMakeLists.txt` | 更新源文件列表 |
| `core/compositor/CMakeLists.txt` | 更新源文件列表 |

### 5.4 保留文件（不变）

以下组件保持不变，被新管线复用：

```
core/render/
├── render_object.h/cpp           # 渲染对象
├── render_tree_builder.h/cpp     # 渲染树构建器
├── render_tree_synchronizer.h/cpp # 渲染树同步器
├── animation_*.h/cpp             # 动画相关
└── ...

core/compositor/
├── compositor_layer.h/cpp        # 合成层
├── layer_tree_builder.h/cpp      # 层树构建器
├── rasterizer.h/cpp              # 光栅化器
├── compositor.h/cpp              # 合成器
├── animation_layer_bridge.h/cpp  # 动画桥接
├── scroll_layer_manager.h/cpp    # 滚动管理器
└── property_tree/                # 属性树系统
    ├── property_trees.h/cpp
    ├── property_tree_builder.h/cpp
    ├── paint_artifact_compositor.h/cpp
    └── ...

core/layout/
├── native_layout_engine.h/cpp    # 布局引擎
└── ...

core/dom/
├── dirty_node_tracker.h/cpp      # 脏节点追踪
└── ...
```

---

## 6. 实现策略

### 6.1 渐进式迁移

为了降低风险，采用渐进式迁移策略：

1. **阶段一**：创建新的 `RenderPipeline`，整合 V1 和 V2 的代码
2. **阶段二**：在 Window 中添加开关，可以切换新旧管线
3. **阶段三**：验证新管线功能正确
4. **阶段四**：删除旧代码

### 6.2 代码复用原则

- **直接复用**：V1 和 V2 中已经测试过的代码，直接复制到新管线
- **不重写**：不要试图"改进"已经工作的代码
- **保持接口**：组件（如 Rasterizer、Compositor）的接口保持不变

### 6.3 测试策略

- 使用现有的测试用例验证功能
- 对比新旧管线的渲染结果
- 性能测试确保没有退化

---

## 7. 风险与注意事项

### 7.1 风险

1. **动画状态丢失**：合并时需要确保动画状态正确传递
2. **脏标记不一致**：两套管线的脏标记逻辑需要统一
3. **组件依赖**：某些组件可能有隐式依赖关系

### 7.2 注意事项

1. **不要重写渲染逻辑**：只是合并，不是重新实现
2. **保持向后兼容**：Window 的公共接口尽量不变
3. **充分测试**：每个阶段都要验证功能

---

## 8. 附录：组件依赖关系

```
RenderPipeline
├── RenderTreeBuilder
│   └── StyleResolver
├── RenderTreeSynchronizer
│   └── DirtyNodeTracker
├── NativeLayoutEngine
│   └── Taffy (Rust FFI)
├── LayerTreeBuilder
│   └── CompositorLayer
├── Rasterizer
│   └── SkCanvas
├── Compositor
│   └── SkSurface
├── AnimationLayerBridge
│   └── CompositorLayer
├── ScrollLayerManager
│   └── CompositorLayer
├── PropertyTrees
│   ├── TransformTree
│   ├── ClipTree
│   ├── EffectTree
│   └── ScrollTree
├── PropertyTreeBuilder
│   └── PropertyTrees
└── PaintArtifactCompositor
    └── PropertyTrees
```
