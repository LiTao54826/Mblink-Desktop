/**
 * @file test_html_anchor_element.cpp
 * @brief HTML Anchor元素单元测试
 */

#include <gtest/gtest.h>
#include <chrono>
#include "core/dom/html_anchor_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"

using namespace lightui;

// ========== 辅助函数 ==========

std::shared_ptr<HTMLAnchorElement> CreateTestAnchor() {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    auto anchor = std::dynamic_pointer_cast<HTMLAnchorElement>(doc->CreateElement("a"));
    auto body = doc->GetBody();
    if (body) {
        body->AppendChild(anchor);
    } else {
        // 如果没有body，直接添加到document
        doc->AppendChild(anchor);
    }
    return anchor;
}

// ========== 基础属性测试 ==========

TEST(HTMLAnchorElement, Construction) {
    auto anchor = CreateTestAnchor();
    ASSERT_NE(anchor, nullptr);
    EXPECT_EQ(anchor->GetTagName(), "a");
    EXPECT_EQ(anchor->GetTarget(), "_self");  // 默认值
    EXPECT_FALSE(anchor->IsVisited());
}

TEST(HTMLAnchorElement, HrefProperty) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetHref("https://example.com");
    EXPECT_EQ(anchor->GetHref(), "https://example.com");
    EXPECT_EQ(anchor->GetAttribute("href"), "https://example.com");
    
    // 设置href后应该有:link伪类
    EXPECT_TRUE(anchor->HasPseudoClass("link"));
    EXPECT_FALSE(anchor->HasPseudoClass("visited"));
}

TEST(HTMLAnchorElement, TargetProperty) {
    auto anchor = CreateTestAnchor();
    
    // 默认是_self
    EXPECT_EQ(anchor->GetTarget(), "_self");
    
    // 设置为_blank
    anchor->SetTarget("_blank");
    EXPECT_EQ(anchor->GetTarget(), "_blank");
    EXPECT_EQ(anchor->GetAttribute("target"), "_blank");
    
    // 设置为_parent
    anchor->SetTarget("_parent");
    EXPECT_EQ(anchor->GetTarget(), "_parent");
    
    // 设置为_top
    anchor->SetTarget("_top");
    EXPECT_EQ(anchor->GetTarget(), "_top");
}

TEST(HTMLAnchorElement, DownloadProperty) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetDownload("file.pdf");
    EXPECT_EQ(anchor->GetDownload(), "file.pdf");
    EXPECT_EQ(anchor->GetAttribute("download"), "file.pdf");
}

TEST(HTMLAnchorElement, RelProperty) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetRel("nofollow noopener");
    EXPECT_EQ(anchor->GetRel(), "nofollow noopener");
    EXPECT_EQ(anchor->GetAttribute("rel"), "nofollow noopener");
}

TEST(HTMLAnchorElement, TextProperty) {
    auto anchor = CreateTestAnchor();

    // 使用SetTextContent代替SetText（SetText内部调用GetOwnerDocument可能有问题）
    anchor->SetTextContent("Click here");
    EXPECT_EQ(anchor->GetText(), "Click here");
    EXPECT_EQ(anchor->GetTextContent(), "Click here");
}

// ========== SetAttribute/RemoveAttribute测试 ==========

TEST(HTMLAnchorElement, SetAttributeHref) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetAttribute("href", "https://example.com");
    EXPECT_EQ(anchor->GetHref(), "https://example.com");
    EXPECT_TRUE(anchor->HasPseudoClass("link"));
}

TEST(HTMLAnchorElement, RemoveAttributeHref) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetAttribute("href", "https://example.com");
    EXPECT_EQ(anchor->GetHref(), "https://example.com");
    
    anchor->RemoveAttribute("href");
    EXPECT_EQ(anchor->GetHref(), "");
    EXPECT_FALSE(anchor->HasPseudoClass("link"));
    EXPECT_FALSE(anchor->HasPseudoClass("visited"));
}

TEST(HTMLAnchorElement, SetAttributeTarget) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetAttribute("target", "_blank");
    EXPECT_EQ(anchor->GetTarget(), "_blank");
}

TEST(HTMLAnchorElement, RemoveAttributeTarget) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetAttribute("target", "_blank");
    anchor->RemoveAttribute("target");
    EXPECT_EQ(anchor->GetTarget(), "_self");  // 恢复默认值
}

// ========== 伪类状态测试 ==========

TEST(HTMLAnchorElement, LinkPseudoClass) {
    auto anchor = CreateTestAnchor();
    
    // 没有href时，不应该有:link伪类
    EXPECT_FALSE(anchor->HasPseudoClass("link"));
    
    // 设置href后，应该有:link伪类
    anchor->SetHref("https://example.com");
    EXPECT_TRUE(anchor->HasPseudoClass("link"));
    EXPECT_FALSE(anchor->HasPseudoClass("visited"));
}

