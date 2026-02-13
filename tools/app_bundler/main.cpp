/**
 * @file main.cpp
 * @brief MBink App Bundler - 将 Preact/JS 应用打包成独立 exe
 *
 * 用法: app_bundler <app.js> -o <output.exe> [选项]
 *
 * 选项:
 *   -o, --output <file>     输出文件路径
 *   --width <value>         窗口宽度 (默认: 800)
 *   --height <value>        窗口高度 (默认: 600)
 *   --title <value>         窗口标题 (默认: MBink App)
 *   --include <file>        包含额外的 JS 文件 (可多次使用)
 *   --template <file>       指定模板 exe 路径
 *   --verbose               显示详细信息
 *   --no-overwrite          不覆盖已存在的输出文件
 *   --help                  显示帮助信息
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#include "module_resolver.h"
#include "bytecode_compiler.h"
#include "payload.h"
#include "exe_writer.h"
#include "upx_compressor.h"

namespace fs = std::filesystem;
using namespace mbink;

// 打包选项
struct BundlerOptions {
    std::string input_file;                    // 主 JS 文件
    std::string output_file;                   // 输出 exe 路径
    std::string template_exe;                  // 模板 exe 路径
    std::vector<std::string> include_files;   // 额外包含的 JS 文件
    std::vector<std::string> asset_files;     // 单个资源文件
    std::vector<std::string> asset_dirs;      // 资源目录

    // 窗口配置
    int width = 800;
    int height = 600;
    std::string title = "MBink App";
    bool borderless = false;                   // 无边框窗口模式
    bool transparent = false;                  // 透明窗口
    int min_width = 0;                         // 窗口最小宽度（0 表示不限制）
    int min_height = 0;                        // 窗口最小高度（0 表示不限制）
    int max_width = 0;                         // 窗口最大宽度（0 表示不限制）
    int max_height = 0;                        // 窗口最大高度（0 表示不限制）

    bool verbose = false;
    bool no_overwrite = false;
    bool debug = false;                        // 调试模式（显示控制台窗口）
    bool compress = false;                     // 使用 UPX 压缩
};

// 打印帮助信息
void PrintUsage(const char* program_name) {
    std::cout << "MBink App Bundler - 将 Preact/JS 应用打包成独立 exe\n";
    std::cout << "\n";
    std::cout << "用法: " << program_name << " <app.js> -o <output.exe> [选项]\n";
    std::cout << "\n";
    std::cout << "选项:\n";
    std::cout << "  -o, --output <file>     输出文件路径 (必需)\n";
    std::cout << "  --width <value>         窗口宽度 (默认: 800)\n";
    std::cout << "  --height <value>        窗口高度 (默认: 600)\n";
    std::cout << "  --title <value>         窗口标题 (默认: MBink App)\n";
    std::cout << "  --borderless            无边框窗口模式（支持不规则窗体）\n";
    std::cout << "  --transparent           透明窗口（需配合 --borderless 使用）\n";
    std::cout << "  --min-width <value>     窗口最小宽度\n";
    std::cout << "  --min-height <value>    窗口最小高度\n";
    std::cout << "  --max-width <value>     窗口最大宽度\n";
    std::cout << "  --max-height <value>    窗口最大高度\n";
    std::cout << "  --include <file>        包含额外的 JS 文件 (可多次使用)\n";
    std::cout << "  --template <file>       指定模板 exe 路径\n";
    std::cout << "  --verbose               显示详细信息\n";
    std::cout << "  --no-overwrite          不覆盖已存在的输出文件\n";
    std::cout << "  --debug                 调试模式（输出 exe 显示控制台窗口）\n";
    std::cout << "  --compress              使用 UPX 压缩输出文件\n";
    std::cout << "  --asset <file>          打包单个资源文件 (可多次使用)\n";
    std::cout << "  --assets <dir>          打包资源目录 (可多次使用)\n";
    std::cout << "  --help                  显示帮助信息\n";
    std::cout << "\n";
    std::cout << "示例:\n";
    std::cout << "  " << program_name << " my_app.js -o my_app.exe\n";
    std::cout << "  " << program_name << " my_app.js -o my_app.exe --width 1024 --height 768\n";
    std::cout << "  " << program_name << " my_app.js -o my_app.exe --assets ./resources\n";
    std::cout << "  " << program_name << " my_app.js -o my_app.exe --asset logo.png --asset config.json\n";
}

// 解析命令行参数
bool ParseArguments(int argc, char** argv, BundlerOptions& options) {
    if (argc < 2) {
        return false;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "错误: " << arg << " 需要一个参数\n";
                return false;
            }
            options.output_file = argv[++i];
        } else if (arg == "--width") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --width 需要一个参数\n";
                return false;
            }
            try {
                options.width = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "错误: --width 参数无效\n";
                return false;
            }
        } else if (arg == "--height") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --height 需要一个参数\n";
                return false;
            }
            try {
                options.height = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "错误: --height 参数无效\n";
                return false;
            }
        } else if (arg == "--title") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --title 需要一个参数\n";
                return false;
            }
            options.title = argv[++i];
        } else if (arg == "--borderless") {
            options.borderless = true;
        } else if (arg == "--transparent") {
            options.transparent = true;
        } else if (arg == "--min-width") {
            if (i + 1 >= argc) { std::cerr << "错误: --min-width 需要一个参数\n"; return false; }
            try { options.min_width = std::stoi(argv[++i]); } catch (...) { std::cerr << "错误: --min-width 参数无效\n"; return false; }
        } else if (arg == "--min-height") {
            if (i + 1 >= argc) { std::cerr << "错误: --min-height 需要一个参数\n"; return false; }
            try { options.min_height = std::stoi(argv[++i]); } catch (...) { std::cerr << "错误: --min-height 参数无效\n"; return false; }
        } else if (arg == "--max-width") {
            if (i + 1 >= argc) { std::cerr << "错误: --max-width 需要一个参数\n"; return false; }
            try { options.max_width = std::stoi(argv[++i]); } catch (...) { std::cerr << "错误: --max-width 参数无效\n"; return false; }
        } else if (arg == "--max-height") {
            if (i + 1 >= argc) { std::cerr << "错误: --max-height 需要一个参数\n"; return false; }
            try { options.max_height = std::stoi(argv[++i]); } catch (...) { std::cerr << "错误: --max-height 参数无效\n"; return false; }
        } else if (arg == "--include") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --include 需要一个参数\n";
                return false;
            }
            options.include_files.push_back(argv[++i]);
        } else if (arg == "--template") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --template 需要一个参数\n";
                return false;
            }
            options.template_exe = argv[++i];
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--no-overwrite") {
            options.no_overwrite = true;
        } else if (arg == "--debug") {
            options.debug = true;
        } else if (arg == "--compress") {
            options.compress = true;
        } else if (arg == "--asset") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --asset 需要一个参数\n";
                return false;
            }
            options.asset_files.push_back(argv[++i]);
        } else if (arg == "--assets") {
            if (i + 1 >= argc) {
                std::cerr << "错误: --assets 需要一个参数\n";
                return false;
            }
            options.asset_dirs.push_back(argv[++i]);
        } else if (arg[0] != '-') {
            if (options.input_file.empty()) {
                options.input_file = arg;
            } else {
                std::cerr << "错误: 多个输入文件: " << arg << "\n";
                return false;
            }
        } else {
            std::cerr << "错误: 未知选项: " << arg << "\n";
            return false;
        }
    }

    return true;
}

// 验证选项
bool ValidateOptions(const BundlerOptions& options) {
    if (options.input_file.empty()) {
        std::cerr << "错误: 未指定输入文件\n";
        return false;
    }

    if (options.output_file.empty()) {
        std::cerr << "错误: 未指定输出文件 (使用 -o 选项)\n";
        return false;
    }

    if (!fs::exists(options.input_file)) {
        std::cerr << "错误: 输入文件不存在: " << options.input_file << "\n";
        return false;
    }

    if (options.no_overwrite && fs::exists(options.output_file)) {
        std::cerr << "错误: 输出文件已存在: " << options.output_file << "\n";
        return false;
    }

    for (const auto& include : options.include_files) {
        if (!fs::exists(include)) {
            std::cerr << "错误: 包含文件不存在: " << include << "\n";
            return false;
        }
    }

    if (!options.template_exe.empty() && !fs::exists(options.template_exe)) {
        std::cerr << "错误: 模板文件不存在: " << options.template_exe << "\n";
        return false;
    }

    return true;
}

// 读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 获取嵌入的 Preact 源码（从 app_loader 的嵌入资源）
// 注意：这里我们需要从 js/ 目录读取，因为 bundler 不嵌入这些资源
std::string GetPreactSource(const std::string& bundler_path) {
    fs::path base = fs::path(bundler_path).parent_path();
    std::vector<fs::path> search_paths = {
        base / "js" / "preact" / "preact.js",
        base / ".." / "js" / "preact" / "preact.js",
        base / ".." / ".." / "js" / "preact" / "preact.js",
        "js/preact/preact.js",
    };
    for (const auto& p : search_paths) {
        if (fs::exists(p)) return ReadFile(p.string());
    }
    return "";
}

std::string GetHooksSource(const std::string& bundler_path) {
    fs::path base = fs::path(bundler_path).parent_path();
    std::vector<fs::path> search_paths = {
        base / "js" / "preact" / "hooks.js",
        base / ".." / "js" / "preact" / "hooks.js",
        base / ".." / ".." / "js" / "preact" / "hooks.js",
        "js/preact/hooks.js",
    };
    for (const auto& p : search_paths) {
        if (fs::exists(p)) return ReadFile(p.string());
    }
    return "";
}

int main(int argc, char** argv) {
    // Windows: 设置控制台输出为 UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    BundlerOptions options;

    // 解析命令行参数
    if (!ParseArguments(argc, argv, options)) {
        std::cerr << "\n";
        PrintUsage(argv[0]);
        return 1;
    }

    // 验证选项
    if (!ValidateOptions(options)) {
        return 1;
    }

    // 显示配置信息
    std::cout << "========================================\n";
    std::cout << "  MBink App Bundler\n";
    std::cout << "========================================\n";
    std::cout << "  输入: " << options.input_file << "\n";
    std::cout << "  输出: " << options.output_file << "\n";
    std::cout << "  窗口: " << options.width << "x" << options.height << "\n";
    std::cout << "  标题: " << options.title << "\n";
    if (!options.include_files.empty()) {
        std::cout << "  包含: " << options.include_files.size() << " 个额外文件\n";
    }
    if (!options.asset_files.empty() || !options.asset_dirs.empty()) {
        std::cout << "  资源: " << options.asset_files.size() << " 个文件, " 
                  << options.asset_dirs.size() << " 个目录\n";
    }
    std::cout << "========================================\n";
    std::cout << "\n";

    // 1. 查找模板 exe
    std::string template_path = options.template_exe;
    if (template_path.empty()) {
        template_path = ExeWriter::FindTemplate(argv[0]);
        if (template_path.empty()) {
            std::cerr << "错误: 找不到模板 exe (esm_loader.exe 或 app_loader.exe)\n";
            std::cerr << "请使用 --template 选项指定模板路径\n";
            return 1;
        }
    }
    if (options.verbose) {
        std::cout << "模板: " << template_path << "\n\n";
    }

    // 2. 解析模块依赖
    std::cout << "[1/4] 解析模块依赖...\n";
    
    ModuleResolver resolver;
    resolver.SetVerbose(options.verbose);
    
    // 注册内置模块
    std::string preact_src = GetPreactSource(argv[0]);
    std::string hooks_src = GetHooksSource(argv[0]);
    if (!preact_src.empty()) {
        resolver.RegisterBuiltinModule("preact", preact_src);
        if (options.verbose) {
            std::cout << "  注册内置模块: preact (" << preact_src.size() << " bytes)\n";
        }
    }
    if (!hooks_src.empty()) {
        resolver.RegisterBuiltinModule("preact/hooks", hooks_src);
        if (options.verbose) {
            std::cout << "  注册内置模块: preact/hooks (" << hooks_src.size() << " bytes)\n";
        }
    }
    
    auto modules = resolver.Resolve(options.input_file);
    
    if (resolver.HasErrors()) {
        std::cerr << "  ✗ 模块解析失败:\n";
        for (const auto& err : resolver.GetErrors()) {
            std::cerr << "    " << err.message;
            if (!err.file.empty()) {
                std::cerr << " (" << err.file;
                if (err.line > 0) std::cerr << ":" << err.line;
                std::cerr << ")";
            }
            std::cerr << "\n";
        }
        return 1;
    }
    
    std::cout << "  ✓ 解析到 " << modules.size() << " 个模块\n";

    // 3. 编译字节码
    std::cout << "[2/4] 编译字节码...\n";
    
    BytecodeCompiler compiler;
    compiler.SetVerbose(options.verbose);
    compiler.SetStripSource(true);  // 去除源码信息减小体积
    
    auto compiled = compiler.CompileModules(modules);
    
    if (compiler.HasErrors()) {
        std::cerr << "  ✗ 编译失败:\n";
        for (const auto& err : compiler.GetErrors()) {
            std::cerr << "    " << err.message;
            if (!err.file.empty()) {
                std::cerr << " (" << err.file;
                if (err.line > 0) std::cerr << ":" << err.line;
                if (err.column > 0) std::cerr << ":" << err.column;
                std::cerr << ")";
            }
            std::cerr << "\n";
        }
        return 1;
    }
    
    // 合并字节码
    auto merged_bytecode = BytecodeCompiler::MergeBytecode(compiled);
    std::cout << "  ✓ 编译完成 (" << merged_bytecode.size() << " bytes)\n";

    // 4. 构建 payload
    std::cout << "[3/4] 构建 payload...\n";
    
    PayloadBuilder builder;
    builder.SetWidth(options.width);
    builder.SetHeight(options.height);
    builder.SetTitle(options.title);
    builder.SetModuleCount(static_cast<uint32_t>(compiled.size()));
    builder.SetBytecode(merged_bytecode);

    // 设置无边框/透明窗口配置
    {
        PayloadConfig cfg;
        cfg.width = options.width;
        cfg.height = options.height;
        cfg.title = options.title;
        cfg.module_count = static_cast<uint32_t>(compiled.size());
        cfg.borderless = options.borderless;
        cfg.transparent = options.transparent;
        cfg.min_width = options.min_width;
        cfg.min_height = options.min_height;
        cfg.max_width = options.max_width;
        cfg.max_height = options.max_height;
        builder.SetConfig(cfg);
    }
    
    // 添加资源文件
    for (const auto& asset_file : options.asset_files) {
        fs::path p(asset_file);
        std::string name = p.filename().string();
        if (!builder.AddAssetFromFile(name, asset_file)) {
            std::cerr << "  ✗ 无法读取资源文件: " << asset_file << "\n";
            return 1;
        }
        if (options.verbose) {
            std::cout << "  添加资源: " << name << "\n";
        }
    }
    
    // 添加资源目录
    for (const auto& asset_dir : options.asset_dirs) {
        // 使用目录名作为前缀，保持路径一致性
        fs::path dir_path(asset_dir);
        std::string prefix = dir_path.filename().string();
        
        if (!builder.AddAssetsFromDirectory(asset_dir, prefix)) {
            std::cerr << "  ✗ 无法读取资源目录: " << asset_dir << "\n";
            return 1;
        }
        if (options.verbose) {
            std::cout << "  添加资源目录: " << asset_dir << " (前缀: " << prefix << ")\n";
        }
    }
    
    if (builder.GetAssetCount() > 0) {
        std::cout << "  ✓ 添加了 " << builder.GetAssetCount() << " 个资源\n";
    }
    
    auto payload = builder.Build();
    std::cout << "  ✓ Payload 构建完成 (" << payload.size() << " bytes)\n";

    // 5. 写入输出文件
    std::cout << "[4/4] 写入输出文件...\n";
    
    ExeWriter writer;
    if (!writer.LoadTemplate(template_path)) {
        std::cerr << "  ✗ " << writer.GetError() << "\n";
        return 1;
    }
    
    if (!writer.WriteOutput(options.output_file, payload, options.debug)) {
        std::cerr << "  ✗ " << writer.GetError() << "\n";
        return 1;
    }
    
    size_t total_size = writer.GetTemplateSize() + payload.size();
    std::cout << "  ✓ 写入完成 (" << total_size << " bytes)\n";

    // 6. 可选：UPX 压缩
    if (options.compress) {
        std::cout << "[5/5] UPX 压缩...\n";
        
        UPXCompressor compressor;
        if (!compressor.IsAvailable()) {
            std::cerr << "  ⚠ UPX 不可用，跳过压缩\n";
        } else {
            if (compressor.Compress(options.output_file, UPXCompressor::CompressionLevel::Best)) {
                size_t original = compressor.GetOriginalSize();
                size_t compressed = compressor.GetCompressedSize();
                int ratio = static_cast<int>((1.0 - static_cast<double>(compressed) / original) * 100);
                std::cout << "  ✓ 压缩完成: " << (original / 1024) << " KB -> " 
                          << (compressed / 1024) << " KB (" << ratio << "% 减少)\n";
                total_size = compressed;
            } else {
                std::cerr << "  ⚠ 压缩失败: " << compressor.GetError() << "\n";
            }
        }
    }

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  ✓ 打包完成!\n";
    std::cout << "  输出: " << options.output_file << "\n";
    std::cout << "  大小: " << (total_size / 1024) << " KB\n";
    std::cout << "========================================\n";

    return 0;
}
