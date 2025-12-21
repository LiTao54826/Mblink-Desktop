/**
 * @file test_module_resolver_properties.cpp
 * @brief 模块解析器属性测试
 *
 * **Feature: app-bundler, Property 3: 模块解析完整性**
 * **Feature: app-bundler, Property 4: 依赖顺序保持**
 * **Validates: Requirements 2.2, 2.5, 2.7**
 */

#include <gtest/gtest.h>
#include "tools/app_bundler/module_resolver.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <set>

namespace fs = std::filesystem;
using namespace mbink;

class ModuleResolverPropertyTest : public ::testing::Test {
protected:
    std::string test_dir_;
    std::mt19937 rng_{42};

    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = (fs::temp_directory_path() / "module_resolver_test").string();
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        // 清理测试目录
        fs::remove_all(test_dir_);
    }

    // 创建测试文件
    void CreateFile(const std::string& relative_path, const std::string& content) {
        fs::path full_path = fs::path(test_dir_) / relative_path;
        fs::create_directories(full_path.parent_path());
        std::ofstream file(full_path);
        file << content;
    }

    std::string GetFullPath(const std::string& relative_path) {
        return (fs::path(test_dir_) / relative_path).string();
    }
};

/**
 * Property 3: 模块解析完整性 - 基本 import 解析
 */
TEST_F(ModuleResolverPropertyTest, ParseImportsBasic) {
    std::string source = R"(
import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import App from './App.js';
import * as utils from './utils';
import './styles.css';
)";

    auto imports = ModuleResolver::ParseImports(source);
    
    ASSERT_EQ(imports.size(), 5);
    EXPECT_EQ(imports[0].module_path, "preact");
    EXPECT_EQ(imports[1].module_path, "preact/hooks");
    EXPECT_EQ(imports[2].module_path, "./App.js");
    EXPECT_EQ(imports[3].module_path, "./utils");
    EXPECT_EQ(imports[4].module_path, "./styles.css");
}

/**
 * Property 3: 模块解析完整性 - 单引号和双引号
 */
TEST_F(ModuleResolverPropertyTest, ParseImportsQuoteStyles) {
    std::string source = R"(
import foo from "double-quotes";
import bar from 'single-quotes';
)";

    auto imports = ModuleResolver::ParseImports(source);
    
    ASSERT_EQ(imports.size(), 2);
    EXPECT_EQ(imports[0].module_path, "double-quotes");
    EXPECT_EQ(imports[1].module_path, "single-quotes");
}

/**
 * Property 3: 模块解析完整性 - 空源码
 */
TEST_F(ModuleResolverPropertyTest, ParseImportsEmpty) {
    auto imports = ModuleResolver::ParseImports("");
    EXPECT_TRUE(imports.empty());
    
    imports = ModuleResolver::ParseImports("const x = 1;");
    EXPECT_TRUE(imports.empty());
}

/**
 * Property 3: 路径解析 - 相对路径
 */
TEST_F(ModuleResolverPropertyTest, ResolvePathRelative) {
    // 创建测试文件
    CreateFile("src/main.js", "");
    CreateFile("src/utils.js", "");
    CreateFile("src/lib/helper.js", "");

    std::string from_file = GetFullPath("src/main.js");
    
    // ./utils -> src/utils.js
    std::string resolved = ModuleResolver::ResolvePath("./utils.js", from_file);
    EXPECT_TRUE(fs::exists(resolved));
    
    // ./lib/helper -> src/lib/helper.js
    resolved = ModuleResolver::ResolvePath("./lib/helper.js", from_file);
    EXPECT_TRUE(fs::exists(resolved));
}

/**
 * Property 3: 路径解析 - 自动添加 .js 扩展名
 */
TEST_F(ModuleResolverPropertyTest, ResolvePathAutoExtension) {
    CreateFile("src/main.js", "");
    CreateFile("src/utils.js", "");

    std::string from_file = GetFullPath("src/main.js");
    
    // ./utils (无扩展名) -> src/utils.js
    std::string resolved = ModuleResolver::ResolvePath("./utils", from_file);
    EXPECT_TRUE(fs::exists(resolved));
    EXPECT_TRUE(resolved.ends_with(".js") || resolved.ends_with(".js\""));
}

/**
 * Property 3: 路径解析 - 裸模块名
 */
TEST_F(ModuleResolverPropertyTest, ResolvePathBareModule) {
    std::string from_file = GetFullPath("src/main.js");
    
    // 裸模块名应该原样返回
    std::string resolved = ModuleResolver::ResolvePath("preact", from_file);
    EXPECT_EQ(resolved, "preact");
    
    resolved = ModuleResolver::ResolvePath("preact/hooks", from_file);
    EXPECT_EQ(resolved, "preact/hooks");
}

/**
 * Property 4: 依赖顺序保持 - 简单依赖链
 */
TEST_F(ModuleResolverPropertyTest, DependencyOrderSimpleChain) {
    // A -> B -> C
    CreateFile("a.js", "import './b.js';");
    CreateFile("b.js", "import './c.js';");
    CreateFile("c.js", "const c = 1;");

    ModuleResolver resolver;
    auto modules = resolver.Resolve(GetFullPath("a.js"));
    
    ASSERT_FALSE(resolver.HasErrors()) << "Should resolve without errors";
    ASSERT_EQ(modules.size(), 3);
    
    // C 应该在 B 之前，B 应该在 A 之前
    std::map<std::string, size_t> order;
    for (size_t i = 0; i < modules.size(); i++) {
        order[fs::path(modules[i].id).filename().string()] = i;
    }
    
    EXPECT_LT(order["c.js"], order["b.js"]) << "c.js should come before b.js";
    EXPECT_LT(order["b.js"], order["a.js"]) << "b.js should come before a.js";
}

