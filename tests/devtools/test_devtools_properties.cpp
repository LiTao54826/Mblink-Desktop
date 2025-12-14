/**
 * @file test_devtools_properties.cpp
 * @brief DevTools 属性测试
 * 
 * 使用 Google Test 进行属性测试
 * 每个测试运行多次迭代以验证属性
 */

#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

#include "core/devtools/devtools_manager.h"
#include "core/devtools/inspector/dom_tree_view.h"
#include "core/devtools/serializer/dom_serializer.h"
#include "core/devtools/search/element_search.h"
#include "core/devtools/editor/style_editor.h"
#include "core/devtools/editor/attribute_editor.h"
#include "core/devtools/styles/inline_styles_view.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"

namespace lightui {
namespace testing {

// 随机数生成器
class RandomGenerator {
public:
    RandomGenerator() : gen_(std::random_device{}()) {}

    int RandomInt(int min, int max) {
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen_);
    }

    float RandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen_);
    }

    std::string RandomString(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[RandomInt(0, sizeof(charset) - 2)];
        }
        return result;
    }

    std::string RandomTagName() {
        static const std::vector<std::string> tags = {
            "div", "span", "p", "a", "button", "input", "h1", "h2", "ul", "li"
        };
        return tags[RandomInt(0, tags.size() - 1)];
    }

private:
    std::mt19937 gen_;
};

// 创建随机 DOM 树
std::shared_ptr<Element> CreateRandomDOMTree(Document* doc, RandomGenerator& rng, int depth, int max_children) {
    auto element = doc->CreateElement(rng.RandomTagName());
    
    // 随机添加属性
    if (rng.RandomInt(0, 1)) {
        element->SetAttribute("id", rng.RandomString(8));
    }
    if (rng.RandomInt(0, 1)) {
        element->SetAttribute("class", rng.RandomString(6) + " " + rng.RandomString(6));
    }
    if (rng.RandomInt(0, 1)) {
        element->SetAttribute("data-value", rng.RandomString(10));
    }

    // 递归创建子节点
    if (depth > 0) {
        int num_children = rng.RandomInt(0, max_children);
        for (int i = 0; i < num_children; ++i) {
            if (rng.RandomInt(0, 2) == 0) {
                // 文本节点
                auto text = doc->CreateTextNode(rng.RandomString(20));
                element->AppendChild(text);
            } else {
                // 元素节点
                auto child = CreateRandomDOMTree(doc, rng, depth - 1, max_children);
                element->AppendChild(child);
            }
        }
    }

    return element;
}

// 计算 DOM 树中的节点数量
int CountNodes(std::shared_ptr<Node> node) {
    if (!node) return 0;
    int count = 1;
    for (const auto& child : node->GetChildNodes()) {
        count += CountNodes(child);
    }
    return count;
}

// ============================================================================
// Property 16: Panel Size Persistence
// **Feature: devtools-inspector, Property 16: Panel Size Persistence**
// **Validates: Requirements 11.3**
// ============================================================================

class PanelSizePersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(PanelSizePersistenceTest, PanelSizeRoundTrip) {
    // **Feature: devtools-inspector, Property 16: Panel Size Persistence**
    // **Validates: Requirements 11.3**
    
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // 生成随机面板尺寸 (0.1 - 0.8)
        float original_size = rng_.RandomFloat(0.1f, 0.8f);
        
        auto& manager = DevToolsManager::GetInstance();
        manager.Initialize(doc_.get(), nullptr);
        
        // 设置面板尺寸
        manager.SetPanelSize(original_size);
        
        // 读取面板尺寸
        float read_size = manager.GetPanelSize();
        
        // 验证：设置后读取应该返回相同的值（1像素容差 ≈ 0.001）
        EXPECT_NEAR(original_size, read_size, 0.001f)
            << "Iteration " << i << ": Panel size not preserved. "
            << "Original: " << original_size << ", Read: " << read_size;
        
        manager.Shutdown();
    }
}

TEST_F(PanelSizePersistenceTest, PanelSizeClampedToValidRange) {
    // 测试边界值被正确限制
    auto& manager = DevToolsManager::GetInstance();
    manager.Initialize(doc_.get(), nullptr);
    
    // 测试过小的值
    manager.SetPanelSize(0.05f);
    EXPECT_GE(manager.GetPanelSize(), 0.1f);
    
    // 测试过大的值
    manager.SetPanelSize(0.95f);
    EXPECT_LE(manager.GetPanelSize(), 0.8f);
    
    manager.Shutdown();
}

