/**
 * @file rasterizer.cpp
 * @brief 光栅化器实现
 */

#include "rasterizer.h"
#include "compositor_layer.h"
#include "animation/animation_bounds_calculator.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRegion.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif


namespace mblink {

namespace {

struct TrackedRasterOffsetState {
    float base_layout_x = 0.0f;
    float base_layout_y = 0.0f;
    float fixed_offset_x = 0.0f;
    float fixed_offset_y = 0.0f;
    float anim_offset_x = 0.0f;
    float anim_offset_y = 0.0f;
    float transform_offset_x = 0.0f;
    float transform_offset_y = 0.0f;
    float bounds_left = 0.0f;
    float bounds_top = 0.0f;
    LayerPromotionReason reason = LayerPromotionReason::None;

    bool operator==(const TrackedRasterOffsetState& other) const {
        return base_layout_x == other.base_layout_x
            && base_layout_y == other.base_layout_y
            && fixed_offset_x == other.fixed_offset_x
            && fixed_offset_y == other.fixed_offset_y
            && anim_offset_x == other.anim_offset_x
            && anim_offset_y == other.anim_offset_y
            && transform_offset_x == other.transform_offset_x
            && transform_offset_y == other.transform_offset_y
            && bounds_left == other.bounds_left
            && bounds_top == other.bounds_top
            && reason == other.reason;
    }
};

bool IsTrackedGutterDigit(const std::string& text) {
    return text.size() == 1 && text[0] >= '1' && text[0] <= '4';
}

bool HasTrackedGutterClassChain(const std::shared_ptr<Node>& node) {
    auto current = node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element && (element->HasClass("cm-gutterElement")
                || element->HasClass("cm-activeLineGutter")
                || element->HasClass("cm-lineNumbers"))) {
                return true;
            }
        }
        current = current->GetParentNode();
    }
    return false;
}

const RenderText* FindTrackedGutterDigitText(const RenderObject* obj) {
    if (!obj) return nullptr;

    if (obj->GetType() == RenderObjectType::TEXT) {
        auto text_obj = static_cast<const RenderText*>(obj);
        auto node = text_obj->GetNode();
        if (node && IsTrackedGutterDigit(text_obj->GetText()) && HasTrackedGutterClassChain(node)) {
            return text_obj;
        }
    }

    for (const auto& child : obj->GetChildren()) {
        if (const RenderText* found = FindTrackedGutterDigitText(child.get())) {
            return found;
        }
    }
    return nullptr;
}

#ifdef _WIN32
void PrintRasterizerDebugCallStack() {
    void* stack[32] = {0};
    USHORT frames = CaptureStackBackTrace(0, 32, stack, nullptr);
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(calloc(sizeof(SYMBOL_INFO) + 256, 1));
    if (!symbol) return;
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (USHORT i = 0; i < frames; ++i) {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
        if (SymFromAddr(process, address, 0, symbol)) {
            std::cout << "    [" << i << "] " << symbol->Name << "\n";
        }
    }
    free(symbol);
}
#else
void PrintRasterizerDebugCallStack() {}
#endif

void LogTrackedRasterOffset(RenderObject* render_obj,
                            CompositorLayer* layer,
                            const TrackedRasterOffsetState& state) {
    const RenderText* tracked_text = FindTrackedGutterDigitText(render_obj);
    if (!tracked_text) {
        return;
    }

    std::cout << "[TRACE_RASTER_OFFSET]"
              << " render_obj=" << render_obj
              << " layer=" << layer
              << " text=" << tracked_text->GetText()
              << " reason=" << CompositorLayer::PromotionReasonToString(state.reason)
              << " layout_x=" << state.base_layout_x
              << " layout_y=" << state.base_layout_y
              << " bounds_left=" << state.bounds_left
              << " bounds_top=" << state.bounds_top
              << " fixed_offset_x=" << state.fixed_offset_x
              << " fixed_offset_y=" << state.fixed_offset_y
              << " anim_offset_x=" << state.anim_offset_x
              << " anim_offset_y=" << state.anim_offset_y
              << " transform_offset_x=" << state.transform_offset_x
              << " transform_offset_y=" << state.transform_offset_y
              << " final_canvas_tx=" << -(state.base_layout_x + state.fixed_offset_x + state.anim_offset_x + state.transform_offset_x)
              << " final_canvas_ty=" << -(state.base_layout_y + state.fixed_offset_y + state.anim_offset_y + state.transform_offset_y)
              << "\n";
}

}  // namespace