/**
 * Property 4: 依赖顺序保持 - 菱形依赖
 */
TEST_F(ModuleResolverPropertyTest, DependencyOrderDiamond) {
    //     A
    //    / \
    //   B   C
    //    \ /
    //     D
    CreateFile("a.js", "import './b.js';\nimport './c.js';");
    CreateFile("b.js", "import './d.js';");
    CreateFile("c.js", "import './d.js';");
    CreateFile("d.js", "const d = 1;");

    ModuleResolver resolver;
    auto modules = resolver.Resolve(GetFullPath("a.js"));
    
    ASSERT_FALSE(resolver.HasErrors());
    ASSERT_EQ(modules.size(), 4);
    
    // D 应该在 B 和 C 之前
    std::map<std::string, size_t> order;
    for (size_t i = 0; i < modules.size(); i++) {
        order[fs::path(modules[i].id).filename().string()] = i;
    }
    
    EXPECT_LT(order["d.js"], order["b.js"]) << "d.js should come before b.js";
    EXPECT_LT(order["d.js"], order["c.js"]) << "d.js should come before c.js";
    EXPECT_LT(order["b.js"], order["a.js"]) << "b.js should come before a.js";
    EXPECT_LT(order["c.js"], order["a.js"]) << "c.js should come before a.js";
}

/**
 * Property 4: 依赖顺序保持 - 内置模块
 */
TEST_F(ModuleResolverPropertyTest, DependencyOrderWithBuiltins) {
    CreateFile("app.js", R"(
import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import './component.js';
)");
    CreateFile("component.js", "import { h } from 'preact';");

    ModuleResolver resolver;
    resolver.RegisterBuiltinModule("preact", "// preact source");
    resolver.RegisterBuiltinModule("preact/hooks", "// hooks source");
    
    auto modules = resolver.Resolve(GetFullPath("app.js"));
    
    ASSERT_FALSE(resolver.HasErrors());
    
    // 应该有 4 个模块：preact, preact/hooks, component.js, app.js
    ASSERT_EQ(modules.size(), 4);
    
    // 内置模块应该在依赖它们的模块之前
    std::map<std::string, size_t> order;
    for (size_t i = 0; i < modules.size(); i++) {
        std::string name = modules[i].is_builtin 
            ? modules[i].id 
            : fs::path(modules[i].id).filename().string();
        order[name] = i;
    }
    
    EXPECT_LT(order["preact"], order["app.js"]);
    EXPECT_LT(order["preact/hooks"], order["app.js"]);
    EXPECT_LT(order["preact"], order["component.js"]);
}

/**
 * Property 3: 错误处理 - 文件不存在
 */
TEST_F(ModuleResolverPropertyTest, ErrorFileNotFound) {
    CreateFile("main.js", "import './nonexistent.js';");

    ModuleResolver resolver;
    auto modules = resolver.Resolve(GetFullPath("main.js"));
    
    EXPECT_TRUE(resolver.HasErrors());
    EXPECT_FALSE(resolver.GetErrors().empty());
}

/**
 * Property 3: 错误处理 - 未知裸模块
 */
TEST_F(ModuleResolverPropertyTest, ErrorUnknownBareModule) {
    CreateFile("main.js", "import something from 'unknown-module';");

    ModuleResolver resolver;
    // 不注册 unknown-module
    auto modules = resolver.Resolve(GetFullPath("main.js"));
    
    EXPECT_TRUE(resolver.HasErrors());
}

/**
 * Property 4: 循环依赖处理
 */
TEST_F(ModuleResolverPropertyTest, CircularDependencyHandling) {
    // A -> B -> A (循环)
    CreateFile("a.js", "import './b.js';");
    CreateFile("b.js", "import './a.js';");

    ModuleResolver resolver;
    resolver.SetVerbose(false);
    auto modules = resolver.Resolve(GetFullPath("a.js"));
    
    // 循环依赖应该被处理（不是错误，只是警告）
    EXPECT_FALSE(resolver.HasErrors()) << "Circular dependency should not be an error";
    EXPECT_EQ(modules.size(), 2);
}

/**
 * Property 3: 多重导入去重
 */
TEST_F(ModuleResolverPropertyTest, DuplicateImportDeduplication) {
    CreateFile("main.js", R"(
import './utils.js';
import './utils.js';
import { foo } from './utils.js';
)");
    CreateFile("utils.js", "export const foo = 1;");

    ModuleResolver resolver;
    auto modules = resolver.Resolve(GetFullPath("main.js"));
    
    ASSERT_FALSE(resolver.HasErrors());
    // utils.js 应该只出现一次
    EXPECT_EQ(modules.size(), 2);
}

/**
 * Property 3: 深层嵌套目录
 */
TEST_F(ModuleResolverPropertyTest, DeepNestedDirectories) {
    CreateFile("src/app/main.js", "import '../../lib/utils/helper.js';");
    CreateFile("lib/utils/helper.js", "export const help = 1;");

    ModuleResolver resolver;
    auto modules = resolver.Resolve(GetFullPath("src/app/main.js"));
    
    ASSERT_FALSE(resolver.HasErrors());
    EXPECT_EQ(modules.size(), 2);
}

