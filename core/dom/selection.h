/**
 * @file selection.h
 * @brief DOM Selection API 实现
 *
 * 功能：
 * - 表示用户的文本选择状态
 * - 支持获取和设置选择的锚点和焦点
 * - 支持选择操作（折叠、扩展、选择所有子节点）
 * - 支持 Range 管理
 *
 * 参考：https://w3c.github.io/selection-api/
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lightui {

// 前向声明
class Node;
class Document;
class Range;

/**
 * @brief DOM Selection 类
 *
 * Selection 表示用户选择的文本范围或光标位置。
 * 选择由锚点（anchor）和焦点（focus）定义：
 * - 锚点是选择开始的位置
 * - 焦点是选择结束的位置（用户拖动到的位置）
 */
class Selection : public std::enable_shared_from_this<Selection> {
public:
    /**
     * @brief 构造函数
     * @param document 所属文档
     */
    explicit Selection(std::shared_ptr<Document> document);

    /**
     * @brief 析构函数
     */
    ~Selection();

    // ========== 锚点和焦点属性 ==========

    /**
     * @brief 获取锚点节点
     * @return 选择开始的节点
     */
    std::shared_ptr<Node> GetAnchorNode() const { return anchor_node_.lock(); }

    /**
     * @brief 获取焦点节点
     * @return 选择结束的节点
     */
    std::shared_ptr<Node> GetFocusNode() const { return focus_node_.lock(); }

    /**
     * @brief 获取锚点偏移量
     * @return 锚点在节点内的偏移量
     */
    int GetAnchorOffset() const { return anchor_offset_; }

    /**
     * @brief 获取焦点偏移量
     * @return 焦点在节点内的偏移量
     */
    int GetFocusOffset() const { return focus_offset_; }

    /**
     * @brief 检查选择是否折叠（光标状态）
     * @return true 如果锚点和焦点在同一位置
     */
    bool IsCollapsed() const;

    /**
     * @brief 获取 Range 数量
     * @return Range 数量（通常为 0 或 1）
     */
    int GetRangeCount() const { return ranges_.empty() ? 0 : 1; }

    // ========== 选择操作 ==========

    /**
     * @brief 折叠选择到指定位置
     * @param node 目标节点
     * @param offset 偏移量
     *
     * 将锚点和焦点都移动到指定位置
     */
    void Collapse(std::shared_ptr<Node> node, int offset);

    /**
     * @brief 扩展选择到指定位置
     * @param node 目标节点
     * @param offset 偏移量
     *
     * 保持锚点不变，将焦点移动到指定位置
     */
    void Extend(std::shared_ptr<Node> node, int offset);

    /**
     * @brief 选择节点的所有子节点
     * @param node 目标节点
     */
    void SelectAllChildren(std::shared_ptr<Node> node);

    /**
     * @brief 折叠到选择的开始位置
     */
    void CollapseToStart();

    /**
     * @brief 折叠到选择的结束位置
     */
    void CollapseToEnd();

    // ========== Range 管理 ==========

    /**
     * @brief 移除所有 Range
     */
    void RemoveAllRanges();

    /**
     * @brief 添加 Range
     * @param range 要添加的 Range
     */
    void AddRange(std::shared_ptr<Range> range);

    /**
     * @brief 获取指定索引的 Range
     * @param index 索引
     * @return Range 对象，如果索引无效返回 nullptr
     */
    std::shared_ptr<Range> GetRangeAt(int index) const;

    // ========== 转换 ==========

    /**
     * @brief 获取选中的文本
     * @return 选中的文本内容
     */
    std::string ToString() const;

    // ========== 内部更新 ==========

    /**
     * @brief 从用户操作更新选择状态
     * @param anchor 锚点节点
     * @param anchor_offset 锚点偏移量
     * @param focus 焦点节点
     * @param focus_offset 焦点偏移量
     *
     * 由事件系统调用，用于响应用户的鼠标或键盘选择操作
     */
    void UpdateFromUserAction(
        std::shared_ptr<Node> anchor, int anchor_offset,
        std::shared_ptr<Node> focus, int focus_offset);

    // ========== 所属文档 ==========

    /**
     * @brief 获取所属文档
     * @return 所属文档
     */
    std::shared_ptr<Document> GetDocument() const { return document_.lock(); }

private:
    /**
     * @brief 从选择状态更新内部 Range
     */
    void UpdateRangeFromSelection();

private:
    std::weak_ptr<Document> document_;
    std::weak_ptr<Node> anchor_node_;
    std::weak_ptr<Node> focus_node_;
    int anchor_offset_ = 0;
    int focus_offset_ = 0;
    std::vector<std::shared_ptr<Range>> ranges_;
};

} // namespace lightui
