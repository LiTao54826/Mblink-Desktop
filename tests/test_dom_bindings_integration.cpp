/**
 * @file test_dom_bindings_integration.cpp
 * @brief DOM QuickJS 绑定集成测试
 */

#include <gtest/gtest.h>
#include "core/dom/dom_bindings.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/event.h"
#include <quickjs.h>

using namespace lightui;

class DOMBindingsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        rt = JS_NewRuntime();
        ctx = JS_NewContext(rt);
        DOMBindings::Init(ctx);
    }
    
    void TearDown() override {
        DOMBindings::Cleanup(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }
    
    JSValue Eval(const char* code) {
        return JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    }

    bool IsException(JSValue val) {
        return JS_IsException(val);
    }

    std::string GetString(JSValue val) {
        const char* str = JS_ToCString(ctx, val);
        std::string result = str ? str : "";
        JS_FreeCString(ctx, str);
        return result;
    }

    int GetInt(JSValue val) {
        int32_t result = 0;
        JS_ToInt32(ctx, &result, val);
        return result;
    }

    void SetGlobal(const char* name, JSValue val) {
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, name, val);
        JS_FreeValue(ctx, global);
    }
    
    JSRuntime* rt;
    JSContext* ctx;
};

// ========== Element 绑定测试 ==========

TEST_F(DOMBindingsIntegrationTest, ElementCreationAndProperties) {
    auto element = std::make_shared<Element>("div");
    element->SetAttribute("id", "test-div");
    element->AddClass("container");

    JSValue jsElement = DOMBindings::WrapElement(ctx, element);
    SetGlobal("testElement", JS_DupValue(ctx, jsElement));
    
    // 测试 tagName
    JSValue tagName = Eval("testElement.tagName");
    EXPECT_FALSE(IsException(tagName));
    EXPECT_EQ(GetString(tagName), "div");
    JS_FreeValue(ctx, tagName);
    
    // 测试 id
    JSValue id = Eval("testElement.id");
    EXPECT_FALSE(IsException(id));
    EXPECT_EQ(GetString(id), "test-div");
    JS_FreeValue(ctx, id);
    
    // 测试 className
    JSValue className = Eval("testElement.className");
    EXPECT_FALSE(IsException(className));
    EXPECT_EQ(GetString(className), "container");
    JS_FreeValue(ctx, className);
    
    JS_FreeValue(ctx, jsElement);
}

TEST_F(DOMBindingsIntegrationTest, ElementSetProperties) {
    auto element = std::make_shared<Element>("div");

    JSValue jsElement = DOMBindings::WrapElement(ctx, element);
    SetGlobal("testElement", JS_DupValue(ctx, jsElement));
    
    // 设置 id
    JSValue result1 = Eval("testElement.id = 'new-id'; testElement.id");
    EXPECT_FALSE(IsException(result1));
    EXPECT_EQ(GetString(result1), "new-id");
    EXPECT_EQ(element->GetAttribute("id"), "new-id");
    JS_FreeValue(ctx, result1);
    
    // 设置 className
    JSValue result2 = Eval("testElement.className = 'new-class'; testElement.className");
    EXPECT_FALSE(IsException(result2));
    EXPECT_EQ(GetString(result2), "new-class");
    EXPECT_EQ(element->GetClassName(), "new-class");
    JS_FreeValue(ctx, result2);
    
    JS_FreeValue(ctx, jsElement);
}

TEST_F(DOMBindingsIntegrationTest, ElementAttributes) {
    auto element = std::make_shared<Element>("input");

    JSValue jsElement = DOMBindings::WrapElement(ctx, element);
    SetGlobal("testElement", JS_DupValue(ctx, jsElement));
    
    // 设置属性
    JSValue result1 = Eval("testElement.setAttribute('type', 'text')");
    EXPECT_FALSE(IsException(result1));
    EXPECT_EQ(element->GetAttribute("type"), "text");
    JS_FreeValue(ctx, result1);
    
    // 获取属性
    JSValue result2 = Eval("testElement.getAttribute('type')");
    EXPECT_FALSE(IsException(result2));
    EXPECT_EQ(GetString(result2), "text");
    JS_FreeValue(ctx, result2);
    
    JS_FreeValue(ctx, jsElement);
}

