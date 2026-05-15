/**
 * @file bytecode_compiler.cpp
 * @brief QuickJS 字节码编译器实现
 */

#include "bytecode_compiler.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <algorithm>

extern "C" {
#include "quickjs/quickjs.h"
}

namespace fs = std::filesystem;

namespace mbink {

BytecodeCompiler::BytecodeCompiler() : runtime_(nullptr), ctx_(nullptr) {
    Initialize();
}

BytecodeCompiler::~BytecodeCompiler() {
    Cleanup();
}

bool BytecodeCompiler::Initialize() {
    runtime_ = JS_NewRuntime();
    if (!runtime_) {
        AddError("Failed to create QuickJS runtime");
        return false;
    }

    ctx_ = JS_NewContext(runtime_);
    if (!ctx_) {
        AddError("Failed to create QuickJS context");
        JS_FreeRuntime(runtime_);
        runtime_ = nullptr;
        return false;
    }

    // 设置模块加载器，让 QuickJS 编译 ES module 时能解析 import 依赖
    JS_SetModuleLoaderFunc(runtime_, ModuleNormalize, ModuleLoader, this);

    return true;
}

char* BytecodeCompiler::ModuleNormalize(JSContext* ctx, const char* module_base,
                                         const char* module_name, void* opaque) {
    BytecodeCompiler* compiler = static_cast<BytecodeCompiler*>(opaque);
    std::string name(module_name);
    std::string base(module_base ? module_base : "");

    // 裸模块名（如 "preact", "preact/hooks"）直接返回
    if (!name.starts_with("./") && !name.starts_with("../")) {
        return js_strdup(ctx, module_name);
    }

    // 相对路径：基于 module_base 解析
    if (base.empty()) {
        return js_strdup(ctx, module_name);
    }

    // module_base 现在是相对路径（如 "app.js", "src/components.js"）
    // 基于它的 parent 目录拼接 import 路径
    fs::path base_path(base);
    fs::path resolved = (base_path.parent_path() / name).lexically_normal();

    // 统一使用正斜杠
    std::string resolved_str = resolved.string();
    std::replace(resolved_str.begin(), resolved_str.end(), '\\', '/');

    if (compiler) {
        if (compiler->module_sources_.find(resolved_str) != compiler->module_sources_.end()) {
            return js_strdup(ctx, resolved_str.c_str());
        }

        const std::string with_js = resolved_str + ".js";
        if (compiler->module_sources_.find(with_js) != compiler->module_sources_.end()) {
            return js_strdup(ctx, with_js.c_str());
        }

        const std::string with_index = resolved_str + "/index.js";
        if (compiler->module_sources_.find(with_index) != compiler->module_sources_.end()) {
            return js_strdup(ctx, with_index.c_str());
        }
    }

    return js_strdup(ctx, resolved_str.c_str());
}

JSModuleDef* BytecodeCompiler::ModuleLoader(JSContext* ctx, const char* module_name,
                                             void* opaque) {
    BytecodeCompiler* compiler = static_cast<BytecodeCompiler*>(opaque);
    if (!compiler) {
        JS_ThrowInternalError(ctx, "Compiler not found in module loader");
        return nullptr;
    }

    // 从预存的模块源码映射中查找
    auto it = compiler->module_sources_.find(module_name);
    if (it == compiler->module_sources_.end()) {
        JS_ThrowReferenceError(ctx, "could not load module '%s'", module_name);
        return nullptr;
    }

    // 编译模块源码
    JSValue module_val = JS_Eval(ctx, it->second.c_str(), it->second.length(),
                                  module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(module_val)) {
        return nullptr;
    }

    JSModuleDef* m = (JSModuleDef*)JS_VALUE_GET_PTR(module_val);
    JS_FreeValue(ctx, module_val);
    return m;
}

void BytecodeCompiler::Cleanup() {
    if (ctx_) {
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }
    if (runtime_) {
        JS_FreeRuntime(runtime_);
        runtime_ = nullptr;
    }
}

void BytecodeCompiler::AddError(const std::string& message,
                                 const std::string& file,
                                 int line,
                                 int column) {
    errors_.push_back({message, file, line, column});
}

void BytecodeCompiler::ExtractException() {
    if (!ctx_) return;

    JSValue exception = JS_GetException(ctx_);
    if (JS_IsNull(exception) || JS_IsUndefined(exception)) {
        JS_FreeValue(ctx_, exception);
        return;
    }

    // 获取错误消息
    const char* msg = JS_ToCString(ctx_, exception);
    std::string error_msg = msg ? msg : "Unknown error";
    if (msg) JS_FreeCString(ctx_, msg);

    // 尝试获取堆栈信息
    JSValue stack = JS_GetPropertyStr(ctx_, exception, "stack");
    if (!JS_IsUndefined(stack)) {
        const char* stack_str = JS_ToCString(ctx_, stack);
        if (stack_str) {
            error_msg += "\n" + std::string(stack_str);
            JS_FreeCString(ctx_, stack_str);
        }
    }
    JS_FreeValue(ctx_, stack);

    // 尝试获取文件名和行号
    std::string filename;
    int line = 0, column = 0;

    JSValue fn = JS_GetPropertyStr(ctx_, exception, "fileName");
    if (!JS_IsUndefined(fn)) {
        const char* fn_str = JS_ToCString(ctx_, fn);
        if (fn_str) {
            filename = fn_str;
            JS_FreeCString(ctx_, fn_str);
        }
    }
    JS_FreeValue(ctx_, fn);

    JSValue ln = JS_GetPropertyStr(ctx_, exception, "lineNumber");
    if (!JS_IsUndefined(ln)) {
        JS_ToInt32(ctx_, &line, ln);
    }
    JS_FreeValue(ctx_, ln);

    JSValue col = JS_GetPropertyStr(ctx_, exception, "columnNumber");
    if (!JS_IsUndefined(col)) {
        JS_ToInt32(ctx_, &column, col);
    }
    JS_FreeValue(ctx_, col);

    JS_FreeValue(ctx_, exception);

    AddError(error_msg, filename, line, column);
}

std::vector<uint8_t> BytecodeCompiler::CompileModule(const std::string& source,
                                                      const std::string& filename,
                                                      bool is_module) {
    if (!ctx_) {
        AddError("Compiler not initialized");
        return {};
    }

    // 编译标志
    int eval_flags = JS_EVAL_FLAG_COMPILE_ONLY;
    if (is_module) {
        eval_flags |= JS_EVAL_TYPE_MODULE;
    } else {
        eval_flags |= JS_EVAL_TYPE_GLOBAL;
    }

    // 编译源码
    JSValue compiled = JS_Eval(ctx_, source.c_str(), source.length(),
                               filename.c_str(), eval_flags);

    if (JS_IsException(compiled)) {
        ExtractException();
        return {};
    }

    // 序列化为字节码
    int write_flags = JS_WRITE_OBJ_BYTECODE;
    if (strip_source_) {
        write_flags |= JS_WRITE_OBJ_STRIP_SOURCE;
    }
    if (strip_debug_) {
        write_flags |= JS_WRITE_OBJ_STRIP_DEBUG;
    }

    size_t bytecode_size = 0;
    uint8_t* bytecode_data = JS_WriteObject(ctx_, &bytecode_size, compiled, write_flags);

    JS_FreeValue(ctx_, compiled);

    if (!bytecode_data) {
        AddError("Failed to serialize bytecode for: " + filename);
        return {};
    }

    // 复制到 vector
    std::vector<uint8_t> result(bytecode_data, bytecode_data + bytecode_size);
    js_free(ctx_, bytecode_data);

    if (verbose_) {
        std::cout << "  Compiled: " << filename 
                  << " (" << result.size() << " bytes)" << std::endl;
    }

    return result;
}

std::vector<CompiledModule> BytecodeCompiler::CompileModules(
    const std::vector<ResolvedModule>& modules) {

    errors_.clear();
    std::vector<CompiledModule> result;
    result.reserve(modules.size());

    // 预填充模块源码映射，供 ModuleLoader 回调使用
    module_sources_.clear();
    for (const auto& module : modules) {
        module_sources_[module.id] = module.source;
    }

    for (size_t i = 0; i < modules.size(); i++) {
        const auto& module = modules[i];

        // 内置模块（preact, preact/hooks）在运行时由 esm_loader 的
        // RegisterPreactModules 提供，编译时只需注册到 context 供依赖解析，
        // 不需要打包字节码
        if (module.is_builtin) {
            // 仍然编译注册到 context 中，让后续模块的 import 能找到它
            CompileModule(module.source, module.id, true);
            if (verbose_) {
                std::cout << "  Skipped builtin: " << module.id << std::endl;
            }
            continue;
        }

        auto bytecode = CompileModule(module.source, module.id, true);
        if (bytecode.empty()) {
            // 错误已记录
            continue;
        }

        CompiledModule compiled;
        compiled.id = module.id;
        compiled.bytecode = std::move(bytecode);
        compiled.is_entry = (i == modules.size() - 1);  // 最后一个是入口

        result.push_back(std::move(compiled));
    }

    return result;
}

std::vector<uint8_t> BytecodeCompiler::MergeBytecode(
    const std::vector<CompiledModule>& compiled_modules) {
    
    std::vector<uint8_t> result;
    
    // 写入模块数量
    uint32_t module_count = static_cast<uint32_t>(compiled_modules.size());
    result.push_back(static_cast<uint8_t>(module_count & 0xFF));
    result.push_back(static_cast<uint8_t>((module_count >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((module_count >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((module_count >> 24) & 0xFF));

    // 写入每个模块
    for (const auto& module : compiled_modules) {
        // 写入 ID 长度和 ID
        uint32_t id_len = static_cast<uint32_t>(module.id.size());
        result.push_back(static_cast<uint8_t>(id_len & 0xFF));
        result.push_back(static_cast<uint8_t>((id_len >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>((id_len >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((id_len >> 24) & 0xFF));
        result.insert(result.end(), module.id.begin(), module.id.end());

        // 写入 is_entry 标志
        result.push_back(module.is_entry ? 1 : 0);

        // 写入字节码长度和字节码
        uint32_t bc_len = static_cast<uint32_t>(module.bytecode.size());
        result.push_back(static_cast<uint8_t>(bc_len & 0xFF));
        result.push_back(static_cast<uint8_t>((bc_len >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>((bc_len >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((bc_len >> 24) & 0xFF));
        result.insert(result.end(), module.bytecode.begin(), module.bytecode.end());
    }

    return result;
}

std::vector<CompiledModule> BytecodeCompiler::ParseMergedBytecode(
    const std::vector<uint8_t>& merged) {
    
    std::vector<CompiledModule> result;
    
    if (merged.size() < 4) return result;

    size_t offset = 0;

    // 读取模块数量
    auto read_u32 = [&merged, &offset]() -> uint32_t {
        if (offset + 4 > merged.size()) return 0;
        uint32_t value = 
            static_cast<uint32_t>(merged[offset]) |
            (static_cast<uint32_t>(merged[offset + 1]) << 8) |
            (static_cast<uint32_t>(merged[offset + 2]) << 16) |
            (static_cast<uint32_t>(merged[offset + 3]) << 24);
        offset += 4;
        return value;
    };

    uint32_t module_count = read_u32();
    result.reserve(module_count);

    for (uint32_t i = 0; i < module_count; i++) {
        CompiledModule module;

        // 读取 ID
        uint32_t id_len = read_u32();
        if (offset + id_len > merged.size()) break;
        module.id = std::string(merged.begin() + offset, 
                                merged.begin() + offset + id_len);
        offset += id_len;

        // 读取 is_entry 标志
        if (offset >= merged.size()) break;
        module.is_entry = (merged[offset++] != 0);

        // 读取字节码
        uint32_t bc_len = read_u32();
        if (offset + bc_len > merged.size()) break;
        module.bytecode.assign(merged.begin() + offset,
                               merged.begin() + offset + bc_len);
        offset += bc_len;

        result.push_back(std::move(module));
    }

    return result;
}

}  // namespace mbink
