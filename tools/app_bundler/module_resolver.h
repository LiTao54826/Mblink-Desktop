/**
 * @file module_resolver.h
 * @brief ES6 模块解析器 - 解析 import 语句并构建依赖图
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace mbink {

/**
 * @brief Import 语句信息
 */
struct ImportInfo {
    std::string module_path;     // import 的模块路径
    std::string raw_statement;   // 原始 import 语句
    size_t line_number;          // 行号
};

/**
 * @brief 解析后的模块信息
 */
struct ResolvedModule {
    std::string id;              // 模块唯一标识（规范化路径或内置名）
    std::string path;            // 文件路径（内置模块为空）
    std::string source;          // 源代码
    bool is_builtin;             // 是否为内置模块
    std::vector<std::string> dependencies;  // 依赖的模块 ID
};

/**
 * @brief 模块解析错误
 */
struct ModuleError {
    std::string message;
    std::string file;
    size_t line;
};

/**
 * @brief ES6 模块解析器
 */
class ModuleResolver {
public:
    ModuleResolver();
    ~ModuleResolver() = default;

    /**
     * @brief 注册内置模块
     * @param name 模块名（如 "preact", "preact/hooks"）
     * @param source 模块源代码
     */
    void RegisterBuiltinModule(const std::string& name, const std::string& source);

    /**
     * @brief 从入口文件解析所有依赖
     * @param entry_file 入口文件路径
     * @return 按依赖顺序排列的模块列表（依赖在前）
     */
    std::vector<ResolvedModule> Resolve(const std::string& entry_file);

    /**
     * @brief 获取解析过程中的错误
     */
    const std::vector<ModuleError>& GetErrors() const { return errors_; }

    /**
     * @brief 是否有错误
     */
    bool HasErrors() const { return !errors_.empty(); }

    /**
     * @brief 设置详细输出
     */
    void SetVerbose(bool verbose) { verbose_ = verbose; }

    // 公开用于测试
    static std::vector<ImportInfo> ParseImports(const std::string& source);
    static std::string ResolvePath(const std::string& import_path, 
                                   const std::string& from_file);

private:
    // 内置模块映射
    std::unordered_map<std::string, std::string> builtin_modules_;
    
    // 已解析的模块缓存
    std::unordered_map<std::string, ResolvedModule> resolved_cache_;
    
    // 解析错误
    std::vector<ModuleError> errors_;
    
    // 详细输出
    bool verbose_ = false;

    // 递归解析模块
    bool ResolveModule(const std::string& module_id, 
                       const std::string& from_file,
                       std::unordered_set<std::string>& visiting);

    // 拓扑排序
    std::vector<ResolvedModule> TopologicalSort();

    // 读取文件内容
    static std::optional<std::string> ReadFile(const std::string& path);

    // 规范化路径
    static std::string NormalizePath(const std::string& path);

    // 检查是否为内置模块名
    bool IsBuiltinModule(const std::string& name) const;

    // 添加错误
    void AddError(const std::string& message, 
                  const std::string& file = "", 
                  size_t line = 0);
};

}  // namespace mbink
