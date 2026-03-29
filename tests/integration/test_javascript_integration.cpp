/**
 * @file test_javascript_integration.cpp
 * @brief JavaScript 集成测试
 * 
 * 注意：这些测试需要 QuickJS 运行时支持
 * 目前只测试基本的 DOM 操作，不涉及实际的 JS 执行
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"

namespace mbink {
namespace test {

class JavaScriptIntegrationTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
    }

    void TearDown() override {
        DOMTestBase::TearDown();
    }
};

// ========== DOM 操作测试（模拟 JS 操作） ==========

TEST_F(JavaScriptIntegrationTest, CreateAndAppendElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 模拟 JS: document.createElement('div')
    auto div = doc->CreateElement("div");
    
    // 模拟 JS: div.id = 'js-created'
    div->SetAttribute("id", "js-created");
    
    // 模拟 JS: div.className = 'test-class'
    div->SetClassName("test-class");
    
    // 模拟 JS: div.textContent = 'Created by JS'
    div->SetTextContent("Created by JS");
    
    // 模拟 JS: document.body.appendChild(div)
    body->AppendChild(div);
    
    // 验证
    auto elem = doc->GetElementById("js-created");
    EXPECT_NE(elem, nullptr);
    EXPECT_TRUE(elem->HasClass("test-class"));
    EXPECT_EQ(elem->GetTextContent(), "Created by JS");
}

TEST_F(JavaScriptIntegrationTest, ModifyExistingElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 先创建元素
    auto div = doc->CreateElement("div");
    div->SetAttribute("id", "existing");
    body->AppendChild(div);
    
    // 模拟 JS 修改
    // var elem = document.getElementById('existing');
    auto elem = doc->GetElementById("existing");
    ASSERT_NE(elem, nullptr);
    
    // elem.className = 'modified';
    elem->SetClassName("modified");
    
    // elem.setAttribute('data-value', '123');
    elem->SetAttribute("data-value", "123");
    
    // elem.style.color = 'red';
    elem->SetStyle("color", "red");
    
    // 验证
    EXPECT_TRUE(elem->HasClass("modified"));
    EXPECT_EQ(elem->GetAttribute("data-value"), "123");
    EXPECT_EQ(elem->GetStyle("color"), "red");
}

TEST_F(JavaScriptIntegrationTest, RemoveElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto div = doc->CreateElement("div");
    div->SetAttribute("id", "to-remove");
    body->AppendChild(div);
    
    // 验证元素存在
    EXPECT_NE(doc->GetElementById("to-remove"), nullptr);
    
    // 模拟 JS: elem.parentNode.removeChild(elem)
    body->RemoveChild(div);
    
    // 验证元素已移除
    EXPECT_EQ(doc->GetElementById("to-remove"), nullptr);
}

// ========== 查询选择器测试 ==========

TEST_F(JavaScriptIntegrationTest, QuerySelector) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto container = doc->CreateElement("div");
    container->AddClass("container");
    body->AppendChild(container);
    
    for (int i = 0; i < 3; i++) {
        auto item = doc->CreateElement("span");
        item->AddClass("item");
        container->AppendChild(item);
    }
    
    // 模拟 JS: document.querySelectorAll('.item')
    auto items = body->QuerySelectorAll(".item");
    EXPECT_EQ(items.size(), 3);
}

TEST_F(JavaScriptIntegrationTest, QuerySelectorWithModification) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto list = doc->CreateElement("ul");
    list->SetAttribute("id", "list");
    body->AppendChild(list);
    
    for (int i = 0; i < 2; i++) {
        auto li = doc->CreateElement("li");
        li->SetTextContent("Item " + std::to_string(i));
        list->AppendChild(li);
    }
    
    // 模拟 JS: items.forEach((item, index) => item.className = 'item-' + index)
    auto items = list->QuerySelectorAll("li");
    for (size_t i = 0; i < items.size(); i++) {
        items[i]->SetClassName("item-" + std::to_string(i));
    }
    
    EXPECT_EQ(items[0]->GetClassName(), "item-0");
    EXPECT_EQ(items[1]->GetClassName(), "item-1");
}

// ========== innerHTML 测试 ==========

TEST_F(JavaScriptIntegrationTest, SetInnerHTML) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    auto container = doc->CreateElement("div");
    container->SetAttribute("id", "container");
    body->AppendChild(container);
    
    // 模拟 JS: container.innerHTML = '<p>Paragraph 1</p><p>Paragraph 2</p>'
    container->SetInnerHTML("<p>Paragraph 1</p><p>Paragraph 2</p>");
    
    auto paragraphs = container->QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 2);
}

TEST_F(JavaScriptIntegrationTest, GetInnerHTML) {
    auto doc = CreateDocument();
    auto container = doc->CreateElement("div");
    
    auto p = doc->CreateElement("p");
    p->SetTextContent("Test");
    container->AppendChild(p);
    
    // 模拟 JS: container.innerHTML
    auto html = container->GetInnerHTML();
    EXPECT_TRUE(html.find("p") != std::string::npos);
    EXPECT_TRUE(html.find("Test") != std::string::npos);
}

// ========== classList 测试 ==========

TEST_F(JavaScriptIntegrationTest, ClassListOperations) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    // 模拟 JS: elem.classList.add('foo')
    div->AddClass("foo");
    
    // 模拟 JS: elem.classList.add('bar')
    div->AddClass("bar");
    
    // 模拟 JS: elem.classList.toggle('baz')
    div->ToggleClass("baz");
    
    EXPECT_TRUE(div->HasClass("foo"));
    EXPECT_TRUE(div->HasClass("bar"));
    EXPECT_TRUE(div->HasClass("baz"));
    
    // 模拟 JS: elem.classList.remove('bar')
    div->RemoveClass("bar");
    
    // 模拟 JS: elem.classList.toggle('baz')
    div->ToggleClass("baz");
    
    EXPECT_TRUE(div->HasClass("foo"));
    EXPECT_FALSE(div->HasClass("bar"));
    EXPECT_FALSE(div->HasClass("baz"));
}

// ========== style 操作测试 ==========

TEST_F(JavaScriptIntegrationTest, StyleOperations) {
    auto doc = CreateDocument();
    auto div = doc->CreateElement("div");
    
    // 模拟 JS: elem.style.width = '100px'
    div->SetStyle("width", "100px");
    
    // 模拟 JS: elem.style.height = '50px'
    div->SetStyle("height", "50px");
    
    // 模拟 JS: elem.style.backgroundColor = 'red'
    div->SetStyle("background-color", "red");
    
    EXPECT_EQ(div->GetStyle("width"), "100px");
    EXPECT_EQ(div->GetStyle("height"), "50px");
    EXPECT_EQ(div->GetStyle("background-color"), "red");
}

// ========== 复杂交互测试 ==========

TEST_F(JavaScriptIntegrationTest, TodoAppSimulation) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();
    
    // 创建列表
    auto list = doc->CreateElement("ul");
    list->SetAttribute("id", "list");
    body->AppendChild(list);
    
    // 模拟添加几个项目
    std::vector<std::string> tasks = {"Task 1", "Task 2", "Task 3"};
    for (const auto& task : tasks) {
        auto li = doc->CreateElement("li");
        li->SetTextContent(task);
        list->AppendChild(li);
    }
    
    auto items = list->QuerySelectorAll("li");
    EXPECT_EQ(items.size(), 3);
    EXPECT_EQ(items[0]->GetTextContent(), "Task 1");
    EXPECT_EQ(items[1]->GetTextContent(), "Task 2");
    EXPECT_EQ(items[2]->GetTextContent(), "Task 3");
}

} // namespace test
} // namespace mbink
