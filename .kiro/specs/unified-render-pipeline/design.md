# 渲染管线重构 - 设计文档

## 1. 架构概览

### 1.1 当前架构（问题）

```
Window
  ├── render_pipeline_ (V1)           ← DOM同步、布局
  ├── compositor_adapter_             ← 适配层（冗余）
  │     └── RenderPipelineV2          ← 层树、光栅化、合成
  ├── render_tree_synchronizer_       ← 散落的组件
  ├── layout_engine_                  ← 散落的组件
  ├── render_tree_builder_            ← 散落的组件
  └── animation_applicator_           ← 散落的组件
```

### 1.2 目标架构

```
Window
  └── render_pipeline_
        └── RenderPipeline
              ├── 阶段管理器
              │     ├── DOMSyncStage
              │     ├── StyleStage
              │     ├── LayoutStage
              │     ├── LayerTreeStage
              │     ├── RasterizeStage
              │     └── CompositeStage
              │
              ├── 核心组件
              │     ├── DirtyNodeTracker
              │     ├── RenderTreeSynchronizer
              │     ├── NativeLayoutEngine
              │     ├── LayerTreeBuilder
              │     ├── Rasterizer
              │     └── Compositor
              │
              └── 优化组件
                    ├── AnimationLayerBridge
                    ├── ScrollLayerManager
                    └── PropertyTrees
```

## 2. 核心类设计

### 2.1 RenderPipeline

```cpp
namespace lightui {

/**
 * @brief 渲染生命周期阶段
 */
enum class RenderStage {
    Idle,           // 空闲
    DOMSync,        // DOM 同步
    Style,          // 样式计算
    Layout,         // 布局
    LayerTree,      // 层树构建
    Rasterize,      // 光栅化
    Composite       // 合成
};

/**
 * @brief 渲染管线配置
 */
struct RenderPipelineConfig {
    // 功能开关
    bool enable_gpu_compositing = true;
    bool enable_layer_promotion = true;
    bool enable_incremental_rasterize = true;
    bool enable_scroll_optimization = true;
    bool enable_animation_optimization = true;
    bool enable_frame_skip = true;
    bool enable_property_trees = true;
    
    // 调试选项
    bool show_layer_borders = false;
    bool show_dirty_regions = false;
    bool enable_stats = true;
};

/**
 * @brief 帧统计信息
 */
struct FrameStats {
    double dom_sync_time = 0.0;
    double style_time = 0.0;
    double layout_time = 0.0;
    double layer_tree_time = 0.0;
    double rasterize_time = 0.0;
    double composite_time = 0.0;
    double total_time = 0.0;
    
    int dirty_nodes = 0;
    int layers_built = 0;
    int layers_rasterized = 0;
    bool frame_skipped = false;
};

/**
 * @brief 渲染管线
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();
    
    // ========== 初始化 ==========
    
    bool Initialize(int width, int height, 
                    const RenderPipelineConfig& config = {});
    void Shutdown();
    void Resize(int width, int height);
    bool IsInitialized() const;
    
    // ========== 配置 ==========
    
    void SetDocument(std::shared_ptr<Document> doc);
    void SetConfig(const RenderPipelineConfig& config);
    const RenderPipelineConfig& GetConfig() const;
    void SetDpiScale(float scale);
    
    // ========== 渲染 ==========
    
    /**
     * @brief 处理一帧（主入口）
     * 
     * 执行完整渲染流程：
     * 1. DOM 同步（如果有变化）
     * 2. 样式计算（如果需要）
     * 3. 布局（如果需要）
     * 4. 层树构建/更新
     * 5. 光栅化脏层
     * 6. 合成到屏幕
     */
    bool ProcessFrame(SkCanvas* canvas);
    
    /**
     * @brief 检查是否需要更新
     */
    bool NeedsUpdate() const;
    
    // ========== 脏标记 ==========
    
    void MarkNeedsStyleRecalc();
    void MarkNeedsLayout();
    void MarkNeedsPaint();
    void MarkNeedsLayerTreeRebuild();
    void ForceFullUpdate();
    
    // ========== 滚动 ==========
    
    bool HandleScroll(RenderObject* container, float dx, float dy);
    bool ScrollTo(RenderObject* container, float x, float y);
    
    // ========== 动画 ==========
    
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
    
private:
    // 阶段执行
    void DoDOMSync();
    void DoStyleRecalc();
    void DoLayout();
    void DoLayerTreeBuild();
    void DoRasterize();
    void DoComposite(SkCanvas* canvas);
    
    // 状态
    bool initialized_ = false;
    RenderStage current_stage_ = RenderStage::Idle;
    RenderPipelineConfig config_;
    
    // 脏标记
    bool needs_dom_sync_ = false;
    bool needs_style_recalc_ = false;
    bool needs_layout_ = false;
    bool needs_paint_ = false;
    bool needs_layer_tree_rebuild_ = false;
    
    // 视口
    int viewport_width_ = 0;
    int viewport_height_ = 0;
    float dpi_scale_ = 1.0f;
    
    // 文档和渲染树
    std::weak_ptr<Document> document_;
    std::shared_ptr<RenderObject> render_tree_;
    
    // 核心组件
    std::unique_ptr<RenderTreeBuilder> render_tree_builder_;
    std::unique_ptr<RenderTreeSynchronizer> synchronizer_;
    std::unique_ptr<NativeLayoutEngine> layout_engine_;
    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;
    
    // 优化组件
    std::unique_ptr<AnimationLayerBridge> animation_bridge_;
    std::unique_ptr<ScrollLayerManager> scroll_manager_;
    std::unique_ptr<PropertyTrees> property_trees_;
    std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
    std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;
    
    // 层树
    std::shared_ptr<CompositorLayer> root_layer_;
    
    // 统计
    FrameStats last_frame_stats_;
    FrameStats current_frame_stats_;
};

} // namespace lightui
```

