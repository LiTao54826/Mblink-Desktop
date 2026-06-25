/**
 * @file test_quickjs_runtime.cpp
 * @brief QuickJS 运行时单元测试
 */

#include <gtest/gtest.h>
#include "quickjs/quickjs_runtime.h"

namespace mblink {
namespace test {

class QuickJSRuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime_ = std::make_unique<QuickJSRuntime>();
    }

    void TearDown() override {
        runtime_.reset();
    }

protected:
    std::unique_ptr<QuickJSRuntime> runtime_;
};

// ========== 基本执行测试 ==========

TEST_F(QuickJSRuntimeTest, EvalSimple) {
    auto result = runtime_->Eval("1 + 2");
    EXPECT_EQ(result, 3);
}

TEST_F(QuickJSRuntimeTest, EvalString) {
    auto result = runtime_->Eval("'hello' + ' ' + 'world'");
    EXPECT_EQ(result, "hello world");
}

TEST_F(QuickJSRuntimeTest, EvalBoolean) {
    auto result = runtime_->Eval("true && false");
    EXPECT_EQ(result, false);
}

TEST_F(QuickJSRuntimeTest, EvalNull) {
    auto result = runtime_->Eval("null");
    EXPECT_TRUE(result.is_null());
}

TEST_F(QuickJSRuntimeTest, EvalUndefined) {
    auto result = runtime_->Eval("undefined");
    // undefined 可能被转换为 null 或特殊值
}

TEST_F(QuickJSRuntimeTest, EvalArray) {
    auto result = runtime_->Eval("[1, 2, 3]");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 3);
}

TEST_F(QuickJSRuntimeTest, EvalObject) {
    auto result = runtime_->Eval("({a: 1, b: 2})");
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["a"], 1);
    EXPECT_EQ(result["b"], 2);
}

// ========== 函数调用测试 ==========

TEST_F(QuickJSRuntimeTest, DefineAndCallFunction) {
    runtime_->Eval("function add(a, b) { return a + b; }");
    auto result = runtime_->CallFunction("add", {3, 4});
    EXPECT_EQ(result, 7);
}

TEST_F(QuickJSRuntimeTest, CallFunctionWithArray) {
    runtime_->Eval("function sum(arr) { return arr.reduce((a, b) => a + b, 0); }");
    auto result = runtime_->CallFunction("sum", {{1, 2, 3, 4, 5}});
    EXPECT_EQ(result, 15);
}

TEST_F(QuickJSRuntimeTest, CallFunctionWithObject) {
    runtime_->Eval("function getName(obj) { return obj.name; }");
    auto result = runtime_->CallFunction("getName", {{{"name", "test"}}});
    EXPECT_EQ(result, "test");
}

// ========== 全局属性测试 ==========

TEST_F(QuickJSRuntimeTest, SetGlobalProperty) {
    runtime_->SetGlobalProperty("myValue", 42);
    auto result = runtime_->Eval("myValue");
    EXPECT_EQ(result, 42);
}

TEST_F(QuickJSRuntimeTest, GetGlobalProperty) {
    runtime_->Eval("var globalVar = 'hello'");
    auto result = runtime_->GetGlobalProperty("globalVar");
    EXPECT_EQ(result, "hello");
}

TEST_F(QuickJSRuntimeTest, SetGlobalObject) {
    runtime_->SetGlobalProperty("config", {{"debug", true}, {"version", "1.0"}});
    auto result = runtime_->Eval("config.debug");
    EXPECT_EQ(result, true);
}

// ========== 原生函数注册测试 ==========

TEST_F(QuickJSRuntimeTest, RegisterNativeFunction) {
    runtime_->RegisterFunction("nativeAdd", [](const json& args) -> json {
        return args[0].get<int>() + args[1].get<int>();
    });

    auto result = runtime_->Eval("nativeAdd(10, 20)");
    EXPECT_EQ(result, 30);
}

TEST_F(QuickJSRuntimeTest, RegisterNativeFunctionWithString) {
    runtime_->RegisterFunction("greet", [](const json& args) -> json {
        return "Hello, " + args[0].get<std::string>() + "!";
    });

    auto result = runtime_->Eval("greet('World')");
    EXPECT_EQ(result, "Hello, World!");
}

// ========== 模块测试 ==========

