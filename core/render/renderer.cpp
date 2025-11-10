/**
 * @file renderer.cpp
 * @brief Skia 渲染器基类实现
 */

#include "renderer.h"

namespace lightui {

// ========== 构造函数 ==========

Renderer::Renderer(SkCanvas* canvas)
    : surface_(nullptr)
    , canvas_(canvas)
    , owns_surface_(false) {
    if (!canvas_) {
        throw std::invalid_argument("Canvas cannot be null");
    }
}

Renderer::Renderer(sk_sp<SkSurface> surface)
    : surface_(surface)
    , canvas_(surface ? surface->getCanvas() : nullptr)
    , owns_surface_(true) {
    if (!surface_) {
        throw std::invalid_argument("Surface cannot be null");
    }
    if (!canvas_) {
        throw std::runtime_error("Failed to get canvas from surface");
    }
}

// ========== 基础操作 ==========

void Renderer::Clear(SkColor color) {
    if (canvas_) {
        canvas_->clear(color);
    }
}

void Renderer::Flush() {
    // Flush is handled automatically by Skia when needed
    // For raster surfaces, no explicit flush is required
    // For GPU surfaces, flush would be called on the GrDirectContext
}

// ========== 状态管理 ==========

int Renderer::Save() {
    if (canvas_) {
        return canvas_->save();
    }
    return 0;
}

void Renderer::Restore() {
    if (canvas_) {
        canvas_->restore();
    }
}

int Renderer::GetSaveCount() const {
    if (canvas_) {
        return canvas_->getSaveCount();
    }
    return 0;
}

// ========== 变换操作 ==========

void Renderer::Translate(float dx, float dy) {
    if (canvas_) {
        canvas_->translate(dx, dy);
    }
}

void Renderer::Scale(float sx, float sy) {
    if (canvas_) {
        canvas_->scale(sx, sy);
    }
}

void Renderer::Rotate(float degrees) {
    if (canvas_) {
        canvas_->rotate(degrees);
    }
}

void Renderer::Rotate(float degrees, float px, float py) {
    if (canvas_) {
        canvas_->rotate(degrees, px, py);
    }
}

// ========== 裁剪操作 ==========

void Renderer::ClipRect(const SkRect& rect, bool doAntiAlias) {
    if (canvas_) {
        canvas_->clipRect(rect, doAntiAlias);
    }
}

void Renderer::ClipPath(const SkPath& path, bool doAntiAlias) {
    if (canvas_) {
        canvas_->clipPath(path, doAntiAlias);
    }
}

} // namespace lightui
