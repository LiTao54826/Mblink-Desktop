/**
 * @file devtools_state.cpp
 * @brief DevTools 状态持久化实现
 */

#include "devtools_state.h"
#include <sstream>
#include <fstream>

namespace lightui {

std::string DevToolsState::ToJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"is_open\": " << (is_open ? "true" : "false") << ",\n";
    oss << "  \"dock_position\": " << static_cast<int>(dock_position) << ",\n";
    oss << "  \"panel_size\": " << panel_size << ",\n";
    oss << "  \"active_styles_tab\": " << active_styles_tab << ",\n";
    oss << "  \"splitter_position\": " << splitter_position << ",\n";
    
    // 展开的节点 ID
    oss << "  \"expanded_node_ids\": [";
    for (size_t i = 0; i < expanded_node_ids.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << expanded_node_ids[i] << "\"";
    }
    oss << "],\n";
    
    // 选中的元素路径
    oss << "  \"selected_element_path\": \"" << selected_element_path << "\"\n";
    
    oss << "}";
    return oss.str();
}

DevToolsState DevToolsState::FromJSON(const std::string& json) {
    DevToolsState state;
    
    if (json.empty()) return state;
    
    // 简单的 JSON 解析（不使用外部库）
    auto find_value = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        
        pos += search.length();
        
        // 跳过空白
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
            pos++;
        }
        
        if (pos >= json.length()) return "";
        
        // 检查值类型
        if (json[pos] == '"') {
            // 字符串值
            size_t start = pos + 1;
            size_t end = json.find('"', start);
            if (end != std::string::npos) {
                return json.substr(start, end - start);
            }
        } else if (json[pos] == '[') {
            // 数组值
            size_t start = pos;
            size_t end = json.find(']', start);
            if (end != std::string::npos) {
                return json.substr(start, end - start + 1);
            }
        } else {
            // 数字或布尔值
            size_t start = pos;
            size_t end = start;
            while (end < json.length() && json[end] != ',' && json[end] != '\n' && json[end] != '}') {
                end++;
            }
            std::string value = json.substr(start, end - start);
            // 去除尾部空白
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                value.pop_back();
            }
            return value;
        }
        
        return "";
    };
    
    // 解析各字段
    std::string is_open_str = find_value("is_open");
    if (is_open_str == "true") state.is_open = true;
    else if (is_open_str == "false") state.is_open = false;
    
    std::string dock_pos_str = find_value("dock_position");
    if (!dock_pos_str.empty()) {
        int dock_pos = std::stoi(dock_pos_str);
        state.dock_position = static_cast<DockPosition>(dock_pos);
    }
    
    std::string panel_size_str = find_value("panel_size");
    if (!panel_size_str.empty()) {
        state.panel_size = std::stof(panel_size_str);
    }
    
    std::string active_tab_str = find_value("active_styles_tab");
    if (!active_tab_str.empty()) {
        state.active_styles_tab = std::stoi(active_tab_str);
    }
    
    std::string splitter_str = find_value("splitter_position");
    if (!splitter_str.empty()) {
        state.splitter_position = std::stof(splitter_str);
    }
    
    state.selected_element_path = find_value("selected_element_path");
    
    // 解析展开的节点 ID 数组
    std::string expanded_str = find_value("expanded_node_ids");
    if (!expanded_str.empty() && expanded_str[0] == '[') {
        size_t pos = 1;
        while (pos < expanded_str.length()) {
            size_t quote_start = expanded_str.find('"', pos);
            if (quote_start == std::string::npos) break;
            
            size_t quote_end = expanded_str.find('"', quote_start + 1);
            if (quote_end == std::string::npos) break;
            
            state.expanded_node_ids.push_back(
                expanded_str.substr(quote_start + 1, quote_end - quote_start - 1)
            );
            
            pos = quote_end + 1;
        }
    }
    
    return state;
}

} // namespace lightui
