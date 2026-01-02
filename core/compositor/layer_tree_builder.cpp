/**
 * @file layer_tree_builder.cpp
 * @brief 层树构建器实现
 */

#include "layer_tree_builder.h"
#include "layer_tree_manager.h"
#include "animation/animation_bounds_calculator.h"
#include "core/render/objects/render_object.h"
#include "core/render/animation/keyframes.h"
#include "core/dom/element.h"
#include <algorithm>
#include <iostream>

namespace lightui {

LayerTreeBuilder::LayerTreeBuilder() = default;

LayerTreeBuilder::~LayerTreeBuilder() {
    Clear();
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::Build(RenderObject* root) {
    if (!root) {
        return nullptr;
    }

    // 清除旧的层树
    Clear();

    // 创建根层
    root_layer_ = CreateLayer(root, LayerPromotionReason::RootLayer);
    root_layer_->SetDebugName("root");

    // 递归构建层树
    BuildRecursive(root, root_layer_.get());

    return root_layer_;
}

void LayerTreeBuilder::Update(RenderObject* changed_node) {
    if (!changed_node || !root_layer_) {
        return;
    }

    // 检查节点是否需要层提升
    LayerPromotionReason reason = ShouldPromote(changed_node);

    // 查找现有层
    auto it = render_object_to_layer_.find(changed_node);
    bool has_layer = (it != render_object_to_layer_.end());

    if (reason != LayerPromotionReason::None && !has_layer) {
        // 需要层但没有 -> 创建新层
        // TODO: 找到正确的父层并插入
    } else if (reason == LayerPromotionReason::None && has_layer) {
        // 不需要层但有 -> 移除层
        auto layer = it->second;
        if (auto parent = layer->GetParent()) {
            parent->RemoveChild(layer.get());
        }
        render_object_to_layer_.erase(it);
        layer_count_--;
    } else if (has_layer) {
        // 更新现有层的边界
        UpdateLayerBounds(it->second.get(), changed_node);
    }
}

LayerPromotionReason LayerTreeBuilder::ShouldPromote(RenderObject* obj) const {
    if (!obj || !layer_promotion_enabled_) {
        return LayerPromotionReason::None;
    }

    // 检查各种提升条件（按优先级）

    // 1. will-change: transform
    if (HasWillChangeTransform(obj)) {
        return LayerPromotionReason::WillChangeTransform;
    }

    // 2. will-change: opacity
    if (HasWillChangeOpacity(obj)) {
        return LayerPromotionReason::WillChangeOpacity;
    }

    // 3. position: fixed - 提升为独立合成层
    // fixed 元素需要在滚动条之上绘制，通过独立层实现正确的 stacking order
    if (HasPositionFixed(obj)) {
        return LayerPromotionReason::PositionFixed;
    }

    // 4. transform 动画
    if (HasTransformAnimation(obj)) {
        return LayerPromotionReason::TransformAnimation;
    }

    // 5. opacity 动画
    if (HasOpacityAnimation(obj)) {
        return LayerPromotionReason::OpacityAnimation;
    }

    // 6. 可滚动容器
    if (IsScrollableContainer(obj)) {
        return LayerPromotionReason::ScrollableContent;
    }

    return LayerPromotionReason::None;
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::GetLayerForRenderObject(RenderObject* obj) const {
    auto it = render_object_to_layer_.find(obj);
    if (it != render_object_to_layer_.end()) {
        return it->second;
    }
    return nullptr;
}

void LayerTreeBuilder::Clear() {
    render_object_to_layer_.clear();
    root_layer_.reset();
    layer_count_ = 0;
}

void LayerTreeBuilder::BuildRecursive(RenderObject* obj, CompositorLayer* parent_layer) {
    if (!obj || !parent_layer) {
        return;
    }

    CompositorLayer* current_layer = parent_layer;

    // 检查节点是否已经有层了（例如根节点）
    auto existing_layer = render_object_to_layer_.find(obj);
    if (existing_layer != render_object_to_layer_.end()) {
        // 节点已经有层了，使用现有层
        current_layer = existing_layer->second.get();
    } else {
        LayerPromotionReason reason = ShouldPromote(obj);
        if (reason != LayerPromotionReason::None) {
            auto new_layer = CreateLayer(obj, reason);
            
            // 关键修复：Fixed 元素直接挂在根层下，不受 DOM 层级影响
            // 这确保 fixed 元素不会被滚动容器的层结构影响
            CompositorLayer* target_parent = parent_layer;
            if (reason == LayerPromotionReason::PositionFixed && root_layer_) {
                target_parent = root_layer_.get();
            }
            
            target_parent->AddChild(new_layer);
            current_layer = new_layer.get();
            
            // 关键修复：在添加到父层后重新计算 bounds
            // 因为 UpdateLayerBounds 需要知道父层来正确计算相对位置
            UpdateLayerBounds(current_layer, obj);
        }
    }

    // 递归处理子节点
    for (const auto& child : obj->GetChildren()) {
        BuildRecursive(child.get(), current_layer);
    }
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::CreateLayer(
    RenderObject* obj, LayerPromotionReason reason) {

    auto layer = CreateCompositorLayer();
    layer->SetRenderObject(obj);
    layer->SetPromotionReason(reason);
    layer->SetDpiScale(dpi_scale_);  // 设置 DPI 缩放

    // 调试日志：打印被提升的元素信息
    if (obj) {
        auto node = obj->GetNode();
        std::string tag_name = "unknown";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::static_pointer_cast<Element>(node);
            tag_name = element->GetTagName();
        }
    }

    // 设置层边界
    UpdateLayerBounds(layer.get(), obj);
    
    // 关键修复：新创建的层需要标记为脏，以便被光栅化
    layer->MarkFullDirty();

    // 关键修复：设置 RenderObject 的层关联
    // 这使得 HasOwnCompositorLayer() 和 GetCompositorLayer() 能正确工作
    if (obj) {
        obj->SetCompositorLayer(layer);
    }

    // 记录映射
    render_object_to_layer_[obj] = layer;
    layer_count_++;

    return layer;
}

void LayerTreeBuilder::UpdateLayerBounds(CompositorLayer* layer, RenderObject* obj) {
    if (!layer || !obj) {
        return;
    }

    const auto& layout = obj->GetLayoutInfo();
    const auto& style = obj->GetComputedStyle();
    
    // 调试日志
    static bool debug_layer = std::getenv("LIGHTUI_DEBUG_LAYER") != nullptr;
    
    // 性能优化：跳过 0 大小的元素（如空的 Toast 容器）
    // 这些元素没有可见内容，不需要计算复杂的 transform 边界
    if (layout.width <= 0 && layout.height <= 0) {
        // 设置一个最小边界，避免后续处理出错
        layer->SetBounds(SkRect::MakeXYWH(layout.x, layout.y, 0, 0));
        return;
    }
    
    // 对于根层，边界从 (0,0) 开始
    // 对于 body 元素，使用视口尺寸而不是布局尺寸，以确保滚动条能正确绘制
    if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer) {
        float width = obj->GetEffectiveVisibleWidth();
        float height = obj->GetEffectiveVisibleHeight();
        // 如果视口尺寸无效，回退到布局尺寸
        if (width <= 0) width = layout.width;
        if (height <= 0) height = layout.height;
        SkRect bounds = SkRect::MakeWH(width, height);
        layer->SetBounds(bounds);
        return;
    }
    
    // 获取层树父层对应的 RenderObject
    auto parent_layer = layer->GetParent();
    RenderObject* parent_layer_obj = parent_layer ? parent_layer->GetRenderObject() : nullptr;
    
    // 计算相对于层树父层的位置
    float rel_x = layout.x;
    float rel_y = layout.y;
    
    // 关键修复：position: fixed 元素的位置是相对于视口的
    // 不需要累加父元素的位置，因为 layout.x/y 已经是视口坐标
    // 同时，fixed 元素应该直接作为根层的子层，位置就是视口坐标
    bool is_fixed = (style.position == "fixed");
    
    // 检查是否有动画（用于调试）
    bool has_animation = false;
    for (const auto& anim : style.animations) {
        if (!anim.name.empty() && anim.name != "none") {
            has_animation = true;
            break;
        }
    }
    
    // 调试日志：输出 fixed 元素的布局信息
    if (is_fixed) {
        auto node = obj->GetNode();
        std::string tag_name = "unknown";
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::static_pointer_cast<Element>(node);
            tag_name = element->GetTagName();
        }
        std::cout << "[UpdateLayerBounds] FIXED element: " << tag_name 
                  << " layout=(" << layout.x << "," << layout.y 
                  << "," << layout.width << "x" << layout.height << ")"
                  << " has_transform=" << (style.transform.has_value() ? "yes" : "no")
                  << std::endl;
    }
    
    if (!is_fixed) {
        // 对于非 fixed 元素，需要累加父元素位置
        // 从当前元素的直接父元素开始，累加位置
        // 直到到达层树父层对应的 RenderObject
        auto parent = obj->GetParent();
        while (parent && parent.get() != parent_layer_obj) {
            const auto& parent_layout = parent->GetLayoutInfo();
            rel_x += parent_layout.x;
            rel_y += parent_layout.y;
            
            parent = parent->GetParent();
        }
        
        // 注意：不在这里减去滚动偏移！
        // 层的 bounds 保持文档坐标，滚动偏移在 CompositeLayerCPU 中应用
        // 这样可以避免双重减去滚动偏移的问题
        
        // 调试日志：只输出有动画的元素
        if (debug_layer && has_animation) {
            auto node = obj->GetNode();
            std::string tag_name = "unknown";
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto element = std::static_pointer_cast<Element>(node);
                tag_name = element->GetTagName();
            }
            std::cout << "[UpdateLayerBounds] " << tag_name 
                      << " layout=(" << layout.x << "," << layout.y << ")"
                      << " rel=(" << rel_x << "," << rel_y << ")"
                      << " parent_layer_obj=" << (parent_layer_obj ? "yes" : "no")
                      << std::endl;
        }
    }
    // 对于 fixed 元素，rel_x 和 rel_y 保持为 layout.x 和 layout.y（视口坐标）
    
