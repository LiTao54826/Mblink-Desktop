/**
 * @file comment.h
 * @brief 注释节点类
 */

#pragma once

#include "text.h"

namespace mbink {

class Comment : public Text {
public:
    explicit Comment(const std::string& data = "");
    ~Comment() override = default;

    std::shared_ptr<Node> CloneNode(bool deep) override;
};

} // namespace mbink