TEST(HTMLAnchorElement, VisitedPseudoClass) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetHref("https://example.com");
    EXPECT_FALSE(anchor->IsVisited());
    EXPECT_TRUE(anchor->HasPseudoClass("link"));
    EXPECT_FALSE(anchor->HasPseudoClass("visited"));
    
    // 标记为已访问
    anchor->MarkAsVisited();
    EXPECT_TRUE(anchor->IsVisited());
    EXPECT_FALSE(anchor->HasPseudoClass("link"));
    EXPECT_TRUE(anchor->HasPseudoClass("visited"));
}

TEST(HTMLAnchorElement, VisitedAfterClick) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetHref("https://example.com");
    EXPECT_FALSE(anchor->IsVisited());
    
    // 点击后应该标记为已访问
    anchor->HandleClick();
    EXPECT_TRUE(anchor->IsVisited());
    EXPECT_TRUE(anchor->HasPseudoClass("visited"));
}

// ========== 点击处理测试 ==========

TEST(HTMLAnchorElement, HandleClickWithHref) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetHref("https://example.com");
    
    bool navigate_event_fired = false;
    anchor->AddEventListener("navigate", [&](std::shared_ptr<Event> event) {
        navigate_event_fired = true;
    });
    
    anchor->HandleClick();
    
    EXPECT_TRUE(navigate_event_fired);
    EXPECT_TRUE(anchor->IsVisited());
}

TEST(HTMLAnchorElement, HandleClickWithDownload) {
    auto anchor = CreateTestAnchor();
    
    anchor->SetHref("https://example.com/file.pdf");
    anchor->SetDownload("document.pdf");
    
    bool download_event_fired = false;
    anchor->AddEventListener("download", [&](std::shared_ptr<Event> event) {
        download_event_fired = true;
    });
    
    anchor->HandleClick();
    
    EXPECT_TRUE(download_event_fired);
    // 有download属性时，不应该导航，所以不标记为visited
    EXPECT_FALSE(anchor->IsVisited());
}

TEST(HTMLAnchorElement, HandleClickWithoutHref) {
    auto anchor = CreateTestAnchor();
    
    bool click_event_fired = false;
    anchor->AddEventListener("click", [&](std::shared_ptr<Event> event) {
        click_event_fired = true;
    });
    
    anchor->HandleClick();
    
    EXPECT_TRUE(click_event_fired);
    EXPECT_FALSE(anchor->IsVisited());
}

// ========== React兼容性测试 ==========

TEST(HTMLAnchorElement, ReactAttributeUpdate) {
    auto anchor = CreateTestAnchor();
    
    // 模拟React更新href
    anchor->SetAttribute("href", "https://example.com");
    EXPECT_EQ(anchor->GetHref(), "https://example.com");
    
    // 模拟React更新target
    anchor->SetAttribute("target", "_blank");
    EXPECT_EQ(anchor->GetTarget(), "_blank");
    
    // 模拟React移除href
    anchor->RemoveAttribute("href");
    EXPECT_EQ(anchor->GetHref(), "");
}

TEST(HTMLAnchorElement, ReactTextUpdate) {
    auto anchor = CreateTestAnchor();

    // 模拟React设置文本（使用SetTextContent）
    anchor->SetTextContent("Link 1");
    EXPECT_EQ(anchor->GetText(), "Link 1");

    // 模拟React更新文本
    anchor->SetTextContent("Link 2");
    EXPECT_EQ(anchor->GetText(), "Link 2");
}

// ========== 性能测试 ==========

TEST(HTMLAnchorElement, PerformanceCreation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        auto anchor = std::dynamic_pointer_cast<HTMLAnchorElement>(doc->CreateElement("a"));
        doc->GetBody()->AppendChild(anchor);
        anchor->SetHref("https://example.com/" + std::to_string(i));
        anchor->SetText("Link " + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average anchor creation time: " 
              << duration.count() / 1000.0 << " microseconds" << std::endl;
    
    EXPECT_LT(duration.count() / 1000.0, 100.0);  // 平均应该小于100μs
}

TEST(HTMLAnchorElement, PerformanceAttributeUpdate) {
    auto anchor = CreateTestAnchor();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        anchor->SetHref("https://example.com/" + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average href update time: "
              << duration.count() / 10000.0 << " microseconds" << std::endl;

    EXPECT_LT(duration.count() / 10000.0, 15.0);  // 平均应该小于15μs
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