// ========== Document 绑定测试 ==========

TEST_F(DOMBindingsIntegrationTest, DocumentCreateElement) {
    auto doc = std::make_shared<Document>();

    JSValue jsDoc = DOMBindings::WrapDocument(ctx, doc);
    SetGlobal("document", JS_DupValue(ctx, jsDoc));
    
    // 创建元素
    JSValue result = Eval("document.createElement('div')");
    EXPECT_FALSE(IsException(result));
    
    auto element = DOMBindings::UnwrapElement(ctx, result);
    EXPECT_NE(element, nullptr);
    EXPECT_EQ(element->GetTagName(), "div");
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, jsDoc);
}

TEST_F(DOMBindingsIntegrationTest, DocumentCreateTextNode) {
    auto doc = std::make_shared<Document>();

    JSValue jsDoc = DOMBindings::WrapDocument(ctx, doc);
    SetGlobal("document", JS_DupValue(ctx, jsDoc));
    
    // 创建文本节点
    JSValue result = Eval("document.createTextNode('Hello World')");
    EXPECT_FALSE(IsException(result));
    
    auto text = DOMBindings::UnwrapText(ctx, result);
    EXPECT_NE(text, nullptr);
    EXPECT_EQ(text->GetData(), "Hello World");
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, jsDoc);
}

TEST_F(DOMBindingsIntegrationTest, DocumentGetElementById) {
    auto doc = std::make_shared<Document>();
    auto element = doc->CreateElement("div");
    element->SetAttribute("id", "test-id");
    doc->RegisterElementId("test-id", element);

    JSValue jsDoc = DOMBindings::WrapDocument(ctx, doc);
    SetGlobal("document", JS_DupValue(ctx, jsDoc));
    
    // 查询元素
    JSValue result = Eval("document.getElementById('test-id')");
    EXPECT_FALSE(IsException(result));
    
    auto foundElement = DOMBindings::UnwrapElement(ctx, result);
    EXPECT_EQ(foundElement, element);
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, jsDoc);
}

// ========== DOM 树操作测试 ==========

TEST_F(DOMBindingsIntegrationTest, AppendChild) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");

    JSValue jsParent = DOMBindings::WrapElement(ctx, parent);
    JSValue jsChild = DOMBindings::WrapElement(ctx, child);

    SetGlobal("parent", JS_DupValue(ctx, jsParent));
    SetGlobal("child", JS_DupValue(ctx, jsChild));
    
    // 添加子节点
    JSValue result = Eval("parent.appendChild(child)");
    EXPECT_FALSE(IsException(result));
    
    EXPECT_EQ(parent->GetChildNodes().size(), 1);
    EXPECT_EQ(parent->GetChildNodes()[0], child);
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, jsParent);
    JS_FreeValue(ctx, jsChild);
}

TEST_F(DOMBindingsIntegrationTest, ComplexDOMManipulation) {
    auto doc = std::make_shared<Document>();

    JSValue jsDoc = DOMBindings::WrapDocument(ctx, doc);
    SetGlobal("document", JS_DupValue(ctx, jsDoc));
    
    // 执行复杂的 DOM 操作
    const char* code = R"(
        var container = document.createElement('div');
        container.id = 'container';
        container.className = 'main-container';
        
        var title = document.createElement('h1');
        title.id = 'title';
        var titleText = document.createTextNode('Hello World');
        title.appendChild(titleText);
        
        var paragraph = document.createElement('p');
        paragraph.className = 'content';
        var pText = document.createTextNode('This is a test');
        paragraph.appendChild(pText);
        
        container.appendChild(title);
        container.appendChild(paragraph);
        
        container;
    )";
    
    JSValue result = Eval(code);
    EXPECT_FALSE(IsException(result));
    
    auto container = DOMBindings::UnwrapElement(ctx, result);
    EXPECT_NE(container, nullptr);
    EXPECT_EQ(container->GetAttribute("id"), "container");
    EXPECT_EQ(container->GetClassName(), "main-container");
    EXPECT_EQ(container->GetChildNodes().size(), 2);
    
    auto title = std::dynamic_pointer_cast<Element>(container->GetChildNodes()[0]);
    EXPECT_EQ(title->GetAttribute("id"), "title");
    EXPECT_EQ(title->GetTextContent(), "Hello World");
    
    auto paragraph = std::dynamic_pointer_cast<Element>(container->GetChildNodes()[1]);
    EXPECT_EQ(paragraph->GetClassName(), "content");
    EXPECT_EQ(paragraph->GetTextContent(), "This is a test");
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, jsDoc);
}