Rasterizer::Rasterizer() = default;
Rasterizer::~Rasterizer() = default;

// =========================================================================
// 完整光栅化
// =========================================================================

bool Rasterizer::RasterizeLayer(CompositorLayer* layer) {
    // 调试日志已移除

    if (!layer) {
        return false;
    }

    if (!layer->AllowsBitmapBacking()) {
        layer->ClearDirtyRegions();
        return true;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 确保位图已分配
    if (!layer->EnsureBitmap()) {
        return false;
    }

    SkCanvas* canvas = layer->GetCanvas();
    if (!canvas) {
        return false;
    }

    // 获取关联的渲染对象
    RenderObject* render_obj = layer->GetRenderObject();
    if (!render_obj) {
        // 没有渲染对象，清除为透明
        canvas->clear(SK_ColorTRANSPARENT);
        layer->ClearDirtyRegions();
        // 标记纹理需要更新
        layer->MarkTextureDirty(SkIRect::MakeWH(
            static_cast<int>(layer->GetBounds().width()),
            static_cast<int>(layer->GetBounds().height())
        ));
        // 更新统计
        stats_.layers_rasterized++;
        stats_.full_rasterizations++;
        stats_.pixels_rasterized += static_cast<int>(layer->GetBounds().width() * layer->GetBounds().height());
        return true;
    }

    const auto& layout = render_obj->GetLayoutInfo();
    const SkRect& bounds = layer->GetBounds();
    float dpi_scale = layer->GetDpiScale();

    // 调试日志已移除
    bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);

    // 清除整个位图
    canvas->clear(SK_ColorTRANSPARENT);

    // 保存 Canvas 状态
    canvas->save();

    // 调试日志已移除

    // 关键修复：光栅化阶段不应用滚动偏移
    // 滚动偏移应该在合成阶段应用，这样滚动时只需要更新合成参数，
    // 不需要重新光栅化，性能更好。
    //
    // 旧代码（已移除）：
    // const SkPoint& scroll = layer->GetScrollOffset();
    // if (scroll.fX != 0 || scroll.fY != 0) {
    //     canvas->translate(-scroll.fX, -scroll.fY);
    // }

    // 应用非根层的 canvas 偏移补偿（layout 位置、transform、动画边界等）
    ApplyLayerCanvasOffset(canvas, layer, render_obj, layout);

    // 绘制渲染对象
    // 注意：RenderObject::Paint 内部已经递归绘制子对象了，不需要额外递归
    // 调试日志已移除

    render_obj->Paint(canvas);

    // 调试日志已移除

    // 恢复 Canvas 状态
    canvas->restore();

    // 记录本次成功绘制后的边界，供下次增量脏区计算使用
    UpdatePreviousPaintBoundsForSubtree(render_obj);


    // 清除脏区域
    layer->ClearDirtyRegions();

    // 标记纹理需要更新
    layer->MarkTextureDirty(SkIRect::MakeWH(
        static_cast<int>(layer->GetBounds().width()),
        static_cast<int>(layer->GetBounds().height())
    ));

    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    stats_.layers_rasterized++;
    stats_.full_rasterizations++;
    stats_.pixels_rasterized += static_cast<int>(layer->GetBounds().width() * layer->GetBounds().height());
    stats_.rasterize_time_ms += duration.count() / 1000.0;

    return true;
}

int Rasterizer::RasterizeDirtyLayers(CompositorLayer* root) {
    if (!root) {
        return 0;
    }

    int count = 0;

    // 光栅化当前层（如果有脏区域）
    if (root->HasDirtyRegions()) {
        if (incremental_enabled_) {
            if (RasterizeDirtyRegions(root)) {
                count++;
            }
        } else {
            if (RasterizeLayer(root)) {
                count++;
            }
        }
    }

    // 递归处理子层
    for (const auto& child : root->GetChildren()) {
        count += RasterizeDirtyLayers(child.get());
    }

    return count;
}

// =========================================================================
// 增量光栅化
// =========================================================================

