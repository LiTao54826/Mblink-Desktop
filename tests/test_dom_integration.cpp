/**
 * @file test_dom_integration.cpp
 * @brief DOM API 集成测试
 */

#include <gtest/gtest.h>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/event.h"

using namespace lightui;

// ========== 完整 DOM 树构建测试 ==========

TEST(DOMIntegrationTest, CompleteDocumentTree) {
    // 创建一个完整的 HTML 文档结构
    auto doc = std::make_shared<Document>();
    
    // <html>
    auto html = doc->CreateElement("html");
    
    // <head>
    auto head = doc->CreateElement("head");
    auto title = doc->CreateElement("title");
    auto titleText = doc->CreateTextNode("Test Page");
    title->AppendChild(titleText);
    head->AppendChild(title);
    
    // <body>
    auto body = doc->CreateElement("body");
    body->SetAttribute("id", "main-body");
    
    // <div class="container">
    auto container = doc->CreateElement("div");
    container->AddClass("container");
    
    // <h1>Hello World</h1>
    auto h1 = doc->CreateElement("h1");
    auto h1Text = doc->CreateTextNode("Hello World");
    h1->AppendChild(h1Text);
    
    // <p id="intro">Welcome to the test</p>
    auto p = doc->CreateElement("p");
    p->SetAttribute("id", "intro");
    auto pText = doc->CreateTextNode("Welcome to the test");
    p->AppendChild(pText);
    
    // 组装 DOM 树
    container->AppendChild(h1);
    container->AppendChild(p);
    body->AppendChild(container);
    html->AppendChild(head);
    html->AppendChild(body);
    
    // 验证结构
    EXPECT_EQ(html->GetChildNodes().size(), 2);
    EXPECT_EQ(body->GetChildNodes().size(), 1);
    EXPECT_EQ(container->GetChildNodes().size(), 2);
    
    // 验证查询
    auto foundP = body->QuerySelector("#intro");
    EXPECT_EQ(foundP, p);
    
    auto foundH1 = body->QuerySelector("h1");
    EXPECT_EQ(foundH1, h1);
    
    auto allDivs = html->QuerySelectorAll("div");
    EXPECT_EQ(allDivs.size(), 1);
    EXPECT_EQ(allDivs[0], container);
}

TEST(DOMIntegrationTest, DynamicTreeModification) {
    auto root = std::make_shared<Element>("div");
    
    // 动态添加子节点
    for (int i = 0; i < 5; i++) {
        auto child = std::make_shared<Element>("span");
        child->SetAttribute("id", "item-" + std::to_string(i));
        child->AddClass("item");
        auto text = std::make_shared<Text>("Item " + std::to_string(i));
        child->AppendChild(text);
        root->AppendChild(child);
    }
    
    EXPECT_EQ(root->GetChildNodes().size(), 5);
    
    // 查询所有 item
    auto items = root->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 5);
    
    // 移除中间的元素
    auto item2 = root->QuerySelector("#item-2");
    EXPECT_NE(item2, nullptr);
    root->RemoveChild(item2);
    
    EXPECT_EQ(root->GetChildNodes().size(), 4);
    
    // 插入新元素
    auto newItem = std::make_shared<Element>("span");
    newItem->SetAttribute("id", "new-item");
    newItem->AddClass("item");
    root->InsertBefore(newItem, root->GetChildNodes()[2]);
    
    EXPECT_EQ(root->GetChildNodes().size(), 5);
    EXPECT_EQ(root->GetChildNodes()[2], newItem);
}

// ========== 事件传播集成测试 ==========

TEST(DOMIntegrationTest, EventBubblingThroughTree) {
    // 创建三层嵌套结构
    auto grandparent = std::make_shared<Element>("div");
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("button");
    
    grandparent->SetAttribute("id", "grandparent");
    parent->SetAttribute("id", "parent");
    child->SetAttribute("id", "child");
    
    parent->AppendChild(child);
    grandparent->AppendChild(parent);
    
    // 记录事件触发顺序
    std::vector<std::string> eventLog;
    
    grandparent->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("grandparent");
    });
    
    parent->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("parent");
    });
    
    child->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("child");
    });
    
    // 在 child 上触发事件
    auto event = std::make_shared<Event>("click", true, false);
    child->DispatchEvent(event);
    
    // 验证事件传播顺序：child -> parent -> grandparent
    ASSERT_EQ(eventLog.size(), 3);
    EXPECT_EQ(eventLog[0], "child");
    EXPECT_EQ(eventLog[1], "parent");
    EXPECT_EQ(eventLog[2], "grandparent");
}

TEST(DOMIntegrationTest, EventStopPropagation) {
    auto grandparent = std::make_shared<Element>("div");
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("button");
    
    parent->AppendChild(child);
    grandparent->AppendChild(parent);
    
    std::vector<std::string> eventLog;
    
    grandparent->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("grandparent");
    });
    
    parent->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("parent");
        e->StopPropagation();  // 停止传播
    });
    
    child->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        eventLog.push_back("child");
    });
    
    auto event = std::make_shared<Event>("click", true, false);
    child->DispatchEvent(event);
    
    // 验证事件在 parent 停止
    ASSERT_EQ(eventLog.size(), 2);
    EXPECT_EQ(eventLog[0], "child");
    EXPECT_EQ(eventLog[1], "parent");
}

