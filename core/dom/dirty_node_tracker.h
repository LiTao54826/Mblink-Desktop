/**
 * @file dirty_node_tracker.h
 * @brief 脏节点追踪器
 * 
 * 功能：
 * - 收集 DOM 变化，延迟到渲染前统一处理
 * - 避免中间状态不一致的问题
 * - 支持变化合并优化
 * 
 * 设计参考：
 * - Chromium Blink 的延迟同步机制
 * - React Fiber 的批量更新
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace lightui {

// 前向声明
class Node;
class Element;

/**
 * @brief 脏节点追踪器
 * 
 * 收集 DOM 变化，延迟到渲染前统一处理。
 * 这避免了中间状态不一致的问题，特别是 ReplaceChild 的时序问题。
 */
class DirtyNodeTracker {
public:
    /**
     * @brief 结构变化类型
     */
    enum class StructuralChangeType {
        Added,      ///< 节点被添加
        Removed,    ///< 节点被移除
        Moved,      ///< 节点被移动（从一个父节点移到另一个）
        Replaced,   ///< 节点被替换（ReplaceChild 的原子操作）
    };
    
    /**
     * @brief 结构变化记录
     */
    struct StructuralChange {
        StructuralChangeType type;          ///< 变化类型
        std::weak_ptr<Node> node;           ///< 变化的节点（Added/Removed/Moved 时使用）
        std::weak_ptr<Node> parent;         ///< 父节点
        std::weak_ptr<Node> old_parent;     ///< 旧父节点（Moved 时使用）
        std::weak_ptr<Node> new_node;       ///< 新节点（Replaced 时使用）
        std::weak_ptr<Node> old_node;       ///< 旧节点（Replaced 时使用）
        size_t index;                       ///< 在父节点中的索引
        
        StructuralChange() : type(StructuralChangeType::Added), index(0) {}
    };
    
    /**
     * @brief 样式变化记录
     */
    struct StyleChange {
        std::weak_ptr<Element> element;     ///< 变化的元素
        std::string property;               ///< 样式属性名
        std::string old_value;              ///< 旧值
        std::string new_value;              ///< 新值
    };
    
    /**
     * @brief 文本变化记录
     */
    struct TextChange {
        std::weak_ptr<Node> node;           ///< 变化的文本节点
        std::string old_text;               ///< 旧文本
        std::string new_text;               ///< 新文本
    };
    
public:
    DirtyNodeTracker() = default;
    ~DirtyNodeTracker() = default;
    
    // 禁止拷贝
    DirtyNodeTracker(const DirtyNodeTracker&) = delete;
    DirtyNodeTracker& operator=(const DirtyNodeTracker&) = delete;
    
    // ========== 记录变化 ==========
    
    /**
     * @brief 记录节点添加
     * @param node 被添加的节点
     * @param parent 父节点
     * @param index 在父节点中的索引
     */
    void RecordNodeAdded(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, size_t index);
    
    /**
     * @brief 记录节点移除
     * @param node 被移除的节点
     * @param parent 原父节点
     * @param index 在父节点中的原索引
     */
    void RecordNodeRemoved(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, size_t index);
    
    /**
     * @brief 记录节点替换（原子操作）
     * @param old_node 被替换的旧节点
     * @param new_node 替换的新节点
     * @param parent 父节点
     * @param index 在父节点中的索引
     */
    void RecordNodeReplaced(std::shared_ptr<Node> old_node, 
                           std::shared_ptr<Node> new_node, 
                           std::shared_ptr<Node> parent, 
                           size_t index);
    
    /**
     * @brief 记录节点移动
     * @param node 被移动的节点
     * @param old_parent 旧父节点
     * @param new_parent 新父节点
     * @param new_index 在新父节点中的索引
     */
    void RecordNodeMoved(std::shared_ptr<Node> node,
                        std::shared_ptr<Node> old_parent,
                        std::shared_ptr<Node> new_parent,
                        size_t new_index);
    
    /**
     * @brief 记录样式变化
     * @param element 变化的元素
     * @param property 样式属性名
     * @param old_value 旧值
     * @param new_value 新值
     */
    void RecordStyleChanged(std::shared_ptr<Element> element,
                           const std::string& property,
                           const std::string& old_value,
                           const std::string& new_value);
    
    /**
     * @brief 记录文本变化
     * @param node 变化的文本节点
     * @param old_text 旧文本
     * @param new_text 新文本
     */
    void RecordTextChanged(std::shared_ptr<Node> node,
                          const std::string& old_text,
                          const std::string& new_text);
    
    // ========== 查询 ==========
    
    /**
     * @brief 检查是否有待处理的变化
     * @return true 表示有待处理的变化
     */
    bool HasPendingChanges() const;
    
    /**
     * @brief 获取待处理的结构变化
     * @return 结构变化列表
     */
    const std::vector<StructuralChange>& GetStructuralChanges() const { return structural_changes_; }
    
    /**
     * @brief 获取待处理的样式变化
     * @return 样式变化列表
     */
    const std::vector<StyleChange>& GetStyleChanges() const { return style_changes_; }
    
    /**
     * @brief 获取待处理的文本变化
     * @return 文本变化列表
     */
    const std::vector<TextChange>& GetTextChanges() const { return text_changes_; }
    
    /**
     * @brief 获取结构变化数量
     */
    size_t GetStructuralChangeCount() const { return structural_changes_.size(); }
    
    /**
     * @brief 获取样式变化数量
     */
    size_t GetStyleChangeCount() const { return style_changes_.size(); }
    
    /**
     * @brief 获取文本变化数量
     */
    size_t GetTextChangeCount() const { return text_changes_.size(); }
    
    // ========== 管理 ==========
    
    /**
     * @brief 清除所有待处理的变化
     */
    void Clear();
    
    /**
     * @brief 优化变化列表
     * 
     * 合并冗余变化，例如：
     * - 添加后又移除同一节点 -> 取消两个操作
     * - 同一元素的多次样式变化 -> 只保留最终值
     */
    void Optimize();
    
private:
    /// 待处理的结构变化
    std::vector<StructuralChange> structural_changes_;
    
    /// 待处理的样式变化
    std::vector<StyleChange> style_changes_;
    
    /// 待处理的文本变化
    std::vector<TextChange> text_changes_;
    
    /// 已添加的节点集合（用于优化）
    std::unordered_set<Node*> added_nodes_;
    
    /// 已移除的节点集合（用于优化）
    std::unordered_set<Node*> removed_nodes_;
};

} // namespace lightui