    // 计算边界尺寸和偏移
    float width = layout.width;
    float height = layout.height;
    float offset_x = 0;
    float offset_y = 0;
    
    // 修复：如果 CSS 指定了宽度/高度，使用 CSS 值而不是 layout 值
    // 这是因为 layout.width/height 可能是内容宽度（shrink-to-fit），
    // 而不是 CSS 指定的盒子尺寸
    if (style.width.unit != CSSUnit::NONE && style.width.unit != CSSUnit::AUTO && style.width.value > 0) {
        width = style.width.value;
    }
    if (style.height.unit != CSSUnit::NONE && style.height.unit != CSSUnit::AUTO && style.height.value > 0) {
        height = style.height.value;
    }
    
    // 首先尝试使用动画边界计算器（处理动画的完整范围）
    AnimationBounds anim_bounds;
    bool has_animation_bounds = false;
    
    // 用于合并多个动画边界的绝对坐标范围
    // 这些坐标是相对于元素原点的
    float abs_min_x = 0.0f;
    float abs_min_y = 0.0f;
    float abs_max_x = width;  // 使用修正后的宽度
    float abs_max_y = height; // 使用修正后的高度
    
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 使用 AnimationBoundsCalculator 计算动画边界
        // 使用修正后的尺寸（CSS 指定的尺寸优先）
        SkSize element_size = SkSize::Make(width, height);
        AnimationBounds bounds = AnimationBoundsCalculator::Calculate(
            element_size, anim.name, style.transform_origin);
        
