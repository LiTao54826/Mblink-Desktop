/**
 * @file compositor_layer.cpp
 * @brief 合成层实现
 */

#include "compositor_layer.h"
#include "animation/animation_bounds_calculator.h"
#include "core/render/objects/render_object.h"
#include "include/core/SkImageInfo.h"
#include <algorithm>
#include <cstring>

// OpenGL headers
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

// OpenGL 扩展常量（Windows gl.h 可能不包含这些）
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_UNPACK_SKIP_PIXELS
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#endif

#ifndef GL_UNPACK_SKIP_ROWS
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

namespace lightui {

// 静态 ID 生成器
uint32_t CompositorLayer::next_id_ = 1;
uint64_t CompositorLayer::next_layer_identity_ = 1;

CompositorLayer::CompositorLayer(uint32_t id)
    : id_(id == 0 ? next_id_++ : id)
    , layer_identity_(next_layer_identity_++) {
}

CompositorLayer::~CompositorLayer() {
    DestroyTexture();
    ReleaseBitmap();
}

// =========================================================================
// 边界和变换
// =========================================================================

void CompositorLayer::SetBounds(const SkRect& bounds) {
    if (bounds_ == bounds) {
        return;
    }

    // 检查大小是否改变
    bool size_changed = (bounds_.width() != bounds.width() ||
                         bounds_.height() != bounds.height());

    bounds_ = bounds;

    if (size_changed) {
        // 大小改变，需要重新分配位图
        bitmap_valid_ = false;
        // 标记整个层为脏
        MarkFullDirty();
    }
}

// =========================================================================
// CPU 位图管理
// =========================================================================

SkCanvas* CompositorLayer::GetCanvas() {
    if (!EnsureBitmap()) {
        return nullptr;
    }
    return canvas_.get();
}

bool CompositorLayer::EnsureBitmap() {
    // 使用物理像素大小（DPI 缩放后的大小）以保证清晰度
    int width = static_cast<int>(std::ceil(bounds_.width() * dpi_scale_));
    int height = static_cast<int>(std::ceil(bounds_.height() * dpi_scale_));
    
    if (width <= 0 || height <= 0) {
        return false;
    }

    // 检查是否需要重新分配
    if (bitmap_valid_ && bitmap_.width() == width && bitmap_.height() == height) {
        return true;
    }

    // 分配新位图（物理像素大小）
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    if (!bitmap_.tryAllocPixels(info)) {
        bitmap_valid_ = false;
        return false;
    }

    // 清除为透明
    bitmap_.eraseColor(SK_ColorTRANSPARENT);

    // 创建 Canvas 并应用 DPI 缩放，使绘制使用逻辑坐标
    canvas_ = std::make_unique<SkCanvas>(bitmap_);
    if (dpi_scale_ != 1.0f) {
        canvas_->scale(dpi_scale_, dpi_scale_);
    }
    bitmap_valid_ = true;

    return true;
}

void CompositorLayer::ReleaseBitmap() {
    canvas_.reset();
    bitmap_.reset();
    bitmap_valid_ = false;
}

// =========================================================================
// 脏区域管理
// =========================================================================

void CompositorLayer::MarkDirty(const SkRect& region) {
    // 转换为整数矩形
    SkIRect iregion = region.roundOut();

    // 裁剪到层边界
    SkIRect layer_bounds = SkIRect::MakeWH(
        static_cast<int>(bounds_.width()),
        static_cast<int>(bounds_.height())
    );

    // 关键修复：对于根层，不裁剪脏区域
    // 因为根层的边界是视口大小，但内容可能超出视口（滚动内容）
    // 脏区域会在光栅化时被正确裁剪
    if (promotion_reason_ != LayerPromotionReason::RootLayer) {
        if (!iregion.intersect(layer_bounds)) {
            return;  // 区域在层外
        }
    } else {
        // 对于根层，只确保区域不是完全在负坐标区域
        // 但允许超出视口边界的区域（滚动内容）
        if (iregion.right() <= 0 || iregion.bottom() <= 0) {
            return;  // 完全在可见区域外
        }
        // 裁剪负坐标部分
        if (iregion.left() < 0) iregion.fLeft = 0;
        if (iregion.top() < 0) iregion.fTop = 0;
    }

    dirty_regions_.push_back(iregion);
}

void CompositorLayer::MarkFullDirty() {
    dirty_regions_.clear();
    dirty_regions_.push_back(SkIRect::MakeWH(
        static_cast<int>(bounds_.width()),
        static_cast<int>(bounds_.height())
    ));
}

void CompositorLayer::ClearDirtyRegions() {
    dirty_regions_.clear();
}

void CompositorLayer::MergeDirtyRegions() {
    if (dirty_regions_.size() <= 1) {
        return;
    }

    // 简单的合并策略：合并重叠或相邻的矩形
    std::vector<SkIRect> merged;
    merged.reserve(dirty_regions_.size());

    // 按 Y 坐标排序
    std::sort(dirty_regions_.begin(), dirty_regions_.end(),
        [](const SkIRect& a, const SkIRect& b) {
            if (a.top() != b.top()) return a.top() < b.top();
            return a.left() < b.left();
        });

    for (const auto& rect : dirty_regions_) {
        bool was_merged = false;

        // 尝试与已有矩形合并
        for (auto& existing : merged) {
            // 检查是否重叠或相邻（允许 1 像素间隙）
            SkIRect expanded = existing;
            expanded.outset(1, 1);

            if (SkIRect::Intersects(expanded, rect)) {
                // 合并
                existing.join(rect);
                was_merged = true;
                break;
            }
        }

        if (!was_merged) {
            merged.push_back(rect);
        }
    }

    // 如果合并后的矩形总面积超过层面积的 50%，直接使用整层
    int layer_area = static_cast<int>(bounds_.width() * bounds_.height());
    int merged_area = 0;
    for (const auto& rect : merged) {
        merged_area += rect.width() * rect.height();
    }

    if (merged_area > layer_area / 2) {
        dirty_regions_.clear();
        dirty_regions_.push_back(SkIRect::MakeWH(
            static_cast<int>(bounds_.width()),
            static_cast<int>(bounds_.height())
        ));
    } else {
        dirty_regions_ = std::move(merged);
    }
}

// =========================================================================
// GPU 纹理管理
// =========================================================================

void CompositorLayer::MarkTextureDirty(const SkIRect& region) {
    texture_dirty_regions_.push_back(region);
}

bool CompositorLayer::UploadDirtyRegions() {
    if (!bitmap_valid_ || texture_id_ == 0) {
        return false;
    }

    if (texture_dirty_regions_.empty()) {
        return true;  // 没有需要上传的区域
    }

    // 检查纹理大小是否匹配
    int bmp_width = bitmap_.width();
    int bmp_height = bitmap_.height();

    if (texture_width_ != bmp_width || texture_height_ != bmp_height) {
        // 纹理大小不匹配，需要重新创建
        DestroyTexture();
        if (!CreateTexture()) {
            return false;
        }
        // 上传整个位图
        texture_dirty_regions_.clear();
        texture_dirty_regions_.push_back(SkIRect::MakeWH(bmp_width, bmp_height));
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // 上传每个脏区域
    for (const auto& region : texture_dirty_regions_) {
        // 裁剪到位图边界
        SkIRect clipped = region;
        if (!clipped.intersect(SkIRect::MakeWH(bmp_width, bmp_height))) {
            continue;
        }

        // 获取像素数据
        const void* pixels = bitmap_.getAddr(clipped.left(), clipped.top());
        int row_bytes = static_cast<int>(bitmap_.rowBytes());

        // 设置像素存储参数
        glPixelStorei(GL_UNPACK_ROW_LENGTH, bmp_width);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        // 上传子区域
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,  // mipmap level
            clipped.left(),
            clipped.top(),
            clipped.width(),
            clipped.height(),
            GL_BGRA_EXT,  // Skia 使用 BGRA 格式
            GL_UNSIGNED_BYTE,
            pixels
        );

        // 重置像素存储参数
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    texture_dirty_regions_.clear();

    return true;
}

bool CompositorLayer::CreateTexture() {
    if (texture_id_ != 0) {
        return true;  // 已存在
    }

    int width = static_cast<int>(bounds_.width());
    int height = static_cast<int>(bounds_.height());

    if (width <= 0 || height <= 0) {
        return false;
    }

    glGenTextures(1, &texture_id_);
    if (texture_id_ == 0) {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 分配纹理存储
    glTexImage2D(
        GL_TEXTURE_2D,
        0,  // mipmap level
        GL_RGBA,
        width,
        height,
        0,  // border
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE,
        nullptr  // 不初始化数据
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    texture_width_ = width;
    texture_height_ = height;

    return true;
}

void CompositorLayer::DestroyTexture() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
        texture_width_ = 0;
        texture_height_ = 0;
    }
    texture_dirty_regions_.clear();
}

// =========================================================================
// 父子关系
// =========================================================================

void CompositorLayer::AddChild(std::shared_ptr<CompositorLayer> child) {
    if (!child) {
        return;
    }

    // 从旧父层移除
    if (auto old_parent = child->GetParent()) {
        old_parent->RemoveChild(child.get());
    }

    child->SetParent(shared_from_this());
    children_.push_back(child);
}

void CompositorLayer::RemoveChild(CompositorLayer* child) {
    if (!child) {
        return;
    }

    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::shared_ptr<CompositorLayer>& c) {
            return c.get() == child;
        });

