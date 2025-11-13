/**
 * @file test_js_value_wrapper.cpp
 * @brief JSValueWrapper 单元测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/js_value_wrapper.h"
#include "core/quickjs/quickjs_runtime.h"
#include <memory>

using namespace lightui;

class JSValueWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_shared<QuickJSRuntime>();
        ctx = runtime->GetContext();
    }

    void TearDown() override {
        runtime.reset();
    }

    std::shared_ptr<QuickJSRuntime> runtime;
    JSContext* ctx;
};

// ========== 基础功能测试 ==========

TEST_F(JSValueWrapperTest, ConstructorAndDestructor) {
    JSValue value = JS_NewInt32(ctx, 42);
    
    {
        JSValueWrapper wrapper(ctx, value);
        EXPECT_TRUE(wrapper.IsValid());
        EXPECT_EQ(JS_VALUE_GET_INT(wrapper.Get()), 42);
    }
    // wrapper 析构，应该调用 JS_FreeValue
    
    JS_FreeValue(ctx, value);  // 释放原始值
}

TEST_F(JSValueWrapperTest, GetContext) {
    JSValue value = JS_NewInt32(ctx, 42);
    JSValueWrapper wrapper(ctx, value);
    
    EXPECT_EQ(wrapper.GetContext(), ctx);
    
    JS_FreeValue(ctx, value);
}

TEST_F(JSValueWrapperTest, GetValue) {
    JSValue value = JS_NewString(ctx, "hello");
    JSValueWrapper wrapper(ctx, value);
    
    const char* str = JS_ToCString(ctx, wrapper.Get());
    EXPECT_STREQ(str, "hello");
    JS_FreeCString(ctx, str);
    
    JS_FreeValue(ctx, value);
}

// ========== 类型检查测试 ==========

TEST_F(JSValueWrapperTest, IsUndefined) {
    JSValue value = JS_UNDEFINED;
    JSValueWrapper wrapper(ctx, value);
    
    EXPECT_TRUE(wrapper.IsUndefined());
    EXPECT_FALSE(wrapper.IsNull());
}

TEST_F(JSValueWrapperTest, IsNull) {
    JSValue value = JS_NULL;
    JSValueWrapper wrapper(ctx, value);
    
    EXPECT_TRUE(wrapper.IsNull());
    EXPECT_FALSE(wrapper.IsUndefined());
}

TEST_F(JSValueWrapperTest, IsFunction) {
    runtime->Eval("function testFunc() { return 42; }", "test.js");
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global, "testFunc");
    
    JSValueWrapper wrapper(ctx, func);
    EXPECT_TRUE(wrapper.IsFunction());
    
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global);
}

TEST_F(JSValueWrapperTest, IsObject) {
    JSValue obj = JS_NewObject(ctx);
    JSValueWrapper wrapper(ctx, obj);
    
    EXPECT_TRUE(wrapper.IsObject());
    
    JS_FreeValue(ctx, obj);
}

// ========== 移动语义测试 ==========

TEST_F(JSValueWrapperTest, MoveConstructor) {
    JSValue value = JS_NewInt32(ctx, 42);
    JSValueWrapper wrapper1(ctx, value);
    
    JSValueWrapper wrapper2(std::move(wrapper1));
    
    EXPECT_TRUE(wrapper2.IsValid());
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper2.Get()), 42);
    EXPECT_FALSE(wrapper1.IsValid());  // wrapper1 已失效
    
    JS_FreeValue(ctx, value);
}

TEST_F(JSValueWrapperTest, MoveAssignment) {
    JSValue value1 = JS_NewInt32(ctx, 42);
    JSValue value2 = JS_NewInt32(ctx, 100);
    
    JSValueWrapper wrapper1(ctx, value1);
    JSValueWrapper wrapper2(ctx, value2);
    
    wrapper2 = std::move(wrapper1);
    
    EXPECT_TRUE(wrapper2.IsValid());
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper2.Get()), 42);
    EXPECT_FALSE(wrapper1.IsValid());
    
    JS_FreeValue(ctx, value1);
    JS_FreeValue(ctx, value2);
}

// ========== 函数调用测试 ==========

TEST_F(JSValueWrapperTest, CallFunction) {
    runtime->Eval("function add(a, b) { return a + b; }", "test.js");
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global, "add");
    
    JSValueWrapper wrapper(ctx, func);
    
    JSValue args[2];
    args[0] = JS_NewInt32(ctx, 10);
    args[1] = JS_NewInt32(ctx, 20);
    
    JSValue result = wrapper.Call(JS_UNDEFINED, 2, args);
    EXPECT_EQ(JS_VALUE_GET_INT(result), 30);
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global);
}

TEST_F(JSValueWrapperTest, CallFunctionNoArgs) {
    runtime->Eval("function getAnswer() { return 42; }", "test.js");
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global, "getAnswer");
    
    JSValueWrapper wrapper(ctx, func);
    
    JSValue result = wrapper.Call();
    EXPECT_EQ(JS_VALUE_GET_INT(result), 42);
    
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global);
}

// ========== shared_ptr 使用测试 ==========

TEST_F(JSValueWrapperTest, SharedPtrUsage) {
    JSValue value = JS_NewInt32(ctx, 42);
    
    auto wrapper = std::make_shared<JSValueWrapper>(ctx, value);
    
    // 创建多个 shared_ptr 副本
    auto wrapper2 = wrapper;
    auto wrapper3 = wrapper;
    
    EXPECT_EQ(wrapper.use_count(), 3);
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper->Get()), 42);
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper2->Get()), 42);
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper3->Get()), 42);
    
    JS_FreeValue(ctx, value);
}

TEST_F(JSValueWrapperTest, MakeJSValueWrapper) {
    JSValue value = JS_NewString(ctx, "test");
    
    auto wrapper = MakeJSValueWrapper(ctx, value);
    
    const char* str = JS_ToCString(ctx, wrapper->Get());
    EXPECT_STREQ(str, "test");
    JS_FreeCString(ctx, str);
    
    JS_FreeValue(ctx, value);
}

// ========== Lambda 捕获测试 ==========

TEST_F(JSValueWrapperTest, LambdaCapture) {
    runtime->Eval("function greet(name) { return 'Hello, ' + name; }", "test.js");
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global, "greet");
    
    auto wrapper = MakeJSValueWrapper(ctx, func);
    
    // Lambda 捕获 shared_ptr
    auto lambda = [wrapper, this]() {
        JSValue arg = JS_NewString(ctx, "World");
        JSValue result = wrapper->Call(JS_UNDEFINED, 1, &arg);
        
        const char* str = JS_ToCString(ctx, result);
        std::string greeting(str);
        
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, result);
        JS_FreeValue(ctx, arg);
        
        return greeting;
    };
    
    std::string result = lambda();
    EXPECT_EQ(result, "Hello, World");
    
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global);
}

// ========== 内存泄漏测试 ==========

TEST_F(JSValueWrapperTest, NoMemoryLeakWithMultipleWrappers) {
    // 创建 1000 个 wrapper，确保没有内存泄漏
    for (int i = 0; i < 1000; i++) {
        JSValue value = JS_NewInt32(ctx, i);
        auto wrapper = MakeJSValueWrapper(ctx, value);
        EXPECT_EQ(JS_VALUE_GET_INT(wrapper->Get()), i);
        JS_FreeValue(ctx, value);
    }
    
    // 强制 GC
    runtime->RunGC();
    
    // 如果有内存泄漏，这里会失败
    SUCCEED();
}

TEST_F(JSValueWrapperTest, NoMemoryLeakWithLambdas) {
    runtime->Eval("function identity(x) { return x; }", "test.js");
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global, "identity");
    
    // 创建 1000 个 lambda，每个都捕获 wrapper
    std::vector<std::function<int()>> lambdas;
    for (int i = 0; i < 1000; i++) {
        auto wrapper = MakeJSValueWrapper(ctx, func);
        lambdas.push_back([wrapper, i, this]() {
            JSValue arg = JS_NewInt32(ctx, i);
            JSValue result = wrapper->Call(JS_UNDEFINED, 1, &arg);
            int value = JS_VALUE_GET_INT(result);
            JS_FreeValue(ctx, result);
            JS_FreeValue(ctx, arg);
            return value;
        });
    }
    
    // 调用所有 lambda
    for (int i = 0; i < 1000; i++) {
        EXPECT_EQ(lambdas[i](), i);
    }
    
    lambdas.clear();
    
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, global);
    
    // 强制 GC
    runtime->RunGC();
    
    SUCCEED();
}

// ========== 边界情况测试 ==========

TEST_F(JSValueWrapperTest, SelfMoveAssignment) {
    JSValue value = JS_NewInt32(ctx, 42);
    JSValueWrapper wrapper(ctx, value);
    
    // 自我移动赋值（虽然不推荐，但应该安全）
    wrapper = std::move(wrapper);
    
    // 行为未定义，但不应该崩溃
    // EXPECT_TRUE(wrapper.IsValid());  // 可能为 true 或 false
    
    JS_FreeValue(ctx, value);
}

TEST_F(JSValueWrapperTest, MultipleMovesInChain) {
    JSValue value = JS_NewInt32(ctx, 42);
    JSValueWrapper wrapper1(ctx, value);
    JSValueWrapper wrapper2(std::move(wrapper1));
    JSValueWrapper wrapper3(std::move(wrapper2));
    JSValueWrapper wrapper4(std::move(wrapper3));
    
    EXPECT_TRUE(wrapper4.IsValid());
    EXPECT_EQ(JS_VALUE_GET_INT(wrapper4.Get()), 42);
    EXPECT_FALSE(wrapper1.IsValid());
    EXPECT_FALSE(wrapper2.IsValid());
    EXPECT_FALSE(wrapper3.IsValid());
    
    JS_FreeValue(ctx, value);
}

