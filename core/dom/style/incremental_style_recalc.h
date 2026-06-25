/**
 * @file incremental_style_recalc.h
 * @brief 增量样式重算 - 只处理脏节点的样式更新
 * 
 * 功能：
 * - 遍历 DOM 树，只访问有 NeedsStyleRecalc 或 ChildNeedsStyleRecalc 标志的节点
 * - 跳过干净的子树
 * - 根据 StyleChangeType 决定处理范围（LocalStyleChange vs SubtreeStyleChange）
 * - 跟踪统计信息用于性能分析
 * 
 * 参考：Chromium Blink 引擎的增量样式重算机制
 */

#pragma once

#include <memory>

namespace mblink {

// 前向声明
class Document;
class Node;
class Element;
struct ComputedStyle;
class StyleResolver;

/**
 * @brief 增量样式重算统计信息
 */
struct IncrementalStyleRecalcStats {
    int nodes_visited = 0;        // 访问的节点数
    int nodes_recalculated = 0;   // 重新计算样式的节点数
    int subtrees_skipped = 0;     // 跳过的干净子树数
    
    void Reset() {
        nodes_visited = 0;
        nodes_recalculated = 0;
        subtrees_skipped = 0;
    }
};

/**
 * @brief 增量样式重算器
 * 
 * 只处理标记为脏的节点，跳过干净的子树，提高样式重算效率。
 */
class IncrementalStyleRecalc {
public:
    /**
     * @brief 构造函数
     */
    IncrementalStyleRecalc();
    
    /**
     * @brief 析构函数
     */
    ~IncrementalStyleRecalc() = default;
    
    /**
     * @brief 执行增量样式重算
     * @param document 文档对象
     * 
     * 从文档根开始遍历，只访问有脏标志的节点。
     * 处理完成后清除所有样式重算标志。
     */
    void RecalcStyle(Document* document);
    
    /**
     * @brief 获取访问的节点数
     */
    int GetNodesVisited() const { return stats_.nodes_visited; }
    
    /**
     * @brief 获取重新计算样式的节点数
     */
    int GetNodesRecalculated() const { return stats_.nodes_recalculated; }
    
    /**
     * @brief 获取跳过的子树数
     */
    int GetSubtreesSkipped() const { return stats_.subtrees_skipped; }
    
    /**
     * @brief 获取统计信息
     */
    const IncrementalStyleRecalcStats& GetStats() const { return stats_; }
    
    /**
     * @brief 重置统计信息
     */
    void ResetStats() { stats_.Reset(); }

private:
    /**
     * @brief 递归处理节点的样式重算
     * @param node 当前节点
     * @param parent_style 父节点的计算样式（用于继承）
     * @param force_recalc 是否强制重算（用于 SubtreeStyleChange）
     */
    void RecalcStyleForNode(std::shared_ptr<Node> node, 
                            const ComputedStyle* parent_style,
                            bool force_recalc);
    
    /**
     * @brief 处理元素节点的样式重算
     * @param element 元素节点
     * @param parent_style 父节点的计算样式
     * @param force_recalc 是否强制重算
     */
    void RecalcStyleForElement(std::shared_ptr<Element> element,
                               const ComputedStyle* parent_style,
                               bool force_recalc);
    
    /**
     * @brief 处理子节点
     * @param node 父节点
     * @param parent_style 父节点的计算样式
     * @param force_recalc 是否强制重算子树
     */
    void RecalcStyleForChildren(std::shared_ptr<Node> node,
                                const ComputedStyle* parent_style,
                                bool force_recalc);

private:
    IncrementalStyleRecalcStats stats_;
    Document* current_document_ = nullptr;
};

} // namespace mblink
