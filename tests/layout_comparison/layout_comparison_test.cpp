/**
 * @file layout_comparison_test.cpp
 * @brief 布局比较测试 - 与浏览器渲染结果对比
 * 
 * 这个测试程序：
 * 1. 加载测试 HTML 文件
 * 2. 渲染布局
 * 3. 提取布局信息并输出 JSON
 * 4. 与浏览器数据进行比较
 */

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/render/render_object.h"
#include "core/layout/layout_engine.h"

using namespace lightui;

/**
 * @brief 布局数据结构
 */
struct LayoutData {
    std::string test_id;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    // 转换为 JSON 字符串
    std::string ToJSON() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "{\n";
        ss << "  \"testId\": \"" << test_id << "\",\n";
        ss << "  \"viewport\": {\n";
        ss << "    \"x\": " << x << ",\n";
        ss << "    \"y\": " << y << ",\n";
        ss << "    \"width\": " << width << ",\n";
        ss << "    \"height\": " << height << "\n";
        ss << "  }\n";
        ss << "}";
        return ss.str();
    }
};

/**
 * @brief 布局比较测试类
 */
class LayoutComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建隐藏窗口
        WindowConfig config;
        config.title = "Layout Comparison Test";
        config.width = 800;
        config.height = 600;
        config.hidden = true;  // 隐藏窗口
        config.backend = RenderBackend::CPU;
        
        window_ = std::make_shared<Window>(config);
        
        // 创建文档
        document_ = std::make_shared<Document>();
        document_->Initialize();
    }
    
    void TearDown() override {
        window_.reset();
        document_.reset();
    }
    
    /**
     * @brief 读取 HTML 文件
     */
    std::string ReadFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    /**
     * @brief 递归遍历渲染树，收集带有 data-test 属性的元素布局
     */
    void CollectLayoutData(std::shared_ptr<RenderObject> render_obj,
                           std::vector<LayoutData>& layouts,
                           float parent_x = 0.0f,
                           float parent_y = 0.0f) {
        if (!render_obj) return;
        
        auto node = render_obj->GetNode();
        if (node) {
            auto element = std::dynamic_pointer_cast<Element>(node);
            if (element) {
                std::string test_id = element->GetAttribute("data-test");
                if (!test_id.empty()) {
                    const auto& layout = render_obj->GetLayoutInfo();
                    
                    LayoutData data;
                    data.test_id = test_id;
                    data.x = parent_x + layout.x;
                    data.y = parent_y + layout.y;
                    data.width = layout.width;
                    data.height = layout.height;
                    
                    layouts.push_back(data);
                }
            }
        }
        
        // 计算当前元素的绝对位置
        const auto& layout = render_obj->GetLayoutInfo();
        float abs_x = parent_x + layout.x;
        float abs_y = parent_y + layout.y;
        
        // 递归处理子元素
        for (const auto& child : render_obj->GetChildren()) {
            CollectLayoutData(child, layouts, abs_x, abs_y);
        }
    }
    
    /**
     * @brief 输出所有布局数据为 JSON
     */
    std::string LayoutsToJSON(const std::vector<LayoutData>& layouts) {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"source\": \"MBink\",\n";
        ss << "  \"viewport\": {\n";
        ss << "    \"width\": 800,\n";
        ss << "    \"height\": 600\n";
        ss << "  },\n";
        ss << "  \"elements\": {\n";
        
        for (size_t i = 0; i < layouts.size(); ++i) {
            const auto& layout = layouts[i];
            ss << "    \"" << layout.test_id << "\": {\n";
            ss << std::fixed << std::setprecision(2);
            ss << "      \"viewport\": {\n";
            ss << "        \"x\": " << layout.x << ",\n";
            ss << "        \"y\": " << layout.y << ",\n";
            ss << "        \"width\": " << layout.width << ",\n";
            ss << "        \"height\": " << layout.height << "\n";
            ss << "      }\n";
            ss << "    }";
            if (i < layouts.size() - 1) ss << ",";
            ss << "\n";
        }
        
        ss << "  }\n";
        ss << "}\n";
        return ss.str();
    }
    
    std::shared_ptr<Window> window_;
    std::shared_ptr<Document> document_;
};

/**
 * @brief 测试基本文本流布局
 */