bool Rasterizer::RasterizeDirtyRegions(CompositorLayer* layer) {
    if (!layer || !layer->HasDirtyRegions()) {
        return false;
    }

    if (!layer->AllowsBitmapBacking()) {
        layer->ClearDirtyRegions();
        return true;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 确保位图已分配
    if (!layer->EnsureBitmap()) {
        return false;
    }

    SkCanvas* canvas = layer->GetCanvas();
    if (!canvas) {
        return false;
    }

    // 获取关联的渲染对象
    RenderObject* render_obj = layer->GetRenderObject();

    // 调试日志已移除
    bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);

    // 合并脏区域
    layer->MergeDirtyRegions();

    const auto& dirty_regions = layer->GetDirtyRegions();
    int total_dirty_pixels = 0;
    int layer_pixels = static_cast<int>(layer->GetBounds().width() * layer->GetBounds().height());

    // 对每个脏区域进行光栅化
    for (const auto& region : dirty_regions) {
        if (render_obj) {
            if (!RasterizeRegion(layer, region)) {
                continue;
            }
        } else {
            // 没有渲染对象，只清除区域
            canvas->save();
            canvas->clipIRect(region);
            ClearRegion(canvas, region);
            canvas->restore();
        }

        total_dirty_pixels += region.width() * region.height();

        // 标记纹理脏区域
        layer->MarkTextureDirty(region);
    }

    // 清除脏区域
    layer->ClearDirtyRegions();

    // 更新统计
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    stats_.layers_rasterized++;
    stats_.incremental_rasterizations++;
    stats_.pixels_rasterized += total_dirty_pixels;
    stats_.pixels_skipped += layer_pixels - total_dirty_pixels;
    stats_.rasterize_time_ms += duration.count() / 1000.0;

    return true;
}

bool Rasterizer::RasterizeRegion(CompositorLayer* layer, const SkIRect& region) {
    if (!layer) {
        return false;
    }

    SkCanvas* canvas = layer->GetCanvas();
    if (!canvas) {
        return false;
    }

    RenderObject* render_obj = layer->GetRenderObject();
    if (!render_obj) {
        return false;
    }

    const auto& layout = render_obj->GetLayoutInfo();

    // 提前判断是否为 fixed 元素
    bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);

    // 保存 Canvas 状态
    canvas->save();

    // 清除当前脏区为透明（在设置绘制裁剪之前）
    canvas->save();
    canvas->clipIRect(region);
    ClearRegion(canvas, region);
    canvas->restore();

    // 应用非根层的 canvas 偏移补偿（layout 位置、transform、动画边界等）
    // 注意：此处之前缺少静态变换偏移的 fallback 逻辑，现在通过公共方法补全
    ApplyLayerCanvasOffset(canvas, layer, render_obj, layout);

    // 设置裁剪区域
    // 对于根层：region 是位图坐标，Paint 会 translate(layout.x, layout.y)
    //          所以裁剪区域不需要偏移
    // 对于非根层：canvas 已经 translate(-layout.x, -layout.y)
    //           需要将位图坐标的 region 转换到 canvas 坐标系
    SkRect clip_rect = SkRect::Make(region);
    if (layer->GetPromotionReason() != LayerPromotionReason::RootLayer) {
        float offset_x = layout.x;
        float offset_y = layout.y;

        if (is_fixed) {
            const auto& style = render_obj->GetComputedStyle();
            if (style.transform.has_value() && !style.transform->IsEmpty()) {
                const SkRect& bounds = layer->GetBounds();
                offset_x += (bounds.left() - layout.x);
                offset_y += (bounds.top() - layout.y);
            }
        } else {
            const AnimationBounds* anim_bounds = layer->GetAnimationBounds();
            if (anim_bounds && anim_bounds->needs_expansion) {
                offset_x += anim_bounds->offset.fX;
                offset_y += anim_bounds->offset.fY;
            }
        }
        clip_rect.offset(offset_x, offset_y);
    }

    canvas->clipRect(clip_rect);

    // 绘制渲染对象
    render_obj->Paint(canvas);

    // 恢复 Canvas 状态
    canvas->restore();


    // 记录本次成功绘制后的边界，供下次增量脏区计算使用
    UpdatePreviousPaintBoundsForSubtree(render_obj);

    return true;
}

