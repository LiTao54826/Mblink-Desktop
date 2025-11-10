/**
 * @file render_cache.cpp
 * @brief 渲染缓存系统实现
 */

#include "render_cache.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include <algorithm>

namespace lightui {

// ========== RenderCache 实现 ==========

RenderCache::RenderCache(size_t max_memory_bytes)
    : max_memory_bytes_(max_memory_bytes)
    , current_memory_usage_(0)
    , hit_count_(0)
    , miss_count_(0) {
}

void RenderCache::Put(const std::string& key, sk_sp<SkImage> image, const SkRect& bounds) {
    if (!image) return;

    // 计算图像大小
    size_t image_size = CalculateImageSize(image);

    // 如果已存在，先移除旧的
    if (Has(key)) {
        Remove(key);
    }

    // 检查是否需要清理缓存
    while (current_memory_usage_ + image_size > max_memory_bytes_ && !cache_.empty()) {
        EvictLRU();
    }

    // 添加新缓存
    CacheEntry entry;
    entry.image = image;
    entry.bounds = bounds;
    entry.last_access_time = std::chrono::steady_clock::now();
    entry.memory_size = image_size;
    entry.is_dirty = false;

    cache_[key] = entry;
    current_memory_usage_ += image_size;
}

CacheEntry* RenderCache::Get(const std::string& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // 更新访问时间
        it->second.last_access_time = std::chrono::steady_clock::now();
        hit_count_++;
        return &it->second;
    }
    miss_count_++;
    return nullptr;
}

bool RenderCache::Has(const std::string& key) const {
    return cache_.find(key) != cache_.end();
}

void RenderCache::Remove(const std::string& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        current_memory_usage_ -= it->second.memory_size;
        cache_.erase(it);
    }
}

void RenderCache::Clear() {
    cache_.clear();
    current_memory_usage_ = 0;
}

void RenderCache::MarkDirty(const std::string& key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.is_dirty = true;
    }
}

void RenderCache::MarkAllDirty() {
    for (auto& pair : cache_) {
        pair.second.is_dirty = true;
    }
}

void RenderCache::CleanDirtyEntries() {
    std::vector<std::string> keys_to_remove;
    for (const auto& pair : cache_) {
        if (pair.second.is_dirty) {
            keys_to_remove.push_back(pair.first);
        }
    }
    for (const auto& key : keys_to_remove) {
        Remove(key);
    }
}

void RenderCache::EvictLRU() {
    if (cache_.empty()) return;

    // 找到最久未访问的条目
    auto oldest = cache_.begin();
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.last_access_time < oldest->second.last_access_time) {
            oldest = it;
        }
    }

    // 移除最久未访问的条目
    current_memory_usage_ -= oldest->second.memory_size;
    cache_.erase(oldest);
}

void RenderCache::EvictIfNeeded() {
    while (current_memory_usage_ > max_memory_bytes_ && !cache_.empty()) {
        EvictLRU();
    }
}

size_t RenderCache::CalculateImageSize(sk_sp<SkImage> image) const {
    if (!image) return 0;
    return image->width() * image->height() * 4; // RGBA
}

// ========== BatchRenderer 实现 ==========

void BatchRenderer::AddDrawRect(float x, float y, float width, float height, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::DRAW_RECT);
    cmd.x = x;
    cmd.y = y;
    cmd.width = width;
    cmd.height = height;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddFillRect(float x, float y, float width, float height, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::FILL_RECT);
    cmd.x = x;
    cmd.y = y;
    cmd.width = width;
    cmd.height = height;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddDrawCircle(float x, float y, float radius, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::DRAW_CIRCLE);
    cmd.x = x;
    cmd.y = y;
    cmd.radius = radius;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddFillCircle(float x, float y, float radius, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::FILL_CIRCLE);
    cmd.x = x;
    cmd.y = y;
    cmd.radius = radius;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddDrawLine(float x1, float y1, float x2, float y2, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::DRAW_LINE);
    cmd.x = x1;
    cmd.y = y1;
    cmd.width = x2;
    cmd.height = y2;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddDrawText(const std::string& text, float x, float y, const SkPaint& paint) {
    RenderCommand cmd(RenderCommandType::DRAW_TEXT);
    cmd.text = text;
    cmd.x = x;
    cmd.y = y;
    cmd.paint = paint;
    commands_.push_back(cmd);
}