TEST_F(LayoutComparisonTest, BasicTextFlow) {
    // 简单的 HTML
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial; font-size: 16px; padding: 20px; }
        .content { width: 300px; background: #f0f0f0; }
        span { background: rgba(255, 200, 200, 0.5); }
    </style>
</head>
<body>
    <div class="content" data-test="test1-content">
        <span data-test="test1-span">Hello World</span>
    </div>
</body>
</html>
    )";
    
    // 加载 HTML
    document_->LoadHTML(html);
    window_->SetDocument(document_);
    
    // 强制布局
    window_->EnsureRenderTree();

    // 获取渲染树
    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);
    
    // 收集布局数据
    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);
    
    // 输出 JSON
    std::string json = LayoutsToJSON(layouts);
    std::cout << "\n=== MBink Layout Data ===\n" << json << std::endl;
    
    // 验证找到了测试元素
    EXPECT_GE(layouts.size(), 1);
    
    // 验证宽度正确
    for (const auto& layout : layouts) {
        if (layout.test_id == "test1-content") {
            // 容器宽度应该是 300px
            EXPECT_NEAR(layout.width, 300.0f, 1.0f);
        }
    }
}

/**
 * @brief 测试内联块布局
 */
TEST_F(LayoutComparisonTest, InlineBlockLayout) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial; font-size: 16px; padding: 20px; }
        .content { width: 300px; }
        .inline-block-item {
            display: inline-block;
            width: 80px;
            height: 40px;
            background: rgba(255, 255, 200, 0.5);
            border: 1px solid #888;
            vertical-align: middle;
        }
    </style>
</head>
<body>
    <div class="content" data-test="test4-content">
        <div class="inline-block-item" data-test="test4-item1">Box 1</div>
        <div class="inline-block-item" data-test="test4-item2">Box 2</div>
        <div class="inline-block-item" data-test="test4-item3">Box 3</div>
    </div>
</body>
</html>
    )";
    
    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    std::string json = LayoutsToJSON(layouts);
    std::cout << "\n=== MBink Inline-Block Layout ===\n" << json << std::endl;

    // 验证 inline-block 项的尺寸
    for (const auto& layout : layouts) {
        if (layout.test_id.find("test4-item") != std::string::npos) {
            EXPECT_NEAR(layout.width, 80.0f, 2.0f);
            EXPECT_NEAR(layout.height, 40.0f, 2.0f);
        }
    }
}

/**
 * @brief 测试文本缩进
 */
TEST_F(LayoutComparisonTest, TextIndent) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial; font-size: 16px; padding: 20px; }
        .content { width: 300px; text-indent: 40px; }
    </style>
</head>
<body>
    <div class="content" data-test="test6-content">
        <span data-test="test6-span">This paragraph has text indent.</span>
    </div>
</body>
</html>
    )";
    
    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    std::string json = LayoutsToJSON(layouts);
    std::cout << "\n=== MBink Text Indent Layout ===\n" << json << std::endl;

    EXPECT_GE(layouts.size(), 1);
}

/**
 * @brief 调试测试：检查元素类型
 */
TEST_F(LayoutComparisonTest, DebugElementTypes) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial; font-size: 16px; padding: 20px; }
        .content { width: 300px; }
        .inline-block-item {
            display: inline-block;
            width: 80px;
            height: 40px;
            background: rgba(255, 255, 200, 0.5);
        }
    </style>
</head>
<body>
    <div class="content" data-test="test4-content">
        <div class="inline-block-item" data-test="test4-item1">Box 1</div>
        <div class="inline-block-item" data-test="test4-item2">Box 2</div>
        <div class="inline-block-item" data-test="test4-item3">Box 3</div>
    </div>
</body>
</html>
    )";

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    // 递归打印元素类型
    std::function<void(std::shared_ptr<RenderObject>, int)> printTypes;
    printTypes = [&printTypes](std::shared_ptr<RenderObject> obj, int depth) {
        if (!obj) return;

        std::string indent(depth * 2, ' ');
        std::string test_id;
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            test_id = elem->GetAttribute("data-test");
        }

        std::string type_name;
        switch (obj->GetType()) {
            case RenderObjectType::BLOCK: type_name = "BLOCK"; break;
            case RenderObjectType::INLINE: type_name = "INLINE"; break;
            case RenderObjectType::TEXT: type_name = "TEXT"; break;
            case RenderObjectType::INLINE_BLOCK: type_name = "INLINE_BLOCK"; break;
            case RenderObjectType::FLEX: type_name = "FLEX"; break;
            default: type_name = "OTHER"; break;
        }

        std::cout << indent << type_name;
        if (!test_id.empty()) {
            std::cout << " [" << test_id << "]";
        }
        std::cout << " w=" << obj->GetLayoutInfo().width
                  << " h=" << obj->GetLayoutInfo().height << std::endl;

        for (auto& child : obj->GetChildren()) {
            printTypes(child, depth + 1);
        }
    };

    std::cout << "\n=== Element Types Debug ===\n";
    printTypes(render_tree, 0);
}

/**
 * @brief 加载完整测试 HTML 并输出布局
 */
