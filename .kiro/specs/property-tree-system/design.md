# 属性树系统设计文档

## 概述

本设计文档描述了一个完整的属性树系统，参考 Chromium Blink 渲染引擎的设计，用于实现高效的 GPU 增量渲染。系统的核心思想是将渲染属性（变换、裁剪、效果、滚动）组织成独立的树结构，通过属性树状态描述每个绘制块的渲染上下文，支持直接属性更新和智能层合并。

### 设计目标

1. **降低 GPU 占用**：transform/opacity 动画从 20-30% 降到 10% 以下
2. **支持直接属性更新**：动画只更新属性树节点，不触发重新光栅化
3. **智能层合并**：减少 GPU 内存占用，避免过度分层
4. **精确光栅化失效**：只重新光栅化真正变化的区域
5. **与现有系统兼容**：渐进式迁移，不破坏现有功能

### 参考资料

- Chromium Blink: `third_party/blink/renderer/platform/graphics/paint/`
- Chromium Blink: `third_party/blink/renderer/platform/graphics/compositing/`
- Chromium cc: `cc/trees/property_tree.h`

## 架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         RenderObject Tree                            │
│                    (DOM 结构 + 计算样式 + 布局)                        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PropertyTreeBuilder                             │
│                   (从 RenderObject 构建属性树)                        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
```

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Property Trees                               │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐           │
│  │ Transform │ │   Clip    │ │  Effect   │ │  Scroll   │           │
│  │   Tree    │ │   Tree    │ │   Tree    │ │   Tree    │           │
│  └───────────┘ └───────────┘ └───────────┘ └───────────┘           │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         PaintController                              │
│              (生成 DisplayItems 和 PaintChunks)                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        PaintArtifact                                 │
│           (DisplayItems + PaintChunks + PropertyTreeState)           │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     PaintArtifactCompositor                          │
│                        (层化 + 合成)                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │
│  │  Layerizer  │  │   Raster    │  │  Compositor │                  │
│  │  (层化算法)  │  │ Invalidator │  │   (合成器)   │                  │
│  └─────────────┘  └─────────────┘  └─────────────┘                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      CompositorLayer List                            │
│                    (GPU 纹理 + 属性树引用)                            │
└─────────────────────────────────────────────────────────────────────┘
```

### 数据流

```
1. 样式变化 → PropertyTreeBuilder 更新属性树节点
2. 布局变化 → PropertyTreeBuilder 更新节点边界
3. 绘制变化 → PaintController 生成新的 PaintArtifact
4. 层化决策 → Layerizer 决定层结构
5. 光栅化 → Rasterizer 光栅化脏层
6. 合成 → Compositor 合成到屏幕
```

### 更新路径

```
┌─────────────────────────────────────────────────────────────────────┐
│                          更新类型                                    │
├─────────────────────────────────────────────────────────────────────┤
│ 1. 直接属性更新 (Direct Property Update)                             │
│    - transform 动画                                                  │
│    - opacity 动画                                                    │
│    - 滚动偏移                                                        │
│    路径: 更新属性树节点 → GPU 重新合成                                 │
│    不触发: 光栅化                                                    │
├─────────────────────────────────────────────────────────────────────┤
│ 2. 重绘更新 (Repaint Update)                                         │
│    - 颜色变化                                                        │
│    - 文本变化                                                        │
│    - 背景变化                                                        │
│    路径: 更新 PaintArtifact → 光栅化失效 → 增量光栅化 → GPU 合成       │
│    不触发: 层化重建                                                  │
├─────────────────────────────────────────────────────────────────────┤
│ 3. 完整更新 (Full Update)                                            │
│    - 层结构变化                                                      │
│    - 新元素出现/消失                                                 │
│    - will-change 变化                                                │
│    路径: 重建属性树 → 重新层化 → 完整光栅化 → GPU 合成                  │
└─────────────────────────────────────────────────────────────────────┘
```

## 组件和接口

### 1. 属性树节点基类

```cpp
// property_tree_node.h

namespace lightui {

// 属性树节点 ID
using PropertyTreeNodeId = uint32_t;
constexpr PropertyTreeNodeId kInvalidNodeId = 0;

// 属性树节点基类
template <typename NodeType>
class PropertyTreeNode {
public:
    PropertyTreeNodeId GetId() const { return id_; }
    NodeType* GetParent() const { return parent_; }
    const std::vector<NodeType*>& GetChildren() const { return children_; }
    
    // 是否是根节点
    bool IsRoot() const { return parent_ == nullptr; }
    
    // 获取到根节点的路径
    std::vector<NodeType*> GetPathToRoot() const;
    
    // 查找与另一个节点的最近公共祖先
    NodeType* FindCommonAncestor(NodeType* other) const;
    
    // 脏标记
    bool IsDirty() const { return dirty_; }
    void MarkDirty();
    void ClearDirty() { dirty_ = false; }
    
    // 版本号（用于缓存失效）
    uint64_t GetVersion() const { return version_; }
    
protected:
    PropertyTreeNodeId id_ = kInvalidNodeId;
    NodeType* parent_ = nullptr;
    std::vector<NodeType*> children_;
    bool dirty_ = true;
    uint64_t version_ = 0;
};

} // namespace lightui
```

### 2. 变换树节点

```cpp
// transform_tree_node.h

namespace lightui {

// 变换类型
enum class TransformType {
    kIdentity,          // 单位变换
    k2DTranslation,     // 2D 平移
    k2DScale,           // 2D 缩放
    k2DRotation,        // 2D 旋转
    k2DAffine,          // 2D 仿射变换
    k3DTransform,       // 3D 变换
    kPerspective,       // 透视
    kScrollTranslation, // 滚动平移
};

class TransformTreeNode : public PropertyTreeNode<TransformTreeNode> {
public:
    // 变换矩阵（4x4）
    const SkM44& GetMatrix() const { return matrix_; }
    void SetMatrix(const SkM44& matrix);
    
    // 变换原点
    const SkPoint3& GetOrigin() const { return origin_; }
    void SetOrigin(const SkPoint3& origin);
    
    // 是否扁平化（flatten into 2D）
    bool ShouldFlatten() const { return flatten_; }
    void SetFlatten(bool flatten);
    
    // 变换类型（用于优化）
    TransformType GetTransformType() const { return type_; }
    
    // 是否是滚动变换
    bool IsScrollTranslation() const { return type_ == TransformType::kScrollTranslation; }
    
    // 关联的滚动节点（如果是滚动变换）
    ScrollTreeNode* GetScrollNode() const { return scroll_node_; }
    void SetScrollNode(ScrollTreeNode* node) { scroll_node_ = node; }
    
    // 计算到根节点的累积变换
    SkM44 GetAccumulatedTransform() const;
    
    // 计算从此节点到目标节点的变换
    SkM44 GetTransformTo(const TransformTreeNode* target) const;
    
    // 是否可以直接更新（不影响层结构）
    bool CanDirectlyUpdate() const;
    
    // 关联的 RenderObject（用于调试）
    RenderObject* GetRenderObject() const { return render_object_; }
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }
    
private:
    SkM44 matrix_ = SkM44::I();
    SkPoint3 origin_ = {0, 0, 0};
    bool flatten_ = true;
    TransformType type_ = TransformType::kIdentity;
    ScrollTreeNode* scroll_node_ = nullptr;
    RenderObject* render_object_ = nullptr;
    
    // 缓存的累积变换
    mutable SkM44 cached_accumulated_transform_;
    mutable bool accumulated_transform_valid_ = false;
};

} // namespace lightui
```

