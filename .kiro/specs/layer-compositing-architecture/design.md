# Design Document: Layer Compositing Architecture

## Overview

本设计文档描述 LightUI 渲染引擎的分层合成架构重构。新架构采用业界标准的 CPU 光栅化 + GPU 合成模式，将渲染过程分为两个独立阶段：

1. **光栅化阶段（Rasterization）**：使用 Skia CPU 后端将矢量图形绘制到内存位图
2. **合成阶段（Compositing）**：使用 GPU 将各层纹理混合显示到屏幕

这种架构的核心优势是支持真正的分层合成，使得滚动、动画等操作可以只移动/变换纹理而不需要重新光栅化。

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Main Thread                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────┐  │
│  │   DOM Tree  │───►│ Render Tree │───►│   Layer Tree        │  │
│  └─────────────┘    └─────────────┘    │  ┌───────────────┐  │  │
│                                         │  │ Root Layer    │  │  │
│                                         │  │  ┌─────────┐  │  │  │
│                                         │  │  │ Layer 1 │  │  │  │
│                                         │  │  │ Layer 2 │  │  │  │
│                                         │  │  │ Layer 3 │  │  │  │
│                                         │  │  └─────────┘  │  │  │
│                                         │  └───────────────┘  │  │
│                                         └─────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     Rasterization (CPU)                          │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │ Layer 1     │    │ Layer 2     │    │ Layer 3     │          │
│  │ ┌─────────┐ │    │ ┌─────────┐ │    │ ┌─────────┐ │          │
│  │ │ Bitmap  │ │    │ │ Bitmap  │ │    │ │ Bitmap  │ │          │
│  │ └─────────┘ │    │ └─────────┘ │    │ └─────────┘ │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
├─────────────────────────────────────────────────────────────────┤
│                     Compositing (GPU)                            │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │ Texture 1   │    │ Texture 2   │    │ Texture 3   │          │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘          │
│         │                  │                  │                  │
│         └──────────────────┼──────────────────┘                  │
│                            ▼                                     │
│                    ┌───────────────┐                             │
│                    │  GPU Blend    │                             │
│                    └───────┬───────┘                             │
│                            ▼                                     │
│                    ┌───────────────┐                             │
│                    │    Screen     │                             │
│                    └───────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. CompositorLayer

表示一个合成层，包含 CPU 位图和对应的 GPU 纹理。

```cpp
class CompositorLayer {
public:
    // 层标识
    uint32_t GetId() const;
    
    // 关联的 RenderObject（可能为空，如根层）
    RenderObject* GetRenderObject() const;
    
    // 层的边界（相对于父层）
    SkRect GetBounds() const;
    void SetBounds(const SkRect& bounds);
    
    // 变换矩阵（用于动画）
    SkMatrix GetTransform() const;
    void SetTransform(const SkMatrix& transform);
    
    // 不透明度（用于动画）
    float GetOpacity() const;
    void SetOpacity(float opacity);
    
    // CPU 位图
    SkBitmap& GetBitmap();
    SkCanvas* GetCanvas();  // 用于光栅化
    
    // GPU 纹理
    GLuint GetTextureId() const;
    bool IsTextureDirty() const;
    void MarkTextureDirty(const SkIRect& region);
    void UploadDirtyRegions();  // 上传脏区域到 GPU
    
    // 脏区域管理
    void MarkDirty(const SkRect& region);
    void ClearDirtyRegions();
    const std::vector<SkIRect>& GetDirtyRegions() const;
    bool HasDirtyRegions() const;
    
    // 子层
    void AddChild(std::shared_ptr<CompositorLayer> child);
    void RemoveChild(CompositorLayer* child);
    const std::vector<std::shared_ptr<CompositorLayer>>& GetChildren() const;
    
    // 滚动
    SkPoint GetScrollOffset() const;
    void SetScrollOffset(const SkPoint& offset);
    
private:
    uint32_t id_;
    RenderObject* render_object_ = nullptr;
    SkRect bounds_;
    SkMatrix transform_ = SkMatrix::I();
    float opacity_ = 1.0f;
    
    SkBitmap bitmap_;
    std::unique_ptr<SkCanvas> canvas_;
    
    GLuint texture_id_ = 0;
    std::vector<SkIRect> dirty_regions_;
    std::vector<SkIRect> texture_dirty_regions_;
    
    std::weak_ptr<CompositorLayer> parent_;
    std::vector<std::shared_ptr<CompositorLayer>> children_;
    
    SkPoint scroll_offset_ = {0, 0};
};
```