TEST_F(QuickJSRuntimeTest, RegisterModule) {
    runtime_->RegisterModule("myModule", R"(
        export function multiply(a, b) {
            return a * b;
        }
        export const PI = 3.14159;
    )");

    auto result = runtime_->EvalModule(R"(
        import { multiply, PI } from 'myModule';
        multiply(2, PI);
    )");

    EXPECT_NEAR(result.get<double>(), 6.28318, 0.001);
}

TEST_F(QuickJSRuntimeTest, SetBaseModulePath) {
    runtime_->SetBaseModulePath("/path/to/modules");
    // 后续模块导入应该相对于这个路径
}

// ========== 定时器测试 ==========

TEST_F(QuickJSRuntimeTest, SetTimeout) {
    runtime_->Eval(R"(
        var called = false;
        setTimeout(function() {
            called = true;
        }, 10);
    )");

    // 运行事件循环
    runtime_->RunEventLoop(10);

    auto result = runtime_->GetGlobalProperty("called");
    EXPECT_EQ(result, true);
}

TEST_F(QuickJSRuntimeTest, ClearTimeout) {
    runtime_->Eval(R"(
        var called = false;
        var id = setTimeout(function() {
            called = true;
        }, 100);
        clearTimeout(id);
    )");

    runtime_->RunEventLoop(5);

    auto result = runtime_->GetGlobalProperty("called");
    EXPECT_EQ(result, false);
}

TEST_F(QuickJSRuntimeTest, SetInterval) {
    runtime_->Eval(R"(
        var count = 0;
        var id = setInterval(function() {
            count++;
            if (count >= 3) {
                clearInterval(id);
            }
        }, 10);
    )");

    runtime_->RunEventLoop(20);

    auto result = runtime_->GetGlobalProperty("count");
    EXPECT_GE(result.get<int>(), 3);
}

// ========== 错误处理测试 ==========

TEST_F(QuickJSRuntimeTest, SyntaxError) {
    EXPECT_THROW({
        runtime_->Eval("function {");
    }, std::runtime_error);
}

TEST_F(QuickJSRuntimeTest, ReferenceError) {
    EXPECT_THROW({
        runtime_->Eval("undefinedVariable");
    }, std::runtime_error);
}

TEST_F(QuickJSRuntimeTest, TypeError) {
    EXPECT_THROW({
        runtime_->Eval("null.property");
    }, std::runtime_error);
}

// ========== Console API 测试 ==========

TEST_F(QuickJSRuntimeTest, ConsoleLog) {
    // console.log 应该不抛出异常
    EXPECT_NO_THROW({
        runtime_->Eval("console.log('test message')");
    });
}

TEST_F(QuickJSRuntimeTest, ConsoleError) {
    EXPECT_NO_THROW({
        runtime_->Eval("console.error('error message')");
    });
}

TEST_F(QuickJSRuntimeTest, ConsoleWarn) {
    EXPECT_NO_THROW({
        runtime_->Eval("console.warn('warning message')");
    });
}

// ========== 垃圾回收测试 ==========

TEST_F(QuickJSRuntimeTest, RunGC) {
    // 创建大量对象
    runtime_->Eval(R"(
        for (var i = 0; i < 10000; i++) {
            var obj = { data: new Array(100) };
        }
    )");

    // 运行 GC 不应该崩溃
    EXPECT_NO_THROW({
        runtime_->RunGC();
    });
}

// ========== 微任务测试 ==========

TEST_F(QuickJSRuntimeTest, ProcessMicrotasks) {
    runtime_->Eval(R"(
        var resolved = false;
        Promise.resolve().then(function() {
            resolved = true;
        });
    )");

    runtime_->ProcessMicrotasks();

    auto result = runtime_->GetGlobalProperty("resolved");
    EXPECT_EQ(result, true);
}

// ========== 上下文访问测试 ==========

TEST_F(QuickJSRuntimeTest, GetContext) {
    JSContext* ctx = runtime_->GetContext();
    EXPECT_NE(ctx, nullptr);
}

TEST_F(QuickJSRuntimeTest, GetRuntime) {
    JSRuntime* rt = runtime_->GetRuntime();
    EXPECT_NE(rt, nullptr);
}

} // namespace test
} // namespace mblink
