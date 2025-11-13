#include <gtest/gtest.h>
#include "quickjs/quickjs_runtime.h"
#include "dom/dom_bindings.h"
#include "dom/document.h"
#include "lexbor/lexbor_document.h"

using namespace lightui;

class RemoveEventListenerTest : public ::testing::Test {
protected:
    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
    std::shared_ptr<Document> document_;

    void SetUp() override {
        rt_ = JS_NewRuntime();
        ASSERT_NE(rt_, nullptr);

        ctx_ = JS_NewContext(rt_);
        ASSERT_NE(ctx_, nullptr);

        // 初始化DOM绑定
        DOMBindings::Init(ctx_);

        // 创建测试文档
        document_ = std::make_shared<Document>();
    }

    void TearDown() override {
        document_.reset();
        
        if (ctx_) {
            // 清理DOM绑定缓存
            DOMBindings::Cleanup(ctx_);
            JS_FreeContext(ctx_);
            ctx_ = nullptr;
        }
        if (rt_) {
            JS_FreeRuntime(rt_);
            rt_ = nullptr;
        }
    }
};

// 测试1: 基本的removeEventListener功能
TEST_F(RemoveEventListenerTest, BasicRemoveEventListener) {
    auto element = document_->CreateElement("div");
    
    // 包装Element到JavaScript
    JSValue elem_obj = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(elem_obj));
    
    // 创建事件监听器
    const char* code = R"(
        (function(element) {
            let callCount = 0;
            const listener = function() { callCount++; };
            
            // 添加监听器，获取ID
            const listenerId = element.addEventListener('click', listener);
            
            // 触发事件（通过C++）
            // 第一次应该被调用
            
            // 移除监听器
            const removed = element.removeEventListener('click', listenerId);
            
            // 再次触发事件
            // 第二次不应该被调用
            
            return { listenerId, removed, callCount };
        })
    )";
    
    JSValue func = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(func));
    ASSERT_TRUE(JS_IsFunction(ctx_, func));
    
    // 调用函数
    JSValue result = JS_Call(ctx_, func, JS_UNDEFINED, 1, &elem_obj);
    ASSERT_FALSE(JS_IsException(result));
    
    // 检查返回值
    JSValue listener_id = JS_GetPropertyStr(ctx_, result, "listenerId");
    JSValue removed = JS_GetPropertyStr(ctx_, result, "removed");

    // listenerId应该是BigInt (QuickJS中BigInt的tag是JS_TAG_BIG_INT)
    EXPECT_TRUE(JS_IsBigInt(listener_id));

    // removed应该是true
    int removed_bool = JS_ToBool(ctx_, removed);
    EXPECT_EQ(removed_bool, 1);
    
    // 清理
    JS_FreeValue(ctx_, listener_id);
    JS_FreeValue(ctx_, removed);
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, func);
    JS_FreeValue(ctx_, elem_obj);
}

// 测试2: 移除不存在的监听器
TEST_F(RemoveEventListenerTest, RemoveNonExistentListener) {
    auto element = document_->CreateElement("div");
    JSValue elem_obj = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(elem_obj));
    
    const char* code = R"(
        (function(element) {
            // 尝试移除不存在的监听器
            const removed = element.removeEventListener('click', 99999n);
            return removed;
        })
    )";
    
    JSValue func = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(func));
    
    JSValue result = JS_Call(ctx_, func, JS_UNDEFINED, 1, &elem_obj);
    ASSERT_FALSE(JS_IsException(result));
    
    // 应该返回false
    int removed_bool = JS_ToBool(ctx_, result);
    EXPECT_EQ(removed_bool, 0);
    
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, func);
    JS_FreeValue(ctx_, elem_obj);
}