## 3. 渲染流程

### 3.1 ProcessFrame 流程

```
ProcessFrame(canvas)
    │
    ├─► 检查 DOM 变化
    │   └─► DoDOMSync()
    │         ├─ 获取 DirtyNodeTracker
    │         ├─ 调用 RenderTreeSynchronizer
    │         └─ 标记 needs_layout_
    │
    ├─► 检查样式脏标记
    │   └─► DoStyleRecalc()
    │         └─ 重算脏节点样式
    │
    ├─► 检查布局脏标记
    │   └─► DoLayout()
    │         ├─ 调用 NativeLayoutEngine
    │         └─ 标记 needs_paint_
    │
    ├─► 检查层树脏标记
    │   └─► DoLayerTreeBuild()
    │         ├─ 调用 LayerTreeBuilder
    │         ├─ 构建 PropertyTrees
    │         └─ 注册滚动容器
    │
    ├─► 检查绘制脏标记
    │   └─► DoRasterize()
    │         └─ 调用 Rasterizer
    │
    └─► DoComposite(canvas)
          ├─ 检查帧跳过
          └─ 调用 Compositor
```

### 3.2 脏标记传播

```
MarkNeedsStyleRecalc()
    └─► needs_style_recalc_ = true
        └─► needs_layout_ = true
            └─► needs_paint_ = true

MarkNeedsLayout()
    └─► needs_layout_ = true
        └─► needs_paint_ = true

MarkNeedsPaint()
    └─► needs_paint_ = true
```

## 4. 迁移策略

### 4.1 阶段一：创建统一管线
1. 创建 `UnifiedRenderPipeline` 类
2. 整合 V1 和 V2 的功能
3. 保持旧代码可用

### 4.2 阶段二：Window 迁移
1. 在 Window 中添加 `unified_pipeline_`
2. 添加开关切换新旧管线
3. 验证功能一致性

### 4.3 阶段三：清理
1. 移除 `WindowCompositorAdapter`
2. 移除 Window 中散落的组件
3. 废弃旧的 `RenderPipeline` 和 `RenderPipelineV2`

## 5. 文件结构

```
core/render/
  ├── render_pipeline.h              # 重写（新实现）
  ├── render_pipeline.cpp            # 重写（新实现）
  ├── render_pipeline_legacy.h       # 旧 V1 重命名，过渡期保留
  ├── render_pipeline_legacy.cpp     # 旧 V1 重命名，过渡期保留
  └── ...

core/compositor/
  ├── render_pipeline_v2.h           # 过渡期保留，后续删除
  ├── render_pipeline_v2.cpp         # 过渡期保留，后续删除
  ├── window_compositor_adapter.h    # 过渡期保留，后续删除
  ├── window_compositor_adapter.cpp  # 过渡期保留，后续删除
  └── ...（其他组件保留）
```

## 6. 接口兼容

