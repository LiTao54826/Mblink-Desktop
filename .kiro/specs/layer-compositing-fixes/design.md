# Design Document: Layer Compositing Bug Fixes

## Overview

本设计文档描述修复 LightUI 分层合成系统中发现的关键 Bug。修复目标是让系统符合原始设计文档中定义的属性和需求。

## Architecture Changes

### 1. 移除强制全量光栅化

**当前问题代码:**

```cpp
// render_pipeline_v2.cpp:360-362
if (root_layer_ && CheckRenderObjectNeedsPaint(root)) {
    root_layer_->MarkFullDirty();  // 问题：总是标记根层为脏
}

// window.cpp:1523-1526
if (has_animations_running) {
    compositor_adapter_->ForceRasterize();  // 问题：强制全量光栅化
}
```

**修复方案:**

```cpp
// render_pipeline_v2.cpp - 修改 BuildLayerTree()
void RenderPipelineV2::BuildLayerTree(RenderObject* root) {
    if (!root_layer_ || needs_rebuild_layer_tree_) {
        root_layer_ = layer_tree_builder_->Build(root);
        needs_rebuild_layer_tree_ = false;
    } else {
        UpdateLayerTreeBounds(root_layer_.get());
    }
    
    // 修复：不再无条件标记根层为脏
    // 改为让各层自己追踪脏区域
    // 删除: if (root_layer_ && CheckRenderObjectNeedsPaint(root)) {
    //           root_layer_->MarkFullDirty();
    //       }
    
    current_frame_stats_.layers_built = static_cast<int>(layer_tree_builder_->GetLayerCount());
    RegisterScrollableElements(root);
}

// window.cpp - 修改动画处理
if (has_animations_running) {
    needs_repaint_ = true;
    // 修复：移除 ForceRasterize 调用
    // 动画更新应该只更新层的 transform/opacity，不触发重新光栅化
    // 删除: compositor_adapter_->ForceRasterize();
}
```

### 2. 实现 will-change 检测

**修复 layer_tree_builder.cpp:**

```cpp
bool LayerTreeBuilder::HasWillChangeTransform(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    
    // 检查 will_change 属性
    // will_change 可能是 "transform", "transform, opacity" 等
    const std::string& will_change = style.will_change;
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    // 检查是否包含 "transform"
    return will_change.find("transform") != std::string::npos;
}

bool LayerTreeBuilder::HasWillChangeOpacity(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    const std::string& will_change = style.will_change;
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    return will_change.find("opacity") != std::string::npos;
}
```

**需要在 ComputedStyle 中添加 will_change 字段（如果不存在）:**

```cpp
// computed_style.h
struct ComputedStyle {
    // ... existing fields ...
    std::string will_change;  // "auto", "transform", "opacity", "transform, opacity" 等
};
```

### 3. 修复动画检测逻辑

**修复 HasTransformAnimation:**

```cpp
bool LayerTreeBuilder::HasTransformAnimation(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    
    // 方法1：检查是否有正在运行的 transform 动画
    // 需要查询 AnimationController
    if (animation_controller_) {
        auto running = animation_controller_->GetRunningAnimationsForElement(obj);
        for (const auto& anim : running) {
            if (anim->AffectsProperty("transform")) {
                return true;
            }
        }
    }
    
    // 方法2：检查 CSS animation 配置中是否有 transform 动画
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 查询 keyframes 定义，检查是否影响 transform
        if (style_manager_) {
            auto keyframes = style_manager_->GetKeyframes(anim.name);
            if (keyframes && keyframes->AffectsProperty("transform")) {
                return true;
            }
        }
    }
    
    return false;
}
```

### 4. 集成 AnimationLayerBridge

**在 Window 或 AnimationController 中添加通知:**

```cpp
// animation_controller.cpp 或 window.cpp
void AnimationController::StartAnimation(Element* element, const AnimationConfig& config) {
    // ... existing animation start logic ...
    
    // 通知层系统
    if (animation_layer_bridge_) {
        animation_layer_bridge_->OnAnimationStart(element->GetRenderObject(), config);
    }
}

void AnimationController::OnAnimationComplete(Element* element, const std::string& animation_name) {
    // ... existing completion logic ...
    
    // 通知层系统
    if (animation_layer_bridge_) {
        animation_layer_bridge_->OnAnimationEnd(element->GetRenderObject());
    }
}
```

**在 WindowCompositorAdapter 中暴露 AnimationLayerBridge:**

