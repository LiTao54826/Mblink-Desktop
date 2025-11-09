/**
 * @file text.h
 * @brief 文本节点类
 *
 * 功能：
 * - 表示DOM树中的文本节点
 * - 存储和管理文本数据
 * - 不能包含子节点
 */

#pragma once

#include "node.h"
#include <string>

namespace lightui {

/**
 * @brief 文本节点类
 */
class Text : public Node {
public:
    /**
     * @brief 构造函数
     * @param data 文本数据
     */
    explicit Text(const std::string& data = "");

    /**
     * @brief 析构函数
     */
    ~Text() override = default;

    /**
     * @brief 获取文本数据
     * @return 文本数据
     */
    std::string GetData() const { return data_; }

    /**
     * @brief 设置文本数据
     * @param data 文本数据
     */
    void SetData(const std::string& data);

    /**
     * @brief 克隆节点
     * @param deep 是否深度克隆（对Text节点无效）
     * @return 克隆的节点
     */
    std::shared_ptr<Node> CloneNode(bool deep) override;

    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    std::string GetTextContent() const override;

    /**
     * @brief 设置文本内容
     * @param content 文本内容
     */
    void SetTextContent(const std::string& content) override;

private:
    std::string data_;
};

} // namespace lightui