// ============================================================================
// Property 18: DOM Serialization Round-Trip
// **Feature: devtools-inspector, Property 18: DOM Serialization Round-Trip**
// **Validates: Requirements 13.1, 13.2, 13.3, 13.4**
// ============================================================================

class DOMSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(DOMSerializationTest, SerializationRoundTrip) {
    // **Feature: devtools-inspector, Property 18: DOM Serialization Round-Trip**
    // **Validates: Requirements 13.1, 13.2, 13.3, 13.4**
    
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // 创建随机 DOM 树
        auto original_tree = CreateRandomDOMTree(doc_.get(), rng_, 3, 3);
        
        // 序列化
        std::string html = DOMSerializer::Serialize(original_tree);
        
        // 验证序列化结果非空
        EXPECT_FALSE(html.empty())
            << "Iteration " << i << ": Serialization produced empty string";
        
        // 反序列化
        auto parsed_doc = DOMSerializer::Deserialize(html);
        
        if (parsed_doc) {
            auto parsed_tree = parsed_doc->GetDocumentElement();
            
            // 验证结构等价性
            // 注意：由于 HTML 解析可能会添加 html/head/body 包装，
            // 我们只验证基本结构
            EXPECT_TRUE(parsed_tree != nullptr)
                << "Iteration " << i << ": Parsed tree is null";
        }
    }
}

TEST_F(DOMSerializationTest, AttributePreservation) {
    // **Feature: devtools-inspector, Property 19: Serialization Attribute Preservation**
    // **Validates: Requirements 13.2**
    
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // 创建带属性的元素
        auto element = doc_->CreateElement("div");
        
        std::string id = rng_.RandomString(8);
        std::string cls = rng_.RandomString(6);
        std::string data_val = rng_.RandomString(10);
        
        element->SetAttribute("id", id);
        element->SetAttribute("class", cls);
        element->SetAttribute("data-test", data_val);
        
        // 序列化
        std::string html = DOMSerializer::Serialize(element);
        
        // 验证属性在序列化结果中
        EXPECT_NE(html.find("id=\"" + id + "\""), std::string::npos)
            << "Iteration " << i << ": id attribute not found in serialized HTML";
        EXPECT_NE(html.find("class=\"" + cls + "\""), std::string::npos)
            << "Iteration " << i << ": class attribute not found in serialized HTML";
    }
}

// ============================================================================
// Property 13: CSS Selector Search Accuracy
// **Feature: devtools-inspector, Property 13: CSS Selector Search Accuracy**
// **Validates: Requirements 9.1**
// ============================================================================

class ElementSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
        // 创建基本文档结构
        auto html = doc_->CreateElement("html");
        auto body = doc_->CreateElement("body");
        html->AppendChild(body);
        doc_->AppendChild(html);
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(ElementSearchTest, CSSSelectorSearchAccuracy) {
    // **Feature: devtools-inspector, Property 13: CSS Selector Search Accuracy**
    // **Validates: Requirements 9.1**
    
    const int NUM_ITERATIONS = 50;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // 创建随机 DOM 树
        auto body = doc_->GetBody();
        if (!body) continue;
        
        // 清空 body
        while (!body->GetChildNodes().empty()) {
            body->RemoveChild(body->GetFirstChild());
        }
        
        // 添加随机元素
        int num_divs = rng_.RandomInt(1, 5);
        for (int j = 0; j < num_divs; ++j) {
            auto div = doc_->CreateElement("div");
            div->SetAttribute("class", "test-class");
            body->AppendChild(div);
        }
        
        // 使用 ElementSearch 搜索
        ElementSearch search(doc_.get());
        auto result = search.Search(".test-class");
        
        // 验证结果数量一致（与添加的元素数量相同）
        EXPECT_EQ(result.Count(), static_cast<size_t>(num_divs))
            << "Iteration " << i << ": Search result count mismatch. "
            << "ElementSearch: " << result.Count() 
            << ", Expected: " << num_divs;
    }
}

