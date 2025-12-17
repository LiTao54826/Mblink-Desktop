/**
 * @file test_dom_bindings.cpp
 * @brief DOM JavaScript 绑定单元测试
 */

#include <gtest/gtest.h>
#include "quickjs/quickjs_runtime.h"
#include "dom/document.h"
#include "dom/dom_bindings.h"

namespace lightui {
namespace test {

class DOMBindingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime_ = std::make_unique<QuickJSRuntime>();
        doc_ = std::make_shared<Document>();
        doc_->Initialize();

        // 初始化 DOM 绑定系统
        DOMBindings::Init(runtime_->GetContext());
        
        // 设置全局 document 对象
        DOMBindings::SetGlobalDocument(runtime_->GetContext(), doc_);
    }

    void TearDown() override {
        DOMBindings::Cleanup(runtime_->GetContext());
        doc_.reset();
        runtime_.reset();
    }

protected:
    std::unique_ptr<QuickJSRuntime> runtime_;
    std::shared_ptr<Document> doc_;
};

// ========== document 对象测试 ==========

TEST_F(DOMBindingsTest, DocumentExists) {
    auto result = runtime_->Eval("typeof document");
    EXPECT_EQ(result, "object");
}

TEST_F(DOMBindingsTest, DocumentBody) {
    auto result = runtime_->Eval("document.body !== null");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentHead) {
    auto result = runtime_->Eval("document.head !== null");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentDocumentElement) {
    auto result = runtime_->Eval("document.documentElement !== null");
    EXPECT_EQ(result, true);
}

// ========== createElement 测试 ==========

TEST_F(DOMBindingsTest, CreateElement) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.tagName.toLowerCase();
    )");
    EXPECT_EQ(result, "div");
}

TEST_F(DOMBindingsTest, CreateTextNode) {
    auto result = runtime_->Eval(R"(
        var text = document.createTextNode('Hello');
        text.textContent;
    )");
    EXPECT_EQ(result, "Hello");
}

// ========== 元素属性测试 ==========

TEST_F(DOMBindingsTest, SetAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('id', 'test');
        div.getAttribute('id');
    )");
    EXPECT_EQ(result, "test");
}

TEST_F(DOMBindingsTest, HasAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('class', 'foo');
        div.hasAttribute('class');
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, RemoveAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('id', 'test');
        div.removeAttribute('id');
        div.hasAttribute('id');
    )");
    EXPECT_EQ(result, false);
}

// ========== className 和 classList 测试 ==========

TEST_F(DOMBindingsTest, ClassName) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.className = 'foo bar';
        div.className;
    )");
    EXPECT_EQ(result, "foo bar");
}

TEST_F(DOMBindingsTest, ClassListAdd) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.add('active');
        div.classList.contains('active');
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, ClassListRemove) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.add('active');
        div.classList.remove('active');
        div.classList.contains('active');
    )");
    EXPECT_EQ(result, false);
}

TEST_F(DOMBindingsTest, ClassListToggle) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.toggle('active');
        var first = div.classList.contains('active');
        div.classList.toggle('active');
        var second = div.classList.contains('active');
        first && !second;
    )");
    EXPECT_EQ(result, true);
}

// ========== style 测试 ==========

TEST_F(DOMBindingsTest, StyleProperty) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.style.color = 'red';
        div.style.color;
    )");
    EXPECT_EQ(result, "red");
}

TEST_F(DOMBindingsTest, StyleSetProperty) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.style.setProperty('background-color', 'blue');
        div.style.getPropertyValue('background-color');
    )");
    EXPECT_EQ(result, "blue");
}

// ========== DOM 操作测试 ==========

TEST_F(DOMBindingsTest, AppendChild) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        parent.children.length;
    )");
    EXPECT_EQ(result, 1);
}

TEST_F(DOMBindingsTest, RemoveChild) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        parent.removeChild(child);
        parent.children.length;
    )");
    EXPECT_EQ(result, 0);
}

TEST_F(DOMBindingsTest, InsertBefore) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child1 = document.createElement('span');
        var child2 = document.createElement('p');
        parent.appendChild(child2);
        parent.insertBefore(child1, child2);
        parent.firstChild.tagName.toLowerCase();
    )");
    EXPECT_EQ(result, "span");
}

// ========== 查询选择器测试 ==========

TEST_F(DOMBindingsTest, QuerySelector) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        span.id = 'target';
        div.appendChild(span);
        document.body.appendChild(div);
        var found = document.querySelector('#target');
        found !== null;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, QuerySelectorAll) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span></span><span></span><span></span>';
        document.body.appendChild(div);
        document.querySelectorAll('span').length;
    )");
    EXPECT_GE(result.get<int>(), 3);
}

TEST_F(DOMBindingsTest, GetElementById) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.id = 'myDiv';
        document.body.appendChild(div);
        document.getElementById('myDiv') !== null;
    )");
    EXPECT_EQ(result, true);
}

// ========== 事件监听器测试 ==========

TEST_F(DOMBindingsTest, AddEventListener) {
    auto result = runtime_->Eval(R"(
        var clicked = false;
        var div = document.createElement('div');
        div.addEventListener('click', function() {
            clicked = true;
        });
        // 模拟点击
        var event = new Event('click');
        div.dispatchEvent(event);
        clicked;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, RemoveEventListener) {
    auto result = runtime_->Eval(R"(
        var count = 0;
        var div = document.createElement('div');
        var handler = function() { count++; };
        div.addEventListener('click', handler);
        div.removeEventListener('click', handler);
        div.dispatchEvent(new Event('click'));
        count;
    )");
    EXPECT_EQ(result, 0);
}

// ========== innerHTML/textContent 测试 ==========

TEST_F(DOMBindingsTest, InnerHTML) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span>Hello</span>';
        div.children.length;
    )");
    EXPECT_EQ(result, 1);
}

TEST_F(DOMBindingsTest, TextContent) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span>Hello</span> <span>World</span>';
        div.textContent;
    )");
    EXPECT_EQ(result, "Hello World");
}

// ========== 节点属性测试 ==========

TEST_F(DOMBindingsTest, ParentNode) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        child.parentNode === parent;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, ChildNodes) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.appendChild(document.createElement('span'));
        div.appendChild(document.createTextNode('text'));
        div.childNodes.length;
    )");
    EXPECT_EQ(result, 2);
}

TEST_F(DOMBindingsTest, FirstChild) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        div.appendChild(span);
        div.firstChild === span;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, LastChild) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.appendChild(document.createElement('span'));
        var p = document.createElement('p');
        div.appendChild(p);
        div.lastChild === p;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, NextSibling) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        var p = document.createElement('p');
        div.appendChild(span);
        div.appendChild(p);
        span.nextSibling === p;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, PreviousSibling) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        var p = document.createElement('p');
        div.appendChild(span);
        div.appendChild(p);
        p.previousSibling === span;
    )");
    EXPECT_EQ(result, true);
}

} // namespace test
} // namespace lightui
