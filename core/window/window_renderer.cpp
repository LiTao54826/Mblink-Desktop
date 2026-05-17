/**
 * @file window_renderer.cpp
 * @brief 窗口渲染器实现
 * 
 * 从 window.cpp 提取的渲染相关逻辑
 * 
 * @note 当前实现状态
 * 这是一个初始框架，提取了部分辅助方法。
 * 主渲染逻辑 (Render) 仍在 Window 类中，因为：
 * 1. 渲染逻辑与 Window 状态紧密耦合
 * 2. 需要访问大量 Window 私有成员
 * 3. 完全提取需要大量重构工作
 * 
 * 后续可以逐步将更多渲染逻辑迁移到此类
 */

#include "window_renderer.h"
#include "window.h"
#include "core/dom/document.h"
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/render/objects/render_object.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/pipeline/render_tree_synchronizer.h"
#include "core/render/animation/animation_timeline.h"
#include "core/render/animation/animation_controller.h"
#include "core/render/animation/animation_applicator.h"
#include "core/layout/layout_engine.h"
#include "core/devtools/devtools_manager.h"
#include "core/lexbor/style_manager.h"
#include <iostream>
#include <vector>
#include <cstdlib>

namespace mbink {

WindowRenderer::WindowRenderer(Window* window)
    : window_(window) {
}

WindowRenderer::~WindowRenderer() = default;

void WindowRenderer::Render() {
    // 主渲染逻辑委托给 Window::Render()
    // 这是一个过渡实现，后续可以将渲染逻辑迁移到这里
    if (window_) {
        window_->Render();
    }
}

void WindowRenderer::RenderDevTools(SkCanvas* canvas, float width, float height) {
    // DevTools 渲染委托给 Window
    // 后续可以将 DevTools 渲染逻辑迁移到这里
}

void WindowRenderer::EnsureRenderTree() {
    if (window_) {
        window_->EnsureRenderTree();
    }
}

void WindowRenderer::InvalidateRenderTree() {
    if (window_) {
        window_->InvalidateRenderTree();
    }
    render_tree_valid_ = false;
}

void WindowRenderer::AddDirtyRect(const SkRect& rect) {
    if (rect.isEmpty()) {
        return;
    }
    
    // 合并重叠的脏区域
    for (auto& existing : dirty_rects_) {
        if (existing.intersects(rect)) {
            existing.join(rect);
            return;
        }
    }
    
    dirty_rects_.push_back(rect);
}

void WindowRenderer::CollectDirtyRectsFromRenderTree(RenderObject* root) {
    if (!root) {
        return;
    }

    // 增量优化：如果根节点及其子树都不需要重绘，直接返回
    if (!root->IsDirtyForPaint()) {
        return;
    }

    // 使用迭代方式代替递归，避免深层嵌套时栈溢出
    std::vector<RenderObject*> stack;
    stack.push_back(root);
    
    while (!stack.empty()) {
        RenderObject* obj = stack.back();
        stack.pop_back();
        
        if (!obj) {
            continue;
        }

        // 增量优化：如果该节点及其子树都不需要重绘，跳过
        if (!obj->IsDirtyForPaint()) {
            continue;
        }

        // 如果该渲染对象需要重绘，收集其边界框
        if (obj->NeedsPaint()) {
            // 使用视口坐标系的边界框（考虑滚动偏移）
            // 因为 Paint 函数内部会应用 translate(-scroll_x_, -scroll_y_)
            // 所以绘制实际上是在视口坐标系中进行的
            SkRect rect = obj->GetViewportBoundingRect();
            if (!rect.isEmpty()) {
                AddDirtyRect(rect);
            }
        }

        // 只有当子树需要重绘时才遍历子节点
        if (obj->ChildNeedsPaint()) {
            // 将子节点加入栈（逆序以保持遍历顺序）
            const auto& children = obj->GetChildren();
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                stack.push_back(it->get());
            }
        }
    }
}