        if (bounds.needs_expansion) {
            // 将边界转换为相对于元素原点的绝对坐标
            // bounds.offset 是边界左上角相对于元素原点的偏移
            // bounds.bounds 是边界的尺寸（从 (0,0) 开始）
            float this_min_x = bounds.offset.fX;
            float this_min_y = bounds.offset.fY;
            float this_max_x = bounds.offset.fX + bounds.bounds.width();
            float this_max_y = bounds.offset.fY + bounds.bounds.height();
            
            // 合并多个动画的边界（使用绝对坐标）
            if (!has_animation_bounds) {
                abs_min_x = this_min_x;
                abs_min_y = this_min_y;
                abs_max_x = this_max_x;
                abs_max_y = this_max_y;
                has_animation_bounds = true;
            } else {
                // 合并边界（取并集）
                abs_min_x = std::min(abs_min_x, this_min_x);
                abs_min_y = std::min(abs_min_y, this_min_y);
                abs_max_x = std::max(abs_max_x, this_max_x);
                abs_max_y = std::max(abs_max_y, this_max_y);
            }
        }
    }
    
    // 如果有动画边界，构建最终的 AnimationBounds
    if (has_animation_bounds) {
        anim_bounds.offset = SkPoint::Make(abs_min_x, abs_min_y);
        anim_bounds.bounds = SkRect::MakeWH(abs_max_x - abs_min_x, abs_max_y - abs_min_y);
        anim_bounds.needs_expansion = true;
    }
    
    if (has_animation_bounds) {
        // 使用动画边界
        width = anim_bounds.bounds.width();
        height = anim_bounds.bounds.height();
        offset_x = anim_bounds.offset.fX;
        offset_y = anim_bounds.offset.fY;
        
        // 存储动画边界信息到层（用于光栅化时的偏移）
        layer->SetAnimationBounds(anim_bounds);
    } else if (!is_fixed && style.transform.has_value() && !style.transform->IsEmpty()) {
        // 对于 position: fixed 元素，不计算 transform 偏移
        // 因为在合成时我们使用 layout.x/y 定位，transform 在 Paint 中应用
        // 位图需要足够大以容纳变换后的内容，但不需要偏移
        
        // 如果没有动画边界，但有静态变换，使用当前帧的变换边界
        // 计算变换后的边界框
        SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(local_rect, style.transform_origin);
        
        // 变换四个角点
        SkPoint corners[4] = {
            {0, 0},
            {layout.width, 0},
            {layout.width, layout.height},
            {0, layout.height}
        };
        transform_matrix.mapPoints(corners, 4);
        
        // 计算变换后的边界框
        float min_x = corners[0].x(), max_x = corners[0].x();
        float min_y = corners[0].y(), max_y = corners[0].y();
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x());
            max_x = std::max(max_x, corners[i].x());
            min_y = std::min(min_y, corners[i].y());
            max_y = std::max(max_y, corners[i].y());
        }
        
        // 扩展边界以容纳变换后的内容
        // 添加一些额外的边距以确保动画过程中不会被裁剪
        const float padding = 10.0f;
        width = (max_x - min_x) + padding * 2;
        height = (max_y - min_y) + padding * 2;
        offset_x = min_x - padding;
        offset_y = min_y - padding;
        
        // 清除动画边界（没有动画）
        layer->ClearAnimationBounds();
    } else if (is_fixed && style.transform.has_value() && !style.transform->IsEmpty()) {
        // position: fixed 元素有 transform 时，需要扩展位图大小以容纳变换后的内容
        // 同时需要记录 transform 偏移，以便光栅化和合成时正确处理
        SkRect local_rect = SkRect::MakeWH(layout.width, layout.height);
        SkMatrix transform_matrix = style.transform->ToSkMatrix(local_rect, style.transform_origin);
        
        // 变换四个角点
        SkPoint corners[4] = {
            {0, 0},
            {layout.width, 0},
            {layout.width, layout.height},
            {0, layout.height}
        };
        transform_matrix.mapPoints(corners, 4);
        
        // 计算变换后的边界框
        float min_x = corners[0].x(), max_x = corners[0].x();
        float min_y = corners[0].y(), max_y = corners[0].y();
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x());
            max_x = std::max(max_x, corners[i].x());
            min_y = std::min(min_y, corners[i].y());
            max_y = std::max(max_y, corners[i].y());
        }
        
        // 扩展位图大小以容纳变换后的内容
        // 对于 translateX(-50%)，min_x 会是负值，需要扩展左边
        const float padding = 10.0f;
        float expanded_width = (max_x - min_x) + padding * 2;
        float expanded_height = (max_y - min_y) + padding * 2;
        
        // 使用扩展后的尺寸
        width = expanded_width;
        height = expanded_height;
        
        // 关键修复：对于 fixed 元素，需要应用 transform 偏移
        // 这样合成时才能正确定位
        // min_x - padding 是变换后内容相对于原点的偏移
        offset_x = min_x - padding;
        offset_y = min_y - padding;
        
        // 清除动画边界
        layer->ClearAnimationBounds();
    } else {
        // 没有变换，清除动画边界
        layer->ClearAnimationBounds();
    }
    
    SkRect bounds = SkRect::MakeXYWH(rel_x + offset_x, rel_y + offset_y, width, height);
    layer->SetBounds(bounds);
    
    // 调试日志：输出最终边界
    if (is_fixed) {
        std::cout << "[UpdateLayerBounds] FIXED final bounds=(" 
                  << bounds.left() << "," << bounds.top() 
                  << "," << bounds.width() << "x" << bounds.height() << ")"
                  << " rel=(" << rel_x << "," << rel_y << ")"
                  << " offset=(" << offset_x << "," << offset_y << ")"
                  << std::endl;
    }
}