TEST(DOMIntegrationTest, MultipleEventTypes) {
    auto element = std::make_shared<Element>("button");
    
    int clickCount = 0;
    int mouseoverCount = 0;
    
    element->AddEventListener("click", [&](std::shared_ptr<Event> e) {
        clickCount++;
    });
    
    element->AddEventListener("mouseover", [&](std::shared_ptr<Event> e) {
        mouseoverCount++;
    });
    
    // 触发不同类型的事件
    auto clickEvent = std::make_shared<Event>("click", true, false);
    element->DispatchEvent(clickEvent);
    element->DispatchEvent(clickEvent);
    
    auto mouseoverEvent = std::make_shared<Event>("mouseover", true, false);
    element->DispatchEvent(mouseoverEvent);
    
    EXPECT_EQ(clickCount, 2);
    EXPECT_EQ(mouseoverCount, 1);
}

// ========== 复杂查询测试 ==========

TEST(DOMIntegrationTest, ComplexQueryScenario) {
    auto root = std::make_shared<Element>("div");
    
    // 创建复杂的 DOM 结构
    for (int i = 0; i < 3; i++) {
        auto section = std::make_shared<Element>("section");
        section->AddClass("section");
        section->SetAttribute("data-index", std::to_string(i));
        
        for (int j = 0; j < 3; j++) {
            auto item = std::make_shared<Element>("div");
            item->AddClass("item");
            if (j == 0) {
                item->AddClass("first");
            }
            item->SetAttribute("id", "item-" + std::to_string(i) + "-" + std::to_string(j));
            
            auto text = std::make_shared<Text>("Section " + std::to_string(i) + " Item " + std::to_string(j));
            item->AppendChild(text);
            section->AppendChild(item);
        }
        
        root->AppendChild(section);
    }
    
    // 查询所有 section
    auto sections = root->QuerySelectorAll(".section");
    EXPECT_EQ(sections.size(), 3);
    
    // 查询所有 item
    auto items = root->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 9);
    
    // 查询所有 first
    auto firstItems = root->QuerySelectorAll(".first");
    EXPECT_EQ(firstItems.size(), 3);
    
    // 查询特定 ID
    auto specificItem = root->QuerySelector("#item-1-2");
    EXPECT_NE(specificItem, nullptr);
    EXPECT_EQ(specificItem->GetTextContent(), "Section 1 Item 2");
    
    // 使用 Closest 查找祖先
    auto closestSection = specificItem->Closest(".section");
    EXPECT_NE(closestSection, nullptr);
    EXPECT_EQ(closestSection->GetAttribute("data-index"), "1");
}

// ========== 内存管理测试 ==========

TEST(DOMIntegrationTest, NodeReparenting) {
    auto parent1 = std::make_shared<Element>("div");
    auto parent2 = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    // 添加到 parent1
    parent1->AppendChild(child);
    EXPECT_EQ(parent1->GetChildNodes().size(), 1);
    EXPECT_EQ(parent2->GetChildNodes().size(), 0);
    EXPECT_EQ(child->GetParentNode(), parent1);
    
    // 移动到 parent2
    parent2->AppendChild(child);
    EXPECT_EQ(parent1->GetChildNodes().size(), 0);
    EXPECT_EQ(parent2->GetChildNodes().size(), 1);
    EXPECT_EQ(child->GetParentNode(), parent2);
}

TEST(DOMIntegrationTest, DeepClone) {
    auto original = std::make_shared<Element>("div");
    original->SetAttribute("id", "original");
    original->AddClass("container");
    
    auto child1 = std::make_shared<Element>("span");
    child1->SetAttribute("id", "child1");
    auto text1 = std::make_shared<Text>("Text 1");
    child1->AppendChild(text1);
    
    auto child2 = std::make_shared<Element>("span");
    child2->SetAttribute("id", "child2");
    auto text2 = std::make_shared<Text>("Text 2");
    child2->AppendChild(text2);
    
    original->AppendChild(child1);
    original->AppendChild(child2);
    
    // 深度克隆
    auto cloned = std::dynamic_pointer_cast<Element>(original->CloneNode(true));
    
    EXPECT_NE(cloned, original);
    EXPECT_EQ(cloned->GetAttribute("id"), "original");
    EXPECT_TRUE(cloned->HasClass("container"));
    EXPECT_EQ(cloned->GetChildNodes().size(), 2);
    
    // 验证子节点也被克隆
    auto clonedChild1 = std::dynamic_pointer_cast<Element>(cloned->GetChildNodes()[0]);
    EXPECT_NE(clonedChild1, child1);
    EXPECT_EQ(clonedChild1->GetAttribute("id"), "child1");
    EXPECT_EQ(clonedChild1->GetTextContent(), "Text 1");
}

// ========== innerHTML 集成测试 ==========

TEST(DOMIntegrationTest, InnerHTMLRoundTrip) {
    auto parent = std::make_shared<Element>("div");
    
    auto child1 = std::make_shared<Element>("h1");
    child1->SetAttribute("id", "title");
    auto text1 = std::make_shared<Text>("Title");
    child1->AppendChild(text1);
    
    auto child2 = std::make_shared<Element>("p");
    child2->AddClass("content");
    auto text2 = std::make_shared<Text>("Paragraph");
    child2->AppendChild(text2);
    
    parent->AppendChild(child1);
    parent->AppendChild(child2);
    
    // 获取 innerHTML
    std::string html = parent->GetInnerHTML();
    
    // 验证包含关键内容
    EXPECT_TRUE(html.find("<h1") != std::string::npos);
    EXPECT_TRUE(html.find("id=\"title\"") != std::string::npos);
    EXPECT_TRUE(html.find("Title") != std::string::npos);
    EXPECT_TRUE(html.find("<p") != std::string::npos);
    EXPECT_TRUE(html.find("class=\"content\"") != std::string::npos);
    EXPECT_TRUE(html.find("Paragraph") != std::string::npos);
}