void WindowRenderer::UpdateAnimations(double current_time) {
    if (!window_) {
        return;
    }

    // 调试开关：MBINK_DEBUG_ANIM_LOOP=1
    static const bool debug_anim_loop = (std::getenv("MBINK_DEBUG_ANIM_LOOP") != nullptr);
    static uint64_t debug_frame = 0;
    ++debug_frame;

    // 更新 CSS Transition 动画
    bool has_active_animations = false;
    bool has_running_transitions = false;
    AnimationTimeline* timeline = window_->GetAnimationTimeline();
    if (timeline) {
        timeline->Update(current_time);
        has_running_transitions = timeline->HasRunningTransitions();
        has_active_animations = has_running_transitions;
    }

    // 更新 CSS Animation 动画（使用 StyleManager 的 AnimationController）
    size_t running_css_animations = 0;
    Document* document = window_->GetDocument().get();
    if (document && document->GetStyleManager()) {
        auto& controller = document->GetStyleManager()->GetAnimationController();
        controller.Update(current_time);
        running_css_animations = controller.GetRunningAnimations().size();
        has_active_animations = has_active_animations || running_css_animations > 0;
    }

    // 注意：AnimationApplicator 绑定的是 Document/StyleManager 的 controller。
    // 这里不再更新 Window 自己的 animation_controller_，避免双 controller 状态源不一致。

    // 应用动画值到渲染树
    AnimationApplicator* applicator = window_->GetAnimationApplicator();
    RenderObject* cached_tree = window_->GetCachedRenderTree().get();
    if (applicator && cached_tree) {
        // 递归遍历渲染树，启动新动画并应用动画值
        ApplyAnimationsToRenderTree(cached_tree);
    }

    // 关键修复：只靠 running 动画判定会在某些帧出现“短暂空窗”，
    // 导致主循环停止请求重绘，表现为动画只在交互事件时跳一下。
    // 这里把 pending（样式中已声明但尚未进入 running）也纳入持续重绘条件。
    bool has_pending_animations = false;
    if (cached_tree) {
        has_pending_animations = HasPendingAnimations(cached_tree);
    }

    bool should_request_repaint = has_active_animations || has_pending_animations;
    if (should_request_repaint) {
        window_->SetNeedsRepaintFor(RepaintReason::Animation);
    }

    if (debug_anim_loop && (debug_frame <= 120 || (debug_frame % 60 == 0))) {
        bool pipeline_needs_update = false;
        if (auto pipeline = window_->GetRenderPipeline()) {
            pipeline_needs_update = pipeline->NeedsUpdate();
        }

        std::cout << "[ANIM_LOOP] t=" << current_time
                  << " frame=" << debug_frame
                  << " runningTransitions=" << (has_running_transitions ? 1 : 0)
                  << " runningCss=" << running_css_animations
                  << " pending=" << (has_pending_animations ? 1 : 0)
                  << " requestRepaint=" << (should_request_repaint ? 1 : 0)
                  << " needsRepaintNow=" << (window_->NeedsRepaint() ? 1 : 0)
                  << " pipelineNeedsUpdate=" << (pipeline_needs_update ? 1 : 0)
                  << std::endl;
    }
}



void WindowRenderer::ApplyAnimationsToRenderTree(RenderObject* root) {
    if (!root || !window_) {
        return;
    }

    AnimationApplicator* applicator = window_->GetAnimationApplicator();
    if (!applicator) {
        return;
    }

    // 启动该对象的动画（如果尚未启动）
    applicator->StartAnimationsForObject(root);

    // 应用当前动画值
    applicator->ApplyAnimationValues(root);

    // 递归处理子节点
    for (const auto& child : root->GetChildren()) {
        ApplyAnimationsToRenderTree(child.get());
    }
}

bool WindowRenderer::HasPendingAnimations(RenderObject* root) const {
    if (!root || !window_) {
        return false;
    }

    AnimationApplicator* applicator = window_->GetAnimationApplicator();
    if (!applicator) {
        return false;
    }
    return applicator->HasPendingAnimationStartup(root);
}

