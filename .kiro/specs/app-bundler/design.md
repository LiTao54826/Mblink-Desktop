# App Bundler Design Document

## Overview

App Bundler 是一个将 Preact/JS 应用打包成独立可执行文件的命令行工具。它通过以下方式实现：

1. 解析 ES6 模块依赖图
2. 将所有 JS 模块编译为 QuickJS 字节码
3. 将字节码和配置嵌入到 app_loader.exe 模板的末尾
4. 生成可独立运行的 exe 文件

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        App Bundler                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │   CLI       │  │  Module     │  │  Bytecode   │              │
│  │   Parser    │──│  Resolver   │──│  Compiler   │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│         │                │                │                      │
│         ▼                ▼                ▼                      │
│  ┌─────────────────────────────────────────────────┐            │
│  │              Payload Builder                     │            │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐           │            │
│  │  │ Config  │ │Bytecode │ │Checksum │           │            │
│  │  │ Section │ │ Section │ │ + Magic │           │            │
│  │  └─────────┘ └─────────┘ └─────────┘           │            │
│  └─────────────────────────────────────────────────┘            │
│                          │                                       │
│                          ▼                                       │
│  ┌─────────────────────────────────────────────────┐            │
│  │              Exe Writer                          │            │
│  │  [app_loader.exe template] + [payload]          │            │
│  └─────────────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. CLI Parser

解析命令行参数：

```cpp
struct BundlerOptions {
    std::string input_file;      // 主 JS 文件
    std::string output_file;     // 输出 exe 路径
    std::string template_exe;    // 模板 exe 路径（可选）
    std::vector<std::string> include_files;  // 额外包含的 JS 文件
    
    // 窗口配置
    int width = 800;
    int height = 600;
    std::string title = "MBink App";
    
    bool verbose = false;
    bool no_overwrite = false;
};
```

### 2. Module Resolver

解析 ES6 模块依赖：

```cpp
class ModuleResolver {
public:
    // 解析入口文件及其所有依赖
    std::vector<ResolvedModule> Resolve(const std::string& entry_file);
    
    // 注册内置模块（preact, hooks 等）
    void RegisterBuiltinModule(const std::string& name, const std::string& source);
    
private:
    // 解析单个文件的 import 语句
    std::vector<ImportInfo> ParseImports(const std::string& source);
    
    // 解析模块路径
    std::string ResolvePath(const std::string& import_path, const std::string& from_file);
};

struct ResolvedModule {
    std::string path;           // 模块路径
    std::string source;         // 源代码
    bool is_builtin;            // 是否为内置模块
    std::vector<std::string> dependencies;  // 依赖的模块路径
};
```

### 3. Bytecode Compiler

使用 QuickJS 编译 JS 为字节码：

```cpp
class BytecodeCompiler {
public:
    BytecodeCompiler();
    ~BytecodeCompiler();
    
    // 编译单个模块为字节码
    std::vector<uint8_t> CompileModule(const std::string& source, 
                                        const std::string& filename,
                                        bool is_module = true);
    
    // 编译多个模块，返回合并的字节码
    std::vector<uint8_t> CompileModules(const std::vector<ResolvedModule>& modules);
    
private:
    JSRuntime* runtime_;
    JSContext* ctx_;
};
```

### 4. Payload Builder

构建嵌入数据：

```cpp
class PayloadBuilder {
public:
    void SetConfig(const BundlerOptions& options);
    void SetBytecode(const std::vector<uint8_t>& bytecode);
    
    // 构建完整的 payload
    std::vector<uint8_t> Build();
    
    // 从 payload 解析（用于 app_loader）
    static bool Parse(const std::vector<uint8_t>& data, 
                      PayloadData& out_data);
};
```

### 5. Exe Writer

写入最终的 exe 文件：

```cpp
class ExeWriter {
public:
    // 读取模板 exe
    bool LoadTemplate(const std::string& template_path);
    
    // 追加 payload 并写入输出文件
    bool WriteOutput(const std::string& output_path, 
                     const std::vector<uint8_t>& payload);
};
```

