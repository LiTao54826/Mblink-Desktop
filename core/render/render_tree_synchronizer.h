/**
 * @file render_tree_synchronizer.h
 * @brief 渲染树同步器
 * 
 * 功能：
 * - 在渲染前将 DOM 变化同步到渲染树
 * - 处理所有待处理的变化，确保渲染树与 DOM 树一致
 * - 支持增量更新和子树重建两种策略
 * 
 * 设计参考：
 * - Chromium Blink 的延迟同步机制
 * - 解决 ReplaceChild 时序问题
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace lightui {

// 前向声明
class Node;
class Element;
class RenderObject;
class Document;
class DirtyNodeTracker;
class RenderTreeBuilder;
class LayoutEngine;

/**
 * @brief 渲染树同步器
 * 
 * 在渲染前将 DOM 变化同步到渲染树。
 * 处理所有待处理的变化，确保渲染树与 DOM 树一致。
 */
class RenderTreeSynchronizer {
public:
    RenderTreeSynchronizer();
    ~RenderTreeSynchronizer();
    
    // 禁止拷贝
    RenderTreeSynchronizer(const RenderTreeSynchronizer&) = delete;
    RenderTreeSynchronizer& operator=(const RenderTreeSynchronizer&) = delete;
    
    // ========== 配置 ==========
    
    /**
     * @brief 设置文档（用于样式解析）
     * @param doc 文档对象
     */
    void SetDocument(std::shared_ptr<Document> doc);
    
    /**
     * @brief 设置布局引擎（用于布局树更新）
     * @param engine 布局引擎
     */
    void SetLayoutEngine(std::shared_ptr<LayoutEngine> engine);
    
    /**
     * @brief 设置渲染树构建器（用于创建渲染对象）
     * @param builder 渲染树构建器
     */
    void SetRenderTreeBuilder(std::shared_ptr<RenderTreeBuilder> builder);
    
    // ========== 同步 ==========
    
    /**
     * @brief 同步渲染树
     * 
     * 处理所有待处理的 DOM 变化，更新渲染树。
     * 这是一个原子操作，要么全部成功，要么回滚。
     * 
     * @param tracker 脏节点追踪器
     * @param render_tree 渲染树根节点
     * @return 是否有变化被应用
     */
    bool Synchronize(DirtyNodeTracker& tracker, 
                     std::shared_ptr<RenderObject> render_tree);
    
    // ========== 策略配置 ==========
    
    /**
     * @brief 设置触发子树重建的变化数量阈值
     * @param threshold 阈值（默认 10）
     */
    void SetRebuildThreshold(size_t threshold) { rebuild_threshold_ = threshold; }
    
    /**
     * @brief 设置触发子树重建的被替换节点子节点数阈值
     * @param threshold 阈值（默认 5）
     */
    void SetReplacedChildrenThreshold(size_t threshold) { replaced_children_threshold_ = threshold; }
    
    /**
     * @brief 设置触发子树重建的同一父节点变化数阈值
     * @param threshold 阈值（默认 3）
     */
    void SetParentChangesThreshold(size_t threshold) { parent_changes_threshold_ = threshold; }
    
private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 处理结构变化
     * @param tracker 脏节点追踪器
     */
    void ProcessStructuralChanges(DirtyNodeTracker& tracker);
    
    /**
     * @brief 处理样式变化
     * @param tracker 脏节点追踪器
     */
    void ProcessStyleChanges(DirtyNodeTracker& tracker);
    
    /**
     * @brief 处理文本变化
     * @param tracker 脏节点追踪器
     */
    void ProcessTextChanges(DirtyNodeTracker& tracker);
    
    /**
     * @brief 判断是否需要子树重建
     * @param tracker 脏节点追踪器
     * @return true 表示需要子树重建
     */
    bool NeedsSubtreeRebuild(const DirtyNodeTracker& tracker) const;
    
    /**
     * @brief 重建子树
     * @param root 子树根节点
     */
    void RebuildSubtree(Node* root);
    
    /**
     * @brief 增量更新单个节点
     * @param node DOM 节点
     */
    void UpdateNode(Node* node);
    
    // ========== 渲染对象操作 ==========
    
    /**
     * @brief 插入渲染对象
     * @param node DOM 节点
     * @param parent 父 DOM 节点
     * @param index 在父节点中的索引
     * @return 创建的渲染对象
     */
    std::shared_ptr<RenderObject> InsertRenderObject(Node* node, Node* parent, size_t index);
    
    /**
     * @brief 移除渲染对象
     * @param node DOM 节点
     */
    void RemoveRenderObject(Node* node);
    
    /**
     * @brief 替换渲染对象（原子操作）
     * @param old_node 旧 DOM 节点
     * @param new_node 新 DOM 节点
     * @param parent 父 DOM 节点
     * @param index 在父节点中的索引
     */
    void ReplaceRenderObject(Node* old_node, Node* new_node, Node* parent, size_t index);
    
    /**
     * @brief 创建单个节点的渲染对象
     * @param node DOM 节点
     * @return 创建的渲染对象，如果 display: none 则返回 nullptr
     */
    std::shared_ptr<RenderObject> CreateRenderObjectForNode(Node* node);
    
    /**
     * @brief 递归创建子树的渲染对象
     * @param node 子树根节点
     * @param parent_ro 父渲染对象
     */
    void CreateRenderSubtree(Node* node, RenderObject* parent_ro);
    
    /**
     * @brief 查找渲染对象在父节点中的插入位置
     * @param parent_ro 父渲染对象
     * @param dom_index DOM 节点在父节点中的索引
     * @param parent_node 父 DOM 节点
     * @return 插入位置索引
     */
    size_t FindInsertPosition(RenderObject* parent_ro, size_t dom_index, Node* parent_node);
    
    /**
     * @brief 使祖先链的布局失效
     * @param obj 起始渲染对象
     */
    void InvalidateAncestorLayout(RenderObject* obj);
    
private:
    /// 文档
    std::weak_ptr<Document> document_;
    
    /// 布局引擎
    std::weak_ptr<LayoutEngine> layout_engine_;
    
    /// 渲染树构建器
    std::weak_ptr<RenderTreeBuilder> render_tree_builder_;
    
    /// 渲染树根节点（同步期间使用）
    std::shared_ptr<RenderObject> render_tree_;
    
    // ========== 策略阈值 ==========
    
    /// 触发子树重建的变化数量阈值
    size_t rebuild_threshold_ = 10;
    
    /// 触发子树重建的被替换节点子节点数阈值
    size_t replaced_children_threshold_ = 5;
    
    /// 触发子树重建的同一父节点变化数阈值
    size_t parent_changes_threshold_ = 3;
};

} // namespace lightui