// =========================================================================
// 滚动优化
// =========================================================================

bool Rasterizer::HandleScroll(CompositorLayer* layer,
                               const SkPoint& old_offset,
                               const SkPoint& new_offset) {
    if (!layer || !scroll_optimization_enabled_) {
        return false;
    }

    // 计算滚动增量
    SkPoint delta = {
        new_offset.fX - old_offset.fX,
        new_offset.fY - old_offset.fY
    };

    // 如果没有滚动，直接返回
    if (delta.fX == 0 && delta.fY == 0) {
        return true;
    }

    // 获取层尺寸
    int width = static_cast<int>(layer->GetBounds().width());
    int height = static_cast<int>(layer->GetBounds().height());

    // 如果滚动距离超过层尺寸，需要完整重绘
    if (std::abs(delta.fX) >= width || std::abs(delta.fY) >= height) {
        layer->MarkFullDirty();
        return RasterizeLayer(layer);
    }

    // 复制可重用的像素
    int dx = static_cast<int>(delta.fX);
    int dy = static_cast<int>(delta.fY);

    // 计算源区域和目标位置
    SkIRect src_rect;
    int dst_x, dst_y;

    if (dx >= 0 && dy >= 0) {
        // 向右下滚动：复制左上区域到右下
        src_rect = SkIRect::MakeXYWH(dx, dy, width - dx, height - dy);
        dst_x = 0;
        dst_y = 0;
    } else if (dx >= 0 && dy < 0) {
        // 向右上滚动
        src_rect = SkIRect::MakeXYWH(dx, 0, width - dx, height + dy);
        dst_x = 0;
        dst_y = -dy;
    } else if (dx < 0 && dy >= 0) {
        // 向左下滚动
        src_rect = SkIRect::MakeXYWH(0, dy, width + dx, height - dy);
        dst_x = -dx;
        dst_y = 0;
    } else {
        // 向左上滚动
        src_rect = SkIRect::MakeXYWH(0, 0, width + dx, height + dy);
        dst_x = -dx;
        dst_y = -dy;
    }

    // 复制像素
    CopyPixels(layer, src_rect, dst_x, dst_y);

    // 计算需要重绘的新区域
    std::vector<SkIRect> new_regions;
    CalculateScrollDirtyRegions(layer, delta, new_regions);

    // 标记新区域为脏
    for (const auto& region : new_regions) {
        layer->MarkDirty(SkRect::Make(region));
    }

    // 更新滚动偏移缓存
    last_scroll_offsets_[layer->GetId()] = new_offset;

    return true;
}

void Rasterizer::CalculateScrollDirtyRegions(CompositorLayer* layer,
                                              const SkPoint& scroll_delta,
                                              std::vector<SkIRect>& new_regions) {
    new_regions.clear();

    int width = static_cast<int>(layer->GetBounds().width());
    int height = static_cast<int>(layer->GetBounds().height());
    int dx = static_cast<int>(scroll_delta.fX);
    int dy = static_cast<int>(scroll_delta.fY);

    // 水平滚动产生的新区域
    if (dx != 0) {
        if (dx > 0) {
            // 向右滚动，左边出现新区域
            new_regions.push_back(SkIRect::MakeXYWH(0, 0, dx, height));
        } else {
            // 向左滚动，右边出现新区域
            new_regions.push_back(SkIRect::MakeXYWH(width + dx, 0, -dx, height));
        }
    }

    // 垂直滚动产生的新区域
    if (dy != 0) {
        if (dy > 0) {
            // 向下滚动，上边出现新区域
            new_regions.push_back(SkIRect::MakeXYWH(0, 0, width, dy));
        } else {
            // 向上滚动，下边出现新区域
            new_regions.push_back(SkIRect::MakeXYWH(0, height + dy, width, -dy));
        }
    }
}

// =========================================================================
// 私有辅助方法
// =========================================================================