### 3. 裁剪树节点

```cpp
// clip_tree_node.h

namespace lightui {

// 裁剪类型
enum class ClipType {
    kNone,              // 无裁剪
    kRect,              // 矩形裁剪
    kRoundedRect,       // 圆角矩形裁剪
    kPath,              // 路径裁剪（clip-path）
};

class ClipTreeNode : public PropertyTreeNode<ClipTreeNode> {
public:
    // 裁剪类型
    ClipType GetClipType() const { return type_; }
    
    // 裁剪矩形（在关联变换空间中）
    const SkRect& GetClipRect() const { return clip_rect_; }
    void SetClipRect(const SkRect& rect);
    
    // 圆角半径
    const SkVector* GetRadii() const { return radii_; }
    void SetRadii(const SkVector radii[4]);
    
    // 裁剪路径（clip-path）
    const SkPath* GetClipPath() const { return clip_path_.get(); }
    void SetClipPath(std::unique_ptr<SkPath> path);
    
    // 关联的变换节点
    TransformTreeNode* GetTransformNode() const { return transform_node_; }
    void SetTransformNode(TransformTreeNode* node) { transform_node_ = node; }
    
    // 计算在目标变换空间中的裁剪区域
    SkRect GetClipRectInSpace(const TransformTreeNode* target_space) const;
    
    // 计算累积裁剪区域（考虑所有祖先）
    SkRect GetAccumulatedClipRect(const TransformTreeNode* target_space) const;
    
    // 关联的 RenderObject
    RenderObject* GetRenderObject() const { return render_object_; }
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }
    
private:
    ClipType type_ = ClipType::kNone;
    SkRect clip_rect_ = SkRect::MakeEmpty();
    SkVector radii_[4] = {};  // 四个角的圆角半径
    std::unique_ptr<SkPath> clip_path_;
    TransformTreeNode* transform_node_ = nullptr;
    RenderObject* render_object_ = nullptr;
};

} // namespace lightui
```

### 4. 效果树节点

```cpp
// effect_tree_node.h

namespace lightui {

// 效果类型标志
enum class EffectFlags : uint32_t {
    kNone = 0,
    kHasOpacity = 1 << 0,
    kHasFilter = 1 << 1,
    kHasBackdropFilter = 1 << 2,
    kHasBlendMode = 1 << 3,
    kHasMask = 1 << 4,
    kRequiresIsolation = 1 << 5,  // 需要独立渲染表面
};

class EffectTreeNode : public PropertyTreeNode<EffectTreeNode> {
public:
    // Opacity
    float GetOpacity() const { return opacity_; }
    void SetOpacity(float opacity);
    bool HasOpacity() const { return opacity_ < 1.0f; }
    
    // Filter
    const std::vector<FilterOperation>& GetFilters() const { return filters_; }
    void SetFilters(std::vector<FilterOperation> filters);
    bool HasFilter() const { return !filters_.empty(); }
    
    // Backdrop Filter
    const std::vector<FilterOperation>& GetBackdropFilters() const { return backdrop_filters_; }
    void SetBackdropFilters(std::vector<FilterOperation> filters);
    bool HasBackdropFilter() const { return !backdrop_filters_.empty(); }
    
    // Blend Mode
    SkBlendMode GetBlendMode() const { return blend_mode_; }
    void SetBlendMode(SkBlendMode mode);
    bool HasBlendMode() const { return blend_mode_ != SkBlendMode::kSrcOver; }
    
    // Mask
    RenderObject* GetMask() const { return mask_; }
    void SetMask(RenderObject* mask);
    bool HasMask() const { return mask_ != nullptr; }
    
    // 是否需要隔离（独立渲染表面）
    bool RequiresIsolation() const;
    
    // 关联的变换节点
    TransformTreeNode* GetTransformNode() const { return transform_node_; }
    void SetTransformNode(TransformTreeNode* node) { transform_node_ = node; }
    
    // 关联的裁剪节点（效果输出的裁剪）
    ClipTreeNode* GetOutputClipNode() const { return output_clip_node_; }
    void SetOutputClipNode(ClipTreeNode* node) { output_clip_node_ = node; }
    
    // 计算累积 opacity
    float GetAccumulatedOpacity() const;
    
    // 是否可以直接更新 opacity
    bool CanDirectlyUpdateOpacity() const;
    
    // 关联的 RenderObject
    RenderObject* GetRenderObject() const { return render_object_; }
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }
    
private:
    float opacity_ = 1.0f;
    std::vector<FilterOperation> filters_;
    std::vector<FilterOperation> backdrop_filters_;
    SkBlendMode blend_mode_ = SkBlendMode::kSrcOver;
    RenderObject* mask_ = nullptr;
    TransformTreeNode* transform_node_ = nullptr;
    ClipTreeNode* output_clip_node_ = nullptr;
    RenderObject* render_object_ = nullptr;
    EffectFlags flags_ = EffectFlags::kNone;
};

} // namespace lightui
```

### 5. 滚动树节点

```cpp
// scroll_tree_node.h

namespace lightui {

// 滚动方向
enum class ScrollDirection : uint8_t {
    kNone = 0,
    kHorizontal = 1 << 0,
    kVertical = 1 << 1,
    kBoth = kHorizontal | kVertical,
};

class ScrollTreeNode : public PropertyTreeNode<ScrollTreeNode> {
public:
    // 滚动容器尺寸（可视区域）
    const SkSize& GetContainerSize() const { return container_size_; }
    void SetContainerSize(const SkSize& size);
    
    // 内容尺寸
    const SkSize& GetContentSize() const { return content_size_; }
    void SetContentSize(const SkSize& size);
    
    // 当前滚动偏移
    const SkPoint& GetScrollOffset() const { return scroll_offset_; }
    void SetScrollOffset(const SkPoint& offset);
    
    // 最大滚动偏移
    SkPoint GetMaxScrollOffset() const;
    
    // 滚动方向
    ScrollDirection GetScrollDirection() const;
    bool CanScrollHorizontally() const;
    bool CanScrollVertically() const;
    
    // 关联的变换节点（滚动变换）
    TransformTreeNode* GetScrollTransformNode() const { return scroll_transform_node_; }
    void SetScrollTransformNode(TransformTreeNode* node) { scroll_transform_node_ = node; }
    
    // 是否是根滚动器
    bool IsRootScroller() const { return is_root_scroller_; }
    void SetIsRootScroller(bool is_root) { is_root_scroller_ = is_root; }
    
    // 是否可以合成器线程滚动
    bool CanCompositorScroll() const;
    
    // 关联的 RenderObject
    RenderObject* GetRenderObject() const { return render_object_; }
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }
    
private:
    SkSize container_size_ = {0, 0};
    SkSize content_size_ = {0, 0};
    SkPoint scroll_offset_ = {0, 0};
    TransformTreeNode* scroll_transform_node_ = nullptr;
    bool is_root_scroller_ = false;
    RenderObject* render_object_ = nullptr;
};

} // namespace lightui
```