    if (it != children_.end()) {
        (*it)->SetParent(nullptr);
        children_.erase(it);
    }
}

void CompositorLayer::RemoveAllChildren() {
    for (auto& child : children_) {
        child->SetParent(nullptr);
    }
    children_.clear();
}

// =========================================================================
// 滚动支持
// =========================================================================

void CompositorLayer::ScrollBy(float dx, float dy) {
    scroll_offset_.fX += dx;
    scroll_offset_.fY += dy;
}

// =========================================================================
// 增量光栅化支持
// =========================================================================

void CompositorLayer::AddRasterDirtyRect(const SkRect& rect) {
    // 裁剪到层边界
    SkRect clipped;
    if (!clipped.intersect(rect, SkRect::MakeWH(bounds_.width(), bounds_.height()))) {
        return;  // 区域在层外
    }
    
    // 检查是否与已有区域重叠，如果重叠则合并
    for (auto& existing : raster_dirty_rects_) {
        SkRect expanded = existing;
        expanded.outset(1, 1);  // 允许 1 像素间隙
        if (expanded.intersects(clipped)) {
            existing.join(clipped);
            return;
        }
    }
    
    raster_dirty_rects_.push_back(clipped);
}

// =========================================================================
// DPI 缩放支持
// =========================================================================

void CompositorLayer::SetDpiScale(float scale) {
    if (scale <= 0) {
        scale = 1.0f;
    }
    
    if (dpi_scale_ == scale) {
        return;
    }
    
    dpi_scale_ = scale;
    
    // DPI 缩放改变，需要重新分配位图
    bitmap_valid_ = false;
    MarkFullDirty();
}

