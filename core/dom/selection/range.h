/**
 * @file range.h
 * @brief DOM Range API 实现
 *
 * 功能：
 * - 表示文档中的一个片段（fragment）
 * - 支持设置和获取边界位置
 * - 支持选择节点和节点内容
 * - 支持克隆和文本提取
 *
 * 参考：https://dom.spec.whatwg.org/#interface-range
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mbink {

// 前向声明
class Node;
class Document;

/**
 * @brief DOM Range 类
 *
 * Range 表示文档中的一个片段，包含起始和结束边界。
 * 每个边界由一个节点（container）和一个偏移量（offset）组成。
 */
class Range : public std::enable_shared_from_this<Range> {
public:
    /**
     * @brief 构造函数
     * @param owner_document 所属文档
     */
    explicit Range(std::shared_ptr<Document> owner_document);

    /**
     * @brief 析构函数
     */
    ~Range();

    // ========== 边界设置方法 ==========

    /**
     * @brief 设置起始边界
     * @param node 起始节点
     * @param offset 起始偏移量
     * @throws IndexSizeError 如果 offset 超出范围
     * @throws TypeError 如果 node 为 null
     */
    void SetStart(std::shared_ptr<Node> node, int offset);

    /**
     * @brief 设置结束边界
     * @param node 结束节点
     * @param offset 结束偏移量
     * @throws IndexSizeError 如果 offset 超出范围
     * @throws TypeError 如果 node 为 null
     */
    void SetEnd(std::shared_ptr<Node> node, int offset);

    /**
     * @brief 设置起始边界到指定节点之前
     * @param node 参考节点
     * @throws InvalidNodeTypeError 如果 node 无父节点
     */
    void SetStartBefore(std::shared_ptr<Node> node);

    /**
     * @brief 设置起始边界到指定节点之后
     * @param node 参考节点
     * @throws InvalidNodeTypeError 如果 node 无父节点
     */
    void SetStartAfter(std::shared_ptr<Node> node);

    /**
     * @brief 设置结束边界到指定节点之前
     * @param node 参考节点
     * @throws InvalidNodeTypeError 如果 node 无父节点
     */
    void SetEndBefore(std::shared_ptr<Node> node);

    /**
     * @brief 设置结束边界到指定节点之后
     * @param node 参考节点
     * @throws InvalidNodeTypeError 如果 node 无父节点
     */
    void SetEndAfter(std::shared_ptr<Node> node);

    // ========== 选择方法 ==========

    /**
     * @brief 选择整个节点
     * @param node 要选择的节点
     *
     * 设置 Range 使其完整包含指定节点
     */
    void SelectNode(std::shared_ptr<Node> node);

    /**
     * @brief 选择节点的所有内容
     * @param node 要选择内容的节点
     *
     * 设置 Range 使其包含节点的所有子节点或文本内容
     */
    void SelectNodeContents(std::shared_ptr<Node> node);

    /**
     * @brief 折叠 Range
     * @param to_start true 折叠到起始位置，false 折叠到结束位置
     */
    void Collapse(bool to_start = true);

    // ========== 属性访问 ==========

    /**
     * @brief 获取起始容器节点
     * @return 起始容器节点
     */
    std::shared_ptr<Node> GetStartContainer() const { return start_container_.lock(); }

    /**
     * @brief 获取结束容器节点
     * @return 结束容器节点
     */
    std::shared_ptr<Node> GetEndContainer() const { return end_container_.lock(); }

    /**
     * @brief 获取起始偏移量
     * @return 起始偏移量
     */
    int GetStartOffset() const { return start_offset_; }

    /**
     * @brief 获取结束偏移量
     * @return 结束偏移量
     */
    int GetEndOffset() const { return end_offset_; }

    /**
     * @brief 检查 Range 是否折叠（起始和结束位置相同）
     * @return true 如果折叠
     */
    bool IsCollapsed() const;

    /**
     * @brief 获取公共祖先容器
     * @return 同时包含起始和结束边界的最深节点
     */
    std::shared_ptr<Node> GetCommonAncestorContainer() const;

    // ========== 克隆和转换 ==========

    /**
     * @brief 克隆 Range
     * @return 具有相同边界的新 Range 对象
     */
    std::shared_ptr<Range> CloneRange() const;

    /**
     * @brief 获取 Range 内的文本内容
     * @return Range 边界内的所有文本
     */
    std::string ToString() const;

    // ========== 几何信息 ==========

    /**
     * @brief DOMRect 结构，表示矩形区域
     */
    struct DOMRect {
        float x = 0;
        float y = 0;
        float width = 0;
        float height = 0;
        float top = 0;
        float right = 0;
        float bottom = 0;
        float left = 0;
    };

    /**
     * @brief 获取 Range 的边界矩形
     * @return Range 内容的边界矩形
     */
    DOMRect GetBoundingClientRect() const;

    /**
     * @brief 获取 Range 的所有边界矩形（每行一个）
     * @return Range 内容的边界矩形列表
     */
    std::vector<DOMRect> GetClientRects() const;

    // ========== 所属文档 ==========

    /**
     * @brief 获取所属文档
     * @return 所属文档
     */
    std::shared_ptr<Document> GetOwnerDocument() const { return owner_document_.lock(); }

private:
    /**
     * @brief 获取节点在父节点中的索引
     * @param node 目标节点
     * @return 节点索引，如果无父节点返回 -1
     */
    int GetNodeIndex(std::shared_ptr<Node> node) const;

    /**
     * @brief 查找两个节点的公共祖先
     * @param node1 第一个节点
     * @param node2 第二个节点
     * @return 公共祖先节点
     */
    std::shared_ptr<Node> FindCommonAncestor(
        std::shared_ptr<Node> node1,
        std::shared_ptr<Node> node2) const;

    /**
     * @brief 获取节点的最大有效偏移量
     * @param node 目标节点
     * @return 最大偏移量（文本节点为长度，元素节点为子节点数）
     */
    int GetNodeLength(std::shared_ptr<Node> node) const;

    /**
     * @brief 递归收集 Range 内的文本
     * @param node 当前节点
     * @param result 结果字符串
     * @param in_range 是否在 Range 内
     */
    void CollectText(std::shared_ptr<Node> node, std::string& result, bool& in_range) const;

    /**
     * @brief 计算文本节点在指定偏移范围内的矩形
     * @param text_node 文本节点
     * @param start_offset 起始偏移
     * @param end_offset 结束偏移
     * @return 矩形区域
     */
    DOMRect ComputeTextRect(
        std::shared_ptr<class Text> text_node,
        int start_offset,
        int end_offset) const;

    /**
     * @brief 合并两个矩形
     * @param result 结果矩形（会被修改）
     * @param other 要合并的矩形
     */
    void UnionRect(DOMRect& result, const DOMRect& other) const;

    /**
     * @brief 深度优先遍历收集 Range 内所有节点的矩形
     * @param node 当前节点
     * @param rects 收集的矩形列表
     * @param in_range 是否在 Range 内
     * @param done 是否已完成
     */
    void CollectRects(
        std::shared_ptr<Node> node,
        std::vector<DOMRect>& rects,
        bool& in_range,
        bool& done) const;

private:
    std::weak_ptr<Document> owner_document_;
    std::weak_ptr<Node> start_container_;
    std::weak_ptr<Node> end_container_;
    int start_offset_ = 0;
    int end_offset_ = 0;
};

} // namespace mbink