### 6. 属性树状态

```cpp
// property_tree_state.h

namespace lightui {

// 属性树状态：描述一个绘制块的渲染上下文
class PropertyTreeState {
public:
    PropertyTreeState() = default;
    PropertyTreeState(TransformTreeNode* transform,
                      ClipTreeNode* clip,
                      EffectTreeNode* effect,
                      ScrollTreeNode* scroll = nullptr);
    
    // 获取各树节点
    TransformTreeNode* Transform() const { return transform_; }
    ClipTreeNode* Clip() const { return clip_; }
    EffectTreeNode* Effect() const { return effect_; }
    ScrollTreeNode* Scroll() const { return scroll_; }
    
    // 设置各树节点
    void SetTransform(TransformTreeNode* node) { transform_ = node; }
    void SetClip(ClipTreeNode* node) { clip_ = node; }
    void SetEffect(EffectTreeNode* node) { effect_ = node; }
    void SetScroll(ScrollTreeNode* node) { scroll_ = node; }
    
    // 比较
    bool operator==(const PropertyTreeState& other) const;
    bool operator!=(const PropertyTreeState& other) const;
    
    // 是否可以与另一个状态合并到同一层
    bool CanMergeWith(const PropertyTreeState& other) const;
    
    // 计算与另一个状态的差异
    struct Difference {
        bool transform_changed = false;
        bool clip_changed = false;
        bool effect_changed = false;
        bool scroll_changed = false;
        
        bool HasAnyChange() const {
            return transform_changed || clip_changed || 
                   effect_changed || scroll_changed;
        }
    };
    Difference ComputeDifference(const PropertyTreeState& other) const;
    
    // 获取根状态
    static PropertyTreeState Root();
    
private:
    TransformTreeNode* transform_ = nullptr;
    ClipTreeNode* clip_ = nullptr;
    EffectTreeNode* effect_ = nullptr;
    ScrollTreeNode* scroll_ = nullptr;
};

} // namespace lightui
```

### 7. 属性树集合

```cpp
// property_trees.h

namespace lightui {

// 属性树集合：管理所有四棵属性树
class PropertyTrees {
public:
    PropertyTrees();
    ~PropertyTrees();
    
    // 获取各树
    TransformTree& GetTransformTree() { return transform_tree_; }
    ClipTree& GetClipTree() { return clip_tree_; }
    EffectTree& GetEffectTree() { return effect_tree_; }
    ScrollTree& GetScrollTree() { return scroll_tree_; }
    
    const TransformTree& GetTransformTree() const { return transform_tree_; }
    const ClipTree& GetClipTree() const { return clip_tree_; }
    const EffectTree& GetEffectTree() const { return effect_tree_; }
    const ScrollTree& GetScrollTree() const { return scroll_tree_; }
    
    // 获取根状态
    PropertyTreeState GetRootState() const;
    
    // 清除所有树
    void Clear();
    
    // 版本号（用于检测变化）
    uint64_t GetVersion() const { return version_; }
    void IncrementVersion() { version_++; }
    
    // 是否有任何脏节点
    bool HasDirtyNodes() const;
    
    // 清除所有脏标记
    void ClearAllDirtyFlags();
    
private:
    TransformTree transform_tree_;
    ClipTree clip_tree_;
    EffectTree effect_tree_;
    ScrollTree scroll_tree_;
    uint64_t version_ = 0;
};

// 单棵属性树的模板类
template <typename NodeType>
class PropertyTree {
public:
    // 创建节点
    NodeType* CreateNode(NodeType* parent = nullptr);
    
    // 删除节点
    void RemoveNode(NodeType* node);
    
    // 获取根节点
    NodeType* GetRoot() const { return root_; }
    
    // 通过 ID 查找节点
    NodeType* GetNodeById(PropertyTreeNodeId id) const;
    
    // 通过 RenderObject 查找节点
    NodeType* GetNodeForRenderObject(RenderObject* obj) const;
    
    // 节点数量
    size_t GetNodeCount() const { return nodes_.size(); }
    
    // 清除所有节点
    void Clear();
    
private:
    NodeType* root_ = nullptr;
    std::vector<std::unique_ptr<NodeType>> nodes_;
    std::unordered_map<PropertyTreeNodeId, NodeType*> id_to_node_;
    std::unordered_map<RenderObject*, NodeType*> render_object_to_node_;
    PropertyTreeNodeId next_id_ = 1;
};

using TransformTree = PropertyTree<TransformTreeNode>;
using ClipTree = PropertyTree<ClipTreeNode>;
using EffectTree = PropertyTree<EffectTreeNode>;
using ScrollTree = PropertyTree<ScrollTreeNode>;

} // namespace lightui
```

### 8. 属性树构建器

```cpp
// property_tree_builder.h

namespace lightui {

// 属性树构建器：从 RenderObject 树构建属性树
class PropertyTreeBuilder {
public:
    PropertyTreeBuilder(PropertyTrees& trees);
    
    // 完整构建
    void Build(RenderObject* root);
    
    // 增量更新
    void Update(RenderObject* changed_node);
    
    // 获取 RenderObject 的属性树状态
    PropertyTreeState GetStateForRenderObject(RenderObject* obj) const;
    
private:
    // 递归构建
    void BuildRecursive(RenderObject* obj, const PropertyTreeState& parent_state);
    
    // 创建变换节点（如果需要）
    TransformTreeNode* CreateTransformNodeIfNeeded(
        RenderObject* obj, 
        TransformTreeNode* parent);
    
    // 创建裁剪节点（如果需要）
    ClipTreeNode* CreateClipNodeIfNeeded(
        RenderObject* obj,
        ClipTreeNode* parent,
        TransformTreeNode* transform);
    
    // 创建效果节点（如果需要）
    EffectTreeNode* CreateEffectNodeIfNeeded(
        RenderObject* obj,
        EffectTreeNode* parent,
        TransformTreeNode* transform,
        ClipTreeNode* clip);
    
    // 创建滚动节点（如果需要）
    ScrollTreeNode* CreateScrollNodeIfNeeded(
        RenderObject* obj,
        ScrollTreeNode* parent,
        TransformTreeNode* transform);
    
    // 判断是否需要创建各类节点
    bool NeedsTransformNode(RenderObject* obj) const;
    bool NeedsClipNode(RenderObject* obj) const;
    bool NeedsEffectNode(RenderObject* obj) const;
    bool NeedsScrollNode(RenderObject* obj) const;
    
    PropertyTrees& trees_;
    std::unordered_map<RenderObject*, PropertyTreeState> render_object_states_;
};

} // namespace lightui
```