void BatchRenderer::AddDrawImage(sk_sp<SkImage> image, float x, float y) {
    RenderCommand cmd(RenderCommandType::DRAW_IMAGE);
    cmd.image = image;
    cmd.x = x;
    cmd.y = y;
    commands_.push_back(cmd);
}

void BatchRenderer::Execute(SkCanvas* canvas) {
    if (!canvas) return;

    for (const auto& cmd : commands_) {
        switch (cmd.type) {
            case RenderCommandType::DRAW_RECT: {
                SkRect rect = SkRect::MakeXYWH(cmd.x, cmd.y, cmd.width, cmd.height);
                canvas->drawRect(rect, cmd.paint);
                break;
            }
            case RenderCommandType::FILL_RECT: {
                SkRect rect = SkRect::MakeXYWH(cmd.x, cmd.y, cmd.width, cmd.height);
                canvas->drawRect(rect, cmd.paint);
                break;
            }
            case RenderCommandType::DRAW_CIRCLE: {
                canvas->drawCircle(cmd.x, cmd.y, cmd.radius, cmd.paint);
                break;
            }
            case RenderCommandType::FILL_CIRCLE: {
                canvas->drawCircle(cmd.x, cmd.y, cmd.radius, cmd.paint);
                break;
            }
            case RenderCommandType::DRAW_LINE: {
                canvas->drawLine(cmd.x, cmd.y, cmd.width, cmd.height, cmd.paint);
                break;
            }
            case RenderCommandType::DRAW_TEXT: {
                SkFont font;
                canvas->drawString(cmd.text.c_str(), cmd.x, cmd.y, font, cmd.paint);
                break;
            }
            case RenderCommandType::DRAW_IMAGE: {
                if (cmd.image) {
                    canvas->drawImage(cmd.image, cmd.x, cmd.y);
                }
                break;
            }
        }
    }
}

void BatchRenderer::Clear() {
    commands_.clear();
}

void BatchRenderer::Optimize() {
    // 简单的优化：合并相邻的相同颜色的填充矩形
    // 这里只是一个示例，实际优化可以更复杂
    
    if (commands_.size() < 2) return;

    std::vector<RenderCommand> optimized;
    optimized.reserve(commands_.size());

    for (size_t i = 0; i < commands_.size(); ++i) {
        const auto& cmd = commands_[i];
        
        // 检查是否可以与下一个命令合并
        bool merged = false;
        if (i + 1 < commands_.size() && 
            cmd.type == RenderCommandType::FILL_RECT &&
            commands_[i + 1].type == RenderCommandType::FILL_RECT) {
            
            const auto& next_cmd = commands_[i + 1];
            
            // 检查颜色是否相同
            if (cmd.paint.getColor() == next_cmd.paint.getColor()) {
                // 检查是否相邻（水平或垂直）
                bool adjacent = false;
                SkRect merged_rect;
                
                // 水平相邻
                if (cmd.y == next_cmd.y && cmd.height == next_cmd.height &&
                    cmd.x + cmd.width == next_cmd.x) {
                    merged_rect = SkRect::MakeXYWH(cmd.x, cmd.y, 
                                                   cmd.width + next_cmd.width, cmd.height);
                    adjacent = true;
                }
                // 垂直相邻
                else if (cmd.x == next_cmd.x && cmd.width == next_cmd.width &&
                         cmd.y + cmd.height == next_cmd.y) {
                    merged_rect = SkRect::MakeXYWH(cmd.x, cmd.y, 
                                                   cmd.width, cmd.height + next_cmd.height);
                    adjacent = true;
                }
                
                if (adjacent) {
                    RenderCommand merged_cmd(RenderCommandType::FILL_RECT);
                    merged_cmd.x = merged_rect.left();
                    merged_cmd.y = merged_rect.top();
                    merged_cmd.width = merged_rect.width();
                    merged_cmd.height = merged_rect.height();
                    merged_cmd.paint = cmd.paint;
                    optimized.push_back(merged_cmd);
                    i++; // 跳过下一个命令
                    merged = true;
                }
            }
        }
        
        if (!merged) {
            optimized.push_back(cmd);
        }
    }

    commands_ = std::move(optimized);
}

} // namespace lightui

