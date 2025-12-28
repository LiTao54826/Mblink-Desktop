/**
 * @file property_trees.h
 * @brief 属性树集合
 *
 * 管理所有四棵属性树：
 * - TransformTree（变换树）
 * - ClipTree（裁剪树）
 * - EffectTree（效果树）
 * - ScrollTree（滚动树）
 *
 * 参考 Chromium Blink: cc/trees/property_tree.h
 */

#pragma once

#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include "core/compositor/property_tree/nodes/clip_tree_node.h"
#include "core/compositor/property_tree/nodes/effect_tree_node.h"
#include "core/compositor/property_tree/nodes/scroll_tree_node.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 单棵属性树的模板类
 * @tparam NodeType 节点类型
 */
template <typename NodeType>
class PropertyTree {
public:
    PropertyTree() {
        // 创建根节点
        root_ = CreateNode(nullptr);
    }
    
    ~PropertyTree() = default;

    // 禁止拷贝
    PropertyTree(const PropertyTree&) = delete;
    PropertyTree& operator=(const PropertyTree&) = delete;

    // =========================================================================
    // 节点管理
    // =========================================================================

    /**
     * @brief 创建新节点
     * @param parent 父节点，nullptr 表示根节点
     * @return 新创建的节点
     */
    NodeType* CreateNode(NodeType* parent = nullptr) {
        auto node = std::make_unique<NodeType>();
        node->SetId(next_id_++);
        
        if (parent) {
            node->SetParent(parent);
        }
        
        NodeType* raw_ptr = node.get();
        id_to_node_[raw_ptr->GetId()] = raw_ptr;
        nodes_.push_back(std::move(node));
        
        return raw_ptr;
    }

    /**
     * @brief 删除节点
     * @param node 要删除的节点
     * @note 会同时删除所有子节点
     */
    void RemoveNode(NodeType* node) {
        if (!node || node == root_) return;
        
        // 递归删除子节点
        auto children = node->GetChildren();  // 拷贝，因为删除会修改
        for (NodeType* child : children) {
            RemoveNode(child);
        }
        
        // 从父节点移除
        node->SetParent(nullptr);
        
        // 从映射中移除
        id_to_node_.erase(node->GetId());
        if (node->GetRenderObject()) {
            render_object_to_node_.erase(node->GetRenderObject());
        }
        
        // 从节点列表中移除
        nodes_.erase(
            std::remove_if(nodes_.begin(), nodes_.end(),
                [node](const std::unique_ptr<NodeType>& n) {
                    return n.get() == node;
                }),
            nodes_.end()
        );
    }

    /**
     * @brief 获取根节点
     */
    NodeType* GetRoot() const { return root_; }

    /**
     * @brief 通过 ID 查找节点
     */
    NodeType* GetNodeById(PropertyTreeNodeId id) const {
        auto it = id_to_node_.find(id);
        return it != id_to_node_.end() ? it->second : nullptr;
    }

    /**
     * @brief 通过 RenderObject 查找节点
     */
    NodeType* GetNodeForRenderObject(RenderObject* obj) const {
        auto it = render_object_to_node_.find(obj);
        return it != render_object_to_node_.end() ? it->second : nullptr;
    }

    /**
     * @brief 注册 RenderObject 到节点的映射
     */
    void RegisterRenderObject(RenderObject* obj, NodeType* node) {
        if (obj && node) {
            render_object_to_node_[obj] = node;
            node->SetRenderObject(obj);
        }
    }

    /**
     * @brief 取消注册 RenderObject
     */
    void UnregisterRenderObject(RenderObject* obj) {
        auto it = render_object_to_node_.find(obj);
        if (it != render_object_to_node_.end()) {
            it->second->SetRenderObject(nullptr);
            render_object_to_node_.erase(it);
        }
    }

    /**
     * @brief 获取节点数量
     */
    size_t GetNodeCount() const { return nodes_.size(); }

    /**
     * @brief 清除所有节点（保留根节点）
     */
    void Clear() {
        // 保留根节点
        auto root_node = std::move(nodes_[0]);
        nodes_.clear();
        id_to_node_.clear();
        render_object_to_node_.clear();
        
        // 重新添加根节点
        root_ = root_node.get();
        id_to_node_[root_->GetId()] = root_;
        nodes_.push_back(std::move(root_node));
        
        // 清除根节点的子节点引用
        // 注意：子节点已经被删除，但根节点可能还持有指针
        // 这里需要手动清理（通过重新创建根节点或清理其内部状态）
    }

    /**
     * @brief 检查是否有脏节点
     */
    bool HasDirtyNodes() const {
        for (const auto& node : nodes_) {
            if (node->IsDirty()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 清除所有脏标记
     */
    void ClearAllDirtyFlags() {
        for (auto& node : nodes_) {
            node->ClearDirty();
        }
    }

private:
    NodeType* root_ = nullptr;
    std::vector<std::unique_ptr<NodeType>> nodes_;
    std::unordered_map<PropertyTreeNodeId, NodeType*> id_to_node_;
    std::unordered_map<RenderObject*, NodeType*> render_object_to_node_;
    PropertyTreeNodeId next_id_ = 1;
};

// 类型别名
using TransformTree = PropertyTree<TransformTreeNode>;
using ClipTree = PropertyTree<ClipTreeNode>;
using EffectTree = PropertyTree<EffectTreeNode>;
using ScrollTree = PropertyTree<ScrollTreeNode>;

/**
 * @brief 属性树集合
 *
 * 管理所有四棵属性树，提供统一的访问接口。
 */
class PropertyTrees {
public:
    PropertyTrees();
    ~PropertyTrees();

    // 禁止拷贝
    PropertyTrees(const PropertyTrees&) = delete;
    PropertyTrees& operator=(const PropertyTrees&) = delete;

    // =========================================================================
    // 树访问
    // =========================================================================

    /**
     * @brief 获取变换树
     */
    TransformTree& GetTransformTree() { return transform_tree_; }
    const TransformTree& GetTransformTree() const { return transform_tree_; }

    /**
     * @brief 获取裁剪树
     */
    ClipTree& GetClipTree() { return clip_tree_; }
    const ClipTree& GetClipTree() const { return clip_tree_; }

    /**
     * @brief 获取效果树
     */
    EffectTree& GetEffectTree() { return effect_tree_; }
    const EffectTree& GetEffectTree() const { return effect_tree_; }

    /**
     * @brief 获取滚动树
     */
    ScrollTree& GetScrollTree() { return scroll_tree_; }
    const ScrollTree& GetScrollTree() const { return scroll_tree_; }

    // =========================================================================
    // 根状态
    // =========================================================================

    /**
     * @brief 获取根状态
     */
    PropertyTreeState GetRootState() const;

    // =========================================================================
    // 版本管理
    // =========================================================================

    /**
     * @brief 获取版本号
     */
    uint64_t GetVersion() const { return version_; }

    /**
     * @brief 递增版本号
     */
    void IncrementVersion() { ++version_; }

    // =========================================================================
    // 脏标记管理
    // =========================================================================

    /**
     * @brief 检查是否有脏节点
     */
    bool HasDirtyNodes() const;

    /**
     * @brief 清除所有脏标记
     */
    void ClearAllDirtyFlags();

    // =========================================================================
    // 清理
    // =========================================================================

    /**
     * @brief 清除所有树（保留根节点）
     */
    void Clear();

private:
    TransformTree transform_tree_;
    ClipTree clip_tree_;
    EffectTree effect_tree_;
    ScrollTree scroll_tree_;
    uint64_t version_ = 0;
};

} // namespace lightui
