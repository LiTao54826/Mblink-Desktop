/**
 * @file dom_render_test.cpp
 * @brief DOM 渲染测试程序 - 与浏览器一对一对比验证
 *
 * 使用方法:
 *   dom_render_test.exe          # 正常运行，手动关闭窗口
 *   dom_render_test.exe -q       # 渲染完成后立即退出
 *   dom_render_test.exe -q > output.json  # 输出重定向到文件
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/event/event_loop.h"
#include "core/render/render_object.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>

using namespace lightui;

// 全局变量：是否静默模式（-q 时日志输出到 stderr）
bool g_quiet_mode = false;

// 日志宏：-q 模式下输出到 stderr
#define LOG_INFO(msg) do { \
    if (!g_quiet_mode) std::cout << "[INFO] " << msg << std::endl; \
    else std::cerr << "[INFO] " << msg << std::endl; \
} while(0)

#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 尝试多个路径加载文件
std::string LoadFileFromPaths(const std::vector<std::string>& paths, const std::string& filename) {
    for (const auto& path : paths) {
        std::string code = ReadFile(path);
        if (!code.empty()) {
            LOG_INFO("Found " << filename << " at: " << path);
            return code;
        }
    }
    return "";
}

// 递归提取渲染数据为 JSON
std::string ExtractRenderDataJSON(const std::shared_ptr<RenderObject>& render_obj, int depth = 0) {
    if (!render_obj) return "null";
    
    auto node = render_obj->GetNode();
    auto element = std::dynamic_pointer_cast<Element>(node);
    
    const auto& layout = render_obj->GetLayoutInfo();
    const auto& style = render_obj->GetComputedStyle();
    
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    
    json << "{";
    
    // 基本信息
    std::string tag = element ? element->GetTagName() : "#text";
    std::string id = element ? element->GetAttribute("id") : "";
    std::string className = element ? element->GetAttribute("class") : "";
    
    json << "\"tag\":\"" << tag << "\"";
    if (!id.empty()) json << ",\"id\":\"" << id << "\"";
    if (!className.empty()) json << ",\"class\":\"" << className << "\"";
    json << ",\"depth\":" << depth;
    
    // 布局数据
    json << ",\"layout\":{";
    json << "\"x\":" << layout.x;
    json << ",\"y\":" << layout.y;
    json << ",\"width\":" << layout.width;
    json << ",\"height\":" << layout.height;
    json << "}";
    
    // 盒模型
    json << ",\"box\":{";
    json << "\"marginTop\":" << style.margin_top.value;
    json << ",\"marginRight\":" << style.margin_right.value;
    json << ",\"marginBottom\":" << style.margin_bottom.value;
    json << ",\"marginLeft\":" << style.margin_left.value;
    json << ",\"paddingTop\":" << style.padding_top.value;
    json << ",\"paddingRight\":" << style.padding_right.value;
    json << ",\"paddingBottom\":" << style.padding_bottom.value;
    json << ",\"paddingLeft\":" << style.padding_left.value;
    json << ",\"borderTop\":" << style.border_top_width;
    json << ",\"borderRight\":" << style.border_right_width;
    json << ",\"borderBottom\":" << style.border_bottom_width;
    json << ",\"borderLeft\":" << style.border_left_width;
    json << "}";
    
    // 布局模式
    std::string display_str;
    switch (style.display) {
        case RenderObjectType::BLOCK: display_str = "block"; break;
        case RenderObjectType::INLINE: display_str = "inline"; break;
        case RenderObjectType::INLINE_BLOCK: display_str = "inline-block"; break;
        case RenderObjectType::FLEX: display_str = "flex"; break;
        case RenderObjectType::GRID: display_str = "grid"; break;
        case RenderObjectType::TEXT: display_str = "text"; break;
        case RenderObjectType::NONE: display_str = "none"; break;
        default: display_str = "block"; break;
    }
    json << ",\"style\":{";
    json << "\"display\":\"" << display_str << "\"";
    json << ",\"position\":\"" << style.position << "\"";
    json << ",\"textAlign\":\"" << style.text_align << "\"";
    json << "}";
    
    // 递归子元素
    const auto& children = render_obj->GetChildren();
    if (!children.empty()) {
        json << ",\"children\":[";
        bool first = true;
        for (const auto& child : children) {
            if (!first) json << ",";
            first = false;
            json << ExtractRenderDataJSON(child, depth + 1);
        }
        json << "]";
    }
    
    json << "}";
    return json.str();
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    bool quick_exit = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-q" || arg == "--quick") {
            quick_exit = true;
            g_quiet_mode = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: dom_render_test.exe [options]\n"
                      << "Options:\n"
                      << "  -q, --quick    Exit immediately after rendering, output JSON to stdout\n"
                      << "  -h, --help     Show this help message\n";
            return 0;
        }
    }

    try {
        LOG_INFO("MBink DOM Render Test");
        if (quick_exit) {
            LOG_INFO("Quick exit mode enabled");
        }

        // 1. 创建窗口 (800x600 固定尺寸，与浏览器一致)
        LOG_INFO("Creating window (800x600)...");
        WindowConfig config;
        config.title = "MBink DOM Render Test";
        config.width = 800;
        config.height = 600;
        auto window = std::make_shared<Window>(config);

        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);

        // 2. 创建文档
        LOG_INFO("Creating document...");
        auto document = std::make_shared<Document>();
        document->Initialize();

        // 3. 创建 QuickJS 运行时
        LOG_INFO("Creating QuickJS runtime...");
        auto runtime = std::make_unique<QuickJSRuntime>();
        JSContext* ctx = runtime->GetContext();

        // 4. 初始化 DOM 绑定
        LOG_INFO("Initializing DOM bindings...");
        DOMBindings::Init(ctx);
        DOMBindings::SetGlobalDocument(ctx, document);

        // 5. 创建 body 元素
        auto body = document->CreateElement("body");
        body->SetAttribute("style", "margin: 0; padding: 0; width: 800px; height: 600px;");
        document->SetBody(body);

        // 6. 加载 Preact 库
        LOG_INFO("Loading Preact library...");
        std::vector<std::string> preact_paths = {
            "js/preact/preact.js",
            "../js/preact/preact.js",
            "../../js/preact/preact.js",
            "examples/demo_html/js/preact/preact.js",
            "../examples/demo_html/js/preact/preact.js",
        };
        std::string preact_code = LoadFileFromPaths(preact_paths, "preact.js");
        if (preact_code.empty()) {
            LOG_ERROR("Failed to load preact.js");
            return 1;
        }
        runtime->Eval(preact_code, "preact.js");

        std::vector<std::string> hooks_paths = {
            "js/preact/hooks.js",
            "../js/preact/hooks.js",
            "../../js/preact/hooks.js",
            "examples/demo_html/js/preact/hooks.js",
            "../examples/demo_html/js/preact/hooks.js",
        };
        std::string hooks_code = LoadFileFromPaths(hooks_paths, "hooks.js");
        if (hooks_code.empty()) {
            LOG_ERROR("Failed to load hooks.js");
            return 1;
        }
        runtime->Eval(hooks_code, "hooks.js");

        // 7. 加载测试用例 app.js
        LOG_INFO("Loading test app.js...");
        std::vector<std::string> app_paths = {
            "dom_render_test_cases/app.js",
            "../dom_render_test_cases/app.js",
            "../../dom_render_test_cases/app.js",
            "tests/dom_render_comparison/test_cases/app.js",
            "../tests/dom_render_comparison/test_cases/app.js",
        };
        std::string app_code = LoadFileFromPaths(app_paths, "app.js");
        if (app_code.empty()) {
            LOG_ERROR("Failed to load app.js");
            return 1;
        }
        runtime->Eval(app_code, "app.js");

        // 8. 执行渲染
        LOG_INFO("Rendering...");
        window->SetDocument(document);
        window->Show();
        window->RenderDocument();
        window->SwapBuffers();

        // 9. 提取渲染数据并输出 JSON
        LOG_INFO("Extracting render data...");
        auto render_tree = window->GetCachedRenderTree();
        if (render_tree) {
            std::string json = ExtractRenderDataJSON(render_tree);
            // JSON 数据始终输出到 stdout
            std::cout << "__RENDER_DATA__" << json << std::endl;
        } else {
            LOG_ERROR("No render tree available");
            std::cout << "__RENDER_DATA__null" << std::endl;
        }

        LOG_INFO("Render complete.");

        // 10. 根据 -q 参数决定是否等待
        if (!quick_exit) {
            LOG_INFO("Close window to exit. (Use -q to auto exit)");
            EventLoop event_loop;
            event_loop.SetRenderCallback([window]() {
                if (window->NeedsRepaint()) {
                    window->RenderDocument();
                    window->SwapBuffers();
                }
            });
            event_loop.Run();
        }

        // 清理
        DOMBindings::Cleanup(ctx);
        LOG_INFO("Done.");

        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR(e.what());
        return 1;
    }
}