### 2. LayerTreeBuilder

负责从渲染树构建层树，决定哪些元素需要独立层。

```cpp
class LayerTreeBuilder {
public:
    // 构建层树
    std::shared_ptr<CompositorLayer> Build(RenderObject* root);
    
    // 增量更新层树
    void Update(RenderObject* changed_node);
    
    // 层提升策略
    enum class PromotionReason {
        None,
        WillChangeTransform,
        WillChangeOpacity,
        PositionFixed,
        TransformAnimation,
        OpacityAnimation,
        ScrollableContent,
        Explicit  // 显式请求
    };
    
    // 检查元素是否需要独立层
    PromotionReason ShouldPromote(RenderObject* obj) const;
    
private:
    void BuildRecursive(RenderObject* obj, CompositorLayer* parent_layer);
    std::shared_ptr<CompositorLayer> CreateLayer(RenderObject* obj);
};
```

### 3. Rasterizer

负责将渲染对象光栅化到层的 CPU 位图。

```cpp
class Rasterizer {
public:
    // 光栅化整个层
    void RasterizeLayer(CompositorLayer* layer);
    
    // 增量光栅化（只绘制脏区域）
    void RasterizeDirtyRegions(CompositorLayer* layer);
    
    // 设置是否启用增量光栅化
    void SetIncrementalEnabled(bool enabled);
    
private:
    void RasterizeRegion(CompositorLayer* layer, const SkIRect& region);
    bool incremental_enabled_ = true;
};
```

### 4. Compositor

负责将所有层合成到最终画面。

```cpp
class Compositor {
public:
    // 初始化（创建 GPU 资源）
    bool Initialize(int width, int height);
    void Shutdown();
    
    // 调整大小
    void Resize(int width, int height);
    
    // 合成所有层到屏幕
    void Composite(CompositorLayer* root_layer);
    
    // 是否使用 GPU 加速
    bool IsGPUAccelerated() const;
    void SetGPUAccelerated(bool enabled);
    
    // 调试：显示层边界
    void SetShowLayerBorders(bool show);
    
private:
    void CompositeLayer(CompositorLayer* layer, const SkMatrix& parent_transform);
    void DrawLayerToScreen(CompositorLayer* layer, const SkMatrix& transform);
    void DrawQuad(GLuint texture, const SkRect& bounds, const SkMatrix& transform, float opacity);
    
    bool gpu_accelerated_ = true;
    bool show_layer_borders_ = false;
    
    // GPU 资源
    GLuint shader_program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    
    // 屏幕尺寸
    int width_ = 0;
    int height_ = 0;
};
```

### 5. RenderPipelineV2

新的渲染管线，整合层树构建、光栅化和合成。

```cpp
class RenderPipelineV2 {
public:
    RenderPipelineV2();
    ~RenderPipelineV2();
    
    // 初始化
    bool Initialize(int width, int height, bool gpu_accelerated = true);
    void Shutdown();
    
    // 渲染一帧
    void Render(RenderObject* root);
    
    // 处理滚动（不触发重新光栅化）
    void HandleScroll(RenderObject* scrollable, float dx, float dy);
    
    // 处理动画（只更新层变换）
    void UpdateAnimations(double current_time);
    
    // 调整大小
    void Resize(int width, int height);
    
    // 获取层数量（调试用）
    size_t GetLayerCount() const;
    
    // 调试选项
    void SetShowLayerBorders(bool show);
    void SetIncrementalRasterization(bool enabled);
    
private:
    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;
    
    std::shared_ptr<CompositorLayer> root_layer_;
};
```

## Data Models

### LayerInfo

存储在 RenderObject 中的层信息。

```cpp
struct LayerInfo {
    // 关联的合成层（如果有）
    std::weak_ptr<CompositorLayer> compositor_layer;
    
    // 层提升原因
    LayerTreeBuilder::PromotionReason promotion_reason = 
        LayerTreeBuilder::PromotionReason::None;
    
    // 是否强制独立层
    bool force_own_layer = false;
};
```

### CompositorFrameInfo

一帧的合成信息（用于性能分析）。