### 9. 几何映射器

```cpp
// geometry_mapper.h

namespace lightui {

// 几何映射器：在不同属性树状态之间转换坐标和区域
class GeometryMapper {
public:
    GeometryMapper(const PropertyTrees& trees);
    
    // 点转换
    SkPoint MapPoint(const SkPoint& point,
                     const PropertyTreeState& source,
                     const PropertyTreeState& target) const;
    
    // 矩形转换（考虑变换）
    SkRect MapRect(const SkRect& rect,
                   const PropertyTreeState& source,
                   const PropertyTreeState& target) const;
    
    // 可见区域计算（考虑变换和裁剪）
    SkRect MapVisualRect(const SkRect& rect,
                         const PropertyTreeState& source,
                         const PropertyTreeState& target) const;
    
    // 计算两个状态之间的变换矩阵
    SkM44 GetTransformMatrix(const PropertyTreeState& source,
                             const PropertyTreeState& target) const;
    
    // 计算在目标状态中的裁剪区域
    SkRect GetClipRect(const PropertyTreeState& source,
                       const PropertyTreeState& target) const;
    
    // 检查点是否在裁剪区域内
    bool PointInClip(const SkPoint& point,
                     const PropertyTreeState& state) const;
    
    // 检查矩形是否与裁剪区域相交
    bool RectIntersectsClip(const SkRect& rect,
                            const PropertyTreeState& state) const;
    
    // 缓存管理
    void ClearCache();
    void InvalidateCacheForNode(PropertyTreeNodeId node_id);
    
private:
    // 缓存的变换矩阵
    struct TransformCacheKey {
        PropertyTreeNodeId source_transform;
        PropertyTreeNodeId target_transform;
        bool operator==(const TransformCacheKey& other) const;
    };
    struct TransformCacheKeyHash {
        size_t operator()(const TransformCacheKey& key) const;
    };
    mutable std::unordered_map<TransformCacheKey, SkM44, TransformCacheKeyHash> 
        transform_cache_;
    
    // 缓存的裁剪区域
    struct ClipCacheKey {
        PropertyTreeNodeId clip_node;
        PropertyTreeNodeId target_transform;
        bool operator==(const ClipCacheKey& other) const;
    };
    struct ClipCacheKeyHash {
        size_t operator()(const ClipCacheKey& key) const;
    };
    mutable std::unordered_map<ClipCacheKey, SkRect, ClipCacheKeyHash> 
        clip_cache_;
    
    const PropertyTrees& trees_;
};

} // namespace lightui
```

### 10. 绘制块

```cpp
// paint_chunk.h

namespace lightui {

// 绘制块：具有相同属性树状态的连续绘制指令
class PaintChunk {
public:
    // 属性树状态
    const PropertyTreeState& GetState() const { return state_; }
    void SetState(const PropertyTreeState& state) { state_ = state; }
    
    // 绘制指令范围
    size_t GetBeginIndex() const { return begin_index_; }
    size_t GetEndIndex() const { return end_index_; }
    void SetRange(size_t begin, size_t end);
    
    // 边界（在本地坐标系中）
    const SkRect& GetBounds() const { return bounds_; }
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }
    void UnionBounds(const SkRect& rect);
    
    // 是否可以与另一个块合并
    bool CanMergeWith(const PaintChunk& other) const;
    
    // 合并另一个块
    void MergeWith(const PaintChunk& other);
    
    // 光栅化失效区域
    const std::vector<SkRect>& GetRasterInvalidationRects() const { 
        return raster_invalidation_rects_; 
    }
    void AddRasterInvalidationRect(const SkRect& rect);
    void ClearRasterInvalidationRects();
    
    // 合成原因
    CompositingReasons GetCompositingReasons() const { return compositing_reasons_; }
    void SetCompositingReasons(CompositingReasons reasons) { 
        compositing_reasons_ = reasons; 
    }
    
    // 关联的 RenderObject（主要用于调试）
    RenderObject* GetRenderObject() const { return render_object_; }
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }
    
    // 唯一标识符
    uint64_t GetId() const { return id_; }
    
private:
    PropertyTreeState state_;
    size_t begin_index_ = 0;
    size_t end_index_ = 0;
    SkRect bounds_ = SkRect::MakeEmpty();
    std::vector<SkRect> raster_invalidation_rects_;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;
    RenderObject* render_object_ = nullptr;
    uint64_t id_ = 0;
    static uint64_t next_id_;
};

} // namespace lightui
```

### 11. 绘制产物

```cpp
// paint_artifact.h

namespace lightui {

// 绘制产物：包含所有绘制指令和绘制块
class PaintArtifact {
public:
    // 绘制指令列表
    const std::vector<DisplayItem>& GetDisplayItems() const { return display_items_; }
    std::vector<DisplayItem>& GetDisplayItems() { return display_items_; }
    
    // 绘制块列表
    const std::vector<PaintChunk>& GetPaintChunks() const { return paint_chunks_; }
    std::vector<PaintChunk>& GetPaintChunks() { return paint_chunks_; }
    
    // 添加绘制指令
    void AppendDisplayItem(DisplayItem item);
    
    // 开始新的绘制块
    void StartNewChunk(const PropertyTreeState& state);
    
    // 结束当前绘制块
    void FinishCurrentChunk();
    
    // 清除所有内容
    void Clear();
    
    // 是否为空
    bool IsEmpty() const { return display_items_.empty(); }
    
    // 与上一帧比较，计算变化
    struct ChangeInfo {
        std::vector<size_t> added_chunks;
        std::vector<size_t> removed_chunks;
        std::vector<size_t> modified_chunks;
        bool property_trees_changed = false;
    };
    ChangeInfo ComputeChanges(const PaintArtifact& previous) const;
    
private:
    std::vector<DisplayItem> display_items_;
    std::vector<PaintChunk> paint_chunks_;
    size_t current_chunk_begin_ = 0;
};

} // namespace lightui
```

### 12. 层化器