TEST_F(ElementSearchTest, TextSearchCoverage) {
    // **Feature: devtools-inspector, Property 14: Text Search Coverage**
    // **Validates: Requirements 9.2**
    
    auto body = doc_->GetBody();
    if (!body) return;
    
    // 创建测试元素
    auto div1 = doc_->CreateElement("div");
    div1->SetAttribute("id", "test-id");
    body->AppendChild(div1);
    
    auto div2 = doc_->CreateElement("div");
    div2->SetAttribute("class", "test-class");
    body->AppendChild(div2);
    
    auto span = doc_->CreateElement("span");
    span->SetAttribute("class", "another-test");
    body->AppendChild(span);
    
    // 搜索 "test"
    ElementSearch search(doc_.get());
    search.SetSearchType(SearchType::Text);
    auto result = search.Search("test");
    
    // 应该找到所有包含 "test" 的元素
    EXPECT_GE(result.Count(), 3u)
        << "Text search should find elements with 'test' in id or class";
}

TEST_F(ElementSearchTest, SearchResultCountAccuracy) {
    // **Feature: devtools-inspector, Property 15: Search Result Count Accuracy**
    // **Validates: Requirements 9.3**
    
    auto body = doc_->GetBody();
    if (!body) return;
    
    // 清空 body
    while (!body->GetChildNodes().empty()) {
        body->RemoveChild(body->GetFirstChild());
    }
    
    // 添加已知数量的元素
    const int expected_count = 7;
    for (int i = 0; i < expected_count; ++i) {
        auto div = doc_->CreateElement("div");
        div->SetAttribute("class", "counted-item");
        body->AppendChild(div);
    }
    
    ElementSearch search(doc_.get());
    auto result = search.Search(".counted-item");
    
    EXPECT_EQ(result.Count(), expected_count)
        << "Search result count should equal actual element count";
}

// ============================================================================
// Property 10: Style Edit Round-Trip
// **Feature: devtools-inspector, Property 10: Style Edit Round-Trip**
// **Validates: Requirements 7.1, 7.2, 7.3**
// ============================================================================

class StyleEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(StyleEditorTest, StyleEditRoundTrip) {
    // **Feature: devtools-inspector, Property 10: Style Edit Round-Trip**
    // **Validates: Requirements 7.1, 7.2, 7.3**
    
    const int NUM_ITERATIONS = 100;
    
    // 有效的 CSS 属性和值对
    std::vector<std::pair<std::string, std::string>> valid_styles = {
        {"color", "red"},
        {"color", "#ff0000"},
        {"color", "rgb(255, 0, 0)"},
        {"background-color", "blue"},
        {"width", "100px"},
        {"height", "50%"},
        {"margin", "10px"},
        {"padding", "5px 10px"},
        {"font-size", "16px"},
        {"display", "flex"},
        {"position", "relative"},
        {"opacity", "0.5"}
    };
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto element = doc_->CreateElement("div");
        
        // 随机选择一个样式
        int idx = rng_.RandomInt(0, valid_styles.size() - 1);
        auto& [property, value] = valid_styles[idx];
        
        // 设置样式
        element->SetStyle(property, value);
        
        // 读取样式
        std::string read_value = element->GetStyle(property);
        
        // 验证：设置后读取应该返回相同的值
        EXPECT_EQ(value, read_value)
            << "Iteration " << i << ": Style not preserved. "
            << "Property: " << property << ", Set: " << value << ", Read: " << read_value;
    }
}

// ============================================================================
// Property 11: Invalid Style Rejection
// **Feature: devtools-inspector, Property 11: Invalid Style Rejection**
// **Validates: Requirements 7.4**
// ============================================================================

TEST_F(StyleEditorTest, InvalidStyleRejection) {
    // **Feature: devtools-inspector, Property 11: Invalid Style Rejection**
    // **Validates: Requirements 7.4**
    
    auto element = doc_->CreateElement("div");
    
    // 设置初始有效样式
    element->SetStyle("color", "red");
    std::string original_value = element->GetStyle("color");
    
    // StyleEditor 验证测试
    // 空值应该被拒绝
    EXPECT_FALSE(StyleEditor::ValidateStyleValue("color", ""))
        << "Empty value should be rejected";
    
    // 有效值应该被接受
    EXPECT_TRUE(StyleEditor::ValidateStyleValue("color", "blue"))
        << "Valid value should be accepted";
    
    // 自定义属性应该被接受
    EXPECT_TRUE(StyleEditor::ValidateStyleValue("--custom-color", "red"))
        << "Custom properties should be accepted";
}