### 6.1 Window 类变化

```cpp
// 旧接口（过渡期保留）
class Window {
    std::unique_ptr<RenderPipelineLegacy> render_pipeline_legacy_;
    std::unique_ptr<WindowCompositorAdapter> compositor_adapter_;
    // ...
};

// 新接口
class Window {
    std::unique_ptr<RenderPipeline> render_pipeline_;
    bool use_new_pipeline_ = true;  // 开关
    // ...
};
```

### 6.2 AnimationApplicator 适配

```cpp
// 需要更新 AnimationApplicator 使用新管线
void AnimationApplicator::SetPipeline(RenderPipeline* pipeline) {
    pipeline_ = pipeline;
    // 获取属性树系统
    property_trees_ = pipeline->GetPropertyTrees();
    paint_artifact_compositor_ = pipeline->GetPaintArtifactCompositor();
}
```

## 7. 冗余代码分析

### 7.1 需要删除的文件

| 文件 | 原因 |
|------|------|
| `window_compositor_adapter.h/cpp` | 适配层，合并后不需要 |
| `render_pipeline_v2.h/cpp` | 功能整合到新 RenderPipeline |
| `render_pipeline_legacy.h/cpp` | 旧 V1，过渡期后删除 |

### 7.2 需要评估的潜在冗余

| 组件 | 位置 | 与...可能重复 | 处理建议 |
|------|------|--------------|----------|
| `Layer` | core/render/ | `CompositorLayer` | 不同用途：Layer 用于 z-index 分层绘制，CompositorLayer 用于合成优化，保留两者 |
| `LayerManager` | core/render/ | `LayerTreeBuilder` | 不同用途：LayerManager 管理 z-index 层级，LayerTreeBuilder 构建合成层树，保留两者 |
| `DirtyRegion` | core/render/ | `Rasterizer` 脏区域 | DirtyRegion 是通用脏区域工具，Rasterizer 内部使用，保留 |
| `DirtyRegionCollector` | core/render/ | `DirtyNodeTracker` | 功能重叠，考虑合并到 DirtyNodeTracker |
| `UnifiedRenderer` | core/render/ | 无 | 高层 API 封装，用于 JS 绑定，保留 |

### 7.3 组件职责澄清

```
渲染层级系统（保留）:
├── Layer              → CSS z-index 分层，用于正确绘制顺序
├── LayerManager       → 管理多个 Layer（Base/Overlay/Modal）
└── 用途：处理 dropdown、modal 等高 z-index 元素

合成层系统（保留）:
├── CompositorLayer    → GPU 合成层，用于性能优化
├── LayerTreeBuilder   → 构建合成层树
└── 用途：transform/opacity 动画、滚动优化

脏区域系统（需整理）:
├── DirtyRegion           → 通用脏区域工具类（保留）
├── DirtyRegionCollector  → 从 DOM 收集脏区域（考虑合并）
├── DirtyNodeTracker      → 追踪 DOM 节点变化（保留）
└── Rasterizer 内部脏区域  → 层级脏区域追踪（保留）
```

### 7.4 最终目录结构

```
core/render/
  ├── render_pipeline.h/cpp      # 新统一管线
  ├── render_object.h/cpp        # 渲染对象
  ├── render_tree_synchronizer.h/cpp  # 渲染树同步
  ├── layer.h/cpp                # z-index 层（保留）
  ├── layer_manager.h/cpp        # 层管理器（保留）
  ├── dirty_region.h/cpp         # 脏区域工具（保留）
  ├── animation_*.h/cpp          # 动画相关（保留）
  └── ...其他渲染工具

core/compositor/
  ├── compositor_layer.h/cpp     # 合成层（保留）
  ├── layer_tree_builder.h/cpp   # 层树构建（保留）
  ├── rasterizer.h/cpp           # 光栅化（保留）
  ├── compositor.h/cpp           # 合成器（保留）
  ├── animation_layer_bridge.h/cpp    # 动画桥接（保留）
  ├── scroll_layer_manager.h/cpp      # 滚动管理（保留）
  ├── animation_bounds_calculator.h/cpp # 动画边界（保留）
  └── property_tree/             # 属性树系统（保留）

删除:
  ├── window_compositor_adapter.h/cpp  # 删除
  ├── render_pipeline_v2.h/cpp         # 删除
  └── render_pipeline_legacy.h/cpp     # 删除
```