```cpp
// layerizer.h

namespace lightui {

// 待定层：层化过程中的中间表示
class PendingLayer {
public:
    // 合成类型
    enum class CompositingType {
        kNone,              // 不需要独立层
        kOverlap,           // 因重叠需要独立层
        kDirectReason,      // 因直接原因需要独立层（will-change、动画等）
        kForeignLayer,      // 外部层（video、canvas 等）
    };
    
    // 包含的绘制块
    const std::vector<const PaintChunk*>& GetChunks() const { return chunks_; }
    void AddChunk(const PaintChunk* chunk);
    
    // 属性树状态
    const PropertyTreeState& GetState() const { return state_; }
    void SetState(const PropertyTreeState& state) { state_ = state; }
    
    // 边界
    const SkRect& GetBounds() const { return bounds_; }
    void UpdateBounds();
    
    // 合成类型
    CompositingType GetCompositingType() const { return compositing_type_; }
    void SetCompositingType(CompositingType type) { compositing_type_ = type; }
    
    // 合成原因
    CompositingReasons GetCompositingReasons() const { return compositing_reasons_; }
    void SetCompositingReasons(CompositingReasons reasons) { 
        compositing_reasons_ = reasons; 
    }
    
    // 是否可以与另一个待定层合并
    bool CanMergeWith(const PendingLayer& other) const;
    
    // 合并另一个待定层
    void MergeWith(PendingLayer&& other);
    
private:
    std::vector<const PaintChunk*> chunks_;
    PropertyTreeState state_;
    SkRect bounds_ = SkRect::MakeEmpty();
    CompositingType compositing_type_ = CompositingType::kNone;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;
};

// 层化器：将绘制块分配到合成层
class Layerizer {
public:
    Layerizer(const PropertyTrees& trees, const GeometryMapper& mapper);
    
    // 执行层化
    std::vector<PendingLayer> Layerize(const PaintArtifact& artifact);
    
private:
    // 判断绘制块的合成类型
    PendingLayer::CompositingType DetermineCompositingType(
        const PaintChunk& chunk) const;
    
    // 获取绘制块的合成原因
    CompositingReasons GetCompositingReasons(const PaintChunk& chunk) const;
    
    // 检查是否与已有层重叠
    bool OverlapsWithExistingLayers(
        const PaintChunk& chunk,
        const std::vector<PendingLayer>& layers) const;
    
    // 尝试合并到已有层
    bool TryMergeIntoExistingLayer(
        const PaintChunk& chunk,
        std::vector<PendingLayer>& layers) const;
    
    const PropertyTrees& trees_;
    const GeometryMapper& mapper_;
};

} // namespace lightui
```

### 13. 光栅化失效器

```cpp
// raster_invalidator.h

namespace lightui {

// 光栅化失效器：计算需要重新光栅化的区域
class RasterInvalidator {
public:
    RasterInvalidator(const GeometryMapper& mapper);
    
    // 计算失效区域
    struct InvalidationResult {
        std::vector<SkRect> rects;  // 失效区域列表
        bool full_invalidation = false;  // 是否需要完整重绘
    };
    
    // 比较新旧绘制产物，计算失效区域
    InvalidationResult ComputeInvalidation(
        const PaintArtifact& old_artifact,
        const PaintArtifact& new_artifact,
        const PropertyTreeState& layer_state);
    
    // 比较新旧绘制块，计算失效区域
    InvalidationResult ComputeChunkInvalidation(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state);
    
private:
    // 处理出现的绘制块
    void HandleAppearingChunk(
        const PaintChunk& chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);
    
    // 处理消失的绘制块
    void HandleDisappearingChunk(
        const PaintChunk& chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);
    
    // 处理移动的绘制块
    void HandleMovedChunk(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);
    
    // 处理内容变化的绘制块
    void HandleContentChangedChunk(
        const PaintChunk& old_chunk,
        const PaintChunk& new_chunk,
        const PropertyTreeState& layer_state,
        InvalidationResult& result);
    
    const GeometryMapper& mapper_;
};

} // namespace lightui
```

### 14. 合成层

```cpp
// compositor_layer.h

namespace lightui {

// 合成层：最终的 GPU 层
class CompositorLayer {
public:
    CompositorLayer();
    ~CompositorLayer();
    
    // 唯一标识符
    uint64_t GetId() const { return id_; }
    
    // 属性树状态
    const PropertyTreeState& GetState() const { return state_; }
    void SetState(const PropertyTreeState& state) { state_ = state; }
    
    // 边界（在层空间中）
    const SkRect& GetBounds() const { return bounds_; }
    void SetBounds(const SkRect& bounds);
    
    // 位图/纹理
    SkBitmap& GetBitmap() { return bitmap_; }
    const SkBitmap& GetBitmap() const { return bitmap_; }
    bool EnsureBitmap();
    
    // GPU 纹理 ID
    uint32_t GetTextureId() const { return texture_id_; }
    void SetTextureId(uint32_t id) { texture_id_ = id; }
    bool HasTexture() const { return texture_id_ != 0; }
    
    // 脏区域
    void MarkDirty(const SkRect& rect);
    void MarkFullDirty();
    void ClearDirtyRegions();
    bool HasDirtyRegions() const { return !dirty_regions_.empty(); }
    const std::vector<SkIRect>& GetDirtyRegions() const { return dirty_regions_; }
    
    // 纹理脏区域（需要上传到 GPU）
    void MarkTextureDirty(const SkIRect& rect);
    void ClearTextureDirtyRegions();
    bool IsTextureDirty() const { return !texture_dirty_regions_.empty(); }
    const std::vector<SkIRect>& GetTextureDirtyRegions() const { 
        return texture_dirty_regions_; 
    }
    
    // 包含的绘制块
    const std::vector<const PaintChunk*>& GetChunks() const { return chunks_; }
    void SetChunks(std::vector<const PaintChunk*> chunks);
    
    // 子层
    const std::vector<std::shared_ptr<CompositorLayer>>& GetChildren() const { 
        return children_; 
    }
    void AddChild(std::shared_ptr<CompositorLayer> child);
    void RemoveChild(CompositorLayer* child);
    void ClearChildren();
    
    // 父层
    CompositorLayer* GetParent() const { return parent_; }
    
    // 合成原因（用于调试）
    CompositingReasons GetCompositingReasons() const { return compositing_reasons_; }
    void SetCompositingReasons(CompositingReasons reasons) { 
        compositing_reasons_ = reasons; 
    }
    
    // 调试名称
    const std::string& GetDebugName() const { return debug_name_; }
    void SetDebugName(const std::string& name) { debug_name_ = name; }
    
private:
    uint64_t id_;
    PropertyTreeState state_;
    SkRect bounds_ = SkRect::MakeEmpty();
    SkBitmap bitmap_;
    uint32_t texture_id_ = 0;
    std::vector<SkIRect> dirty_regions_;
    std::vector<SkIRect> texture_dirty_regions_;
    std::vector<const PaintChunk*> chunks_;
    std::vector<std::shared_ptr<CompositorLayer>> children_;
    CompositorLayer* parent_ = nullptr;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;
    std::string debug_name_;
    
    static uint64_t next_id_;
};

} // namespace lightui
```

