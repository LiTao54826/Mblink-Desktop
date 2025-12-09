/**
 * @file node.h
 * @brief DOM节点基类
 * 
 * 功能：
 * - DOM树的基础节点类
 * - 节点类型定义（Element, Text, Document等）
 * - 节点树操作（父子关系、兄弟关系）
 * - 节点遍历
 * 
 * 实现要点：
 * - 使用智能指针管理节点生命周期
 * - 支持节点类型判断和转换
 * - 实现DOM标准的节点操作
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "include/core/SkRect.h"

namespace lightui {

// 前向声明
class Element;
class Text;
class Document;
class LayoutBox;
class RenderObject;

/**
 * @brief 脏标记类型枚举
 * 用于精确标记节点的哪些部分需要更新
 */
enum class DirtyType : uint32_t {
    NONE = 0,
    LAYOUT = 1 << 0,    // 需要重新布局（位置、大小改变）
    PAINT = 1 << 1,     // 需要重新绘制（样式改变）
    STYLE = 1 << 2,     // 样式改变
    ALL = LAYOUT | PAINT | STYLE
};

// 位运算支持
inline DirtyType operator|(DirtyType a, DirtyType b) {
    return static_cast<DirtyType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline DirtyType operator&(DirtyType a, DirtyType b) {
    return static_cast<DirtyType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline DirtyType operator~(DirtyType a) {
    return static_cast<DirtyType>(~static_cast<uint32_t>(a));
}

inline uint32_t operator&(uint32_t a, DirtyType b) {
    return a & static_cast<uint32_t>(b);
}

inline uint32_t operator|(uint32_t a, DirtyType b) {
    return a | static_cast<uint32_t>(b);
}

/**
 * @brief 节点类型枚举
 */
enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    DOCUMENT_NODE = 9,
    DOCUMENT_FRAGMENT_NODE = 11
};

/**
 * @brief DOM节点基类
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    /**
     * @brief 构造函数
     * @param type 节点类型
     */
    explicit Node(NodeType type);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~Node() = default;
    
    /**
     * @brief 获取节点类型
     * @return 节点类型
     */
    NodeType GetNodeType() const { return node_type_; }
    
    /**
     * @brief 获取父节点
     * @return 父节点指针
     */
    std::shared_ptr<Node> GetParentNode() const { return parent_node_.lock(); }

    /**
     * @brief 获取所属文档
     * @return 文档指针，如果不属于任何文档则返回nullptr
     */
    std::shared_ptr<Document> GetOwnerDocument() const;

    /**
     * @brief 获取子节点列表
     * @return 子节点列表
     */
    const std::vector<std::shared_ptr<Node>>& GetChildNodes() const { return child_nodes_; }
    
    /**
     * @brief 获取第一个子节点
     * @return 第一个子节点指针，如果没有则返回nullptr
     */
    std::shared_ptr<Node> GetFirstChild() const;
    
    /**
     * @brief 获取最后一个子节点
     * @return 最后一个子节点指针，如果没有则返回nullptr
     */
    std::shared_ptr<Node> GetLastChild() const;
    
    /**
     * @brief 获取下一个兄弟节点
     * @return 下一个兄弟节点指针，如果没有则返回nullptr
     */
    std::shared_ptr<Node> GetNextSibling() const;
    
    /**
     * @brief 获取上一个兄弟节点
     * @return 上一个兄弟节点指针，如果没有则返回nullptr
     */
    std::shared_ptr<Node> GetPreviousSibling() const;
    
    /**
     * @brief 添加子节点
     * @param child 要添加的子节点
     * @return 添加的子节点
     */
    std::shared_ptr<Node> AppendChild(std::shared_ptr<Node> child);
    
    /**
     * @brief 在指定节点前插入子节点
     * @param new_child 要插入的新节点
     * @param ref_child 参考节点
     * @return 插入的节点
     */
    std::shared_ptr<Node> InsertBefore(std::shared_ptr<Node> new_child,
                                       std::shared_ptr<Node> ref_child);
    
    /**
     * @brief 移除子节点
     * @param child 要移除的子节点
     * @return 被移除的节点
     */
    std::shared_ptr<Node> RemoveChild(std::shared_ptr<Node> child);
    
    /**
     * @brief 替换子节点
     * @param new_child 新节点
     * @param old_child 旧节点
     * @return 被替换的旧节点
     */
    std::shared_ptr<Node> ReplaceChild(std::shared_ptr<Node> new_child,
                                       std::shared_ptr<Node> old_child);
    
    /**
     * @brief 克隆节点
     * @param deep 是否深度克隆（包括子节点）
     * @return 克隆的节点
     */
    virtual std::shared_ptr<Node> CloneNode(bool deep) = 0;
    
    /**
     * @brief 检查是否包含指定节点
     * @param other 要检查的节点
     * @return true表示包含
     */
    bool Contains(std::shared_ptr<Node> other) const;
    
    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    virtual std::string GetTextContent() const;
    
    /**
     * @brief 设置文本内容
     * @param content 文本内容
     */
    virtual void SetTextContent(const std::string& content);
    
    /**
     * @brief 标记节点为脏（需要重新布局/渲染）
     * @param type 脏标记类型，默认为ALL
     */
    void MarkDirty(DirtyType type = DirtyType::ALL);

    /**
     * @brief 检查节点是否为脏（兼容旧API）
     * @return true表示需要重新布局/渲染
     */
    bool IsDirty() const { return dirty_flags_ != 0; }

    /**
     * @brief 检查是否需要重新布局
     * @return true表示需要重新布局
     */
    bool IsLayoutDirty() const { return (dirty_flags_ & DirtyType::LAYOUT) != 0; }

    /**
     * @brief 检查是否需要重新绘制
     * @return true表示需要重新绘制
     */
    bool IsPaintDirty() const { return (dirty_flags_ & DirtyType::PAINT) != 0; }

    /**
     * @brief 检查是否有样式改变
     * @return true表示样式改变
     */
    bool IsStyleDirty() const { return (dirty_flags_ & DirtyType::STYLE) != 0; }

    /**
     * @brief 清除脏标记
     * @param type 要清除的脏标记类型，默认为ALL
     */
    void ClearDirty(DirtyType type = DirtyType::ALL);

    /**
     * @brief 获取脏矩形区域（在布局后计算）
     * @return 脏矩形区域
     */
    SkRect GetDirtyRect() const { return dirty_rect_; }

    /**
     * @brief 设置脏矩形区域
     * @param rect 脏矩形区域
     */
    void SetDirtyRect(const SkRect& rect) { dirty_rect_ = rect; }

    /**
     * @brief 获取脏标记标志
     * @return 脏标记标志
     */
    uint32_t GetDirtyFlags() const { return dirty_flags_; }

    // ========== RenderObject 双向绑定 ==========

    /**
     * @brief 设置关联的 RenderObject
     * @param render_obj 渲染对象
     */
    void SetRenderObject(std::shared_ptr<RenderObject> render_obj);

    /**
     * @brief 获取关联的 RenderObject
     * @return 渲染对象，如果没有则返回 nullptr
     */
    std::shared_ptr<RenderObject> GetRenderObject() const;

protected:
    /**
     * @brief 设置父节点
     * @param parent 父节点
     */
    void SetParentNode(std::shared_ptr<Node> parent);
    
    /**
     * @brief 移除所有子节点
     */
    void RemoveAllChildren();

protected:
    NodeType node_type_;
    std::weak_ptr<Node> parent_node_;
    std::weak_ptr<Document> owner_document_;  // 所属文档（弱引用避免循环引用）
    std::vector<std::shared_ptr<Node>> child_nodes_;

    // 脏标记系统
    bool is_dirty_ = true;  // 保留用于向后兼容
    uint32_t dirty_flags_ = static_cast<uint32_t>(DirtyType::ALL);  // 脏标记标志
    SkRect dirty_rect_ = SkRect::MakeEmpty();  // 脏矩形区域

    // RenderObject 双向绑定（使用 weak_ptr 避免循环引用）
    std::weak_ptr<RenderObject> render_object_;

    // 允许 Document 类访问 owner_document_
    friend class Document;
};

} // namespace lightui

