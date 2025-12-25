/**
 * @file display_item.cpp
 * @brief 显示项实现
 */

#include "core/compositor/property_tree/display_item.h"

namespace lightui {

// =========================================================================
// 构造函数
// =========================================================================

DisplayItem::DisplayItem(DisplayItemType type, RenderObject* client)
    : type_(type)
    , client_(client) {
}

} // namespace lightui
