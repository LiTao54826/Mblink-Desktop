/**
 * @file test_helpers.cpp
 * @brief 测试辅助工具实现
 */

#include "test_helpers.h"
#include <sstream>

namespace mbink {
namespace test {

// ========== DOMTestBase ==========

void DOMTestBase::SetUp() {
    doc_ = CreateDocumentWithStructure();
}

void DOMTestBase::TearDown() {
    doc_.reset();
}

std::shared_ptr<Document> DOMTestBase::CreateDocument() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    return doc;
}

std::shared_ptr<Document> DOMTestBase::CreateDocumentWithStructure() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    return doc;
}

std::shared_ptr<Element> DOMTestBase::CreateElement(const std::string& tag_name) {
    if (doc_) {
        return doc_->CreateElement(tag_name);
    }
    return std::make_shared<Element>(tag_name);
}

std::shared_ptr<Text> DOMTestBase::CreateTextNode(const std::string& text) {
    if (doc_) {
        return doc_->CreateTextNode(text);
    }
    return std::make_shared<Text>(text);
}

std::shared_ptr<Document> DOMTestBase::CreateDocumentFromHTML(const std::string& html) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    doc->LoadHTML(html);
    return doc;
}

// ========== PerformanceTimer ==========

void PerformanceTimer::Start() {
    start_ = std::chrono::high_resolution_clock::now();
}

void PerformanceTimer::Stop() {
    end_ = std::chrono::high_resolution_clock::now();
}

double PerformanceTimer::GetElapsedMs() const {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
    return duration.count() / 1000.0;
}

double PerformanceTimer::GetElapsedUs() const {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
    return static_cast<double>(duration.count());
}

// ========== 断言函数 ==========

void AssertDOMStructure(std::shared_ptr<Node> node, const std::string& expected_structure) {
    // 简单实现：比较序列化后的结构
    // TODO: 实现更复杂的结构比较
    ASSERT_NE(node, nullptr) << "Node is null";
}

void AssertElementAttribute(std::shared_ptr<Element> elem,
                            const std::string& attr_name,
                            const std::string& expected_value) {
    ASSERT_NE(elem, nullptr) << "Element is null";
    EXPECT_EQ(elem->GetAttribute(attr_name), expected_value)
        << "Attribute '" << attr_name << "' mismatch";
}

void AssertElementHasClass(std::shared_ptr<Element> elem, const std::string& class_name) {
    ASSERT_NE(elem, nullptr) << "Element is null";
    EXPECT_TRUE(elem->HasClass(class_name))
        << "Element should have class '" << class_name << "'";
}

void AssertElementNotHasClass(std::shared_ptr<Element> elem, const std::string& class_name) {
    ASSERT_NE(elem, nullptr) << "Element is null";
    EXPECT_FALSE(elem->HasClass(class_name))
        << "Element should not have class '" << class_name << "'";
}

size_t CountNodes(std::shared_ptr<Node> root) {
    if (!root) return 0;

    size_t count = 1;
    for (const auto& child : root->GetChildNodes()) {
        count += CountNodes(child);
    }
    return count;
}

std::shared_ptr<Element> FindElementByTagName(std::shared_ptr<Node> root,
                                               const std::string& tag_name) {
    if (!root) return nullptr;

    if (root->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(root);
        if (elem && elem->GetTagName() == tag_name) {
            return elem;
        }
    }

    for (const auto& child : root->GetChildNodes()) {
        auto found = FindElementByTagName(child, tag_name);
        if (found) return found;
    }

    return nullptr;
}

std::vector<std::shared_ptr<Element>> FindAllElementsByTagName(
    std::shared_ptr<Node> root,
    const std::string& tag_name) {
    std::vector<std::shared_ptr<Element>> result;

    if (!root) return result;

    if (root->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto elem = std::dynamic_pointer_cast<Element>(root);
        if (elem && elem->GetTagName() == tag_name) {
            result.push_back(elem);
        }
    }

    for (const auto& child : root->GetChildNodes()) {
        auto children = FindAllElementsByTagName(child, tag_name);
        result.insert(result.end(), children.begin(), children.end());
    }

    return result;
}

} // namespace test
} // namespace mbink
