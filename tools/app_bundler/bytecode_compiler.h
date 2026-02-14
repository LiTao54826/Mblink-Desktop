/**
 * @file bytecode_compiler.h
 * @brief QuickJS 字节码编译器 - 将 JS 模块编译为字节码
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "module_resolver.h"

// Forward declarations for QuickJS types
struct JSRuntime;
struct JSContext;
typedef struct JSModuleDef JSModuleDef;

namespace mbink {

/**
 * @brief 编译后的模块字节码
 */
struct CompiledModule {
    std::string id;              // 模块标识
    std::vector<uint8_t> bytecode;  // 编译后的字节码
    bool is_entry;               // 是否为入口模块
};

/**
 * @brief 编译错误信息
 */
struct CompileError {
    std::string message;
    std::string file;
    int line;
    int column;
};

/**
 * @brief QuickJS 字节码编译器
 */
class BytecodeCompiler {
public:
    BytecodeCompiler();
    ~BytecodeCompiler();

    // 禁止拷贝
    BytecodeCompiler(const BytecodeCompiler&) = delete;
    BytecodeCompiler& operator=(const BytecodeCompiler&) = delete;

    /**
     * @brief 编译单个模块为字节码
     * @param source 源代码
     * @param filename 文件名（用于错误报告）
     * @param is_module 是否为 ES6 模块
     * @return 编译后的字节码，失败返回空
     */
    std::vector<uint8_t> CompileModule(const std::string& source,
                                        const std::string& filename,
                                        bool is_module = true);

    /**
     * @brief 编译多个已解析的模块
     * @param modules 已解析的模块列表（按依赖顺序）
     * @return 编译后的模块列表
     */
    std::vector<CompiledModule> CompileModules(
        const std::vector<ResolvedModule>& modules);

    /**
     * @brief 将多个模块的字节码合并为单个字节码块
     * @param compiled_modules 编译后的模块列表
     * @return 合并后的字节码
     * 
     * 格式: [module_count: 4 bytes]
     *       [module 1: id_len(4) + id + bytecode_len(4) + bytecode]
     *       [module 2: ...]
     *       ...
     */
    static std::vector<uint8_t> MergeBytecode(
        const std::vector<CompiledModule>& compiled_modules);

    /**
     * @brief 从合并的字节码中解析模块
     * @param merged 合并后的字节码
     * @return 解析后的模块列表
     */
    static std::vector<CompiledModule> ParseMergedBytecode(
        const std::vector<uint8_t>& merged);

    /**
     * @brief 获取编译错误
     */
    const std::vector<CompileError>& GetErrors() const { return errors_; }

    /**
     * @brief 是否有错误
     */
    bool HasErrors() const { return !errors_.empty(); }

    /**
     * @brief 设置是否去除源码信息
     */
    void SetStripSource(bool strip) { strip_source_ = strip; }

    /**
     * @brief 设置是否去除调试信息
     */
    void SetStripDebug(bool strip) { strip_debug_ = strip; }

    /**
     * @brief 设置详细输出
     */
    void SetVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief 设置入口文件目录（用于 ModuleNormalize 计算相对路径）
     */
    void SetEntryDir(const std::string& dir) { entry_dir_ = dir; }

private:
    JSRuntime* runtime_;
    JSContext* ctx_;
    std::vector<CompileError> errors_;
    bool strip_source_ = false;
    bool strip_debug_ = false;
    bool verbose_ = false;

    // 入口文件所在目录（用于 ModuleNormalize 计算相对路径）
    std::string entry_dir_;

    // 模块源码映射 (相对路径 -> 源码)，供 module loader 回调使用
    std::unordered_map<std::string, std::string> module_sources_;

    // 初始化 QuickJS 运行时
    bool Initialize();

    // 清理资源
    void Cleanup();

    // 添加错误
    void AddError(const std::string& message,
                  const std::string& file = "",
                  int line = 0,
                  int column = 0);

    // 从 QuickJS 异常中提取错误信息
    void ExtractException();

    // QuickJS 模块名标准化回调（解析相对路径）
    static char* ModuleNormalize(JSContext* ctx, const char* module_base,
                                  const char* module_name, void* opaque);

    // QuickJS 模块加载器回调（从 module_sources_ 中查找源码）
    static JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name,
                                      void* opaque);
};

}  // namespace mbink
