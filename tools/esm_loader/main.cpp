/**
 * @file main.cpp
 * @brief MBink ESM Loader - 支持 ES 模块的应用加载器
 *
 * 用法: esm_loader.exe <entry.js> [选项]
 *
 * 选项:
 *   --width <宽度>      窗口宽度 (默认: 800)
 *   --height <高度>     窗口高度 (默认: 600)
 *   --title <标题>      窗口标题 (默认: MBink App)
 *   --devtools          启动时打开开发者工具
 */

#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/quickjs/dom_binding_map.h"
#include "core/event/loop/task_scheduler.h"
#include "core/event/loop/event_loop.h"
#include "core/devtools/devtools_manager.h"
#include "core/render/text/font_manager.h"
#include "core/bridge/host_bridge.h"
#include "core/bridge/state_manager.h"
#include "embedded_js.h"

extern "C" {
#include "quickjs/quickjs.h"
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

// 打印调用栈
void PrintStackTrace(CONTEXT* context) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    // 初始化符号处理
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(process, NULL, TRUE)) {
        std::cerr << "[CRASH] Failed to initialize symbols" << std::endl;
        return;
    }
    
    STACKFRAME64 stackFrame = {};
    DWORD machineType;
    
#ifdef _M_X64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#else
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#endif
    
    std::cerr << "\n[CRASH] Call Stack:" << std::endl;
    
    char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    
    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    
    int frameNum = 0;
    while (StackWalk64(machineType, process, thread, &stackFrame, context,
                       NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        if (stackFrame.AddrPC.Offset == 0) break;
        if (frameNum >= 30) break;  // 限制栈深度
        
        DWORD64 displacement = 0;
        DWORD lineDisplacement = 0;
        
        std::cerr << "  [" << frameNum << "] 0x" << std::hex << stackFrame.AddrPC.Offset << std::dec;
        
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
            std::cerr << " " << symbol->Name;
        }
        
        if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &line)) {
            std::cerr << " (" << line.FileName << ":" << line.LineNumber << ")";
        }
        
        std::cerr << std::endl;
        frameNum++;
    }
    
    SymCleanup(process);
}

// Windows 崩溃处理
LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    std::cerr << "\n[CRASH] Unhandled exception caught!" << std::endl;
    std::cerr << "[CRASH] Exception code: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionCode << std::dec << std::endl;
    std::cerr << "[CRASH] Exception address: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionAddress << std::dec << std::endl;
    
    // 输出异常类型
    switch (pExceptionInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            std::cerr << "[CRASH] Type: Access Violation (null pointer or invalid memory access)" << std::endl;
            if (pExceptionInfo->ExceptionRecord->NumberParameters >= 2) {
                std::cerr << "[CRASH] " << (pExceptionInfo->ExceptionRecord->ExceptionInformation[0] ? "Write" : "Read") 
                          << " at address: 0x" << std::hex << pExceptionInfo->ExceptionRecord->ExceptionInformation[1] << std::dec << std::endl;
            }
            break;
        case EXCEPTION_STACK_OVERFLOW:
            std::cerr << "[CRASH] Type: Stack Overflow" << std::endl;
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            std::cerr << "[CRASH] Type: Integer Divide by Zero" << std::endl;
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            std::cerr << "[CRASH] Type: Illegal Instruction" << std::endl;
            break;
        default:
            std::cerr << "[CRASH] Type: Unknown" << std::endl;
            break;
    }
    
    // 打印调用栈
    PrintStackTrace(pExceptionInfo->ContextRecord);
    
    std::cerr.flush();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

using namespace lightui;
namespace fs = std::filesystem;

std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void PrintUsage(const char* program_name) {
    std::cout << "MBink ESM Loader - ES 模块加载器" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << program_name << " <entry.js> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --width <宽度>      窗口宽度 (默认: 800)" << std::endl;
    std::cout << "  --height <高度>     窗口高度 (默认: 600)" << std::endl;
    std::cout << "  --title <标题>      窗口标题 (默认: MBink App)" << std::endl;
    std::cout << "  --devtools          启动时打开开发者工具" << std::endl;
    std::cout << "  -q, --quit <帧数>   渲染指定帧数后自动退出 (用于调试)" << std::endl;
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " app.js" << std::endl;
    std::cout << "  " << program_name << " app.js --width 1024 --height 768" << std::endl;
    std::cout << "  " << program_name << " app.js -q 3  # 渲染3帧后退出" << std::endl;
}