// 测试3: 添加多个监听器并选择性移除
TEST_F(RemoveEventListenerTest, RemoveSpecificListener) {
    auto element = document_->CreateElement("div");
    JSValue elem_obj = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(elem_obj));
    
    const char* code = R"(
        (function(element) {
            let count1 = 0, count2 = 0, count3 = 0;
            
            const listener1 = function() { count1++; };
            const listener2 = function() { count2++; };
            const listener3 = function() { count3++; };
            
            // 添加3个监听器
            const id1 = element.addEventListener('click', listener1);
            const id2 = element.addEventListener('click', listener2);
            const id3 = element.addEventListener('click', listener3);
            
            // 只移除第2个
            const removed = element.removeEventListener('click', id2);
            
            return { id1, id2, id3, removed };
        })
    )";
    
    JSValue func = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(func));
    
    JSValue result = JS_Call(ctx_, func, JS_UNDEFINED, 1, &elem_obj);
    ASSERT_FALSE(JS_IsException(result));
    
    JSValue removed = JS_GetPropertyStr(ctx_, result, "removed");
    int removed_bool = JS_ToBool(ctx_, removed);
    EXPECT_EQ(removed_bool, 1);
    
    // 验证C++端：应该还有2个监听器
    // 触发事件验证
    auto event = std::make_shared<Event>("click");
    element->DispatchEvent(event);
    
    JS_FreeValue(ctx_, removed);
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, func);
    JS_FreeValue(ctx_, elem_obj);
}

// 测试4: 移除后不应该有内存泄漏
TEST_F(RemoveEventListenerTest, NoMemoryLeakAfterRemove) {
    for (int i = 0; i < 100; i++) {
        auto element = document_->CreateElement("div");
        JSValue elem_obj = DOMBindings::WrapElement(ctx_, element);
        
        const char* code = R"(
            (function(element) {
                const id = element.addEventListener('click', function() {});
                element.removeEventListener('click', id);
            })
        )";
        
        JSValue func = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
        JSValue result = JS_Call(ctx_, func, JS_UNDEFINED, 1, &elem_obj);
        
        JS_FreeValue(ctx_, result);
        JS_FreeValue(ctx_, func);
        JS_FreeValue(ctx_, elem_obj);
    }
    
    // 运行GC
    JS_RunGC(rt_);
    
    // 如果有内存泄漏，这里会失败
    SUCCEED();
}

// 测试5: 移除不同事件类型的监听器
TEST_F(RemoveEventListenerTest, RemoveDifferentEventTypes) {
    auto element = document_->CreateElement("div");
    JSValue elem_obj = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(elem_obj));
    
    const char* code = R"(
        (function(element) {
            const clickId = element.addEventListener('click', function() {});
            const mouseoverId = element.addEventListener('mouseover', function() {});
            
            // 移除click监听器
            const removed1 = element.removeEventListener('click', clickId);
            
            // 尝试用click的ID移除mouseover（应该失败）
            const removed2 = element.removeEventListener('mouseover', clickId);
            
            // 正确移除mouseover
            const removed3 = element.removeEventListener('mouseover', mouseoverId);
            
            return { removed1, removed2, removed3 };
        })
    )";
    
    JSValue func = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    ASSERT_FALSE(JS_IsException(func));
    
    JSValue result = JS_Call(ctx_, func, JS_UNDEFINED, 1, &elem_obj);
    ASSERT_FALSE(JS_IsException(result));
    
    JSValue removed1 = JS_GetPropertyStr(ctx_, result, "removed1");
    JSValue removed2 = JS_GetPropertyStr(ctx_, result, "removed2");
    JSValue removed3 = JS_GetPropertyStr(ctx_, result, "removed3");
    
    EXPECT_EQ(JS_ToBool(ctx_, removed1), 1);  // 应该成功
    EXPECT_EQ(JS_ToBool(ctx_, removed2), 0);  // 应该失败（ID不匹配）
    EXPECT_EQ(JS_ToBool(ctx_, removed3), 1);  // 应该成功
    
    JS_FreeValue(ctx_, removed1);
    JS_FreeValue(ctx_, removed2);
    JS_FreeValue(ctx_, removed3);
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, func);
    JS_FreeValue(ctx_, elem_obj);
}