// =========================================================================
// 动画边界支持
// =========================================================================

const AnimationBounds* CompositorLayer::GetAnimationBounds() const {
    return animation_bounds_.has_value() ? &animation_bounds_.value() : nullptr;
}

void CompositorLayer::SetAnimationBounds(const AnimationBounds& bounds) {
    // 检查动画边界是否改变
    bool bounds_changed = !animation_bounds_.has_value() ||
                          animation_bounds_->offset != bounds.offset ||
                          animation_bounds_->bounds != bounds.bounds ||
                          animation_bounds_->needs_expansion != bounds.needs_expansion;
    
    animation_bounds_ = bounds;
    
    // 如果动画边界改变，需要重新光栅化
    if (bounds_changed && bounds.needs_expansion) {
        MarkFullDirty();
    }
}

void CompositorLayer::ClearAnimationBounds() {
    animation_bounds_.reset();
}

bool CompositorLayer::HasAnimationBounds() const {
    return animation_bounds_.has_value() && animation_bounds_->needs_expansion;
}

// =========================================================================
// 调试支持
// =========================================================================

const char* CompositorLayer::PromotionReasonToString(LayerPromotionReason reason) {
    switch (reason) {
        case LayerPromotionReason::None:
            return "None";
        case LayerPromotionReason::WillChangeTransform:
            return "will-change: transform";
        case LayerPromotionReason::WillChangeOpacity:
            return "will-change: opacity";
        case LayerPromotionReason::PositionFixed:
            return "position: fixed";
        case LayerPromotionReason::TransformAnimation:
            return "transform animation";
        case LayerPromotionReason::OpacityAnimation:
            return "opacity animation";
        case LayerPromotionReason::ScrollableContent:
            return "scrollable content";
        case LayerPromotionReason::HighZIndex:
            return "high z-index";
        case LayerPromotionReason::Explicit:
            return "explicit";
        case LayerPromotionReason::RootLayer:
            return "root layer";
        default:
            return "unknown";
    }
}

// =========================================================================
// 增量更新支持
// =========================================================================

int CompositorLayer::GetTreeDepth() const {
    int depth = 0;
    auto parent = parent_.lock();
    while (parent) {
        depth++;
        parent = parent->parent_.lock();
    }
    return depth;
}

void CompositorLayer::ReparentTo(std::shared_ptr<CompositorLayer> new_parent) {
    // 从当前父层移除
    if (auto old_parent = parent_.lock()) {
        old_parent->RemoveChild(this);
    }
    
    // 添加到新父层
    if (new_parent) {
        new_parent->AddChild(shared_from_this());
    } else {
        parent_.reset();
    }
}

void CompositorLayer::InsertChildByZIndex(std::shared_ptr<CompositorLayer> child, int z_index) {
    if (!child) {
        return;
    }
    
    // 从旧父层移除
    if (auto old_parent = child->GetParent()) {
        old_parent->RemoveChild(child.get());
    }
    
    child->SetParent(shared_from_this());
    
    // 找到正确的插入位置（按 z-index 升序）
    auto it = std::find_if(children_.begin(), children_.end(),
        [z_index](const std::shared_ptr<CompositorLayer>& c) {
            return c->GetZIndex() > z_index;
        });
    
    children_.insert(it, child);
}

int CompositorLayer::GetZIndex() const {
    if (!render_object_) {
        return 0;
    }
    
    // 从关联的 RenderObject 获取 z-index
    const auto& style = render_object_->GetComputedStyle();
    return style.z_index;
}

// =========================================================================
// 工厂函数
// =========================================================================

std::shared_ptr<CompositorLayer> CreateCompositorLayer() {
    return std::make_shared<CompositorLayer>(0);  // 0 表示自动分配 ID
}

} // namespace lightui