```cpp
// window_compositor_adapter.h
class WindowCompositorAdapter {
public:
    AnimationLayerBridge* GetAnimationLayerBridge() {
        return pipeline_->GetAnimationBridge();
    }
};
```

### 5. 修复 RenderObject 层关联

**修复 LayerTreeBuilder::CreateLayer:**

```cpp
std::shared_ptr<CompositorLayer> LayerTreeBuilder::CreateLayer(
    RenderObject* obj, LayerPromotionReason reason) {

    auto layer = CreateCompositorLayer();
    layer->SetRenderObject(obj);
    layer->SetPromotionReason(reason);
    layer->SetDpiScale(dpi_scale_);

    UpdateLayerBounds(layer.get(), obj);
    layer->MarkFullDirty();

    // 关键修复：设置 RenderObject 的层关联
    if (obj) {
        obj->SetCompositorLayer(layer);  // 需要添加此方法
    }

    render_object_to_layer_[obj] = layer;
    layer_count_++;

    return layer;
}
```

**在 RenderObject 中添加层关联方法:**

```cpp
// render_object.h
class RenderObject {
public:
    void SetCompositorLayer(std::weak_ptr<CompositorLayer> layer) {
        layer_info_.compositor_layer = layer;
    }
    
    bool HasOwnCompositorLayer() const {
        return !layer_info_.compositor_layer.expired();
    }
    
    std::shared_ptr<CompositorLayer> GetCompositorLayer() const {
        return layer_info_.compositor_layer.lock();
    }
};
```

## Data Flow

### 动画帧渲染流程（修复后）

```
┌─────────────────────────────────────────────────────────────────┐
│                     Animation Frame                              │
├─────────────────────────────────────────────────────────────────┤
│  1. UpdateAnimations()                                           │
│     └─> 更新动画值（transform, opacity）                          │
│     └─> 通知 AnimationLayerBridge                                │
│                                                                  │
│  2. BuildLayerTree()                                             │
│     └─> 检查层提升条件（will-change, animation）                  │
│     └─> 为动画元素创建独立层（如果需要）                           │
│     └─> 不再无条件标记根层为脏                                    │
│                                                                  │
│  3. RasterizeDirtyLayers()                                       │
│     └─> 只光栅化有内容变化的层                                    │
│     └─> 动画层如果只有 transform/opacity 变化，跳过光栅化          │
│                                                                  │
│  4. CompositeLayers()                                            │
│     └─> 应用层的 transform 和 opacity                            │
│     └─> GPU 合成所有层                                           │
└─────────────────────────────────────────────────────────────────┘
```

## Correctness Properties

### Property 1: Incremental rasterization works correctly
*For any* animation frame where only transform/opacity changes, the system SHALL NOT re-rasterize layer content.
**Validates: Requirements 1.1, 1.2**

### Property 2: will-change triggers layer promotion
*For any* element with `will-change: transform` or `will-change: opacity`, the system SHALL create a dedicated compositing layer.
**Validates: Requirements 2.1, 2.2**

### Property 3: Animation detection is accurate
*For any* element with active CSS animation on transform/opacity, the system SHALL correctly identify it for layer promotion.
**Validates: Requirements 3.1, 3.2**

### Property 4: AnimationLayerBridge is notified
*For any* animation start/end event, the AnimationLayerBridge SHALL be notified to manage layer promotion/demotion.
**Validates: Requirements 4.1, 4.2**

### Property 5: No visual artifacts
*For any* animated element, there SHALL be no ghosting or visual artifacts at previous positions.
**Validates: Requirements 5.1, 5.2**

## Testing Strategy

### Unit Tests
1. `HasWillChangeTransform()` 正确解析 will-change 属性
2. `HasTransformAnimation()` 正确检测动画
3. `CreateLayer()` 正确设置 RenderObject 层关联

### Integration Tests
1. 运行 CSS 动画，验证不触发全量光栅化
2. 验证 will-change 元素获得独立层
3. 验证动画无重影

### Performance Tests
1. 对比修复前后的 CPU 使用率
2. 对比修复前后的帧率
3. 测量光栅化调用次数

## Error Handling

### will-change 解析失败
- 如果 will-change 值无法解析，默认为 "auto"（不提升）
- 记录警告日志

### AnimationLayerBridge 未连接
- 如果 bridge 为空，跳过通知但不崩溃
- 记录警告日志

### 层关联丢失
- 如果 RenderObject 的层关联过期，重新查找或创建层
- 不应导致渲染失败