bool WindowRenderer::LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height) {
    if (!render_obj) {
        return false;
    }
    
    bool did_layout = false;
    
    // 如果当前节点需要布局
    if (render_obj->NeedsLayout()) {
        render_obj->Layout(parent_width, parent_height);
        render_obj->ClearNeedsLayout();
        did_layout = true;
    }
    
    // 如果有子节点需要布局
    if (render_obj->ChildNeedsLayout()) {
        const auto& layout = render_obj->GetLayoutInfo();
        float child_width = layout.width;
        float child_height = layout.height;
        
        for (const auto& child : render_obj->GetChildren()) {
            if (LayoutDirtySubtree(child.get(), child_width, child_height)) {
                did_layout = true;
            }
        }
        
        // 注意：RenderObject 没有 ClearChildNeedsLayout 方法
        // 子节点布局完成后，child_needs_layout_ 标志会在下次布局时自动清除
    }
    
    return did_layout;
}

void WindowRenderer::MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj) {
    // 委托给 Window 实现，因为该方法需要访问 Window 的私有成员
    // 如 layout_engine_, document_, StyleResolver 等
    if (window_) {
        window_->MarkRenderObjectsDirty(dom_node, render_obj);
    }
}

void WindowRenderer::ClearDirtyFlags(Node* node) {
    if (!node) {
        return;
    }

    // 清除当前节点的脏标记
    node->ClearDirty(DirtyType::ALL);

    // 递归清除子节点
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        ClearDirtyFlags(child.get());
    }
}

void WindowRenderer::ClearRenderObjectDirtyFlags(RenderObject* render_obj) {
    if (!render_obj) {
        return;
    }

    // 增量优化：如果该节点及其子树都不需要重绘，直接返回
    if (!render_obj->IsDirtyForPaint()) {
        return;
    }

    // 清除当前渲染对象的脏标记
    render_obj->ClearDirtyFlags();

    // 只有当子树需要重绘时才递归清除子节点
    const auto& children = render_obj->GetChildren();
    for (const auto& child : children) {
        ClearRenderObjectDirtyFlags(child.get());
    }
}

bool WindowRenderer::HasDirtyLayoutNodes(Node* node) {
    if (!node) {
        return false;
    }

    // 检查当前节点是否需要布局
    if (node->IsLayoutDirty()) {
        return true;
    }

    // 检查关联的 RenderObject
    if (auto render_obj = node->GetRenderObject()) {
        if (render_obj->NeedsLayout()) {
            return true;
        }
    }

    // 递归检查子节点
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        if (HasDirtyLayoutNodes(child.get())) {
            return true;
        }
    }

    return false;
}

void WindowRenderer::SaveScrollPositions(RenderObject* render_obj,
                                         std::unordered_map<Node*, std::pair<float, float>>& scroll_positions) {
    if (!render_obj) {
        return;
    }
    
    // 如果有滚动偏移，保存它
    float scroll_x = render_obj->GetScrollX();
    float scroll_y = render_obj->GetScrollY();
    if (scroll_x != 0.0f || scroll_y != 0.0f) {
        auto node = render_obj->GetNode();
        if (node) {
            scroll_positions[node.get()] = {scroll_x, scroll_y};
        }
    }
    
    // 递归处理子节点
    for (const auto& child : render_obj->GetChildren()) {
        SaveScrollPositions(child.get(), scroll_positions);
    }
}

void WindowRenderer::RestoreScrollPositions(RenderObject* render_obj,
                                            const std::unordered_map<Node*, std::pair<float, float>>& scroll_positions) {
    if (!render_obj) {
        return;
    }

    // 查找是否有保存的滚动位置
    auto node = render_obj->GetNode();
    if (node) {
        auto it = scroll_positions.find(node.get());
        if (it != scroll_positions.end()) {
            // 使用 ScrollTo 统一恢复路径：
            // 1) 自动按当前内容尺寸进行 clamp
            // 2) 触发后代 ViewportBounds 失效
            // 3) 触发必要的重绘标记传播
            render_obj->ScrollTo(it->second.first, it->second.second);
        }
    }

    // 递归处理子节点
    for (const auto& child : render_obj->GetChildren()) {
        RestoreScrollPositions(child.get(), scroll_positions);
    }
}

} // namespace mbink