## Data Models

### Payload 格式

```
┌────────────────────────────────────────────────────────────┐
│                    Original app_loader.exe                  │
├────────────────────────────────────────────────────────────┤
│                    Payload Section                          │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Config Header (JSON)                                  │  │
│  │ {                                                     │  │
│  │   "version": 1,                                       │  │
│  │   "width": 800,                                       │  │
│  │   "height": 600,                                      │  │
│  │   "title": "My App",                                  │  │
│  │   "module_count": 5,                                  │  │
│  │   "bytecode_size": 12345                              │  │
│  │ }                                                     │  │
│  ├──────────────────────────────────────────────────────┤  │
│  │ Bytecode Data                                         │  │
│  │ [QuickJS compiled bytecode for all modules]          │  │
│  ├──────────────────────────────────────────────────────┤  │
│  │ Footer                                                │  │
│  │ [config_size: 4 bytes]                               │  │
│  │ [bytecode_size: 4 bytes]                             │  │
│  │ [crc32: 4 bytes]                                     │  │
│  │ [magic: "MBPK" 4 bytes]                              │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

### Magic Number

```cpp
constexpr uint32_t PAYLOAD_MAGIC = 0x4B50424D;  // "MBPK" in little-endian
constexpr uint32_t PAYLOAD_VERSION = 1;
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Bundling produces valid executable
*For any* valid JS application, bundling it should produce an executable file that contains the original template exe plus a valid payload section.
**Validates: Requirements 1.1, 1.3**

### Property 2: All modules compiled to bytecode
*For any* ES6 module dependency graph, all resolved modules (including transitive dependencies) should be compiled to QuickJS bytecode format in the bundle.
**Validates: Requirements 2.1, 2.6**

### Property 3: Module resolution completeness
*For any* import statement in the source, the resolver should either find the module or report a clear error with path and location.
**Validates: Requirements 2.2, 2.5**

### Property 4: Dependency order preservation
*For any* set of modules with dependencies, the compiled bytecode should preserve initialization order such that dependencies are loaded before dependents.
**Validates: Requirements 2.7**

### Property 5: Configuration embedding
*For any* bundler options (width, height, title), the payload should contain the exact configuration values that can be extracted at runtime.
**Validates: Requirements 4.1, 4.2, 4.3, 4.4**

### Property 6: Checksum integrity
*For any* bundled payload, the CRC32 checksum should match the actual data, and any modification to the payload should cause checksum verification to fail.
**Validates: Requirements 8.1, 8.2, 8.3**

### Property 7: Payload detection round-trip
*For any* bundled executable, reading the payload from the end of the file should recover the exact bytecode and configuration that was embedded.
**Validates: Requirements 7.1, 7.2**

## Error Handling

| Error Type | Handling |
|------------|----------|
| Input file not found | Exit with error message showing path |
| Template exe not found | Exit with instructions on locating app_loader.exe |
| JS syntax error | Report file, line, column, and error message |
| Import not found | Report import path and source location |
| Circular dependency | Handle gracefully, warn if --verbose |
| Output write failure | Exit with error message |
| Checksum mismatch | Refuse to execute, show corruption error |

## Testing Strategy

### Unit Tests
- Module resolver: test import parsing and path resolution
- Bytecode compiler: test compilation of various JS constructs
- Payload builder: test serialization/deserialization
- CRC32: test checksum calculation

### Property-Based Tests
使用 QuickCheck/fast-check 风格的测试：

1. **Bytecode round-trip**: 编译后的字节码应能正确执行
2. **Payload round-trip**: 序列化后反序列化应得到相同数据
3. **Checksum integrity**: 任意修改应导致校验失败
4. **Module order**: 依赖图的拓扑排序应正确

### Integration Tests
- 打包简单应用并运行
- 打包带 ES6 模块的应用
- 打包带 Preact 组件的应用
- 测试各种命令行选项组合