void Rasterizer::PaintRenderObject(SkCanvas* canvas, RenderObject* obj, const SkIRect* clip_rect) {
    if (!canvas || !obj) {
        return;
    }

    // 检查可见性
    const auto& style = obj->GetComputedStyle();
    if (style.visibility == "hidden" || style.display == RenderObjectType::NONE) {
        return;
    }

    // 如果有裁剪区域，检查是否与对象边界相交
    if (clip_rect) {
        SkRect obj_bounds = obj->GetBoundingRect();
        SkIRect obj_ibounds = obj_bounds.roundOut();
        if (!SkIRect::Intersects(*clip_rect, obj_ibounds)) {
            return;  // 不在裁剪区域内，跳过
        }
    }

    // 绘制对象
    obj->Paint(canvas);
}

void Rasterizer::PaintRenderObjectRecursive(SkCanvas* canvas, RenderObject* obj, const SkIRect* clip_rect) {
    if (!canvas || !obj) {
        return;
    }

    // 绘制当前对象
    PaintRenderObject(canvas, obj, clip_rect);

    // 递归绘制子对象
    for (const auto& child : obj->GetChildren()) {
        PaintRenderObjectRecursive(canvas, child.get(), clip_rect);
    }
}

void Rasterizer::UpdatePreviousPaintBoundsForSubtree(RenderObject* obj) {
    if (!obj) {
        return;
    }

    obj->UpdatePreviousPaintBounds(obj->GetBoundingRect(), obj->GetViewportBoundingRect());

    for (const auto& child : obj->GetChildren()) {
        UpdatePreviousPaintBoundsForSubtree(child.get());
    }
}

void Rasterizer::ClearRegion(SkCanvas* canvas, const SkIRect& region) {
    if (!canvas) {
        return;
    }

    SkPaint clear_paint;
    clear_paint.setColor(SK_ColorTRANSPARENT);
    clear_paint.setBlendMode(SkBlendMode::kSrc);

    canvas->drawIRect(region, clear_paint);
}

void Rasterizer::CopyPixels(CompositorLayer* layer,
                            const SkIRect& src_rect,
                            int dst_x, int dst_y) {
    if (!layer) {
        return;
    }

    SkBitmap& bitmap = layer->GetBitmap();
    if (bitmap.isNull() || src_rect.isEmpty()) {
        return;
    }

    int width = src_rect.width();
    int height = src_rect.height();

    // 创建临时缓冲区
    size_t row_bytes = width * 4;  // RGBA
    std::vector<uint8_t> temp_buffer(row_bytes * height);

    // 复制源区域到临时缓冲区
    for (int y = 0; y < height; y++) {
        const uint8_t* src = static_cast<const uint8_t*>(
            bitmap.getAddr(src_rect.left(), src_rect.top() + y));
        uint8_t* dst = temp_buffer.data() + y * row_bytes;
        std::memcpy(dst, src, row_bytes);
    }

    // 从临时缓冲区复制到目标位置
    for (int y = 0; y < height; y++) {
        const uint8_t* src = temp_buffer.data() + y * row_bytes;
        uint8_t* dst = static_cast<uint8_t*>(bitmap.getAddr(dst_x, dst_y + y));
        std::memcpy(dst, src, row_bytes);
    }

    // 通知位图像素已修改
    bitmap.notifyPixelsChanged();
}

// =========================================================================
// 偏移补偿（公共方法，消除 RasterizeLayer/RasterizeRegion 中的重复逻辑）
// =========================================================================