```cpp
struct CompositorFrameInfo {
    // 时间戳
    double timestamp;
    
    // 层统计
    size_t total_layers;
    size_t rasterized_layers;
    size_t composited_layers;
    
    // 脏区域统计
    size_t total_dirty_pixels;
    
    // 耗时
    double rasterization_time_ms;
    double compositing_time_ms;
    double total_time_ms;
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Layer content is rasterized to CPU bitmap
*For any* layer with dirty regions, after rasterization, the layer's bitmap SHALL contain the correct pixel values for all render objects within that layer's bounds.
**Validates: Requirements 1.2, 3.2**

### Property 2: Layer promotion based on CSS properties
*For any* element with `will-change: transform`, `will-change: opacity`, `position: fixed`, or active transform/opacity animation, the system SHALL create a dedicated compositing layer for that element.
**Validates: Requirements 2.1, 2.2, 2.3**

### Property 3: Dirty region marking and merging
*For any* set of dirty regions within a layer, the system SHALL merge overlapping regions and the merged result SHALL cover all originally dirty pixels.
**Validates: Requirements 3.1, 3.4**

### Property 4: Incremental rasterization preserves unchanged pixels
*For any* layer with partial dirty regions, after incremental rasterization, pixels outside the dirty regions SHALL remain unchanged from the previous frame.
**Validates: Requirements 3.2, 3.3**

### Property 5: Scroll optimization - no re-rasterization
*For any* scroll operation on a layer, the system SHALL NOT re-rasterize existing content; only newly visible content SHALL be rasterized.
**Validates: Requirements 3.5, 6.1, 6.2, 6.3**

### Property 6: Z-order compositing correctness
*For any* set of overlapping layers, the final composited pixel values SHALL match the expected result of blending layers in correct z-order with their respective opacities.
**Validates: Requirements 5.1, 5.4**

### Property 7: Transform/opacity animation without re-rasterization
*For any* layer with active transform or opacity animation, the system SHALL update only the layer's transform matrix or opacity value without triggering re-rasterization.
**Validates: Requirements 5.2, 5.3, 7.1, 7.2**

### Property 8: Frame skip when nothing changed
*For any* frame where no layers have dirty regions and no animations are active, the system SHALL skip both rasterization and compositing.
**Validates: Requirements 3.3, 5.5**

### Property 9: Visual output consistency
*For any* render tree, the visual output of the new layer compositing architecture SHALL be pixel-identical to the previous direct rendering architecture.
**Validates: Requirements 8.1, 8.2**

### Property 10: Layer cleanup on condition change
*For any* layer that was promoted due to animation, when the animation completes and no other promotion reasons exist, the layer SHALL be demoted and merged back to its parent.
**Validates: Requirements 2.5, 7.4**

## Error Handling

### GPU Initialization Failure
- 如果 GPU 初始化失败，自动降级到 CPU-only 合成模式
- 使用 SDL 软件渲染器显示最终画面
- 记录警告日志但不中断应用

### Texture Upload Failure
- 如果纹理上传失败（GPU 内存不足），标记层为"软件合成"
- 该层直接在 CPU 上与其他层混合
- 尝试释放不可见层的纹理以腾出空间

### Rasterization Timeout
- 如果单层光栅化超过 100ms，记录警告
- 考虑将该层拆分为更小的 tiles
- 提供性能分析数据帮助开发者优化

## Testing Strategy

### Unit Tests
- LayerTreeBuilder: 测试层提升逻辑
- Rasterizer: 测试增量光栅化
- Compositor: 测试层合成顺序和混合
- DirtyRegion: 测试脏区域合并算法

### Property-Based Tests
使用 RapidCheck (C++ property-based testing library) 实现：

1. **Layer promotion property test**: 生成随机 CSS 属性组合，验证层提升决策正确
2. **Dirty region merging property test**: 生成随机脏区域集合，验证合并后覆盖所有原始区域
3. **Incremental rasterization property test**: 生成随机脏区域，验证非脏区域像素不变
4. **Z-order compositing property test**: 生成随机层叠顺序和透明度，验证合成结果正确
5. **Visual consistency property test**: 对比新旧架构的渲染输出

### Integration Tests
- 滚动场景：验证滚动时不触发重新光栅化
- 动画场景：验证 transform/opacity 动画只更新层属性
- 混合场景：滚动 + 动画同时进行

### Performance Tests
- 测量单层光栅化时间
- 测量合成时间
- 测量帧率稳定性
- 对比新旧架构的 CPU/GPU 使用率
