/**
 * @file bytecode_compiler.cpp
 * @brief QuickJS 字节码编译器实现
 */

#include "bytecode_compiler.h"
#include <iostream>
#include <cstring>

extern "C" {
#include "quickjs/quickjs.h"
}

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

    return true;
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

    for (size_t i = 0; i < modules.size(); i++) {
        const auto& module = modules[i];
        
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
