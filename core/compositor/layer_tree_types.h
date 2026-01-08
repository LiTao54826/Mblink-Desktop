/**
 * @file layer_tree_types.h
 * @brief 层树系统的类型定义
 *
 * 包含 LayerTreeManager 和 LayerTreeBuilder 共用的类型定义。
 */

#pragma once

#include "compositor_layer.h"
#include <cstdint>
#include <functional>
#include <string>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 层更新操作类型
 */
enum class LayerUpdateType {
    Add,            ///< 添加层
    Remove,         ///< 移除层
    UpdateBounds,   ///< 更新边界
    Reparent,       ///< 重新附加父层
    UpdateZIndex    ///< 更新 z-index
};

/**
 * @brief 待处理的层更新操作
 */
struct PendingLayerUpdate {
    LayerUpdateType type;                                   ///< 操作类型
    RenderObject* target = nullptr;                         ///< 目标 RenderObject
    LayerPromotionReason reason = LayerPromotionReason::None;  ///< 层提升原因
    int z_index = 0;                                        ///< z-index 值
    std::string debug_info;                                 ///< 调试信息
};

/**
 * @brief 滚动状态（单一数据源）
 *
 * 所有滚动相关的状态都存储在这里，其他组件只读访问。
 */
struct ScrollState {
    float scroll_x = 0.0f;          ///< 当前 X 滚动位置
    float scroll_y = 0.0f;          ///< 当前 Y 滚动位置
    float max_scroll_x = 0.0f;      ///< 最大 X 滚动位置
    float max_scroll_y = 0.0f;      ///< 最大 Y 滚动位置
    float content_width = 0.0f;     ///< 内容宽度
    float content_height = 0.0f;    ///< 内容高度
    float viewport_width = 0.0f;    ///< 视口宽度
    float viewport_height = 0.0f;   ///< 视口高度
    uint64_t version = 0;           ///< 版本号，用于检测变化
};

/**
 * @brief 坐标空间类型
 */
enum class CoordinateSpace {
    Document,   ///< 文档坐标（相对于文档左上角）
    Viewport,   ///< 视口坐标（相对于可见区域左上角）
    Layer       ///< 层坐标（相对于层左上角）
};

/**
 * @brief 滚动变化监听器类型
 */
using ScrollListener = std::function<void(RenderObject*, const ScrollState&)>;

/**
 * @brief 获取层更新类型的字符串描述
 */
inline const char* LayerUpdateTypeToString(LayerUpdateType type) {
    switch (type) {
        case LayerUpdateType::Add: return "Add";
        case LayerUpdateType::Remove: return "Remove";
        case LayerUpdateType::UpdateBounds: return "UpdateBounds";
        case LayerUpdateType::Reparent: return "Reparent";
        case LayerUpdateType::UpdateZIndex: return "UpdateZIndex";
        default: return "Unknown";
    }
}

} // namespace lightui