// 加载嵌入的 JS 库
bool LoadEmbeddedLibraries(QuickJSRuntime* runtime) {
    using namespace lightui::embedded;

    if (!HasEmbeddedJS()) {
        return false;
    }

    try {
        // 加载 DOM polyfills
        auto polyfills = GetDomPolyfillsJS();
        if (!polyfills.empty()) {
            std::string polyfills_str(polyfills);
            runtime->Eval(polyfills_str, "dom.js");
            std::cout << "  ✓ DOM polyfills loaded" << std::endl;
        }

        // 加载 Preact（作为全局对象）
        auto preact = GetPreactJS();
        if (!preact.empty()) {
            std::string preact_str(preact);
            runtime->Eval(preact_str, "preact.js");
            std::cout << "  ✓ Preact loaded (global)" << std::endl;
        }

        // 加载 Hooks
        auto hooks = GetHooksJS();
        if (!hooks.empty()) {
            std::string hooks_str(hooks);
            runtime->Eval(hooks_str, "hooks.js");
            std::cout << "  ✓ Hooks loaded (global)" << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Error loading embedded JS: " << e.what() << std::endl;
        return false;
    }
}

// 注册 Preact 为 ES 模块
void RegisterPreactModules(QuickJSRuntime* runtime) {
    // 注册 preact 模块（从全局对象导出）
    runtime->RegisterModule("preact", R"(
        export const h = globalThis.Preact.h;
        export const render = globalThis.Preact.render;
        export const Component = globalThis.Preact.Component;
        export const Fragment = globalThis.Preact.Fragment;
        export const createRef = globalThis.Preact.createRef;
        export const createElement = globalThis.Preact.createElement;
        export const createContext = globalThis.Preact.createContext;
        export const cloneElement = globalThis.Preact.cloneElement;
        export const isValidElement = globalThis.Preact.isValidElement;
        export default globalThis.Preact;
    )");

    // 注册 preact/hooks 模块
    runtime->RegisterModule("preact/hooks", R"(
        export const useState = globalThis.PreactHooks.useState;
        export const useEffect = globalThis.PreactHooks.useEffect;
        export const useRef = globalThis.PreactHooks.useRef;
        export const useMemo = globalThis.PreactHooks.useMemo;
        export const useCallback = globalThis.PreactHooks.useCallback;
        export const useContext = globalThis.PreactHooks.useContext;
        export const useReducer = globalThis.PreactHooks.useReducer;
        export const useLayoutEffect = globalThis.PreactHooks.useLayoutEffect;
        export const useImperativeHandle = globalThis.PreactHooks.useImperativeHandle;
        export const useDebugValue = globalThis.PreactHooks.useDebugValue;
        export default globalThis.PreactHooks;
    )");

    std::cout << "  ✓ Preact ES modules registered" << std::endl;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    // 注册崩溃处理器
    SetUnhandledExceptionFilter(CrashHandler);
    
    // 设置 Windows 控制台为 UTF-8 编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // 启用 ANSI 转义序列支持（用于颜色等）
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    std::string entry_path;
    int width = 1200;
    int height = 800;
    std::string title = "MBink App";
    bool open_devtools = false;
    float quit_after_seconds = 0;  // 0 表示不自动退出，单位：秒

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if (arg == "--title" && i + 1 < argc) {
            title = argv[++i];
        } else if (arg == "--devtools") {
            open_devtools = true;
        } else if ((arg == "-q" || arg == "--quit") && i + 1 < argc) {
            quit_after_seconds = std::stof(argv[++i]);
        } else if (arg[0] != '-') {
            entry_path = arg;
        }
    }

    if (entry_path.empty()) {
        std::cerr << "错误: 未指定入口文件" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }

    if (!fs::exists(entry_path)) {
        std::cerr << "错误: 文件不存在: " << entry_path << std::endl;
        return 1;
    }

    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  MBink ESM Loader" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Entry: " << entry_path << std::endl;
        std::cout << "  Size: " << width << "x" << height << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout.flush();

        // 1. 创建窗口
        std::cout << "[1/5] Creating window..." << std::endl;
        std::cout.flush();
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;
        std::cout << "[DEBUG] WindowConfig created" << std::endl; std::cout.flush();

        auto window = std::make_shared<Window>(config);
        std::cout << "[DEBUG] Window object created" << std::endl; std::cout.flush();
        auto& window_manager = WindowManager::Instance();
        std::cout << "[DEBUG] WindowManager instance obtained" << std::endl; std::cout.flush();
        window_manager.RegisterWindow(window);
        std::cout << "  ✓ Window created" << std::endl; std::cout.flush();

        // 2. 创建文档
        std::cout << "[2/5] Creating document..." << std::endl; std::cout.flush();
        auto document = std::make_shared<Document>();
        std::cout << "[DEBUG] Document object created" << std::endl; std::cout.flush();
        document->Initialize();
        std::cout << "[DEBUG] Document initialized" << std::endl; std::cout.flush();
        auto body = document->CreateElement("body");
        std::cout << "[DEBUG] Body element created" << std::endl; std::cout.flush();
        document->SetBody(body);
        std::cout << "[DEBUG] Body set to document" << std::endl; std::cout.flush();
        window->SetDocument(document);
        std::cout << "  ✓ Document initialized" << std::endl; std::cout.flush();

        // 3. 创建 QuickJS 运行时
        std::cout << "[3/5] Creating QuickJS runtime..." << std::endl; std::cout.flush();
        auto runtime = std::make_unique<QuickJSRuntime>();
        std::cout << "[DEBUG] QuickJSRuntime created" << std::endl; std::cout.flush();
        auto task_scheduler = std::make_shared<TaskScheduler>();
        std::cout << "  ✓ QuickJS runtime created" << std::endl; std::cout.flush();

        // 4. 初始化绑定和库
        std::cout << "[4/5] Initializing bindings..." << std::endl; std::cout.flush();
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        std::cout << "[DEBUG] WindowBindings created" << std::endl; std::cout.flush();
        window_bindings.InitBindings();
        std::cout << "  ✓ Window bindings initialized" << std::endl; std::cout.flush();

        // 初始化 StateManager 和 HostBridge
        auto state_manager = std::make_unique<StateManager>();
        auto host_bridge = std::make_unique<HostBridge>(runtime->GetContext(), state_manager.get());
        host_bridge->registerGlobal();
        std::cout << "  ✓ Host bridge initialized" << std::endl; std::cout.flush();

        // 创建事件循环（需要在加载模块之前，以便 getSelection 等 API 可用）
        std::cout << "[DEBUG] Creating EventLoop..." << std::endl; std::cout.flush();
        EventLoop event_loop(task_scheduler);
        std::cout << "[DEBUG] EventLoop created" << std::endl; std::cout.flush();
        event_loop.SetQuickJSRuntime(runtime.get());
        std::cout << "[DEBUG] EventLoop SetQuickJSRuntime done" << std::endl; std::cout.flush();
        DOMBindings::SetGlobalEventLoop(runtime->GetContext(), &event_loop);
        std::cout << "[DEBUG] SetGlobalEventLoop done" << std::endl; std::cout.flush();

        // 加载嵌入的库
        if (lightui::embedded::HasEmbeddedJS()) {
            LoadEmbeddedLibraries(runtime.get());
            RegisterPreactModules(runtime.get());
        } else {
            std::cerr << "  ⚠ No embedded JS libraries" << std::endl;
        }

        // 5. 加载入口模块
        std::cout << "[5/5] Loading entry module..." << std::endl;

        // 设置模块基础路径
        fs::path abs_path = fs::absolute(entry_path);
        runtime->SetBaseModulePath(abs_path.string());

        // 读取入口文件
        std::string entry_code = ReadFile(entry_path);
        if (entry_code.empty()) {
            std::cerr << "  ✗ Failed to read: " << entry_path << std::endl;
            return 1;
        }

        // 使用 EvalModule 执行 ES 模块
        try {
            runtime->EvalModule(entry_code, abs_path.string());
            std::cout << "  ✓ Entry module loaded" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  ✗ Module error: " << e.what() << std::endl;
            return 1;
        }

        // 显示窗口
        window->Show();

        // 初始化 DevTools
        auto& devtools = DevToolsManager::GetInstance();
        devtools.Initialize(document.get(), window.get());
        if (open_devtools) {
            devtools.Open();
        }

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  🚀 Application Started!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Press F12 to toggle DevTools" << std::endl;
        std::cout << std::endl;

        // 设置渲染回调
        event_loop.SetRenderCallback([window]() {
            if (window->NeedsRepaint()) {
                window->Render();
                window->SwapBuffers();
            }
        });
        
        // 如果设置了自动退出，使用更新回调计时
        if (quit_after_seconds > 0) {
            std::cout << "[Auto-quit] Will quit after " << quit_after_seconds << " seconds" << std::endl;
            auto elapsed_time = std::make_shared<float>(0.0f);
            auto frame_count = std::make_shared<int>(0);
            event_loop.SetUpdateCallback([elapsed_time, frame_count, quit_after_seconds, &event_loop](float delta_time) {
                (*frame_count)++;
                *elapsed_time += delta_time;
                // 每 60 帧输出一次调试信息
                if (*elapsed_time >= quit_after_seconds) {
                    std::cout << "[Auto-quit] Completed " << *elapsed_time << " seconds (" << *frame_count << " frames), exiting..." << std::endl;
                    event_loop.Stop();
                }
            });
        }
        
        event_loop.Run();

        // 清理 - 注意顺序：先释放持有 JSValue 的对象，最后释放 QuickJS 运行时
        std::cout << std::endl;
        std::cout << "Shutting down..." << std::endl;
        
        // 1. 关闭 DevTools（可能持有 DOM 引用）
        devtools.Shutdown();
        
        // 2. 清理字体缓存
        FontManager::GetInstance().ClearCache();
        
        // 3. 注销并释放窗口（可能持有事件回调）
        window_manager.UnregisterWindow(window);
        window.reset();
        
        // 4. 释放 document（持有 DOM 树和事件监听器，这些可能包含 JSValue）
        document.reset();
        
        // 5. 清理 DOM 绑定缓存（释放缓存中的 JSValue）
        DOMBindings::Cleanup(runtime->GetContext());
        
        // 6. 清理 DOM 绑定映射（释放所有 Node* -> JSValue 的映射）
        // 必须在 QuickJS 运行时销毁之前调用
        DOMBindingMap::GetInstance().Clear();
        
        // 7. 最后释放 QuickJS 运行时（此时所有 JSValue 应该已被释放）
        runtime.reset();

        std::quick_exit(0);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