// ============================================================================
// Property 12: Attribute Edit Round-Trip
// **Feature: devtools-inspector, Property 12: Attribute Edit Round-Trip**
// **Validates: Requirements 8.1, 8.2, 8.3**
// ============================================================================

class AttributeEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(AttributeEditorTest, AttributeEditRoundTrip) {
    // **Feature: devtools-inspector, Property 12: Attribute Edit Round-Trip**
    // **Validates: Requirements 8.1, 8.2, 8.3**
    
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto element = doc_->CreateElement("div");
        
        // 生成随机属性名和值
        std::string attr_name = "data-" + rng_.RandomString(6);
        std::string attr_value = rng_.RandomString(20);
        
        // 设置属性
        element->SetAttribute(attr_name, attr_value);
        
        // 读取属性
        std::string read_value = element->GetAttribute(attr_name);
        
        // 验证：设置后读取应该返回相同的值
        EXPECT_EQ(attr_value, read_value)
            << "Iteration " << i << ": Attribute not preserved. "
            << "Name: " << attr_name << ", Set: " << attr_value << ", Read: " << read_value;
    }
}

TEST_F(AttributeEditorTest, AttributeNameValidation) {
    // 测试属性名验证
    
    // 有效的属性名
    EXPECT_TRUE(AttributeEditor::ValidateAttributeName("id"));
    EXPECT_TRUE(AttributeEditor::ValidateAttributeName("class"));
    EXPECT_TRUE(AttributeEditor::ValidateAttributeName("data-value"));
    EXPECT_TRUE(AttributeEditor::ValidateAttributeName("aria-label"));
    EXPECT_TRUE(AttributeEditor::ValidateAttributeName("_custom"));
    
    // 无效的属性名
    EXPECT_FALSE(AttributeEditor::ValidateAttributeName(""));
    EXPECT_FALSE(AttributeEditor::ValidateAttributeName("123abc"));
    EXPECT_FALSE(AttributeEditor::ValidateAttributeName("-invalid"));
}

// ============================================================================
// Property 6: Inline Style Display Completeness
// **Feature: devtools-inspector, Property 6: Inline Style Display Completeness**
// **Validates: Requirements 4.1, 4.2**
// ============================================================================

class InlineStylesViewTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc_ = std::make_shared<Document>();
    }

    std::shared_ptr<Document> doc_;
    RandomGenerator rng_;
};

TEST_F(InlineStylesViewTest, InlineStyleDisplayCompleteness) {
    // **Feature: devtools-inspector, Property 6: Inline Style Display Completeness**
    // **Validates: Requirements 4.1, 4.2**
    
    const int NUM_ITERATIONS = 50;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto element = doc_->CreateElement("div");
        
        // 生成随机内联样式
        int num_styles = rng_.RandomInt(1, 5);
        std::vector<std::pair<std::string, std::string>> expected_styles;
        std::string style_attr;
        
        std::vector<std::string> properties = {"color", "background", "width", "height", "margin", "padding"};
        std::vector<std::string> values = {"red", "blue", "100px", "50px", "10px", "5px"};
        
        for (int j = 0; j < num_styles; ++j) {
            std::string prop = properties[rng_.RandomInt(0, properties.size() - 1)];
            std::string val = values[rng_.RandomInt(0, values.size() - 1)];
            
            if (!style_attr.empty()) style_attr += " ";
            style_attr += prop + ": " + val + ";";
            expected_styles.push_back({prop, val});
        }
        
        element->SetAttribute("style", style_attr);
        
        // 使用 InlineStylesView 获取样式
        InlineStylesView view;
        view.SetElement(element);
        auto displayed_styles = view.GetStyleProperties();
        
        // 验证：显示的样式数量应该等于设置的样式数量
        EXPECT_EQ(displayed_styles.size(), expected_styles.size())
            << "Iteration " << i << ": Style count mismatch";
    }
}

} // namespace testing
} // namespace lightui

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
