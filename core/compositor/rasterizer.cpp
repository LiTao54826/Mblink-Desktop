/**
 * @file rasterizer.cpp
 * @brief 光栅化器实现
 */

#include "rasterizer.h"
#include "compositor_layer.h"
#include "animation/animation_bounds_calculator.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRegion.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

namespace lightui {

Rasterizer::Rasterizer() = default;
Rasterizer::~Rasterizer() = default;

// =========================================================================
// 完整光栅化
// =========================================================================

bool Rasterizer::RasterizeLayer(CompositorLayer* layer) {
    if (!layer) {
        return false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 确保位图已分配
    if (!layer->EnsureBitmap()) {
        std::cout << "[DEBUG RasterizeLayer] EnsureBitmap failed for layer " << layer->GetId() << std::endl;
        return false;
    }

    SkCanvas* canvas = layer->GetCanvas();
    if (!canvas) {
        std::cout << "[DEBUG RasterizeLayer] GetCanvas failed for layer " << layer->GetId() << std::endl;
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

    // 清除整个位图
    canvas->clear(SK_ColorTRANSPARENT);

    // 保存 Canvas 状态
    canvas->save();

    // 关键修复：光栅化阶段不应用滚动偏移
    // 滚动偏移应该在合成阶段应用，这样滚动时只需要更新合成参数，
    // 不需要重新光栅化，性能更好。
    // 
    // 旧代码（已移除）：
    // const SkPoint& scroll = layer->GetScrollOffset();
    // if (scroll.fX != 0 || scroll.fY != 0) {
    //     canvas->translate(-scroll.fX, -scroll.fY);
    // }

    // 关键修复：对于非根层，需要抵消元素的 layout 位置
    // 因为 RenderObject::Paint() 内部会 translate(layout.x, layout.y)
    // 但子层应该从 (0,0) 开始绘制，位置由合成器在合成时应用
    if (layer->GetPromotionReason() != LayerPromotionReason::RootLayer) {
        canvas->translate(-layout.x, -layout.y);
        
        // 对于 position: fixed 元素，需要补偿 bounds 中的 transform 偏移
        // bounds 包含了 transform 偏移（例如 translateX(-50%) 导致的负偏移）
        // 我们需要将内容绘制在位图的正确位置
        bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);
        if (is_fixed) {
            const auto& style = render_obj->GetComputedStyle();
            if (style.transform.has_value() && !style.transform->IsEmpty()) {
                // 获取 bounds 中记录的偏移
                // bounds.left() = layout.x + offset_x，其中 offset_x = min_x - padding
                // 所以 offset_x = bounds.left() - layout.x
                const SkRect& bounds = layer->GetBounds();
                float offset_x = bounds.left() - layout.x;
                float offset_y = bounds.top() - layout.y;
                
                // 补偿 bounds 中的偏移
                // 这样内容会绘制在位图的 (-offset_x, -offset_y) 位置
                // 即 (padding - min_x, padding - min_y) 位置
                // Paint 应用 transform 后，内容会移动到 (padding, padding)
                canvas->translate(-offset_x, -offset_y);
            }
        } else {
            // 关键修复：如果层有动画边界扩展，需要额外平移以补偿边界扩展的偏移
            // 动画边界的 offset 表示边界相对于元素原始位置的偏移
            // 我们需要将内容绘制在位图的正确位置，以便合成时显示正确
            const AnimationBounds* anim_bounds = layer->GetAnimationBounds();
            if (anim_bounds && anim_bounds->needs_expansion) {
                // 动画边界偏移通常为负值（边界向左上扩展）
                // 需要将内容向右下移动以补偿
                canvas->translate(-anim_bounds->offset.fX, -anim_bounds->offset.fY);
            } else {
                // 没有动画边界，检查是否有静态变换偏移
                // 层边界的 left/top 可能包含了变换偏移
                const SkRect& bounds = layer->GetBounds();
                
                // 获取元素相对于层树父层的原始位置
                auto parent_layer = layer->GetParent();
                RenderObject* parent_layer_obj = parent_layer ? parent_layer->GetRenderObject() : nullptr;
                
                float orig_rel_x = layout.x;
                float orig_rel_y = layout.y;
                
                // 累加从当前元素到层树父层的位置
                // 注意：不要减去滚动偏移！滚动偏移应该在合成时应用，而不是在光栅化时应用
                // 这样可以避免双重滚动的问题
                auto parent = render_obj->GetParent();
                while (parent && parent.get() != parent_layer_obj) {
                    const auto& parent_layout = parent->GetLayoutInfo();
                    orig_rel_x += parent_layout.x;
                    orig_rel_y += parent_layout.y;
                    parent = parent->GetParent();
                }
                
                // 计算变换偏移
                float transform_offset_x = bounds.left() - orig_rel_x;
                float transform_offset_y = bounds.top() - orig_rel_y;
                
                // 补偿变换偏移
                if (transform_offset_x != 0 || transform_offset_y != 0) {
                    canvas->translate(-transform_offset_x, -transform_offset_y);
                }
            }
        }
    }

    // 绘制渲染对象
    // 注意：RenderObject::Paint 内部已经递归绘制子对象了，不需要额外递归
    render_obj->Paint(canvas);

    // 恢复 Canvas 状态
    canvas->restore();

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

    // 保存 Canvas 状态
    canvas->save();

    // 清除区域为透明（在设置裁剪之前）
    canvas->save();
    canvas->clipIRect(region);
    ClearRegion(canvas, region);
    canvas->restore();

    // 关键修复：对于非根层，需要抵消元素的 layout 位置
    // 因为 RenderObject::Paint() 内部会 translate(layout.x, layout.y)
    // 但子层应该从 (0,0) 开始绘制，位置由合成器在合成时应用
    if (layer->GetPromotionReason() != LayerPromotionReason::RootLayer) {
        canvas->translate(-layout.x, -layout.y);
        
        // 对于 position: fixed 元素，需要补偿 bounds 中的 transform 偏移
        bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);
        if (is_fixed) {
            const auto& style = render_obj->GetComputedStyle();
            if (style.transform.has_value() && !style.transform->IsEmpty()) {
                const SkRect& bounds = layer->GetBounds();
                float offset_x = bounds.left() - layout.x;
                float offset_y = bounds.top() - layout.y;
                canvas->translate(-offset_x, -offset_y);
            }
        } else {
            // 处理动画边界偏移
            const AnimationBounds* anim_bounds = layer->GetAnimationBounds();
            if (anim_bounds && anim_bounds->needs_expansion) {
                canvas->translate(-anim_bounds->offset.fX, -anim_bounds->offset.fY);
            }
        }
    }
    
    // 设置裁剪区域
    // 对于根层：region 是位图坐标，Paint 会 translate(layout.x, layout.y)
    //          所以裁剪区域不需要偏移
    // 对于非根层：canvas 已经 translate(-layout.x, -layout.y)
    //           需要将位图坐标的 region 转换到 canvas 坐标系
    SkRect clip_rect = SkRect::Make(region);
    if (layer->GetPromotionReason() != LayerPromotionReason::RootLayer) {
        float offset_x = layout.x;
        float offset_y = layout.y;
        
        bool is_fixed = (layer->GetPromotionReason() == LayerPromotionReason::PositionFixed);
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

} // namespace lightui
