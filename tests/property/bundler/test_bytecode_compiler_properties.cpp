/**
 * @file test_bytecode_compiler_properties.cpp
 * @brief 字节码编译器属性测试
 *
 * **Feature: app-bundler, Property 2: 所有模块编译为字节码**
 * **Validates: Requirements 2.1, 2.6**
 */

#include <gtest/gtest.h>
#include "tools/app_bundler/bytecode_compiler.h"
#include "tools/app_bundler/module_resolver.h"
#include <filesystem>
#include <fstream>
#include <random>

using namespace mbink;

namespace fs = std::filesystem;

namespace {

fs::path FindRepoRoot() {
    auto current = fs::current_path();
    while (!current.empty()) {
        if (fs::exists(current / "third_party" / "preact" / "package.json")) {
            return current;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate repository root for Preact property tests");
}

std::string NormalizePath(const fs::path& path) {
    std::string normalized = fs::weakly_canonical(path).lexically_normal().string();
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

}  // namespace

class BytecodeCompilerPropertyTest : public ::testing::Test {
protected:
    std::mt19937 rng_{42};
};

/**
 * Property 2: 基本编译测试
 */
TEST_F(BytecodeCompilerPropertyTest, CompileSimpleScript) {
    BytecodeCompiler compiler;
    
    std::string source = "const x = 1 + 2;";
    auto bytecode = compiler.CompileModule(source, "test.js", false);
    
    EXPECT_FALSE(compiler.HasErrors()) << "Should compile without errors";
    EXPECT_FALSE(bytecode.empty()) << "Should produce bytecode";
}

/**
 * Property 2: ES6 模块编译
 */
TEST_F(BytecodeCompilerPropertyTest, CompileES6Module) {
    BytecodeCompiler compiler;
    
    std::string source = R"(
export const foo = 42;
export function bar() { return foo; }
)";
    auto bytecode = compiler.CompileModule(source, "module.js", true);
    
    EXPECT_FALSE(compiler.HasErrors()) << "Should compile ES6 module";
    EXPECT_FALSE(bytecode.empty());
}

/**
 * Property 2: 语法错误检测
 */
TEST_F(BytecodeCompilerPropertyTest, DetectSyntaxError) {
    BytecodeCompiler compiler;
    
    std::string source = "const x = {";  // 语法错误
    auto bytecode = compiler.CompileModule(source, "error.js", false);
    
    EXPECT_TRUE(compiler.HasErrors()) << "Should detect syntax error";
    EXPECT_TRUE(bytecode.empty()) << "Should not produce bytecode on error";
}

/**
 * Property 2: 多模块编译
 */
TEST_F(BytecodeCompilerPropertyTest, CompileMultipleModules) {
    BytecodeCompiler compiler;
    
    std::vector<ResolvedModule> modules = {
        {"utils.js", "", "export const PI = 3.14;", false, {}},
        {"math.js", "", "import { PI } from './utils.js'; export const area = r => PI * r * r;", false, {"utils.js"}},
        {"main.js", "", "import { area } from './math.js'; console.log(area(5));", false, {"math.js"}}
    };
    
    auto compiled = compiler.CompileModules(modules);
    
    EXPECT_FALSE(compiler.HasErrors());
    EXPECT_EQ(compiled.size(), 3);
    
    // 验证每个模块都有字节码
    for (const auto& mod : compiled) {
        EXPECT_FALSE(mod.bytecode.empty()) << "Module " << mod.id << " should have bytecode";
    }
    
    // 最后一个应该是入口
    EXPECT_TRUE(compiled.back().is_entry);
}

TEST_F(BytecodeCompilerPropertyTest, CompileOfficialPreactWrapperModulesWithoutLegacyGlobals) {
    const auto repo_root = FindRepoRoot();
    const auto preact_entry = NormalizePath(repo_root / "third_party" / "preact" / "src" / "index.js");
    const auto hooks_entry = NormalizePath(repo_root / "third_party" / "preact" / "hooks" / "src" / "index.js");

    const fs::path test_dir = fs::temp_directory_path() / "bytecode_compiler_official_preact";
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    const fs::path entry_file = test_dir / "app.js";
    {
        std::ofstream out(entry_file);
        out << "import { h } from 'preact';\n"
               "import { useState } from 'preact/hooks';\n"
               "export const app = { h, useState };\n";
    }

    ModuleResolver resolver;
    resolver.RegisterBuiltinModule("preact", "export * from '" + preact_entry + "';");
    resolver.RegisterBuiltinModule("preact/hooks", "export * from '" + hooks_entry + "';");

    auto modules = resolver.Resolve(entry_file.string());
    ASSERT_FALSE(resolver.HasErrors()) << resolver.GetErrors().front().message;

    BytecodeCompiler compiler;
    compiler.SetEntryDir(test_dir.string());
    auto compiled = compiler.CompileModules(modules);

    EXPECT_FALSE(compiler.HasErrors());
    EXPECT_FALSE(compiled.empty());

    fs::remove_all(test_dir);
}

/**
 * Property 2: 字节码合并往返测试
 */
TEST_F(BytecodeCompilerPropertyTest, MergeBytecodeRoundTrip) {
    BytecodeCompiler compiler;
    
    std::vector<ResolvedModule> modules = {
        {"a.js", "", "export const a = 1;", false, {}},
        {"b.js", "", "export const b = 2;", false, {}},
        {"c.js", "", "import { a } from './a.js'; import { b } from './b.js';", false, {"a.js", "b.js"}}
    };
    
    auto compiled = compiler.CompileModules(modules);
    ASSERT_EQ(compiled.size(), 3);
    
    // 合并字节码
    auto merged = BytecodeCompiler::MergeBytecode(compiled);
    EXPECT_FALSE(merged.empty());
    
    // 解析合并后的字节码
    auto parsed = BytecodeCompiler::ParseMergedBytecode(merged);
    ASSERT_EQ(parsed.size(), compiled.size());
    
    // 验证每个模块
    for (size_t i = 0; i < compiled.size(); i++) {
        EXPECT_EQ(parsed[i].id, compiled[i].id);
        EXPECT_EQ(parsed[i].bytecode, compiled[i].bytecode);
        EXPECT_EQ(parsed[i].is_entry, compiled[i].is_entry);
    }
}

/**
 * Property 2: 空模块列表
 */
TEST_F(BytecodeCompilerPropertyTest, EmptyModuleList) {
    std::vector<CompiledModule> empty;
    auto merged = BytecodeCompiler::MergeBytecode(empty);
    
    // 应该只有模块计数（4字节，值为0）
    EXPECT_EQ(merged.size(), 4);
    
    auto parsed = BytecodeCompiler::ParseMergedBytecode(merged);
    EXPECT_TRUE(parsed.empty());
}

/**
 * Property 2: 大模块编译
 */
TEST_F(BytecodeCompilerPropertyTest, LargeModuleCompilation) {
    BytecodeCompiler compiler;
    
    // 生成一个较大的模块
    std::string source = "const data = [\n";
    for (int i = 0; i < 1000; i++) {
        source += "  " + std::to_string(i) + ",\n";
    }
    source += "];\nexport default data;";
    
    auto bytecode = compiler.CompileModule(source, "large.js", true);
    
    EXPECT_FALSE(compiler.HasErrors());
    EXPECT_FALSE(bytecode.empty());
    // 字节码应该能成功生成（不一定比源码小）
    EXPECT_GT(bytecode.size(), 0);
}

/**
 * Property 2: 去除源码信息
 */
TEST_F(BytecodeCompilerPropertyTest, StripSourceInfo) {
    BytecodeCompiler compiler1;
    BytecodeCompiler compiler2;
    compiler2.SetStripSource(true);
    compiler2.SetStripDebug(true);
    
    std::string source = R"(
// This is a comment
function hello() {
    console.log("Hello, World!");
}
export { hello };
)";
    
    auto bytecode1 = compiler1.CompileModule(source, "test.js", true);
    auto bytecode2 = compiler2.CompileModule(source, "test.js", true);
    
    EXPECT_FALSE(compiler1.HasErrors());
    EXPECT_FALSE(compiler2.HasErrors());
    
    // 去除信息后应该更小
    EXPECT_LT(bytecode2.size(), bytecode1.size());
}

/**
 * Property 2: 各种 JS 特性编译
 */
TEST_F(BytecodeCompilerPropertyTest, CompileJSFeatures) {
    BytecodeCompiler compiler;
    
    // 箭头函数
    auto bc1 = compiler.CompileModule("const f = x => x * 2;", "arrow.js", false);
    EXPECT_FALSE(bc1.empty());
    
    // 解构
    auto bc2 = compiler.CompileModule("const {a, b} = {a: 1, b: 2};", "destruct.js", false);
    EXPECT_FALSE(bc2.empty());
    
    // 模板字符串
    auto bc3 = compiler.CompileModule("const s = `hello ${1 + 2}`;", "template.js", false);
    EXPECT_FALSE(bc3.empty());
    
    // async/await
    auto bc4 = compiler.CompileModule("async function f() { await Promise.resolve(); }", "async.js", false);
    EXPECT_FALSE(bc4.empty());
    
    // class
    auto bc5 = compiler.CompileModule("class Foo { constructor() {} }", "class.js", false);
    EXPECT_FALSE(bc5.empty());
    
    EXPECT_FALSE(compiler.HasErrors());
}