### 15. 绘制产物合成器

```cpp
// paint_artifact_compositor.h

namespace lightui {

// 更新类型
enum class UpdateType {
    kNone,                  // 无需更新
    kDirectPropertyUpdate,  // 直接属性更新（transform/opacity/scroll）
    kRepaint,               // 重绘更新（内容变化，层结构不变）
    kFull,                  // 完整更新（层结构变化）
};

// 绘制产物合成器：核心合成器类
class PaintArtifactCompositor {
public:
    PaintArtifactCompositor();
    ~PaintArtifactCompositor();
    
    // 完整更新
    void Update(const PaintArtifact& artifact, const PropertyTrees& trees);
    
    // 尝试快速路径更新
    // 返回 true 表示快速路径成功，不需要完整更新
    bool TryFastPathUpdate(const PaintArtifact& artifact, 
                           const PropertyTrees& trees);
    
    // 直接属性更新
    bool DirectlyUpdateTransform(TransformTreeNode* node);
    bool DirectlyUpdateOpacity(EffectTreeNode* node);
    bool DirectlyUpdateScrollOffset(ScrollTreeNode* node);
    
    // 获取需要的更新类型
    UpdateType GetNeededUpdateType() const { return needed_update_; }
    void SetNeedsUpdate(UpdateType type);
    void ClearNeedsUpdate() { needed_update_ = UpdateType::kNone; }
    
    // 获取合成层列表
    const std::vector<std::shared_ptr<CompositorLayer>>& GetLayers() const { 
        return layers_; 
    }
    
    // 获取根层
    CompositorLayer* GetRootLayer() const { return root_layer_.get(); }
    
    // 光栅化脏层
    int RasterizeDirtyLayers();
    
    // 合成到 Canvas
    bool CompositeToCanvas(SkCanvas* canvas);
    
    // 合成到 GPU（如果可用）
    bool CompositeToGPU();
    
    // 上传脏纹理到 GPU
    int UploadDirtyTextures();
    
    // 统计信息
    struct Stats {
        int layers_count = 0;
        int layers_rasterized = 0;
        int textures_uploaded = 0;
        double rasterize_time_ms = 0;
        double composite_time_ms = 0;
        bool used_fast_path = false;
    };
    const Stats& GetStats() const { return stats_; }
    void ResetStats();
    
private:
    // 从待定层创建合成层
    void CreateLayersFromPendingLayers(const std::vector<PendingLayer>& pending);
    
    // 更新现有层（重绘路径）
    void UpdateExistingLayers(const PaintArtifact& artifact);
    
    // 匹配新旧待定层
    void MatchPendingLayers(
        const std::vector<PendingLayer>& old_pending,
        const std::vector<PendingLayer>& new_pending);
    
    // 光栅化单个层
    bool RasterizeLayer(CompositorLayer* layer);
    
    // 合成单个层（CPU）
    void CompositeLayerCPU(CompositorLayer* layer, 
                           SkCanvas* canvas, 
                           const SkM44& parent_transform);
    
    // 合成单个层（GPU）
    void CompositeLayerGPU(CompositorLayer* layer, 
                           const SkM44& parent_transform);
    
    std::shared_ptr<CompositorLayer> root_layer_;
    std::vector<std::shared_ptr<CompositorLayer>> layers_;
    std::vector<PendingLayer> pending_layers_;
    
    std::unique_ptr<Layerizer> layerizer_;
    std::unique_ptr<RasterInvalidator> raster_invalidator_;
    std::unique_ptr<GeometryMapper> geometry_mapper_;
    
    UpdateType needed_update_ = UpdateType::kFull;
    Stats stats_;
    
    // GPU 资源
    uint32_t shader_program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    bool gpu_initialized_ = false;
};

} // namespace lightui
```

## 数据模型

### 合成原因

```cpp
// compositing_reasons.h

namespace lightui {

// 合成原因位标志
enum class CompositingReasons : uint64_t {
    kNone = 0,
    
    // 直接原因（元素自身属性）
    k3DTransform = 1ULL << 0,
    kWillChangeTransform = 1ULL << 1,
    kWillChangeOpacity = 1ULL << 2,
    kWillChangeFilter = 1ULL << 3,
    kActiveTransformAnimation = 1ULL << 4,
    kActiveOpacityAnimation = 1ULL << 5,
    kActiveFilterAnimation = 1ULL << 6,
    kFixedPosition = 1ULL << 7,
    kStickyPosition = 1ULL << 8,
    kOverflowScrolling = 1ULL << 9,
    kBackdropFilter = 1ULL << 10,
    kVideo = 1ULL << 11,
    kCanvas = 1ULL << 12,
    kIFrame = 1ULL << 13,
    
    // 间接原因（因其他元素）
    kOverlap = 1ULL << 20,
    kAssumedOverlap = 1ULL << 21,
    kNegativeZIndex = 1ULL << 22,
    
    // 组合
    kDirectReasons = k3DTransform | kWillChangeTransform | kWillChangeOpacity |
                     kWillChangeFilter | kActiveTransformAnimation |
                     kActiveOpacityAnimation | kActiveFilterAnimation |
                     kFixedPosition | kStickyPosition | kOverflowScrolling |
                     kBackdropFilter | kVideo | kCanvas | kIFrame,
    
    kAnimationReasons = kActiveTransformAnimation | kActiveOpacityAnimation |
                        kActiveFilterAnimation,
    
    kWillChangeReasons = kWillChangeTransform | kWillChangeOpacity |
                         kWillChangeFilter,
};

// 位运算支持
inline CompositingReasons operator|(CompositingReasons a, CompositingReasons b) {
    return static_cast<CompositingReasons>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline CompositingReasons operator&(CompositingReasons a, CompositingReasons b) {
    return static_cast<CompositingReasons>(
        static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline bool HasReason(CompositingReasons reasons, CompositingReasons flag) {
    return (reasons & flag) != CompositingReasons::kNone;
}

// 获取合成原因的描述
std::vector<std::string> GetCompositingReasonDescriptions(CompositingReasons reasons);

} // namespace lightui
```

### 显示项

