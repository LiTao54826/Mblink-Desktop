/**
 * @file devtools_state.h
 * @brief DevTools 状态持久化数据结构
 */

#pragma once

#include <string>
#include <vector>
#include "devtools_manager.h"

namespace lightui {

/**
 * @brief DevTools 状态数据
 */
struct DevToolsState {
    bool is_open = false;
    DockPosition dock_position = DockPosition::Bottom;
    float panel_size = 0.3f;

    // 展开的节点 ID 列表
    std::vector<std::string> expanded_node_ids;

    // 上次选中的元素路径（CSS 选择器或 XPath）
    std::string selected_element_path;

    // 活动的样式面板 Tab
    int active_styles_tab = 0;  // 0=InlineStyles, 1=ComputedStyles, 2=BoxModel

    // 分隔条位置
    float splitter_position = 0.5f;

    /**
     * @brief 序列化为 JSON 字符串
     */
    std::string ToJSON() const;

    /**
     * @brief 从 JSON 字符串反序列化
     */
    static DevToolsState FromJSON(const std::string& json);
};

} // namespace lightui
