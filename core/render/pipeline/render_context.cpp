/**
 * @file render_context.cpp
 * @brief 渲染上下文管理实现
 */

#include "render_context.h"
#include <algorithm>

namespace mblink {

// ========== 构造函数 ==========

RenderContext::RenderContext(SkCanvas* canvas)
    : canvas_(canvas)
    , state_stack_()
    , current_state_() {
}

// ========== 状态管理 ==========

void RenderContext::Save() {
    state_stack_.push(current_state_);
    if (canvas_) {
        canvas_->save();
    }
}

void RenderContext::Restore() {
    if (!state_stack_.empty()) {
        current_state_ = state_stack_.top();
        state_stack_.pop();
        
        if (canvas_) {
            canvas_->restore();
        }
    }
}

size_t RenderContext::GetStateStackSize() const {
    return state_stack_.size();
}

// ========== 变换操作 ==========

void RenderContext::Translate(float dx, float dy) {
    current_state_.matrix.preTranslate(dx, dy);
    if (canvas_) {
        canvas_->translate(dx, dy);
    }
}

void RenderContext::Scale(float sx, float sy) {
    current_state_.matrix.preScale(sx, sy);
    if (canvas_) {
        canvas_->scale(sx, sy);
    }
}

void RenderContext::Rotate(float degrees) {
    current_state_.matrix.preRotate(degrees);
    if (canvas_) {
        canvas_->rotate(degrees);
    }
}

void RenderContext::Rotate(float degrees, float px, float py) {
    current_state_.matrix.preRotate(degrees, px, py);
    if (canvas_) {
        canvas_->rotate(degrees, px, py);
    }
}

void RenderContext::SetMatrix(const SkMatrix& matrix) {
    current_state_.matrix = matrix;
    if (canvas_) {
        canvas_->setMatrix(matrix);
    }
}

const SkMatrix& RenderContext::GetMatrix() const {
    return current_state_.matrix;
}

void RenderContext::ResetMatrix() {
    current_state_.matrix.reset();
    if (canvas_) {
        canvas_->resetMatrix();
    }
}

// ========== 裁剪操作 ==========

void RenderContext::ClipRect(const SkRect& rect) {
    current_state_.clip_rect = rect;
    if (canvas_) {
        canvas_->clipRect(rect);
    }
}

void RenderContext::ClipPath(const SkPath& path) {
    if (canvas_) {
        canvas_->clipPath(path);
    }
}

const SkRect& RenderContext::GetClipRect() const {
    return current_state_.clip_rect;
}

// ========== 画笔管理 ==========

void RenderContext::SetPaint(const Paint& paint) {
    current_state_.paint = paint;
}

Paint& RenderContext::GetPaint() {
    return current_state_.paint;
}

const Paint& RenderContext::GetPaint() const {
    return current_state_.paint;
}

// ========== 透明度管理 ==========

void RenderContext::SetGlobalAlpha(float alpha) {
    current_state_.global_alpha = std::clamp(alpha, 0.0f, 1.0f);
}

float RenderContext::GetGlobalAlpha() const {
    return current_state_.global_alpha;
}

// ========== 画布访问 ==========

void RenderContext::ApplyToCanvas() {
    if (!canvas_) {
        return;
    }
    
    // 应用变换矩阵
    canvas_->setMatrix(current_state_.matrix);
    
    // 应用裁剪
    if (!current_state_.clip_rect.isEmpty()) {
        canvas_->clipRect(current_state_.clip_rect);
    }
}

} // namespace mblink

