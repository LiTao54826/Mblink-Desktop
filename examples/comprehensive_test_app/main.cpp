/**
 * @file main.cpp
 * @brief MBink 综合功能测试应用
 * 
 * 功能：
 * - 测试所有 90+ DOM API
 * - 测试事件系统（冒泡、捕获、once）
 * - 测试定时器 API
 * - 测试表单元素
 * - 测试查询选择器
 * - 提供可视化测试结果
 */

#include "core/quickjs/quickjs_runtime.h"
#include "core/dom/document.h"
#include "core/dom/dom_bindings.h"
#include "core/event/task_scheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

using namespace lightui;

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    JSContext* ctx = nullptr;

    std::cout << "=== MBink 综合功能测试应用 ===" << std::endl;
    std::cout << "版本: v1.0" << std::endl;
    std::cout << "日期: 2025-11-15" << std::endl;
    std::cout << std::endl;

    try {
        // 1. 创建 Document
        std::cout << "[1/5] 创建 Document..." << std::endl;
        auto document = std::make_shared<Document>();
        document->Initialize();
        std::cout << "   ✓ Document 创建成功" << std::endl;

        // 2. 创建 TaskScheduler
        std::cout << "[2/5] 创建 TaskScheduler..." << std::endl;
        auto scheduler = std::make_shared<TaskScheduler>();
        std::cout << "   ✓ TaskScheduler 创建成功" << std::endl;

        // 3. 创建 QuickJS 运行时
        std::cout << "[3/5] 创建 QuickJS 运行时..." << std::endl;
        auto runtime = std::make_unique<QuickJSRuntime>();
        ctx = runtime->GetContext();
        std::cout << "   ✓ QuickJS 运行时创建成功" << std::endl;

        // 4. 初始化 DOM 绑定
        std::cout << "[4/5] 初始化 DOM 绑定..." << std::endl;
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);
        DOMBindings::SetGlobalTaskScheduler(ctx, scheduler);
        std::cout << "   ✓ DOM 绑定初始化成功" << std::endl;

        // 5. 加载测试框架
        std::cout << "[5/5] 加载测试框架..." << std::endl;
        std::string test_framework = ReadFile("examples/comprehensive_test_app/test_framework.js");
        if (test_framework.empty()) {
            std::cerr << "错误: 无法加载测试框架" << std::endl;
            return 1;
        }
        runtime->Eval(test_framework, "test_framework.js");
        std::cout << "   ✓ 测试框架加载成功" << std::endl;

        // 6. 加载并运行所有测试
        std::cout << std::endl;
        std::cout << "=== 加载测试用例 ===" << std::endl;
        std::cout << std::endl;

        std::vector<std::string> test_files = {
            "examples/comprehensive_test_app/tests/dom_tests.js",
            "examples/comprehensive_test_app/tests/attribute_tests.js",
            "examples/comprehensive_test_app/tests/event_tests.js",
            "examples/comprehensive_test_app/tests/query_tests.js",
            "examples/comprehensive_test_app/tests/timer_tests.js",
            "examples/comprehensive_test_app/tests/form_tests.js",
            "examples/comprehensive_test_app/tests/html_tests.js"
        };

        for (const auto& file : test_files) {
            std::cout << "加载: " << file << std::endl;
            std::string test_code = ReadFile(file);
            if (!test_code.empty()) {
                runtime->Eval(test_code, file);
            } else {
                std::cerr << "警告: 无法加载 " << file << std::endl;
            }
        }

        std::cout << std::endl;
        std::cout << "=== 开始运行测试 ===" << std::endl;
        std::cout << std::endl;

        // 运行测试
        runtime->Eval("runAllTests();", "run_tests");

        std::cout << std::endl;
        std::cout << "=== 测试完成 ===" << std::endl;
        std::cout << std::endl;

        // 清理 DOM 绑定
        DOMBindings::Cleanup(ctx);

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        if (ctx) {
            DOMBindings::Cleanup(ctx);
        }
        return 1;
    }

    return 0;
}