TEST_F(LayoutComparisonTest, FullTestCases) {
    // 尝试多个可能的路径
    std::vector<std::string> paths = {
        "layout_comparison/ifc_test_cases.html",  // 从 build/bin/Release 运行
        "tests/layout_comparison/ifc_test_cases.html",  // 从项目根目录运行
        "../../../tests/layout_comparison/ifc_test_cases.html",  // 从 build/bin/Release 向上
        "ifc_test_cases.html"  // 当前目录
    };

    std::string html;
    for (const auto& path : paths) {
        html = ReadFile(path);
        if (!html.empty()) {
            std::cout << "Loaded HTML from: " << path << std::endl;
            break;
        }
    }

    if (html.empty()) {
        GTEST_SKIP() << "Test HTML file not found in any of the expected paths";
    }

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    std::string json = LayoutsToJSON(layouts);

    // 保存到文件（尝试多个路径）
    std::vector<std::string> out_paths = {
        "layout_comparison/mbink_layout_data.json",
        "mbink_layout_data.json"
    };

    for (const auto& path : out_paths) {
        std::ofstream out(path);
        if (out.is_open()) {
            out << json;
            out.close();
            std::cout << "\n✓ Layout data saved to " << path << std::endl;
            break;
        }
    }

    std::cout << "\n=== MBink Full Layout Data ===\n" << json << std::endl;
    std::cout << "\nTotal elements with data-test: " << layouts.size() << std::endl;
}

/**
 * @brief 高级布局测试用例
 */
TEST_F(LayoutComparisonTest, AdvancedTestCases) {
    // 尝试多个可能的路径
    std::vector<std::string> paths = {
        "layout_comparison/advanced_test_cases.html",  // 从 build/bin/Release 运行
        "tests/layout_comparison/advanced_test_cases.html",  // 从项目根目录运行
        "../../../tests/layout_comparison/advanced_test_cases.html",  // 从 build/bin/Release 向上
        "advanced_test_cases.html"  // 当前目录
    };

    std::string html;
    for (const auto& path : paths) {
        html = ReadFile(path);
        if (!html.empty()) {
            std::cout << "Loaded HTML from: " << path << std::endl;
            break;
        }
    }

    if (html.empty()) {
        GTEST_SKIP() << "Advanced test HTML file not found in any of the expected paths";
    }

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    std::string json = LayoutsToJSON(layouts);

    // 保存到文件
    std::vector<std::string> out_paths = {
        "layout_comparison/mbink_advanced_layout_data.json",
        "mbink_advanced_layout_data.json"
    };

    for (const auto& path : out_paths) {
        std::ofstream out(path);
        if (out.is_open()) {
            out << json;
            out.close();
            std::cout << "\n✓ Advanced layout data saved to " << path << std::endl;
            break;
        }
    }

    std::cout << "\n=== MBink Advanced Layout Data ===\n" << json << std::endl;
    std::cout << "\nTotal advanced test elements: " << layouts.size() << std::endl;
}

/**
 * @brief 测试 line-height 继承
 */
TEST_F(LayoutComparisonTest, LineHeightInheritance) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial;
            font-size: 16px;
            line-height: 1.5;
            padding: 20px;
        }
        .content { width: 300px; }
    </style>
</head>
<body>
    <div class="content" data-test="test-content">
        <span data-test="test-span">Hello World</span>
    </div>
</body>
</html>
    )";

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    // 递归查找并打印 line-height
    std::function<void(std::shared_ptr<RenderObject>, int)> printLineHeight;
    printLineHeight = [&printLineHeight](std::shared_ptr<RenderObject> obj, int depth) {
        if (!obj) return;

        std::string indent(depth * 2, ' ');
        std::string test_id;
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            test_id = elem->GetAttribute("data-test");
        }

        const auto& style = obj->GetComputedStyle();
        std::cout << indent << "line-height=" << style.line_height
                  << " font-size=" << style.font_size;
        if (!test_id.empty()) {
            std::cout << " [" << test_id << "]";
        }
        std::cout << " h=" << obj->GetLayoutInfo().height << std::endl;

        for (auto& child : obj->GetChildren()) {
            printLineHeight(child, depth + 1);
        }
    };

    std::cout << "\n=== Line Height Debug ===\n";
    printLineHeight(render_tree, 0);

    // 验证 line-height 被正确继承
    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    for (const auto& layout : layouts) {
        if (layout.test_id == "test-content") {
            // 行高应该是 16 * 1.5 = 24px
            EXPECT_NEAR(layout.height, 24.0f, 2.0f) << "Content height should be ~24px (16 * 1.5)";
        }
    }
}

/**
 * @brief 测试 vertical-align 和绝对 line-height
 */