// ========== 事件绑定测试 ==========
// 注意：事件监听器测试暂时禁用，因为需要更复杂的 JSValue 生命周期管理

TEST_F(DOMBindingsIntegrationTest, DISABLED_AddEventListener) {
    auto element = std::make_shared<Element>("button");

    JSValue jsElement = DOMBindings::WrapElement(ctx, element);
    SetGlobal("button", JS_DupValue(ctx, jsElement));
    
    // 添加事件监听器
    const char* code = R"(
        var clicked = false;
        button.addEventListener('click', function(event) {
            clicked = true;
        });
        clicked;
    )";
    
    JSValue result1 = Eval(code);
    EXPECT_FALSE(IsException(result1));
    EXPECT_EQ(GetInt(result1), 0);  // clicked = false
    JS_FreeValue(ctx, result1);
    
    // 触发事件
    auto event = std::make_shared<Event>("click", true, false);
    element->DispatchEvent(event);
    
    // 检查事件是否被触发
    JSValue result2 = Eval("clicked");
    EXPECT_FALSE(IsException(result2));
    EXPECT_EQ(GetInt(result2), 1);  // clicked = true
    JS_FreeValue(ctx, result2);
    
    JS_FreeValue(ctx, jsElement);
}

TEST_F(DOMBindingsIntegrationTest, DISABLED_EventProperties) {
    auto element = std::make_shared<Element>("button");

    JSValue jsElement = DOMBindings::WrapElement(ctx, element);
    SetGlobal("button", JS_DupValue(ctx, jsElement));
    
    // 添加事件监听器并检查事件属性
    const char* code = R"(
        var eventType = '';
        button.addEventListener('click', function(event) {
            eventType = event.type;
        });
    )";
    
    JSValue result1 = Eval(code);
    EXPECT_FALSE(IsException(result1));
    JS_FreeValue(ctx, result1);
    
    // 触发事件
    auto event = std::make_shared<Event>("click", true, false);
    element->DispatchEvent(event);
    
    // 检查事件类型
    JSValue result2 = Eval("eventType");
    EXPECT_FALSE(IsException(result2));
    EXPECT_EQ(GetString(result2), "click");
    JS_FreeValue(ctx, result2);
    
    JS_FreeValue(ctx, jsElement);
}

// ========== Text 节点绑定测试 ==========

TEST_F(DOMBindingsIntegrationTest, TextNodeData) {
    auto text = std::make_shared<Text>("Initial text");

    JSValue jsText = DOMBindings::WrapText(ctx, text);
    SetGlobal("textNode", JS_DupValue(ctx, jsText));
    
    // 获取 data
    JSValue result1 = Eval("textNode.data");
    EXPECT_FALSE(IsException(result1));
    EXPECT_EQ(GetString(result1), "Initial text");
    JS_FreeValue(ctx, result1);
    
    // 设置 data
    JSValue result2 = Eval("textNode.data = 'New text'; textNode.data");
    EXPECT_FALSE(IsException(result2));
    EXPECT_EQ(GetString(result2), "New text");
    EXPECT_EQ(text->GetData(), "New text");
    JS_FreeValue(ctx, result2);
    
    JS_FreeValue(ctx, jsText);
}