void Rasterizer::ApplyLayerCanvasOffset(SkCanvas* canvas, CompositorLayer* layer,
                                         RenderObject* render_obj, const LayoutInfo& layout) {
    if (!canvas || !layer || !render_obj) return;
    // 根层不做 layout 位置抵消：根层 bitmap 代表整个 viewport，
    // body 的 Paint() 会 translate(layout.x, layout.y) 保留 body margin。
    if (layer->GetPromotionReason() == LayerPromotionReason::RootLayer) {
        return;
    }

    // 1. 基础 layout 位置抵消
    // RenderObject::Paint() 内部会 translate(layout.x, layout.y)
    // 但子层应该从 (0,0) 开始绘制，位置由合成器在合成时应用
    canvas->translate(-layout.x, -layout.y);

    TrackedRasterOffsetState trace_state;
    trace_state.base_layout_x = layout.x;
    trace_state.base_layout_y = layout.y;
    trace_state.reason = layer->GetPromotionReason();
    trace_state.bounds_left = layer->GetBounds().left();
    trace_state.bounds_top = layer->GetBounds().top();

    // 2. 根据层类型应用不同的偏移补偿
    bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);

    if (is_fixed) {
        // Fixed 元素：补偿 bounds 中的 transform 偏移
        const auto& style = render_obj->GetComputedStyle();
        if (style.transform.has_value() && !style.transform->IsEmpty()) {
            const SkRect& bounds = layer->GetBounds();
            float offset_x = bounds.left() - layout.x;
            float offset_y = bounds.top() - layout.y;
            trace_state.fixed_offset_x = offset_x;
            trace_state.fixed_offset_y = offset_y;
            canvas->translate(-offset_x, -offset_y);
        }
    } else {
        // 非 Fixed 元素：先检查动画边界，再检查静态变换偏移
        const AnimationBounds* anim_bounds = layer->GetAnimationBounds();
        if (anim_bounds && anim_bounds->needs_expansion) {
            // 动画边界偏移通常为负值（边界向左上扩展）
            // 需要将内容向右下移动以补偿
            trace_state.anim_offset_x = anim_bounds->offset.fX;
            trace_state.anim_offset_y = anim_bounds->offset.fY;
            canvas->translate(-anim_bounds->offset.fX, -anim_bounds->offset.fY);
        } else {
            // 没有动画边界，检查是否有静态变换偏移
            // 层边界的 left/top 可能包含了变换偏移
            const SkRect& bounds = layer->GetBounds();

            // 获取元素相对于层树父层的原始位置
            auto parent_layer = layer->GetParent();
            RenderObject* parent_layer_obj = parent_layer ? parent_layer->GetRenderObject() : nullptr;

            // 关键修复：与 UpdateLayerBounds 保持一致
            // 如果父层没有 RenderObject（如 content_layer），向上查找祖先层。
            // 同时追踪 effective_parent_layer：提供 parent_layer_obj 的实际祖先层，
            // 用于后续 RootLayer 补偿检查，避免中间层（如 ScrollableContent）干扰。
            CompositorLayer* effective_parent_layer = parent_layer.get();
            if (!parent_layer_obj && parent_layer) {
                auto ancestor = parent_layer->GetParent();
                while (ancestor) {
                    if (ancestor->GetRenderObject()) {
                        parent_layer_obj = ancestor->GetRenderObject();
                        effective_parent_layer = ancestor.get();
                        break;
                    }
                    ancestor = ancestor->GetParent();
                }
            }

            float orig_rel_x = layout.x;
            float orig_rel_y = layout.y;

            // 累加从当前元素到层树父层的位置
            // 注意：不要减去滚动偏移！滚动偏移应该在合成时应用
            auto parent = render_obj->GetParent();
            while (parent && parent.get() != parent_layer_obj) {
                const auto& parent_layout = parent->GetLayoutInfo();
                orig_rel_x += parent_layout.x;
                orig_rel_y += parent_layout.y;
                parent = parent->GetParent();
            }

            // 根层补偿：根层不做 translate(-layout.x, -layout.y)，
            // body 的 Paint() 保留了 translate(layout.x, layout.y)，
            // 所以子层 bounds 需要包含根层 RenderObject 的 layout 偏移。
            // 使用 effective_parent_layer 而非 parent_layer 进行检查，
            // 确保即使 ScrollLayerManager 插入了 content_layer 也能正确补偿。
            if (effective_parent_layer &&
                effective_parent_layer->GetPromotionReason() == LayerPromotionReason::RootLayer
                && parent_layer_obj) {
                const auto& root_layout = parent_layer_obj->GetLayoutInfo();
                orig_rel_x += root_layout.x;
                orig_rel_y += root_layout.y;
            }

            // 计算并补偿变换偏移
            float transform_offset_x = bounds.left() - orig_rel_x;
            float transform_offset_y = bounds.top() - orig_rel_y;
            trace_state.transform_offset_x = transform_offset_x;
            trace_state.transform_offset_y = transform_offset_y;

            if (transform_offset_x != 0 || transform_offset_y != 0) {
                canvas->translate(-transform_offset_x, -transform_offset_y);
            }
        }
    }

    LogTrackedRasterOffset(render_obj, layer, trace_state);
}

} // namespace mblink
