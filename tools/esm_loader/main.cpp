/**
 * @file main.cpp
 * @brief MBink ESM Loader - 支持 ES 模块和 HTML 的应用加载器
 *
 * 用法: esm_loader.exe <entry.js|index.html> [选项]
 *
 * 支持两种入口模式：
 *   1. JS 模式 (.js/.mjs) - 加载 ES 模块，适合 Preact 应用
 *   2. HTML 模式 (.html/.htm) - 解析 HTML 文件，执行其中的 <script> 标签
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
#include "core/network/fetch_bindings.h"
#include "core/devtools/devtools_manager.h"
#include "core/render/text/font_manager.h"
#include "core/render/image/image_loader.h"
#include "core/bridge/host_bridge.h"
#include "core/bridge/state_manager.h"
#include "core/utils/encoding_utils.h"
#include "core/quickjs/bindings/js_element.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "embedded_js.h"
#include "payload.h"
#include "bytecode_compiler.h"
#include "asset_manager.h"
#include "ui_dev_snapshot.h"
#include "ui_dev_control.h"
#include "ui_dev_runtime_support.h"

extern "C" {
#include "quickjs/quickjs.h"
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <deque>
#include <mutex>

#include <nlohmann/json.hpp>

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

using namespace mbink;
namespace fs = std::filesystem;

fs::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return fs::path(utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string FsPathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizeFsPath(const fs::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string ReadFile(const std::string& path) {
    std::ifstream file(Utf8PathToFsPath(path), std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 判断入口文件是否为 HTML 文件
bool IsHTMLFile(const std::string& path) {
    fs::path p = Utf8PathToFsPath(path);
    auto ext = FsPathToUtf8String(p.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".html" || ext == ".htm";
}

// 从文档中获取 <title> 标签内容
std::string GetDocumentTitle(std::shared_ptr<Document> doc) {
    auto titles = doc->GetElementsByTagName("title");
    if (!titles.empty()) {
        return titles[0]->GetTextContent();
    }
    return "";
}

void PrintUsage(const char* program_name) {
    std::cout << "MBink Loader - ES 模块 / HTML 应用加载器" << std::endl;
    std::cout << std::endl;
    std::cout << "用法: " << program_name << " <entry.js|index.html> [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "支持两种入口模式:" << std::endl;
    std::cout << "  .js/.mjs   JS 模式 - 加载 ES 模块，适合 Preact 应用" << std::endl;
    std::cout << "  .html/.htm HTML 模式 - 解析 HTML，执行 <script> 标签" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --width <宽度>      窗口宽度 (默认: 1200)" << std::endl;
    std::cout << "  --height <高度>     窗口高度 (默认: 800)" << std::endl;
    std::cout << "  --title <标题>      窗口标题 (默认: MBink App / HTML title)" << std::endl;
    std::cout << "  --borderless        无边框窗口模式（支持不规则窗体）" << std::endl;
    std::cout << "  --transparent       透明窗口（需配合 --borderless 使用）" << std::endl;
    std::cout << "  --no-gpu            关闭GPU加速（使用CPU渲染，减少内存占用）" << std::endl;
    std::cout << "  --min-width <宽度>  窗口最小宽度" << std::endl;
    std::cout << "  --min-height <高度> 窗口最小高度" << std::endl;
    std::cout << "  --max-width <宽度>  窗口最大宽度" << std::endl;
    std::cout << "  --max-height <高度> 窗口最大高度" << std::endl;
    std::cout << "  --no-scripts        不执行脚本 (仅 HTML 模式)" << std::endl;
    std::cout << "  --no-official-preact 禁用内置官方 Preact 模块注册（用于验证 node_modules/npm preact）" << std::endl;
    std::cout << "  --devtools          启动时打开开发者工具" << std::endl;
    std::cout << "  --ui-dev-snapshot-file <路径>   导出 UI Dev snapshot JSON" << std::endl;
    std::cout << "  --ui-dev-command-file <路径>    读取 UI Dev command JSON" << std::endl;
    std::cout << "  --ui-dev-response-file <路径>   写入 UI Dev response JSON" << std::endl;
    std::cout << "  --ui-dev-console-file <路径>    写入结构化 console JSON" << std::endl;
    std::cout << "  --ui-dev-errors-file <路径>     写入结构化 JS error JSON" << std::endl;
    std::cout << "  --ui-dev-lifecycle-file <路径>  写入 runtime 生命周期 JSON" << std::endl;
    std::cout << "  -q, --quit <秒>     自动退出时间（秒）" << std::endl;
    std::cout << "  --help              显示此帮助信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << " app.js" << std::endl;
    std::cout << "  " << program_name << " index.html" << std::endl;
    std::cout << "  " << program_name << " app.html --width 1024 --height 768" << std::endl;
    std::cout << "  " << program_name << " test.html -q 5  # 5秒后自动退出" << std::endl;
    std::cout << "  " << program_name << " app.js --no-official-preact  # 强制只走 node_modules/npm preact" << std::endl;
}

// 加载嵌入的 JS 库
bool LoadEmbeddedLibraries(QuickJSRuntime* runtime) {
    using namespace mbink::embedded;

    if (!HasEmbeddedJS()) {
        return false;
    }

    try {
        auto polyfills = GetDomPolyfillsJS();
        if (!polyfills.empty()) {
            std::string polyfills_str(polyfills);
            runtime->Eval(polyfills_str, "dom.js");
            std::cout << "  ✓ DOM polyfills loaded" << std::endl;
        }

        auto bootstrap = GetBootstrapJS();
        if (!bootstrap.empty()) {
            std::string bootstrap_str(bootstrap);
            runtime->Eval(bootstrap_str, "bootstrap.js");
            std::cout << "  ✓ Runtime bootstrap loaded" << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Error loading embedded JS: " << e.what() << std::endl;
        return false;
    }
}

std::filesystem::path FindOfficialPreactRoot() {
    static const auto kOfficialPreactRelativeRoot =
        Utf8PathToFsPath("third_party") / Utf8PathToFsPath("preact");
    auto current = std::filesystem::absolute(Utf8PathToFsPath(__FILE__)).parent_path();
    while (!current.empty()) {
        const auto candidate = current / kOfficialPreactRelativeRoot / "package.json";
        if (std::filesystem::exists(candidate)) {
            return current / kOfficialPreactRelativeRoot;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate official Preact sources under third_party/preact");
}

std::string BuildOfficialPreactModule(const std::filesystem::path& entry_path) {
    auto utf8 = std::filesystem::absolute(entry_path).lexically_normal().u8string();
    std::string normalized(utf8.begin(), utf8.end());
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return "export * from '" + normalized + "';";
}

// 注册官方 Preact ES 模块
void RegisterPreactModules(QuickJSRuntime* runtime) {
    const auto preact_root = FindOfficialPreactRoot();
    runtime->RegisterModule("preact", BuildOfficialPreactModule(preact_root / "src" / "index.js"));
    runtime->RegisterModule("preact/hooks", BuildOfficialPreactModule(preact_root / "hooks" / "src" / "index.js"));
    runtime->RegisterModule("preact/jsx-runtime", BuildOfficialPreactModule(preact_root / "jsx-runtime" / "src" / "index.js"));
    runtime->RegisterModule("preact/jsx-dev-runtime", BuildOfficialPreactModule(preact_root / "jsx-runtime" / "src" / "index.js"));

    std::cout << "  ✓ Official Preact ES modules registered" << std::endl;
}

// ============================================================
// Payload 嵌入模式支持（作为 app_bundler 基座）
// ============================================================

// JS API: loadAsset(path) - 返回 ArrayBuffer
static JSValue js_loadAsset(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "loadAsset requires a path argument");
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_ThrowTypeError(ctx, "loadAsset path must be a string");
    std::vector<uint8_t> data;
    bool found = mbink::AssetManager::Instance().GetAsset(path, data);
    JS_FreeCString(ctx, path);
    if (!found) return JS_NULL;
    return JS_NewArrayBufferCopy(ctx, data.data(), data.size());
}

// JS API: getAssetUrl(path) - 返回 data:// URL
static JSValue js_getAssetUrl(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "getAssetUrl requires a path argument");
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_ThrowTypeError(ctx, "getAssetUrl path must be a string");
    std::string url = mbink::AssetManager::Instance().GetAssetDataUrl(path);
    JS_FreeCString(ctx, path);
    if (url.empty()) return JS_NULL;
    return JS_NewString(ctx, url.c_str());
}

// JS API: hasAsset(path) - 检查资源是否存在
static JSValue js_hasAsset(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_FALSE;
    const char* path = JS_ToCString(ctx, argv[0]);
    if (!path) return JS_FALSE;
    bool exists = mbink::AssetManager::Instance().HasAsset(path);
    JS_FreeCString(ctx, path);
    return exists ? JS_TRUE : JS_FALSE;
}

// JS API: listAssets() - 列出所有资源
static JSValue js_listAssets(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    auto paths = mbink::AssetManager::Instance().GetAssetPaths();
    JSValue array = JS_NewArray(ctx);
    for (size_t i = 0; i < paths.size(); i++) {
        JS_SetPropertyUint32(ctx, array, i, JS_NewString(ctx, paths[i].c_str()));
    }
    return array;
}

// 注册资源 API 到全局对象
void RegisterAssetAPI(QuickJSRuntime* runtime) {
    JSContext* ctx = runtime->GetContext();
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "loadAsset",
        JS_NewCFunction(ctx, js_loadAsset, "loadAsset", 1));
    JS_SetPropertyStr(ctx, global, "getAssetUrl",
        JS_NewCFunction(ctx, js_getAssetUrl, "getAssetUrl", 1));
    JS_SetPropertyStr(ctx, global, "hasAsset",
        JS_NewCFunction(ctx, js_hasAsset, "hasAsset", 1));
    JS_SetPropertyStr(ctx, global, "listAssets",
        JS_NewCFunction(ctx, js_listAssets, "listAssets", 0));
    JS_FreeValue(ctx, global);
}

// 检测并加载嵌入的 payload
bool TryLoadEmbeddedPayload(const std::string& exe_path, mbink::PayloadData& payload_data) {
    return mbink::PayloadBuilder::ParseFromFile(exe_path, payload_data);
}

// 执行嵌入的字节码模块
bool ExecuteEmbeddedBytecode(QuickJSRuntime* runtime, const mbink::PayloadData& payload_data, bool verbose = true) {
    auto modules = mbink::BytecodeCompiler::ParseMergedBytecode(payload_data.bytecode);
    if (modules.empty()) {
        std::cerr << "  ✗ No modules found in payload" << std::endl;
        return false;
    }
    if (verbose) std::cout << "  Found " << modules.size() << " modules in payload" << std::endl;

    JSContext* ctx = runtime->GetContext();
    if (!ctx) {
        std::cerr << "  ✗ Failed to get QuickJS context" << std::endl;
        return false;
    }

    for (const auto& module : modules) {
        if (verbose) {
            std::cout << "  Loading module: " << module.id;
            if (module.is_entry) std::cout << " (entry)";
            std::cout << std::endl;
        }

        JSValue obj = JS_ReadObject(ctx, module.bytecode.data(), module.bytecode.size(),
                                    JS_READ_OBJ_BYTECODE);
        if (JS_IsException(obj)) {
            JSValue exception = JS_GetException(ctx);
            const char* msg = JS_ToCString(ctx, exception);
            std::cerr << "  ✗ Failed to load bytecode: " << (msg ? msg : "unknown error") << std::endl;
            if (msg) JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, exception);
            return false;
        }

        JSValue result = JS_EvalFunction(ctx, obj);
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx);
            const char* msg = JS_ToCString(ctx, exception);
            std::cerr << "  ✗ Execution error: " << (msg ? msg : "unknown error") << std::endl;
            if (msg) JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, exception);
            return false;
        }
        JS_FreeValue(ctx, result);
    }
    return true;
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

    // ============================================================
    // 检查是否有嵌入的 payload（app_bundler 打包模式）
    // ============================================================
    mbink::PayloadData embedded_payload;
    bool has_embedded = TryLoadEmbeddedPayload(argv[0], embedded_payload);

#ifdef _WIN32
    // 嵌入模式下，如果没有 --verbose 参数，释放控制台
    if (has_embedded && embedded_payload.valid) {
        bool want_console = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--verbose" || arg == "-v") {
                want_console = true;
                break;
            }
        }
        if (!want_console) {
            FreeConsole();
        }
    }
#endif

    // ============================================================
    // 解析命令行参数
    // ============================================================
    std::string entry_path;
    int width = has_embedded ? embedded_payload.config.width : 1200;
    int height = has_embedded ? embedded_payload.config.height : 800;
    std::string title = has_embedded ? embedded_payload.config.title : "MBink App";
    bool title_from_user = false;
    bool open_devtools = false;
    std::string ui_dev_snapshot_file;
    std::string ui_dev_command_file;
    std::string ui_dev_response_file;
    std::string ui_dev_console_file;
    std::string ui_dev_errors_file;
    std::string ui_dev_lifecycle_file;
    bool execute_scripts = true;
    bool disable_official_preact = false;
    bool verbose = !has_embedded;  // 嵌入模式默认静默
    float quit_after_seconds = 0;
    bool borderless = has_embedded ? embedded_payload.config.borderless : false;
    bool transparent = has_embedded ? embedded_payload.config.transparent : false;
    bool gpu = has_embedded ? embedded_payload.config.gpu : true;
    int min_width = has_embedded ? embedded_payload.config.min_width : 0;
    int min_height = has_embedded ? embedded_payload.config.min_height : 0;
    int max_width = has_embedded ? embedded_payload.config.max_width : 0;
    int max_height = has_embedded ? embedded_payload.config.max_height : 0;

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
            title_from_user = true;
        } else if (arg == "--devtools") {
            open_devtools = true;
        } else if (arg == "--ui-dev-snapshot-file" && i + 1 < argc) {
            ui_dev_snapshot_file = argv[++i];
        } else if (arg == "--ui-dev-command-file" && i + 1 < argc) {
            ui_dev_command_file = argv[++i];
        } else if (arg == "--ui-dev-response-file" && i + 1 < argc) {
            ui_dev_response_file = argv[++i];
        } else if (arg == "--ui-dev-console-file" && i + 1 < argc) {
            ui_dev_console_file = argv[++i];
        } else if (arg == "--ui-dev-errors-file" && i + 1 < argc) {
            ui_dev_errors_file = argv[++i];
        } else if (arg == "--ui-dev-lifecycle-file" && i + 1 < argc) {
            ui_dev_lifecycle_file = argv[++i];
        } else if (arg == "--no-scripts") {
            execute_scripts = false;
        } else if (arg == "--no-official-preact") {
            disable_official_preact = true;
        } else if (arg == "--borderless") {
            borderless = true;
        } else if (arg == "--transparent") {
            transparent = true;
        } else if (arg == "--no-gpu") {
            gpu = false;
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if ((arg == "-q" || arg == "--quit") && i + 1 < argc) {
            quit_after_seconds = std::stof(argv[++i]);
        } else if (arg == "--min-width" && i + 1 < argc) {
            min_width = std::stoi(argv[++i]);
        } else if (arg == "--min-height" && i + 1 < argc) {
            min_height = std::stoi(argv[++i]);
        } else if (arg == "--max-width" && i + 1 < argc) {
            max_width = std::stoi(argv[++i]);
        } else if (arg == "--max-height" && i + 1 < argc) {
            max_height = std::stoi(argv[++i]);
        } else if (arg[0] != '-') {
            entry_path = arg;
        }
    }

    // 用于条件输出的宏
    #define LOG(x) if (verbose) { std::cout << x << std::endl; }

    // 非嵌入模式下，必须指定入口文件
    if (!has_embedded || !embedded_payload.valid) {
        if (entry_path.empty()) {
            std::cerr << "错误: 未指定入口文件" << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
        if (!fs::exists(Utf8PathToFsPath(entry_path))) {
            std::cerr << "错误: 文件不存在: " << entry_path << std::endl;
            return 1;
        }
    }

    // 检测入口文件类型（嵌入模式下不需要）
    bool is_html = !entry_path.empty() && IsHTMLFile(entry_path);
    bool is_embedded = has_embedded && embedded_payload.valid;

    try {
        if (is_embedded) {
            LOG("========================================");
            LOG("  MBink Loader (Embedded Bytecode mode)");
            LOG("========================================");
            LOG("  Size: " << width << "x" << height);
            LOG("  Title: " << title);
            LOG("========================================");
        } else {
            LOG("========================================");
            LOG("  MBink Loader (" << (is_html ? "HTML" : "ESM") << " mode)");
            LOG("========================================");
            LOG("  Entry: " << entry_path);
            LOG("  Size: " << width << "x" << height);
            if (is_html) {
                LOG("  Scripts: " << (execute_scripts ? "enabled" : "disabled"));
            }
            LOG("========================================");
        }
        LOG("");
        if (verbose) std::cout.flush();

        // 1. 创建窗口
        LOG("[1/5] Creating window...");
        if (verbose) std::cout.flush();
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.resizable = true;
        config.vsync = true;
        config.borderless = borderless || transparent;  // 透明窗口隐含无边框
        config.transparent = transparent;
        config.gpu = gpu;
        config.min_width = min_width;
        config.min_height = min_height;
        config.max_width = max_width;
        config.max_height = max_height;

        auto window = std::make_shared<Window>(config);
        auto& window_manager = WindowManager::Instance();
        window_manager.RegisterWindow(window);
        LOG("  ✓ Window created");

        // 2. 创建文档
        LOG("[2/5] Creating document...");
        auto document = std::make_shared<Document>();

        if (is_embedded) {
            // ===== 嵌入模式：创建空文档 =====
            document->Initialize();
        } else if (is_html) {
            // ===== HTML 模式：解析 HTML 文件 =====
            fs::path html_dir = fs::absolute(Utf8PathToFsPath(entry_path)).parent_path();
            std::string base_path = NormalizeFsPath(html_dir);
            document->SetBasePath(base_path);
            ImageLoader::SetBasePath(base_path);
            LOG("  ✓ Base path: " << base_path);

            // 读取并解析 HTML
            std::string html_content = ReadFile(entry_path);
            if (html_content.empty()) {
                std::cerr << "  ✗ Failed to read HTML file" << std::endl;
                return 1;
            }
            if (!document->LoadHTML(html_content)) {
                std::cerr << "  ✗ Failed to parse HTML" << std::endl;
                return 1;
            }
            LOG("  ✓ HTML document loaded");

            // 加载外部样式表
            document->LoadExternalStylesheets();

            // 统计标签
            auto styles = document->GetElementsByTagName("style");
            auto links = document->GetElementsByTagName("link");
            auto scripts = document->GetElementsByTagName("script");
            LOG("  ✓ Found " << styles.size() << " <style>, "
                      << links.size() << " <link>, "
                      << scripts.size() << " <script>");

            // 从 HTML 获取 title（如果用户没有通过 --title 指定）
            if (!title_from_user) {
                std::string html_title = GetDocumentTitle(document);
                if (!html_title.empty()) {
                    title = html_title;
                    window->SetTitle(title);
                }
            }
            LOG("  ✓ Title: " << title);
        } else {
            // ===== JS 模式：创建空文档 =====
            document->Initialize();
        }

        window->SetDocument(document);
        LOG("  ✓ Document ready");

        // 3. 创建 QuickJS 运行时
        LOG("[3/5] Creating QuickJS runtime...");
        auto runtime = std::make_unique<QuickJSRuntime>();
        mbink::ui_dev::RuntimeSupportOptions ui_dev_options{
            ui_dev_snapshot_file,
            ui_dev_command_file,
            ui_dev_response_file,
            ui_dev_console_file,
            ui_dev_errors_file,
            ui_dev_lifecycle_file,
            quit_after_seconds,
        };
        mbink::ui_dev::AttachStructuredRuntimeBuffers(runtime.get(), ui_dev_options);
        auto task_scheduler = std::make_shared<TaskScheduler>();
        LOG("  ✓ QuickJS runtime created");

        // 4. 初始化绑定和库
        LOG("[4/5] Initializing bindings...");
        WindowBindings window_bindings(runtime.get(), window, task_scheduler);
        window_bindings.InitBindings();
        LOG("  ✓ Window bindings initialized");

        // 初始化 FetchBindings
        FetchBindings fetch_bindings(runtime->GetContext(), task_scheduler);
        fetch_bindings.InitBindings();
        LOG("  ✓ Fetch bindings initialized");

        // 初始化 StateManager 和 HostBridge
        auto state_manager = std::make_unique<StateManager>();
        auto host_bridge = std::make_unique<HostBridge>(runtime->GetContext(), state_manager.get());
        host_bridge->registerGlobal();
        LOG("  ✓ Host bridge initialized");

        // 创建事件循环（需要在加载模块之前，以便 getSelection 等 API 可用）
        EventLoop event_loop(task_scheduler);
        event_loop.SetQuickJSRuntime(runtime.get());
        WindowBindings::SetActiveEventLoop(&event_loop);
        LOG("  ✓ Event loop created");

        auto& devtools = DevToolsManager::GetInstance();
        bool cleanup_done = false;
        auto shutdown_runtime = [&](int exit_code, bool use_quick_exit) -> int {
            if (cleanup_done) {
                if (use_quick_exit) {
                    std::quick_exit(exit_code);
                }
                return exit_code;
            }
            cleanup_done = true;

            std::cout << std::endl;
            std::cout << "Shutting down..." << std::endl;

            WindowBindings::SetActiveEventLoop(nullptr);
            devtools.Shutdown();
            FontManager::GetInstance().ClearCache();

            if (runtime) {
                try {
                    runtime->Eval(R"(
                        (function() {
                            if (globalThis.__mbinkShutdown) {
                                try { globalThis.__mbinkShutdown(); } catch (_) {}
                            } else {
                                if (globalThis.__preactCleanup) {
                                    try { globalThis.__preactCleanup(); } catch (_) {}
                                }
                                if (globalThis.__preactHooksCleanup) {
                                    try { globalThis.__preactHooksCleanup(); } catch (_) {}
                                }
                                if (globalThis.__mbinkRuntimeCleanup) {
                                    try { globalThis.__mbinkRuntimeCleanup(); } catch (_) {}
                                }
                            }
                        })();
                    )", "<shutdown-cleanup>");
                } catch (...) {
                }
            }

            if (window) {
                window_manager.UnregisterWindow(window);
            }

            if (runtime) {
                window_bindings.Cleanup();
            }
            DOMBindings::Cleanup(nullptr);

            document.reset();

            if (state_manager) {
                state_manager->clearWatchers();
            }
            host_bridge.reset();
            state_manager.reset();

            if (runtime) {
                runtime->RunGC();
            }
            runtime.reset();
            window.reset();

            if (use_quick_exit) {
                std::quick_exit(exit_code);
            }
            return exit_code;
        };

        // 加载嵌入的库（Preact 等）
        if (mbink::embedded::HasEmbeddedJS()) {
            LoadEmbeddedLibraries(runtime.get());
            if (!disable_official_preact) {
                RegisterPreactModules(runtime.get());
            } else {
                LOG("  ✓ Official Preact ES modules skipped (--no-official-preact)");
            }
        } else {
            if (verbose) std::cerr << "  ⚠ No embedded JS libraries" << std::endl;
        }

        // 5. 执行脚本 / 字节码
        LOG("[5/5] Loading entry...");

        if (is_embedded) {
            // ===== 嵌入模式：初始化资源管理器 + 执行字节码 =====
            if (!embedded_payload.assets_index.empty()) {
                mbink::AssetManager::Instance().Initialize(
                    embedded_payload.assets_data,
                    embedded_payload.assets_index
                );

                // 注册 ImageLoader 的资源提供者
                ImageLoader::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return mbink::AssetManager::Instance().GetAsset(path, data);
                });

                // 注册 CSS 的资源提供者
                LexborStyleSheet::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return mbink::AssetManager::Instance().GetAsset(path, data);
                });

                // 注册 Document 的资源提供者（用于 link 元素加载 CSS）
                Document::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
                    return mbink::AssetManager::Instance().GetAsset(path, data);
                });

                LOG("  ✓ Loaded " << embedded_payload.assets_index.size() << " embedded assets");
            }

            // 注册资源 API
            RegisterAssetAPI(runtime.get());

            // 执行嵌入的字节码
            LOG("  Executing embedded bytecode...");
            if (!ExecuteEmbeddedBytecode(runtime.get(), embedded_payload, verbose)) {
                return shutdown_runtime(1, false);
            }
            LOG("  ✓ Bytecode executed");
        } else if (is_html) {
            // ===== HTML 模式：设置 JSRuntime 并执行 <script> 标签 =====
            document->SetJSRuntime(runtime.get());
            if (execute_scripts) {
                document->ExecuteScripts();
                LOG("  ✓ HTML scripts executed");
            } else {
                LOG("  ✓ Scripts skipped (--no-scripts)");
            }
        } else {
            // ===== JS/ESM 模式：加载 ES 模块 =====
            fs::path abs_path = fs::absolute(Utf8PathToFsPath(entry_path));
            std::string normalized_abs_path = NormalizeFsPath(abs_path);
            runtime->SetBaseModulePath(normalized_abs_path);

            std::string entry_code = ReadFile(entry_path);
            if (entry_code.empty()) {
                std::cerr << "  ✗ Failed to read: " << entry_path << std::endl;
                return shutdown_runtime(1, false);
            }

            try {
                runtime->EvalModule(entry_code, normalized_abs_path);
                LOG("  ✓ Entry module loaded");
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Module error: " << e.what() << std::endl;
                return shutdown_runtime(1, false);
            }
        }

        // 显示窗口
        window->Show();

        // 主动绘制首帧，避免首个事件循环周期尚未触发 render callback 时出现空白窗口
        // 某些示例会在首帧完成前看起来像“启动后空白几秒再退出”。
        if (window->NeedsRepaint()) {
            window->Render();
            window->SwapBuffers();
        }

        // 初始化 DevTools
        devtools.Initialize(document.get(), window.get());
        if (open_devtools) {
            devtools.Open();
        }

        LOG("");
        LOG("========================================");
        LOG("  🚀 Application Started!");
        LOG("========================================");
        LOG("  Press F12 to toggle DevTools");
        LOG("");

        mbink::ui_dev::ConfigureRuntimeControl(&event_loop, runtime.get(), window, document, ui_dev_options);

        event_loop.Run();
        LOG("  Event loop exited: shouldQuit=" << (event_loop.ShouldQuit() ? 1 : 0)
            << ", hasWindows=" << (window_manager.HasWindows() ? 1 : 0)
            << ", windowShouldClose=" << (window->ShouldClose() ? 1 : 0));

        return shutdown_runtime(0, true);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
