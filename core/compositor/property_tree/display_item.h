/**
 * @file display_item.h
 * @brief 显示项
 *
 * 显示项是最小的绘制单元，包含：
 * - 类型（绘制、外部层、滚动条等）
 * - 关联的 RenderObject
 * - 绘制记录（SkPicture）
 * - 边界
 *
 * 参考 Chromium Blink: platform/graphics/paint/display_item.h
 */

#pragma once

#include "include/core/SkPicture.h"
#include "include/core/SkRect.h"
#include <cstdint>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 显示项类型
 */
enum class DisplayItemType : uint8_t {
    kDrawing,           // 绘制指令
    kDrawRect,          // 绘制矩形
    kDrawText,          // 绘制文本
    kDrawImage,         // 绘制图像
    kForeignLayer,      // 外部层（video、canvas）
    kScrollbar,         // 滚动条
    kClipPath,          // 裁剪路径
    kMask,              // 遮罩
    kBeginClip,         // 开始裁剪
    kEndClip,           // 结束裁剪
    kBeginEffect,       // 开始效果
    kEndEffect,         // 结束效果
    kBeginTransform,    // 开始变换
    kEndTransform,      // 结束变换
};

/**
 * @brief 显示项 ID
 *
 * 用于缓存匹配和增量更新。
 */
struct DisplayItemId {
    RenderObject* client = nullptr;
    DisplayItemType type = DisplayItemType::kDrawing;
    uint32_t fragment_index = 0;  // 对于分片的元素
    
    bool operator==(const DisplayItemId& other) const {
        return client == other.client &&
               type == other.type &&
               fragment_index == other.fragment_index;
    }
    
    bool operator!=(const DisplayItemId& other) const {
        return !(*this == other);
    }
};

/**
 * @brief DisplayItemId 哈希函数
 */
struct DisplayItemIdHash {
    size_t operator()(const DisplayItemId& id) const {
        size_t h1 = std::hash<void*>()(id.client);
        size_t h2 = std::hash<uint8_t>()(static_cast<uint8_t>(id.type));
        size_t h3 = std::hash<uint32_t>()(id.fragment_index);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief 显示项
 *
 * 最小的绘制单元，包含绘制指令或特殊操作。
 */
class DisplayItem {
public:
    /**
     * @brief 构造函数
     * @param type 显示项类型
     * @param client 关联的 RenderObject
     */
    DisplayItem(DisplayItemType type, RenderObject* client);
    
    /**
     * @brief 默认构造函数
     */
    DisplayItem() = default;
    
    ~DisplayItem() = default;

    // 支持移动
    DisplayItem(DisplayItem&&) = default;
    DisplayItem& operator=(DisplayItem&&) = default;
    
    // 支持拷贝
    DisplayItem(const DisplayItem&) = default;
    DisplayItem& operator=(const DisplayItem&) = default;

    // =========================================================================
    // 类型
    // =========================================================================

    /**
     * @brief 获取类型
     */
    DisplayItemType GetType() const { return type_; }

    /**
     * @brief 设置类型
     */
    void SetType(DisplayItemType type) { type_ = type; }

    /**
     * @brief 是否是绘制类型
     */
    bool IsDrawing() const { return type_ == DisplayItemType::kDrawing; }

    /**
     * @brief 是否是外部层
     */
    bool IsForeignLayer() const { return type_ == DisplayItemType::kForeignLayer; }

    /**
     * @brief 是否是开始类型（需要配对的结束）
     */
    bool IsBeginType() const {
        return type_ == DisplayItemType::kBeginClip ||
               type_ == DisplayItemType::kBeginEffect ||
               type_ == DisplayItemType::kBeginTransform;
    }

    /**
     * @brief 是否是结束类型
     */
    bool IsEndType() const {
        return type_ == DisplayItemType::kEndClip ||
               type_ == DisplayItemType::kEndEffect ||
               type_ == DisplayItemType::kEndTransform;
    }

    // =========================================================================
    // 关联对象
    // =========================================================================

    /**
     * @brief 获取关联的 RenderObject
     */
    RenderObject* GetClient() const { return client_; }

    /**
     * @brief 设置关联的 RenderObject
     */
    void SetClient(RenderObject* client) { client_ = client; }

    // =========================================================================
    // 绘制记录
    // =========================================================================

    /**
     * @brief 获取绘制记录
     */
    const sk_sp<SkPicture>& GetPicture() const { return picture_; }

    /**
     * @brief 设置绘制记录
     */
    void SetPicture(sk_sp<SkPicture> picture) { picture_ = std::move(picture); }

    /**
     * @brief 是否有绘制记录
     */
    bool HasPicture() const { return picture_ != nullptr; }

    // =========================================================================
    // 边界
    // =========================================================================

    /**
     * @brief 获取边界
     */
    const SkRect& GetBounds() const { return bounds_; }

    /**
     * @brief 设置边界
     */
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }

    // =========================================================================
    // 缓存
    // =========================================================================

    /**
     * @brief 是否可缓存
     */
    bool IsCacheable() const { return cacheable_; }

    /**
     * @brief 设置是否可缓存
     */
    void SetCacheable(bool cacheable) { cacheable_ = cacheable; }

    // =========================================================================
    // 标识符
    // =========================================================================

    /**
     * @brief 获取唯一标识符
     */
    DisplayItemId GetId() const {
        return DisplayItemId{client_, type_, fragment_index_};
    }

    /**
     * @brief 获取分片索引
     */
    uint32_t GetFragmentIndex() const { return fragment_index_; }

    /**
     * @brief 设置分片索引
     */
    void SetFragmentIndex(uint32_t index) { fragment_index_ = index; }

    // =========================================================================
    // 比较
    // =========================================================================

    /**
     * @brief 检查是否与另一个显示项匹配（用于缓存）
     */
    bool Matches(const DisplayItem& other) const {
        return GetId() == other.GetId();
    }

private:
    DisplayItemType type_ = DisplayItemType::kDrawing;
    RenderObject* client_ = nullptr;
    sk_sp<SkPicture> picture_;
    SkRect bounds_ = SkRect::MakeEmpty();
    bool cacheable_ = true;
    uint32_t fragment_index_ = 0;
};

} // namespace lightui
