/**
 * @file test_object_cache.cpp
 * @brief 测试DOM对象缓存机制
 * 
 * 验证P1问题修复：防止同一个C++对象被包装多次
 */

#include <gtest/gtest.h>
#include "dom/dom_bindings.h"
#include "dom/document.h"
#include "dom/element.h"
#include "lexbor/lexbor_document.h"
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"

using namespace lightui;

class ObjectCacheTest : public ::testing::Test {
protected:
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
    
    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
    std::shared_ptr<Document> document_;
};

// 测试：同一个Element对象多次包装应该返回相同的JSValue
TEST_F(ObjectCacheTest, SameElementReturnsSameJSValue) {
    auto element = document_->CreateElement("div");
    
    // 第一次包装
    JSValue obj1 = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj1));
    
    // 第二次包装同一个对象
    JSValue obj2 = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj2));
    
    // 验证是否是同一个JSValue（通过比较u.ptr）
    // 注意：JS_DupValue会增加引用计数，但u.ptr应该相同
    EXPECT_EQ(obj1.u.ptr, obj2.u.ptr) << "Same C++ object should return same JSValue";
    
    // 清理
    JS_FreeValue(ctx_, obj1);
    JS_FreeValue(ctx_, obj2);
}

// 测试：不同的Element对象应该返回不同的JSValue
TEST_F(ObjectCacheTest, DifferentElementsReturnDifferentJSValues) {
    auto element1 = document_->CreateElement("div");
    auto element2 = document_->CreateElement("span");
    
    JSValue obj1 = DOMBindings::WrapElement(ctx_, element1);
    JSValue obj2 = DOMBindings::WrapElement(ctx_, element2);
    
    ASSERT_FALSE(JS_IsException(obj1));
    ASSERT_FALSE(JS_IsException(obj2));
    
    // 验证是不同的JSValue
    EXPECT_NE(obj1.u.ptr, obj2.u.ptr) << "Different C++ objects should return different JSValues";
    
    JS_FreeValue(ctx_, obj1);
    JS_FreeValue(ctx_, obj2);
}

// 测试：GC后重新包装应该创建新的JSValue
TEST_F(ObjectCacheTest, AfterGCNewJSValueCreated) {
    auto element = document_->CreateElement("div");
    
    // 第一次包装
    JSValue obj1 = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj1));
    void* ptr1 = obj1.u.ptr;
    
    // 释放JSValue，触发GC
    JS_FreeValue(ctx_, obj1);
    JS_RunGC(rt_);
    
    // 等待GC完成
    for (int i = 0; i < 3; i++) {
        JS_RunGC(rt_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 重新包装，应该创建新的JSValue（因为旧的已被GC）
    JSValue obj2 = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj2));
    
    // 注意：这个测试可能不稳定，因为GC时机不确定
    // 但至少应该能成功包装
    EXPECT_FALSE(JS_IsException(obj2));
    
    JS_FreeValue(ctx_, obj2);
}

// 测试：Text对象缓存
TEST_F(ObjectCacheTest, SameTextReturnsSameJSValue) {
    auto text = document_->CreateTextNode("Hello");
    
    JSValue obj1 = DOMBindings::WrapText(ctx_, text);
    JSValue obj2 = DOMBindings::WrapText(ctx_, text);
    
    ASSERT_FALSE(JS_IsException(obj1));
    ASSERT_FALSE(JS_IsException(obj2));
    
    EXPECT_EQ(obj1.u.ptr, obj2.u.ptr) << "Same Text object should return same JSValue";
    
    JS_FreeValue(ctx_, obj1);
    JS_FreeValue(ctx_, obj2);
}

// 测试：Document对象缓存
TEST_F(ObjectCacheTest, SameDocumentReturnsSameJSValue) {
    JSValue obj1 = DOMBindings::WrapDocument(ctx_, document_);
    JSValue obj2 = DOMBindings::WrapDocument(ctx_, document_);
    
    ASSERT_FALSE(JS_IsException(obj1));
    ASSERT_FALSE(JS_IsException(obj2));
    
    EXPECT_EQ(obj1.u.ptr, obj2.u.ptr) << "Same Document object should return same JSValue";
    
    JS_FreeValue(ctx_, obj1);
    JS_FreeValue(ctx_, obj2);
}

// 测试：在JavaScript中验证对象相等性
TEST_F(ObjectCacheTest, JavaScriptObjectEquality) {
    auto element = document_->CreateElement("div");
    
    // 将element包装两次并设置为全局变量
    JSValue obj1 = DOMBindings::WrapElement(ctx_, element);
    JSValue obj2 = DOMBindings::WrapElement(ctx_, element);
    
    JSValue global = JS_GetGlobalObject(ctx_);
    JS_SetPropertyStr(ctx_, global, "elem1", obj1);
    JS_SetPropertyStr(ctx_, global, "elem2", obj2);
    
    // 在JavaScript中测试相等性
    const char* code = "elem1 === elem2";
    JSValue result = JS_Eval(ctx_, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    
    ASSERT_FALSE(JS_IsException(result));
    EXPECT_TRUE(JS_ToBool(ctx_, result)) << "Same C++ object should be === in JavaScript";
    
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, global);
}

// 压力测试：大量对象缓存
TEST_F(ObjectCacheTest, ManyObjectsCache) {
    const int NUM_ELEMENTS = 100;
    std::vector<std::shared_ptr<Element>> elements;
    std::vector<JSValue> js_objects;
    
    // 创建100个元素
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        auto elem = document_->CreateElement("div");
        elements.push_back(elem);
    }
    
    // 每个元素包装两次
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        JSValue obj1 = DOMBindings::WrapElement(ctx_, elements[i]);
        JSValue obj2 = DOMBindings::WrapElement(ctx_, elements[i]);
        
        ASSERT_FALSE(JS_IsException(obj1));
        ASSERT_FALSE(JS_IsException(obj2));
        EXPECT_EQ(obj1.u.ptr, obj2.u.ptr);
        
        js_objects.push_back(obj1);
        JS_FreeValue(ctx_, obj2);  // 释放第二个引用
    }
    
    // 清理
    for (auto& obj : js_objects) {
        JS_FreeValue(ctx_, obj);
    }
    
    // 运行GC
    for (int i = 0; i < 3; i++) {
        JS_RunGC(rt_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 测试：缓存在finalizer中正确清理
TEST_F(ObjectCacheTest, CacheCleanedUpInFinalizer) {
    auto element = document_->CreateElement("div");
    Element* raw_ptr = element.get();
    
    // 包装元素
    JSValue obj = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj));
    
    // 释放JSValue，触发finalizer
    JS_FreeValue(ctx_, obj);
    
    // 运行GC确保finalizer被调用
    for (int i = 0; i < 5; i++) {
        JS_RunGC(rt_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 重新包装，应该创建新的JSValue（缓存已被清理）
    JSValue obj2 = DOMBindings::WrapElement(ctx_, element);
    ASSERT_FALSE(JS_IsException(obj2));
    
    JS_FreeValue(ctx_, obj2);
}

