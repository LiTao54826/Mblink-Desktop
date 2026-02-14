/**
 * @file module_resolver.cpp
 * @brief ES6 模块解析器实现
 */

#include "module_resolver.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stack>
#include <functional>

namespace fs = std::filesystem;

namespace mbink {

ModuleResolver::ModuleResolver() = default;

void ModuleResolver::RegisterBuiltinModule(const std::string& name, 
                                            const std::string& source) {
    builtin_modules_[name] = source;
    if (verbose_) {
        std::cout << "  Registered builtin module: " << name 
                  << " (" << source.size() << " bytes)" << std::endl;
    }
}

std::vector<ImportInfo> ModuleResolver::ParseImports(const std::string& source) {
    std::vector<ImportInfo> imports;
    
    // 匹配各种 import 语句:
    // import xxx from 'module'
    // import { xxx } from 'module'
    // import * as xxx from 'module'
    // import 'module'
    // 支持单引号和双引号
    std::regex import_regex(
        R"(import\s+(?:[^'"]*\s+from\s+)?['"]([^'"]+)['"])",
        std::regex::ECMAScript
    );
    
    std::string::const_iterator search_start = source.cbegin();
    std::smatch match;
    size_t line_number = 1;
    size_t last_pos = 0;
    
    while (std::regex_search(search_start, source.cend(), match, import_regex)) {
        // 计算行号
        size_t match_pos = match.position() + (search_start - source.cbegin());
        for (size_t i = last_pos; i < match_pos && i < source.size(); i++) {
            if (source[i] == '\n') line_number++;
        }
        last_pos = match_pos;
        
        ImportInfo info;
        info.module_path = match[1].str();
        info.raw_statement = match[0].str();
        info.line_number = line_number;
        imports.push_back(info);
        
        search_start = match.suffix().first;
    }
    
    return imports;
}

std::string ModuleResolver::ResolvePath(const std::string& import_path,
                                         const std::string& from_file) {
    // 如果是相对路径
    if (import_path.starts_with("./") || import_path.starts_with("../")) {
        fs::path base_dir = fs::path(from_file).parent_path();
        fs::path resolved = base_dir / import_path;
        
        // 尝试添加 .js 扩展名
        if (!fs::exists(resolved) && !resolved.has_extension()) {
            fs::path with_js = resolved;
            with_js.replace_extension(".js");
            if (fs::exists(with_js)) {
                return fs::weakly_canonical(with_js).string();
            }
            // 尝试 index.js
            fs::path index_js = resolved / "index.js";
            if (fs::exists(index_js)) {
                return fs::weakly_canonical(index_js).string();
            }
        }
        
        if (fs::exists(resolved)) {
            return fs::weakly_canonical(resolved).string();
        }
        
        // 返回规范化路径（即使不存在，让调用者处理错误）
        return fs::weakly_canonical(resolved).string();
    }
    
    // 裸模块名（如 'preact', 'preact/hooks'）
    // 返回原样，由调用者检查是否为内置模块
    return import_path;
}

std::optional<std::string> ModuleResolver::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ModuleResolver::NormalizePath(const std::string& path) {
    try {
        return fs::weakly_canonical(path).string();
    } catch (...) {
        return path;
    }
}

bool ModuleResolver::IsBuiltinModule(const std::string& name) const {
    return builtin_modules_.find(name) != builtin_modules_.end();
}

void ModuleResolver::AddError(const std::string& message,
                               const std::string& file,
                               size_t line) {
    errors_.push_back({message, file, line});
}