TEST_F(LayoutComparisonTest, VerticalAlignTest) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial;
            font-size: 16px;
            padding: 20px;
        }
        .content {
            width: 400px;
            line-height: 60px;
            background: #e0e0e0;
        }
        .va-top { vertical-align: top; background: #fcc; }
        .va-middle { vertical-align: middle; background: #cfc; }
        .va-bottom { vertical-align: bottom; background: #ccf; }
        .va-baseline { vertical-align: baseline; background: #ffc; }
        span {
            display: inline-block;
            width: 50px;
            height: 30px;
            margin: 0 5px;
        }
    </style>
</head>
<body>
    <div class="content" data-test="test5-content">
        <span class="va-top" data-test="test5-top">Top</span>
        <span class="va-middle" data-test="test5-middle">Mid</span>
        <span class="va-bottom" data-test="test5-bottom">Bot</span>
        <span class="va-baseline" data-test="test5-baseline">Base</span>
    </div>
</body>
</html>
    )";

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    // 递归查找并打印 line-height 和 vertical-align
    std::function<void(std::shared_ptr<RenderObject>, int)> printDebug;
    printDebug = [&printDebug](std::shared_ptr<RenderObject> obj, int depth) {
        if (!obj) return;

        std::string indent(depth * 2, ' ');
        std::string test_id;
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(node);
            test_id = elem->GetAttribute("data-test");
        }

        const auto& style = obj->GetComputedStyle();
        const auto& layout = obj->GetLayoutInfo();

        if (!test_id.empty()) {
            std::cout << indent << "[" << test_id << "] "
                      << "line-height=" << style.line_height
                      << " font-size=" << style.font_size
                      << " vertical-align=" << style.vertical_align
                      << " x=" << layout.x << " y=" << layout.y
                      << " w=" << layout.width << " h=" << layout.height
                      << std::endl;
        }

        for (const auto& child : obj->GetChildren()) {
            printDebug(child, depth + 1);
        }
    };

    std::cout << "\n=== Vertical Align Debug ===\n";
    printDebug(render_tree, 0);

    // 验证容器高度应该是 60px（line-height: 60px）
    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    for (const auto& layout : layouts) {
        if (layout.test_id == "test5-content") {
            // 容器高度应该是 60px（line-height: 60px）
            EXPECT_NEAR(layout.height, 60.0f, 2.0f) << "Content height should be ~60px (line-height: 60px)";
        }
    }
}

/**
 * @brief 测试 letter-spacing 和 word-spacing
 */
TEST_F(LayoutComparisonTest, LetterAndWordSpacing) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial;
            font-size: 16px;
            padding: 20px;
        }
        .letter-spacing {
            width: 300px;
            letter-spacing: 5px;
            background: #e0e0e0;
        }
        .word-spacing {
            width: 300px;
            word-spacing: 20px;
            background: #e0e0e0;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="letter-spacing" data-test="letter-spacing">
        <span data-test="letter-span">ABCDEFG</span>
    </div>
    <div class="word-spacing" data-test="word-spacing">
        <span data-test="word-span">A B C D</span>
    </div>
</body>
</html>
    )";

    document_->LoadHTML(html);
    window_->SetDocument(document_);
    window_->EnsureRenderTree();

    auto render_tree = window_->GetCachedRenderTree();
    ASSERT_NE(render_tree, nullptr);

    // 收集布局数据
    std::vector<LayoutData> layouts;
    CollectLayoutData(render_tree, layouts);

    // 打印调试信息
    std::cout << "\n=== Letter/Word Spacing Debug ===\n";
    for (const auto& layout : layouts) {
        if (!layout.test_id.empty()) {
            std::cout << "[" << layout.test_id << "] "
                      << "w=" << layout.width << " h=" << layout.height << std::endl;
        }
    }

    // 验证
    for (const auto& layout : layouts) {
        if (layout.test_id == "letter-span") {
            // "ABCDEFG" = 7 个字符
            // 基础宽度 = 7 * 8 = 56px (16 * 0.5)
            // letter-spacing = 5px * (7-1) = 30px
            // 总计 = 56 + 30 = 86px
            // 实际可能略有不同，主要检查 letter-spacing 是否生效
            EXPECT_GT(layout.width, 70.0f) << "Letter-spacing should increase width";
        }
        if (layout.test_id == "word-span") {
            // "A B C D" = 7 个字符，3 个空格
            // 基础宽度 = 4 * 8 + 3 * 4 = 32 + 12 = 44px
            // word-spacing = 20px * 3 = 60px
            // 总计 = 44 + 60 = 104px
            EXPECT_GT(layout.width, 80.0f) << "Word-spacing should increase width";
        }
    }
}

// ========== 主函数 ==========

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

