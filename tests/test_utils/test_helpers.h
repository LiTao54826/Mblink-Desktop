/**
 * @file test_helpers.h
 * @brief 测试辅助工具
 *
 * 提供测试中常用的辅助函数和宏
 */

#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "dom/document.h"
#include "dom/element.h"
#include "dom/text.h"
#include "dom/node.h"

namespace mblink {
namespace test {

/**
 * @brief 测试环境基类
 *
 * 提供 Document 和基本 DOM 结构的初始化
 */
class DOMTestBase : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // 创建测试文档
    std::shared_ptr<Document> CreateDocument();

    // 创建带有基本结构的文档 (html > head + body)
    std::shared_ptr<Document> CreateDocumentWithStructure();

    // 快速创建元素
    std::shared_ptr<Element> CreateElement(const std::string& tag_name);
    std::shared_ptr<Text> CreateTextNode(const std::string& text);

    // 从 HTML 字符串创建文档
    std::shared_ptr<Document> CreateDocumentFromHTML(const std::string& html);

protected:
    std::shared_ptr<Document> doc_;
};

/**
 * @brief 性能测试辅助类
 */
class PerformanceTimer {
public:
    void Start();
    void Stop();
    double GetElapsedMs() const;
    double GetElapsedUs() const;

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
};

/**
 * @brief 断言 DOM 树结构
 */
void AssertDOMStructure(std::shared_ptr<Node> node, const std::string& expected_structure);

/**
 * @brief 断言元素属性
 */
void AssertElementAttribute(std::shared_ptr<Element> elem,
                            const std::string& attr_name,
                            const std::string& expected_value);

/**
 * @brief 断言元素有指定的 class
 */
void AssertElementHasClass(std::shared_ptr<Element> elem, const std::string& class_name);

/**
 * @brief 断言元素没有指定的 class
 */
void AssertElementNotHasClass(std::shared_ptr<Element> elem, const std::string& class_name);

/**
 * @brief 计算 DOM 树中的节点数量
 */
size_t CountNodes(std::shared_ptr<Node> root);

/**
 * @brief 查找第一个匹配标签名的元素
 */
std::shared_ptr<Element> FindElementByTagName(std::shared_ptr<Node> root,
                                               const std::string& tag_name);

/**
 * @brief 收集所有匹配标签名的元素
 */
std::vector<std::shared_ptr<Element>> FindAllElementsByTagName(
    std::shared_ptr<Node> root,
    const std::string& tag_name);

} // namespace test
} // namespace mblink

// 便捷宏
#define EXPECT_DOM_EQ(node, expected) \
    mblink::test::AssertDOMStructure(node, expected)

#define EXPECT_ATTR_EQ(elem, attr, value) \
    mblink::test::AssertElementAttribute(elem, attr, value)

#define EXPECT_HAS_CLASS(elem, cls) \
    mblink::test::AssertElementHasClass(elem, cls)

#define EXPECT_NOT_HAS_CLASS(elem, cls) \
    mblink::test::AssertElementNotHasClass(elem, cls)