bool LayerTreeBuilder::HasWillChangeTransform(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    const std::string& will_change = style.will_change;
    
    // 检查 will-change 属性是否包含 "transform"
    // will-change 可以是 "auto", "transform", "opacity", "transform, opacity" 等
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    bool has_transform = will_change.find("transform") != std::string::npos;
    return has_transform;
}

bool LayerTreeBuilder::HasWillChangeOpacity(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    const std::string& will_change = style.will_change;
    
    // 检查 will-change 属性是否包含 "opacity"
    if (will_change.empty() || will_change == "auto") {
        return false;
    }
    
    return will_change.find("opacity") != std::string::npos;
}

bool LayerTreeBuilder::HasPositionFixed(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();
    return style.position == "fixed";
}

bool LayerTreeBuilder::HasTransformAnimation(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查是否有活动的 CSS 动画
    // 策略：如果元素有非空的动画名称，检查是否可能影响 transform
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 方法1：检查元素当前是否有 transform 属性
        // 如果有 transform 且有动画，很可能是 transform 动画
        if (style.transform.has_value()) {
            return true;
        }
        
        // 方法2：检查动画名称是否包含 transform 相关关键字
        // 这是一个启发式方法，不完美但有用
        std::string lower_name = anim.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find("transform") != std::string::npos ||
            lower_name.find("rotate") != std::string::npos ||
            lower_name.find("scale") != std::string::npos ||
            lower_name.find("translate") != std::string::npos ||
            lower_name.find("move") != std::string::npos ||
            lower_name.find("slide") != std::string::npos ||
            lower_name.find("spin") != std::string::npos ||
            lower_name.find("bounce") != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool LayerTreeBuilder::HasOpacityAnimation(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查是否有活动的 CSS 动画
    for (const auto& anim : style.animations) {
        if (anim.name.empty() || anim.name == "none") {
            continue;
        }
        
        // 方法1：检查元素当前 opacity 是否不为 1.0
        // 如果 opacity 不为 1.0 且有动画，很可能是 opacity 动画
        if (style.opacity < 1.0f) {
            return true;
        }
        
        // 方法2：检查动画名称是否包含 opacity 相关关键字
        std::string lower_name = anim.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find("opacity") != std::string::npos ||
            lower_name.find("fade") != std::string::npos ||
            lower_name.find("appear") != std::string::npos ||
            lower_name.find("disappear") != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool LayerTreeBuilder::IsScrollableContainer(RenderObject* obj) const {
    if (!obj) {
        return false;
    }

    const auto& style = obj->GetComputedStyle();

    // 检查 overflow 属性
    bool has_overflow_scroll = 
        style.overflow_x == "scroll" || style.overflow_x == "auto" ||
        style.overflow_y == "scroll" || style.overflow_y == "auto" ||
        style.overflow == "scroll" || style.overflow == "auto";

    if (!has_overflow_scroll) {
        return false;
    }

    // 检查是否有溢出内容
    // 注意：如果缓存的内容尺寸为 0，需要动态计算
    // 这在首次构建层树时很重要，因为 Paint 还没有被调用
    const auto& layout = obj->GetLayoutInfo();
    float content_width = obj->GetContentWidth();
    float content_height = obj->GetContentHeight();
    
    // 如果缓存值为 0，动态计算
    if (content_width <= 0) {
        content_width = obj->CalculateContentWidth();
    }
    if (content_height <= 0) {
        content_height = obj->CalculateContentHeight();
    }

    bool has_overflow_content = 
        content_width > layout.width || 
        content_height > layout.height;

    return has_overflow_content;
}

// ============================================================================
// 增量更新接口（新增）
// ============================================================================

CompositorLayer* LayerTreeBuilder::FindParentLayerForObject(RenderObject* obj) const {
    if (!obj || !root_layer_) {
        return nullptr;
    }
    
    // 如果是 fixed 元素，直接返回根层
    if (HasPositionFixed(obj)) {
        return root_layer_.get();
    }
    
    // 向上遍历 RenderObject 树，找到第一个有层的祖先
    auto parent = obj->GetParent();
    while (parent) {
        auto it = render_object_to_layer_.find(parent.get());
        if (it != render_object_to_layer_.end()) {
            return it->second.get();
        }
        parent = parent->GetParent();
    }
    
    // 如果没有找到有层的祖先，返回根层
    return root_layer_.get();
}

std::shared_ptr<CompositorLayer> LayerTreeBuilder::AddLayerForObject(
    RenderObject* obj, LayerPromotionReason reason) {
    
    if (!obj || !root_layer_) {
        return nullptr;
    }
    
    // 检查是否已经有层
    auto existing = render_object_to_layer_.find(obj);
    if (existing != render_object_to_layer_.end()) {
        return existing->second;  // 已经有层了
    }
    
    // 找到正确的父层
    CompositorLayer* parent_layer = FindParentLayerForObject(obj);
    if (!parent_layer) {
        return nullptr;
    }
    
    // 创建新层
    auto new_layer = CreateLayer(obj, reason);
    if (!new_layer) {
        return nullptr;
    }
    
    // 附加到父层
    parent_layer->AddChild(new_layer);
    
    // 在附加后重新计算边界（此时有父层信息）
    UpdateLayerBoundsDeferred(new_layer.get(), obj);
    
    // 递增版本号
    IncrementTreeVersion();
    
    return new_layer;
}

bool LayerTreeBuilder::RemoveLayerForObject(RenderObject* obj) {
    if (!obj) {
        return false;
    }
    
    auto it = render_object_to_layer_.find(obj);
    if (it == render_object_to_layer_.end()) {
        return false;  // 没有层
    }
    
    auto layer = it->second;
    auto parent = layer->GetParent();
    
    if (parent) {
        // 将子层转移到父层
        const auto& children = layer->GetChildren();
        for (const auto& child : children) {
            parent->AddChild(child);
        }
        
        // 从父层移除当前层
        parent->RemoveChild(layer.get());
    }
    
    // 清理 RenderObject 的层关联
    obj->SetCompositorLayer(nullptr);
    
    // 从映射中移除
    render_object_to_layer_.erase(it);
    layer_count_--;
    
    // 递增版本号
    IncrementTreeVersion();
    
    return true;
}

void LayerTreeBuilder::UpdateLayerBoundsDeferred(CompositorLayer* layer, RenderObject* obj) {
    // 委托给 UpdateLayerBounds，它已经正确处理了父层信息
    UpdateLayerBounds(layer, obj);
}

bool LayerTreeBuilder::CanIncrementalUpdate() const {
    // 检查根层是否存在
    if (!root_layer_) {
        return false;
    }
    
    // 检查层树是否一致（基本检查）
    // 如果层数量为 0 但根层存在，说明有问题
    if (layer_count_ == 0) {
        return false;
    }
    
    return true;
}

bool LayerTreeBuilder::IncrementalBuild(RenderObject* root,
                                         const std::vector<PendingLayerUpdate>& pending_updates) {
    if (!root || !CanIncrementalUpdate()) {
        return false;
    }
    
    // 遍历待处理更新
    for (const auto& update : pending_updates) {
        if (!update.target) {
            continue;
        }
        
        switch (update.type) {
            case LayerUpdateType::Add:
                AddLayerForObject(update.target, update.reason);
                break;
                
            case LayerUpdateType::Remove:
                RemoveLayerForObject(update.target);
                break;
                
            case LayerUpdateType::UpdateBounds: {
                auto layer = GetLayerForRenderObject(update.target);
                if (layer) {
                    UpdateLayerBoundsDeferred(layer.get(), update.target);
                }
                break;
            }
            
            case LayerUpdateType::Reparent: {
                // 重新附加父层：先移除再添加
                auto layer = GetLayerForRenderObject(update.target);
                if (layer) {
                    auto reason = layer->GetPromotionReason();
                    RemoveLayerForObject(update.target);
                    AddLayerForObject(update.target, reason);
                }
                break;
            }
            
            case LayerUpdateType::UpdateZIndex: {
                // z-index 更新：需要重新排序
                // TODO: 实现 z-index 排序
                break;
            }
        }
    }
    
    // 递增版本号
    IncrementTreeVersion();
    
    return true;
}

} // namespace lightui
