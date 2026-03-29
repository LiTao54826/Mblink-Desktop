/**
 * @file test_document.cpp
 * @brief Document 类单元测试
 *
 * 测试内容：
 * - 文档结构 (documentElement, head, body)
 * - 工厂方法 (createElement, createTextNode)
 * - 查询方法 (getElementById, getElementsByTagName, getElementsByClassName)
 * - HTML 加载和保存
 * - 批量更新 API
 * - 焦点管理
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "test_utils/mock_objects.h"
#include "dom/document.h"
#include "dom/element.h"
#include "dom/text.h"

namespace mbink {
namespace test {

class DocumentTest : public DOMTestBase {};

// ========== 文档结构测试 ==========

TEST_F(DocumentTest, HasDocumentElement) {
    EXPECT_NE(doc_->GetDocumentElement(), nullptr);
}

TEST_F(DocumentTest, HasBody) {
    EXPECT_NE(doc_->GetBody(), nullptr);
}

TEST_F(DocumentTest, HasHead) {
    EXPECT_NE(doc_->GetHead(), nullptr);
}

TEST_F(DocumentTest, DocumentElementIsHtml) {
    auto docElem = doc_->GetDocumentElement();
    EXPECT_EQ(docElem->GetTagName(), "html");
}

TEST_F(DocumentTest, BodyIsChildOfHtml) {
    auto body = doc_->GetBody();
    auto html = doc_->GetDocumentElement();

    bool found = false;
    for (const auto& child : html->GetChildNodes()) {
        if (child == body) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ========== 工厂方法测试 ==========

TEST_F(DocumentTest, CreateElement) {
    auto elem = doc_->CreateElement("div");

    EXPECT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetTagName(), "div");
    EXPECT_EQ(elem->GetOwnerDocument(), doc_);
}

TEST_F(DocumentTest, CreateTextNode) {
    auto text = doc_->CreateTextNode("Hello World");

    EXPECT_NE(text, nullptr);
    EXPECT_EQ(text->GetData(), "Hello World");
    EXPECT_EQ(text->GetOwnerDocument(), doc_);
}

TEST_F(DocumentTest, CreateElementVariousTags) {
    std::vector<std::string> tags = {
        "div", "span", "p", "a", "button", "input",
        "form", "table", "ul", "li", "img", "canvas"
    };

    for (const auto& tag : tags) {
        auto elem = doc_->CreateElement(tag);
        EXPECT_NE(elem, nullptr) << "Failed to create element: " << tag;
        EXPECT_EQ(elem->GetTagName(), tag) << "Tag name mismatch for: " << tag;
    }
}

// ========== 查询方法测试 ==========

TEST_F(DocumentTest, GetElementById) {
    auto body = doc_->GetBody();
    auto elem = doc_->CreateElement("div");
    elem->SetAttribute("id", "test-element");
    body->AppendChild(elem);

    auto found = doc_->GetElementById("test-element");
    EXPECT_EQ(found, elem);
}

TEST_F(DocumentTest, GetElementByIdNotFound) {
    auto found = doc_->GetElementById("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(DocumentTest, GetElementByIdAfterRemove) {
    auto body = doc_->GetBody();
    auto elem = doc_->CreateElement("div");
    elem->SetAttribute("id", "temp-element");
    body->AppendChild(elem);

    // 验证能找到
    EXPECT_NE(doc_->GetElementById("temp-element"), nullptr);

    // 移除元素
    body->RemoveChild(elem);

    // 应该找不到了
    EXPECT_EQ(doc_->GetElementById("temp-element"), nullptr);
}

TEST_F(DocumentTest, GetElementsByTagName) {
    auto body = doc_->GetBody();
    body->AppendChild(doc_->CreateElement("div"));
    body->AppendChild(doc_->CreateElement("div"));
    body->AppendChild(doc_->CreateElement("span"));

    auto divs = doc_->GetElementsByTagName("div");
    EXPECT_EQ(divs.size(), 2);

    auto spans = doc_->GetElementsByTagName("span");
    EXPECT_EQ(spans.size(), 1);
}

TEST_F(DocumentTest, GetElementsByTagNameNested) {
    auto body = doc_->GetBody();
    auto container = doc_->CreateElement("div");
    auto nested = doc_->CreateElement("div");

    container->AppendChild(nested);
    body->AppendChild(container);

    auto divs = doc_->GetElementsByTagName("div");
    EXPECT_EQ(divs.size(), 2);
}

TEST_F(DocumentTest, GetElementsByClassName) {
    auto body = doc_->GetBody();

    auto elem1 = doc_->CreateElement("div");
    elem1->AddClass("highlight");
    body->AppendChild(elem1);

    auto elem2 = doc_->CreateElement("span");
    elem2->AddClass("highlight");
    body->AppendChild(elem2);

    auto elem3 = doc_->CreateElement("p");
    elem3->AddClass("other");
    body->AppendChild(elem3);

    auto highlights = doc_->GetElementsByClassName("highlight");
    EXPECT_EQ(highlights.size(), 2);
}

// ========== HTML 加载测试 ==========

TEST_F(DocumentTest, LoadHTMLSimple) {
    auto doc = CreateDocument();
    bool result = doc->LoadHTML("<html><body><div id='test'>Hello</div></body></html>");

    EXPECT_TRUE(result);

    auto elem = doc->GetElementById("test");
    EXPECT_NE(elem, nullptr);
    EXPECT_EQ(elem->GetTextContent(), "Hello");
}

TEST_F(DocumentTest, LoadHTMLWithAttributes) {
    auto doc = CreateDocument();
    doc->LoadHTML("<html><body><div class='foo bar' data-value='123'></div></body></html>");

    auto divs = doc->GetElementsByTagName("div");
    ASSERT_EQ(divs.size(), 1);

    auto div = divs[0];
    EXPECT_TRUE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("bar"));
    EXPECT_EQ(div->GetAttribute("data-value"), "123");
}

TEST_F(DocumentTest, LoadHTMLWithNestedElements) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <div id="container">
                <ul>
                    <li>Item 1</li>
                    <li>Item 2</li>
                    <li>Item 3</li>
                </ul>
            </div>
        </body>
        </html>
    )");

    auto lis = doc->GetElementsByTagName("li");
    EXPECT_EQ(lis.size(), 3);
}

TEST_F(DocumentTest, SaveHTML) {
    auto body = doc_->GetBody();
    auto div = doc_->CreateElement("div");
    div->SetAttribute("id", "test");
    div->AppendChild(doc_->CreateTextNode("Content"));
    body->AppendChild(div);

    auto html = doc_->SaveHTML();

    EXPECT_TRUE(html.find("div") != std::string::npos);
    EXPECT_TRUE(html.find("test") != std::string::npos);
    EXPECT_TRUE(html.find("Content") != std::string::npos);
}

// ========== 批量更新测试 ==========

TEST_F(DocumentTest, BeginEndBatch) {
    EXPECT_FALSE(doc_->IsInBatch());

    doc_->BeginBatch();
    EXPECT_TRUE(doc_->IsInBatch());

    doc_->EndBatch();
    EXPECT_FALSE(doc_->IsInBatch());
}

TEST_F(DocumentTest, NestedBatch) {
    doc_->BeginBatch();
    doc_->BeginBatch();
    EXPECT_TRUE(doc_->IsInBatch());

    doc_->EndBatch();
    EXPECT_TRUE(doc_->IsInBatch());  // 还在批量更新中

    doc_->EndBatch();
    EXPECT_FALSE(doc_->IsInBatch());  // 现在结束了
}

// ========== 焦点管理测试 ==========

TEST_F(DocumentTest, GetActiveElementDefault) {
    auto active = doc_->GetActiveElement();
    // 默认应该是 body 或 nullptr
    EXPECT_TRUE(active == doc_->GetBody() || active == nullptr);
}

TEST_F(DocumentTest, SetActiveElement) {
    auto input = doc_->CreateElement("input");
    doc_->GetBody()->AppendChild(input);

    doc_->SetActiveElement(input);
    EXPECT_EQ(doc_->GetActiveElement(), input);
}

TEST_F(DocumentTest, ClearActiveElement) {
    auto input = doc_->CreateElement("input");
    doc_->GetBody()->AppendChild(input);

    doc_->SetActiveElement(input);
    doc_->SetActiveElement(nullptr);

    auto active = doc_->GetActiveElement();
    EXPECT_TRUE(active == doc_->GetBody() || active == nullptr);
}

// ========== DOM 观察者测试 ==========

TEST_F(DocumentTest, ObserverNodeInserted) {
    MockDOMObserver observer;
    doc_->AddObserver(&observer);

    auto body = doc_->GetBody();
    auto elem = doc_->CreateElement("div");
    body->AppendChild(elem);

    EXPECT_GE(observer.GetAddCount(), 1);

    doc_->RemoveObserver(&observer);
}

TEST_F(DocumentTest, ObserverNodeRemoved) {
    MockDOMObserver observer;

    auto body = doc_->GetBody();
    auto elem = doc_->CreateElement("div");
    body->AppendChild(elem);

    doc_->AddObserver(&observer);
    body->RemoveChild(elem);

    EXPECT_GE(observer.GetRemoveCount(), 1);

    doc_->RemoveObserver(&observer);
}

// ========== 脏区域管理测试 ==========

TEST_F(DocumentTest, AddDirtyRect) {
    doc_->ClearDirtyRects();

    SkRect rect1 = SkRect::MakeXYWH(0, 0, 100, 100);
    SkRect rect2 = SkRect::MakeXYWH(50, 50, 100, 100);

    doc_->AddDirtyRect(rect1);
    doc_->AddDirtyRect(rect2);

    EXPECT_EQ(doc_->GetDirtyRects().size(), 2);
}

TEST_F(DocumentTest, GetMergedDirtyRect) {
    doc_->ClearDirtyRects();

    doc_->AddDirtyRect(SkRect::MakeXYWH(0, 0, 100, 100));
    doc_->AddDirtyRect(SkRect::MakeXYWH(200, 200, 100, 100));

    auto merged = doc_->GetMergedDirtyRect();

    // 合并后应该包含两个矩形
    EXPECT_LE(merged.left(), 0);
    EXPECT_LE(merged.top(), 0);
    EXPECT_GE(merged.right(), 300);
    EXPECT_GE(merged.bottom(), 300);
}

TEST_F(DocumentTest, ClearDirtyRects) {
    doc_->AddDirtyRect(SkRect::MakeXYWH(0, 0, 100, 100));
    doc_->ClearDirtyRects();

    EXPECT_EQ(doc_->GetDirtyRects().size(), 0);
}

// ========== 资源路径测试 ==========

TEST_F(DocumentTest, SetAndGetBasePath) {
    doc_->SetBasePath("/path/to/html");
    EXPECT_EQ(doc_->GetBasePath(), "/path/to/html");
}

TEST_F(DocumentTest, ResolvePath) {
    doc_->SetBasePath("/path/to/html");

    auto resolved = doc_->ResolvePath("script.js");
    EXPECT_TRUE(resolved.find("script.js") != std::string::npos);
}

} // namespace test
} // namespace mbink