```cpp
// display_item.h

namespace lightui {

// 显示项类型
enum class DisplayItemType {
    kDrawing,           // 绘制指令
    kForeignLayer,      // 外部层（video、canvas）
    kScrollbar,         // 滚动条
    kClipPath,          // 裁剪路径
    kMask,              // 遮罩
};

// 显示项：最小绘制单元
class DisplayItem {
public:
    DisplayItem(DisplayItemType type, RenderObject* client);
    
    // 类型
    DisplayItemType GetType() const { return type_; }
    
    // 关联的 RenderObject
    RenderObject* GetClient() const { return client_; }
    
    // 绘制记录（对于 kDrawing 类型）
    const sk_sp<SkPicture>& GetPicture() const { return picture_; }
    void SetPicture(sk_sp<SkPicture> picture) { picture_ = std::move(picture); }
    
    // 边界
    const SkRect& GetBounds() const { return bounds_; }
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }
    
    // 是否可缓存
    bool IsCacheable() const { return cacheable_; }
    void SetCacheable(bool cacheable) { cacheable_ = cacheable; }
    
    // 唯一标识符（用于缓存匹配）
    struct Id {
        RenderObject* client;
        DisplayItemType type;
        uint32_t fragment_index;  // 对于分片的元素
        
        bool operator==(const Id& other) const;
    };
    Id GetId() const;
    
private:
    DisplayItemType type_;
    RenderObject* client_;
    sk_sp<SkPicture> picture_;
    SkRect bounds_ = SkRect::MakeEmpty();
    bool cacheable_ = true;
    uint32_t fragment_index_ = 0;
};

} // namespace lightui
```

## 错误处理

### 错误类型

```cpp
// property_tree_error.h

namespace lightui {

enum class PropertyTreeError {
    kNone,
    kInvalidNodeId,
    kNodeNotFound,
    kCyclicDependency,
    kInvalidState,
    kTransformNotInvertible,
    kClipPathTooComplex,
    kOutOfMemory,
    kGPUContextLost,
    kTextureUploadFailed,
};

class PropertyTreeException : public std::exception {
public:
    PropertyTreeException(PropertyTreeError error, const std::string& message);
    PropertyTreeError GetError() const { return error_; }
    const char* what() const noexcept override { return message_.c_str(); }
    
private:
    PropertyTreeError error_;
    std::string message_;
};

} // namespace lightui
```

### 错误处理策略

1. **节点查找失败**：返回 nullptr，调用者检查
2. **变换不可逆**：使用近似逆变换或回退到单位变换
3. **裁剪路径过于复杂**：简化为边界矩形
4. **GPU 上下文丢失**：回退到 CPU 渲染
5. **纹理上传失败**：标记层为脏，下帧重试
6. **内存不足**：减少层数量，合并更多层

## 测试策略

### 单元测试

1. **属性树节点测试**
   - 节点创建、删除、父子关系
   - 脏标记传播
   - 版本号更新

2. **变换计算测试**
   - 累积变换计算
   - 变换矩阵求逆
   - 3D 变换和透视

3. **裁剪计算测试**
   - 裁剪区域求交
   - 圆角裁剪
   - clip-path

4. **几何映射测试**
   - 点和矩形转换
   - 可见区域计算
   - 缓存正确性

### 属性测试

1. **变换累积一致性**
   - 对于任意变换序列，累积结果应与逐个应用相同

2. **裁剪区域单调性**
   - 子节点的裁剪区域应始终包含在父节点裁剪区域内

3. **层化幂等性**
   - 对同一绘制产物多次层化应产生相同结果

4. **光栅化失效完整性**
   - 任何可见变化都应产生对应的失效区域

### 集成测试

1. **动画性能测试**
   - transform 动画不触发光栅化
   - opacity 动画不触发光栅化
   - 滚动不触发光栅化

2. **内存测试**
   - 层数量合理
   - 纹理内存可控

3. **视觉正确性测试**
   - 与旧渲染路径对比
   - 截图对比测试

## 正确性属性

*属性是系统在所有有效执行中应保持为真的特征或行为——本质上是关于系统应该做什么的形式化陈述。属性作为人类可读规范和机器可验证正确性保证之间的桥梁。*

### Property 1: 变换累积一致性
*对于任意* 变换树和任意节点，该节点的累积变换应等于从根节点到该节点路径上所有变换矩阵的乘积
**Validates: Requirements 1.2**

### Property 2: 变换直接更新不触发光栅化
*对于任意* 已有独立层的元素，当其 transform 属性变化时，光栅化计数应保持不变
**Validates: Requirements 1.3, 8.1**

### Property 3: 滚动偏移正确应用
*对于任意* 滚动容器，滚动后其关联变换节点的矩阵应包含正确的平移分量
**Validates: Requirements 1.4, 4.2**

### Property 4: 裁剪区域单调性
*对于任意* 裁剪树中的节点，其累积裁剪区域应始终包含在其父节点的累积裁剪区域内
**Validates: Requirements 2.3**

### Property 5: 裁剪失效区域正确性
*对于任意* 裁剪区域变化，失效区域应等于旧裁剪区域和新裁剪区域的对称差集
**Validates: Requirements 2.4, 9.4**

### Property 6: Opacity 直接更新不触发光栅化
*对于任意* 已有独立层的元素，当其 opacity 属性变化时，光栅化计数应保持不变
**Validates: Requirements 3.2, 8.2**

### Property 7: 效果隔离正确性
*对于任意* 具有 blend-mode 或 backdrop-filter 的元素，RequiresIsolation 应返回 true
**Validates: Requirements 3.4, 3.5**

### Property 8: 滚动范围正确性
*对于任意* 滚动节点，GetMaxScrollOffset 应返回 (contentSize - containerSize) 且不小于 0
**Validates: Requirements 4.4**

### Property 9: 滚动直接更新不触发光栅化
*对于任意* 滚动容器，滚动偏移变化时光栅化计数应保持不变
**Validates: Requirements 4.5, 8.3, 10.3**

### Property 10: 属性树状态合并正确性
*对于任意* 两个属性树状态，CanMergeWith 返回 true 当且仅当它们的所有属性节点相同或兼容
**Validates: Requirements 5.2**

### Property 11: 公共祖先正确性
*对于任意* 两个节点，FindCommonAncestor 返回的节点应是两者的祖先，且是最近的公共祖先
**Validates: Requirements 5.4**

### Property 12: 几何映射往返一致性
*对于任意* 点和两个属性树状态 A、B，从 A 映射到 B 再映射回 A 应得到原始点（在数值精度范围内）
**Validates: Requirements 6.1, 6.2**

### Property 13: 可见区域包含性
*对于任意* 矩形和属性树状态，MapVisualRect 的结果应包含在 MapRect 的结果与累积裁剪区域的交集内
**Validates: Requirements 6.3**

### Property 14: 缓存一致性
*对于任意* 属性树变化，缓存失效后重新计算的结果应与无缓存计算的结果相同
**Validates: Requirements 6.5**

### Property 15: will-change 层提升
*对于任意* 具有 will-change: transform 或 will-change: opacity 的元素，层化后应有独立层
**Validates: Requirements 7.1**

### Property 16: 动画层提升
*对于任意* 具有活动 transform 或 opacity 动画的元素，层化后应有独立层
**Validates: Requirements 7.2**

### Property 17: 层化幂等性
*对于任意* 绘制产物，多次层化应产生相同的层结构
**Validates: Requirements 7.3**