bool ModuleResolver::ResolveModule(const std::string& module_id,
                                    const std::string& from_file,
                                    std::unordered_set<std::string>& visiting) {
    // 检查是否已解析
    if (resolved_cache_.find(module_id) != resolved_cache_.end()) {
        return true;
    }
    
    // 检查循环依赖
    if (visiting.find(module_id) != visiting.end()) {
        if (verbose_) {
            std::cout << "  Warning: Circular dependency detected: " 
                      << module_id << std::endl;
        }
        // 循环依赖不是错误，只是警告
        return true;
    }
    
    visiting.insert(module_id);
    
    ResolvedModule module;
    module.id = module_id;
    
    // 检查是否为内置模块
    if (IsBuiltinModule(module_id)) {
        module.is_builtin = true;
        module.source = builtin_modules_[module_id];
        module.path = "";
        
        if (verbose_) {
            std::cout << "  Resolved builtin: " << module_id << std::endl;
        }
    } else {
        // 文件系统模块
        module.is_builtin = false;
        module.path = module_id;
        
        auto content = ReadFile(module_id);
        if (!content) {
            AddError("Cannot find module: " + module_id, from_file, 0);
            visiting.erase(module_id);
            return false;
        }
        module.source = *content;
        
        if (verbose_) {
            std::cout << "  Resolved file: " << module_id 
                      << " (" << module.source.size() << " bytes)" << std::endl;
        }
    }
    
    // 解析此模块的依赖
    auto imports = ParseImports(module.source);
    
    for (const auto& import : imports) {
        std::string resolved_path = ResolvePath(import.module_path, 
                                                 module.path.empty() ? from_file : module.path);
        
        // 如果是裸模块名且不是内置模块，报错
        if (!import.module_path.starts_with("./") && 
            !import.module_path.starts_with("../") &&
            !IsBuiltinModule(import.module_path)) {
            AddError("Unknown module: " + import.module_path + 
                     " (not a builtin module)", 
                     module.path.empty() ? module_id : module.path,
                     import.line_number);
            continue;
        }
        
        // 使用内置模块名或解析后的路径作为 ID
        std::string dep_id = IsBuiltinModule(import.module_path) 
                             ? import.module_path 
                             : resolved_path;
        
        module.dependencies.push_back(dep_id);
        
        // 递归解析依赖
        if (!ResolveModule(dep_id, 
                           module.path.empty() ? from_file : module.path, 
                           visiting)) {
            // 继续处理其他依赖，收集所有错误
        }
    }
    
    resolved_cache_[module_id] = std::move(module);
    visiting.erase(module_id);
    return true;
}

std::vector<ResolvedModule> ModuleResolver::TopologicalSort() {
    std::vector<ResolvedModule> result;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> in_stack;
    
    // DFS 后序遍历
    std::function<void(const std::string&)> visit = [&](const std::string& id) {
        if (visited.find(id) != visited.end()) return;
        if (in_stack.find(id) != in_stack.end()) return;  // 循环依赖
        
        in_stack.insert(id);
        
        auto it = resolved_cache_.find(id);
        if (it != resolved_cache_.end()) {
            for (const auto& dep : it->second.dependencies) {
                visit(dep);
            }
            visited.insert(id);
            result.push_back(it->second);
        }
        
        in_stack.erase(id);
    };
    
    // 从所有模块开始遍历
    for (const auto& [id, _] : resolved_cache_) {
        visit(id);
    }
    
    return result;
}

std::string ModuleResolver::ToRelativeId(const std::string& abs_path) const {
    if (entry_dir_.empty()) return abs_path;
    try {
        std::string rel = fs::relative(fs::path(abs_path), fs::path(entry_dir_)).string();
        // 统一使用正斜杠
        std::replace(rel.begin(), rel.end(), '\\', '/');
        return rel;
    } catch (...) {
        return abs_path;
    }
}

std::vector<ResolvedModule> ModuleResolver::Resolve(const std::string& entry_file) {
    errors_.clear();
    resolved_cache_.clear();

    // 规范化入口文件路径
    std::string entry_id = NormalizePath(entry_file);

    // 记录入口文件所在目录（用于后续计算相对路径）
    entry_dir_ = fs::path(entry_id).parent_path().string();

    if (!fs::exists(entry_file)) {
        AddError("Entry file not found: " + entry_file);
        return {};
    }

    if (verbose_) {
        std::cout << "Resolving modules from: " << entry_id << std::endl;
    }

    // 开始解析（内部使用绝对路径进行文件读取和去重）
    std::unordered_set<std::string> visiting;
    ResolveModule(entry_id, "", visiting);

    if (HasErrors()) {
        return {};
    }

    // 拓扑排序
    auto sorted = TopologicalSort();

    // 将所有非 builtin 模块的 id 和 dependencies 转换为相对路径
    // 构建绝对路径 -> 相对路径的映射表
    std::unordered_map<std::string, std::string> path_map;
    for (auto& module : sorted) {
        if (!module.is_builtin) {
            std::string rel_id = ToRelativeId(module.id);
            path_map[module.id] = rel_id;
            module.id = rel_id;
        }
    }
    // 更新 dependencies 中的路径引用
    for (auto& module : sorted) {
        for (auto& dep : module.dependencies) {
            auto it = path_map.find(dep);
            if (it != path_map.end()) {
                dep = it->second;
            }
        }
    }

    if (verbose_) {
        std::cout << "Module order:" << std::endl;
        for (size_t i = 0; i < sorted.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << sorted[i].id;
            if (sorted[i].is_builtin) std::cout << " (builtin)";
            std::cout << std::endl;
        }
    }

    return sorted;
}

}  // namespace mbink
