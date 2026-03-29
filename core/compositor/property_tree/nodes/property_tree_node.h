/**
 * @file property_tree_node.h
 * @brief 属性树节点基类模板
 *
 * 属性树节点是属性树系统的基础组件，提供：
 * - 节点 ID 管理
 * - 父子关系维护
 * - 脏标记传播
 * - 版本号追踪（用于缓存失效）
 * - 祖先查找和公共祖先计算
 *
 * 参考 Chromium Blink: cc/trees/property_tree.h
 */

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

namespace mbink {

// 属性树节点 ID 类型
using PropertyTreeNodeId = uint32_t;

// 无效节点 ID
constexpr PropertyTreeNodeId kInvalidNodeId = 0;

/**
 * @brief 属性树节点基类模板
 * @tparam NodeType 具体的节点类型（CRTP 模式）
 *
 * 使用 CRTP（Curiously Recurring Template Pattern）模式，
 * 允许派生类在基类方法中返回正确的类型。
 */
template <typename NodeType>
class PropertyTreeNode {
public:
    PropertyTreeNode() = default;
    virtual ~PropertyTreeNode() = default;

    // 禁止拷贝，允许移动
    PropertyTreeNode(const PropertyTreeNode&) = delete;
    PropertyTreeNode& operator=(const PropertyTreeNode&) = delete;
    PropertyTreeNode(PropertyTreeNode&&) = default;
    PropertyTreeNode& operator=(PropertyTreeNode&&) = default;

    // =========================================================================
    // 节点标识
    // =========================================================================

    /**
     * @brief 获取节点 ID
     */
    PropertyTreeNodeId GetId() const { return id_; }

    /**
     * @brief 设置节点 ID（由 PropertyTree 管理）
     */
    void SetId(PropertyTreeNodeId id) { id_ = id; }

    // =========================================================================
    // 父子关系
    // =========================================================================

    /**
     * @brief 获取父节点
     */
    NodeType* GetParent() const { return parent_; }

    /**
     * @brief 设置父节点
     */
    void SetParent(NodeType* parent) {
        if (parent_ == parent) return;
        
        // 从旧父节点移除
        if (parent_) {
            auto& siblings = parent_->children_;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), static_cast<NodeType*>(this)),
                siblings.end()
            );
        }
        
        parent_ = parent;
        
        // 添加到新父节点
        if (parent_) {
            parent_->children_.push_back(static_cast<NodeType*>(this));
        }
        
        MarkDirty();
    }

    /**
     * @brief 获取子节点列表
     */
    const std::vector<NodeType*>& GetChildren() const { return children_; }

    /**
     * @brief 是否是根节点
     */
    bool IsRoot() const { return parent_ == nullptr; }

    /**
     * @brief 是否是叶子节点
     */
    bool IsLeaf() const { return children_.empty(); }

    /**
     * @brief 获取节点深度（根节点深度为 0）
     */
    int GetDepth() const {
        int depth = 0;
        const NodeType* node = static_cast<const NodeType*>(this);
        while (node->parent_) {
            ++depth;
            node = node->parent_;
        }
        return depth;
    }

    // =========================================================================
    // 祖先查找
    // =========================================================================

    /**
     * @brief 获取从当前节点到根节点的路径
     * @return 节点指针列表，从当前节点开始，到根节点结束
     */
    std::vector<NodeType*> GetPathToRoot() const {
        std::vector<NodeType*> path;
        NodeType* node = const_cast<NodeType*>(static_cast<const NodeType*>(this));
        while (node) {
            path.push_back(node);
            node = node->parent_;
        }
        return path;
    }

    /**
     * @brief 查找与另一个节点的最近公共祖先
     * @param other 另一个节点
     * @return 最近公共祖先，如果不存在则返回 nullptr
     *
     * 算法：
     * 1. 获取两个节点到根的路径
     * 2. 从根向下遍历，找到最后一个相同的节点
     */
    NodeType* FindCommonAncestor(const NodeType* other) const {
        if (!other) return nullptr;
        
        // 获取两条路径
        std::vector<NodeType*> path1 = GetPathToRoot();
        std::vector<NodeType*> path2 = other->GetPathToRoot();
        
        // 从根向下比较（路径是从叶到根，所以从后向前）
        NodeType* common = nullptr;
        auto it1 = path1.rbegin();
        auto it2 = path2.rbegin();
        
        while (it1 != path1.rend() && it2 != path2.rend()) {
            if (*it1 == *it2) {
                common = *it1;
                ++it1;
                ++it2;
            } else {
                break;
            }
        }
        
        return common;
    }

    /**
     * @brief 检查是否是另一个节点的祖先
     */
    bool IsAncestorOf(const NodeType* other) const {
        if (!other) return false;
        
        const NodeType* node = other->parent_;
        while (node) {
            if (node == static_cast<const NodeType*>(this)) {
                return true;
            }
            node = node->parent_;
        }
        return false;
    }

    /**
     * @brief 检查是否是另一个节点的后代
     */
    bool IsDescendantOf(const NodeType* other) const {
        return other && other->IsAncestorOf(static_cast<const NodeType*>(this));
    }

    // =========================================================================
    // 脏标记
    // =========================================================================

    /**
     * @brief 检查节点是否脏
     */
    bool IsDirty() const { return dirty_; }

    /**
     * @brief 标记节点为脏
     * @note 会递增版本号
     */
    void MarkDirty() {
        if (!dirty_) {
            dirty_ = true;
            ++version_;
        }
    }

    /**
     * @brief 清除脏标记
     */
    void ClearDirty() { dirty_ = false; }

    /**
     * @brief 标记节点及其所有后代为脏
     */
    void MarkSubtreeDirty() {
        MarkDirty();
        for (NodeType* child : children_) {
            child->MarkSubtreeDirty();
        }
    }

    // =========================================================================
    // 版本号
    // =========================================================================

    /**
     * @brief 获取版本号
     * @note 版本号在每次 MarkDirty 时递增，用于缓存失效检测
     */
    uint64_t GetVersion() const { return version_; }

    // =========================================================================
    // 遍历
    // =========================================================================

    /**
     * @brief 前序遍历（先访问父节点，再访问子节点）
     * @tparam Func 回调函数类型
     * @param func 对每个节点调用的函数
     */
    template <typename Func>
    void TraversePreOrder(Func&& func) {
        func(static_cast<NodeType*>(this));
        for (NodeType* child : children_) {
            child->TraversePreOrder(std::forward<Func>(func));
        }
    }

    /**
     * @brief 后序遍历（先访问子节点，再访问父节点）
     * @tparam Func 回调函数类型
     * @param func 对每个节点调用的函数
     */
    template <typename Func>
    void TraversePostOrder(Func&& func) {
        for (NodeType* child : children_) {
            child->TraversePostOrder(std::forward<Func>(func));
        }
        func(static_cast<NodeType*>(this));
    }

protected:
    // 允许派生类访问
    PropertyTreeNodeId id_ = kInvalidNodeId;
    NodeType* parent_ = nullptr;
    std::vector<NodeType*> children_;
    bool dirty_ = true;  // 新节点默认为脏
    uint64_t version_ = 0;
};

} // namespace mbink