### Property 18: 重叠检测正确性
*对于任意* 两个绘制块，如果它们的可见区域相交且无法合并，则应在不同层
**Validates: Requirements 7.4**

### Property 19: 直接更新后只触发合成
*对于任意* 直接属性更新，合成计数应增加但光栅化计数应不变
**Validates: Requirements 8.4**

### Property 20: 层结构变化触发完整更新
*对于任意* 导致层结构变化的属性变化，更新类型应为 kFull
**Validates: Requirements 8.5**

### Property 21: 失效区域最小性
*对于任意* 绘制内容变化，失效区域应是变化内容的边界框
**Validates: Requirements 9.1**

### Property 22: 出现/消失失效正确性
*对于任意* 出现或消失的绘制块，失效区域应包含该块的边界
**Validates: Requirements 9.2**

### Property 23: 移动失效正确性
*对于任意* 移动的绘制块，失效区域应包含旧位置和新位置
**Validates: Requirements 9.3**

### Property 24: 非光栅化属性变化不失效
*对于任意* transform 或 opacity 变化（在已有独立层的元素上），失效区域应为空
**Validates: Requirements 9.5**

## 与现有系统的集成

### 迁移策略

```
阶段 1: 并行运行
┌─────────────────────────────────────────────────────────────────────┐
│                         RenderObject Tree                            │
└─────────────────────────────────────────────────────────────────────┘
                    │                           │
                    ▼                           ▼
    ┌───────────────────────────┐   ┌───────────────────────────┐
    │   旧渲染路径 (LayerTree)   │   │  新渲染路径 (PropertyTree) │
    │   - 默认启用               │   │  - 通过标志启用            │
    │   - 作为回退               │   │  - 逐步验证                │
    └───────────────────────────┘   └───────────────────────────┘

阶段 2: 新路径为主
┌─────────────────────────────────────────────────────────────────────┐
│                         RenderObject Tree                            │
└─────────────────────────────────────────────────────────────────────┘
                    │                           │
                    ▼                           ▼
    ┌───────────────────────────┐   ┌───────────────────────────┐
    │   旧渲染路径 (LayerTree)   │   │  新渲染路径 (PropertyTree) │
    │   - 作为回退               │   │  - 默认启用                │
    │   - 错误时自动切换         │   │  - 完整功能                │
    └───────────────────────────┘   └───────────────────────────┘

阶段 3: 移除旧路径
┌─────────────────────────────────────────────────────────────────────┐
│                         RenderObject Tree                            │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────┐
                    │  新渲染路径 (PropertyTree) │
                    │  - 唯一路径                │
                    └───────────────────────────┘
```

### 接口适配

```cpp
// window_compositor_adapter.h 修改

class WindowCompositorAdapter {
public:
    // 新增：使用属性树系统
    void SetUsePropertyTreeSystem(bool use);
    bool IsUsingPropertyTreeSystem() const;
    
    // 新增：获取属性树
    PropertyTrees* GetPropertyTrees();
    
    // 新增：获取绘制产物合成器
    PaintArtifactCompositor* GetPaintArtifactCompositor();
    
    // 现有接口保持不变
    bool Render(RenderObject* render_tree, SkCanvas* canvas);
    // ...
    
private:
    // 新增
    std::unique_ptr<PropertyTrees> property_trees_;
    std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
    std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;
    bool use_property_tree_system_ = false;
};
```

### RenderObject 扩展

```cpp
// render_object.h 新增

class RenderObject {
public:
    // 新增：属性树状态
    const PropertyTreeState& GetPropertyTreeState() const { 
        return property_tree_state_; 
    }
    void SetPropertyTreeState(const PropertyTreeState& state) { 
        property_tree_state_ = state; 
    }
    
    // 新增：是否需要属性树节点
    bool NeedsTransformNode() const;
    bool NeedsClipNode() const;
    bool NeedsEffectNode() const;
    bool NeedsScrollNode() const;
    
    // 新增：直接属性更新支持
    bool CanDirectlyUpdateTransform() const;
    bool CanDirectlyUpdateOpacity() const;
    
private:
    PropertyTreeState property_tree_state_;
};
```

## 文件结构

```
core/
├── compositor/
│   ├── property_tree/
│   │   ├── property_tree_node.h          # 属性树节点基类
│   │   ├── transform_tree_node.h         # 变换树节点
│   │   ├── transform_tree_node.cpp
│   │   ├── clip_tree_node.h              # 裁剪树节点
│   │   ├── clip_tree_node.cpp
│   │   ├── effect_tree_node.h            # 效果树节点
│   │   ├── effect_tree_node.cpp
│   │   ├── scroll_tree_node.h            # 滚动树节点
│   │   ├── scroll_tree_node.cpp
│   │   ├── property_tree_state.h         # 属性树状态
│   │   ├── property_tree_state.cpp
│   │   ├── property_trees.h              # 属性树集合
│   │   ├── property_trees.cpp
│   │   ├── property_tree_builder.h       # 属性树构建器
│   │   └── property_tree_builder.cpp
│   ├── paint/
│   │   ├── display_item.h                # 显示项
│   │   ├── display_item.cpp
│   │   ├── paint_chunk.h                 # 绘制块
│   │   ├── paint_chunk.cpp
│   │   ├── paint_artifact.h              # 绘制产物
│   │   ├── paint_artifact.cpp
│   │   ├── paint_controller.h            # 绘制控制器
│   │   └── paint_controller.cpp
│   ├── compositing/
│   │   ├── compositing_reasons.h         # 合成原因
│   │   ├── geometry_mapper.h             # 几何映射器
│   │   ├── geometry_mapper.cpp
│   │   ├── layerizer.h                   # 层化器
│   │   ├── layerizer.cpp
│   │   ├── raster_invalidator.h          # 光栅化失效器
│   │   ├── raster_invalidator.cpp
│   │   ├── pending_layer.h               # 待定层
│   │   ├── pending_layer.cpp
│   │   ├── compositor_layer.h            # 合成层（重构）
│   │   ├── compositor_layer.cpp
│   │   ├── paint_artifact_compositor.h   # 绘制产物合成器
│   │   └── paint_artifact_compositor.cpp
│   └── CMakeLists.txt
└── ...

tests/
├── property/
│   └── compositor/
│       ├── test_transform_tree_properties.cpp
│       ├── test_clip_tree_properties.cpp
│       ├── test_effect_tree_properties.cpp
│       ├── test_scroll_tree_properties.cpp
│       ├── test_geometry_mapper_properties.cpp
│       ├── test_layerizer_properties.cpp
│       └── test_raster_invalidator_properties.cpp
└── unit/
    └── compositor/
        ├── test_property_tree_node.cpp
        ├── test_property_tree_builder.cpp
        ├── test_paint_artifact.cpp
        └── test_paint_artifact_compositor.cpp
```
