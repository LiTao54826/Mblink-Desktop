/**
 * @file selection.h
 * @brief DOM Selection API 实现
 *
 * 参考 Blink 实现和 W3C Selection API 规范
 * https://w3c.github.io/selection-api/
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
 * @brief 选择方向枚举
 */
enum class SelectionDirection {
    kNone,      // 无方向（折叠状态）
    kForward,   // 向前选择（anchor 在 focus 之前）
    kBackward   // 向后选择（anchor 在 focus 之后）
};

/**
 * @brief 选择类型枚举
 */
enum class SelectionType {
    kNone,   // 无选择
    kCaret,  // 光标（折叠状态）
    kRange   // 范围选择
};

/**
 * @brief DOM Selection 类
 *
 * Selection 表示用户选择的文本范围或光标位置。
 * 选择由锚点（anchor）和焦点（focus）定义：
 * - 锚点是选择开始的位置（用户开始拖动的位置）
 * - 焦点是选择结束的位置（用户拖动到的位置）
 *
 * 注意：anchor 和 focus 的顺序可能与 start/end 不同
 * - 如果用户从左向右选择，anchor 在 focus 之前
 * - 如果用户从右向左选择，anchor 在 focus 之后
 */
class Selection : public std::enable_shared_from_this<Selection> {
public:
    explicit Selection(std::shared_ptr<Document> document);
    ~Selection();

    // ========== 锚点和焦点属性（用户选择方向） ==========
    // 这些方法返回原始存储的节点和偏移量（可能是 Element 节点）

    std::shared_ptr<Node> GetAnchorNode() const;
    int GetAnchorOffset() const;
    std::shared_ptr<Node> GetFocusNode() const;
    int GetFocusOffset() const;

    // ========== 计算后的位置属性（用于光标渲染） ==========
    // 参考 Blink 的 Position::ComputeContainerNode/ComputeOffsetInContainerNode
    // 这些方法将 Element 节点位置解析为 Text 节点位置

    std::shared_ptr<Node> GetComputedAnchorNode() const;
    int GetComputedAnchorOffset() const;
    std::shared_ptr<Node> GetComputedFocusNode() const;
    int GetComputedFocusOffset() const;

    // ========== 起始和结束属性（文档顺序） ==========

    std::shared_ptr<Node> GetStartNode() const;
    int GetStartOffset() const;
    std::shared_ptr<Node> GetEndNode() const;
    int GetEndOffset() const;

    // ========== 状态属性 ==========

    bool IsCollapsed() const;
    int GetRangeCount() const;
    SelectionType GetType() const;
    SelectionDirection GetDirection() const;
    std::string GetDirectionString() const;

    // ========== 核心选择操作 ==========

    /**
     * @brief 设置选择的基点和扩展点（核心 API）
     * @param anchor_node 锚点节点
     * @param anchor_offset 锚点偏移量
     * @param focus_node 焦点节点
     * @param focus_offset 焦点偏移量
     *
     * 这是最重要的方法，CodeMirror 6 主要使用此方法设置选择
     */
    void SetBaseAndExtent(
        std::shared_ptr<Node> anchor_node, int anchor_offset,
        std::shared_ptr<Node> focus_node, int focus_offset);

    /**
     * @brief 折叠选择到指定位置
     * @param node 目标节点（如果为 null，清除选择）
     * @param offset 偏移量
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

    void CollapseToStart();
    void CollapseToEnd();

    // ========== Range 管理 ==========

    void RemoveAllRanges();
    void AddRange(std::shared_ptr<Range> range);
    void RemoveRange(std::shared_ptr<Range> range);
    std::shared_ptr<Range> GetRangeAt(int index) const;

    // ========== 其他方法 ==========

    void Empty();  // 等同于 RemoveAllRanges
    void DeleteFromDocument();
    bool ContainsNode(std::shared_ptr<Node> node, bool allow_partial = false) const;
    std::string ToString() const;

    // ========== 内部更新 ==========

    void UpdateFromUserAction(
        std::shared_ptr<Node> anchor, int anchor_offset,
        std::shared_ptr<Node> focus, int focus_offset);

    std::shared_ptr<Document> GetDocument() const { return document_.lock(); }

private:
    /**
     * @brief 比较两个位置的文档顺序
     * @return 负数表示 a 在 b 之前，0 表示相同，正数表示 a 在 b 之后
     */
    int ComparePositions(
        std::shared_ptr<Node> node_a, int offset_a,
        std::shared_ptr<Node> node_b, int offset_b) const;

    /**
     * @brief 检查锚点是否在焦点之前
     */
    bool IsAnchorFirst() const;

    /**
     * @brief 从选择状态更新内部 Range
     */
    void UpdateRangeFromSelection();

    /**
     * @brief 验证节点和偏移量是否有效
     */
    bool ValidateNodeOffset(std::shared_ptr<Node> node, int offset) const;

    /**
     * @brief 获取节点的最大有效偏移量
     */
    int GetNodeLength(std::shared_ptr<Node> node) const;

    /**
     * @brief 将 Element 节点位置解析为 Text 节点位置
     * @param node Element 节点
     * @param offset 子节点索引
     * @return pair<解析后的节点, 解析后的偏移量>
     */
    std::pair<std::shared_ptr<Node>, int> ResolveElementPosition(
        std::shared_ptr<Node> node, int offset) const;

    /**
     * @brief 分发 selectionchange 事件到 Document
     * 
     * CodeMirror 6 等编辑器依赖此事件同步选择状态
     */
    void DispatchSelectionChangeEvent();

private:
    std::weak_ptr<Document> document_;

    // 锚点和焦点（保持用户选择方向）
    std::weak_ptr<Node> anchor_node_;
    std::weak_ptr<Node> focus_node_;
    int anchor_offset_ = 0;
    int focus_offset_ = 0;

    // 是否有方向性（用于 modify 等操作）
    bool is_directional_ = false;

    // 是否有待处理的 selectionchange 事件（用于合并多次触发）
    bool pending_selectionchange_ = false;

    // 缓存的 Range
    std::vector<std::shared_ptr<Range>> ranges_;
};

} // namespace lightui
