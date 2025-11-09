# Phase 2.1 快速启动指南

**目的**: 快速恢复 Phase 2.1 JavaScript Runtime 的开发工作

---

## 📍 当前状态

- **进度**: 52/67 任务完成 (78%) 🎉
- **最后更新**: 2025-11-09
- **当前位置**: Tasks 1-9 已完成，所有核心功能已实现！仅剩性能测试

---

## 🚀 快速启动步骤

### 1. 进入构建目录
```powershell
cd d:\code\C\MBink\build
```

### 2. 重新构建项目（如果需要）
```powershell
cmake -S .. -B .
mingw32-make
```

### 3. 运行现有测试
```powershell
# 测试 QuickJS C 接口（应该成功）
.\bin\test_quickjs_c.exe

# 测试简单 C++ 程序（应该成功）
.\bin\test_simple.exe

# 测试 QuickJS Runtime（需要先应用修复）
.\bin\test_quickjs_runtime.exe
```

---

## ⚠️ 重要修复（必须先应用）

### 修复 1: 为所有测试添加静态链接

编辑 `tests/CMakeLists.txt`，为 `test_hello` 和 `test_quickjs_runtime` 添加：

```cmake
# test_hello
if(WIN32 AND MINGW)
    target_link_options(test_hello PRIVATE -static-libgcc -static-libstdc++)
endif()

# test_quickjs_runtime
if(WIN32 AND MINGW)
    target_link_options(test_quickjs_runtime PRIVATE -static-libgcc -static-libstdc++)
endif()
```

### 修复 2: 恢复 test_simple 的完整功能

编辑 `tests/test_simple.cpp`：

```cpp
#include "core/quickjs/quickjs_runtime.h"
#include <cstdio>

using namespace lightui;

int main() {
    try {
        printf("Creating QuickJS runtime...\n");
        QuickJSRuntime runtime;
        
        printf("Evaluating: 1 + 1\n");
        auto result = runtime.Eval("1 + 1");
        printf("Result: %s\n", result.dump().c_str());
        
        printf("Test passed!\n");
        return 0;
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
}
```

编辑 `tests/CMakeLists.txt`，恢复链接：

```cmake
target_link_libraries(test_simple PRIVATE
    lightui_quickjs
)
```

### 应用修复后重新构建
```powershell
cmake -S .. -B .
mingw32-make
.\bin\test_simple.exe
.\bin\test_quickjs_runtime.exe
```

---

## 📋 下一步任务清单

### 优先级 1: 验证现有功能 ✅ 已完成
- ✅ 应用上述修复
- ✅ 运行所有测试，确保通过
- ✅ 验证 Tasks 1-6 的实现正确

### 优先级 2: 实现 Console API (Task 9) ✅ 已完成
**已完成**: Console API 已成功实现并通过所有测试

实现内容：
- ✅ console.log() - 标准日志输出
- ✅ console.error() - 错误日志（带 [ERROR] 前缀）
- ✅ console.warn() - 警告日志（带 [WARN] 前缀）
- ✅ console.info() - 信息日志（带 [INFO] 前缀）
- ✅ 多参数支持 - 参数间用空格分隔
- ✅ 测试覆盖 - 在 `test_quickjs_runtime.cpp` 中添加了完整测试

### 优先级 3: 实现模块加载系统 (Task 7) ✅ 已完成

**已完成**: Module Loading System 已成功实现并通过所有测试

实现内容：
- ✅ 模块加载器回调 - `ModuleLoader()`
- ✅ 模块注册表 - `std::unordered_map<std::string, std::string> module_registry_`
- ✅ RegisterModule() - 注册模块代码
- ✅ LoadModule() - 加载并执行模块
- ✅ LoadModuleFile() - 从文件系统加载模块
- ✅ 测试覆盖 - 包括简单导出、默认导出、多重导出、模块依赖等

### 优先级 4: 实现异步任务队列 (Task 8) ✅ 已完成

**已完成**: Async Task Queue 已成功实现并通过所有测试

实现内容：
- ✅ 事件循环架构 - Task 结构体、任务队列、定时器队列
- ✅ setTimeout/setInterval - 支持参数传递
- ✅ clearTimeout/clearInterval - 正确取消定时器
- ✅ Promise 支持 - 使用 `JS_ExecutePendingJob()`
- ✅ RunEventLoop() - 正确处理微任务和宏任务顺序
- ✅ 测试覆盖 - 定时器、Promise、混合使用等

### 优先级 5: 完成运行时测试 (Task 10) ✅ 基本完成

**已完成**: 9/10 测试任务完成

实现内容：
- ✅ 所有核心功能测试（类型转换、代码执行、函数调用等）
- ✅ Console API 测试
- ✅ 模块加载测试
- ✅ 异步任务测试（定时器和 Promise）
- ⏳ 性能基准测试（可选）

---

## 📁 关键文件位置

### 需要编辑的文件
```
core/quickjs/quickjs_runtime.h       # 添加新方法声明
core/quickjs/quickjs_runtime.cpp     # 实现新功能
tests/test_quickjs_runtime.cpp       # 添加测试
tests/CMakeLists.txt                 # 应用修复
```

### 参考文件
```
PHASE_2_1_PROGRESS.md                # 详细进度记录
PROJECT_PROGRESS.md                  # 项目总进度
third_party/quickjs/quickjs.h        # QuickJS API 参考
```

---

## 🔍 常用命令

### 构建和测试
```powershell
# 完整重新构建
cmake -S .. -B . && mingw32-make

# 只构建测试
mingw32-make test_simple
mingw32-make test_quickjs_runtime

# 运行测试
.\bin\test_simple.exe
.\bin\test_quickjs_runtime.exe

# 查看详细构建过程
mingw32-make VERBOSE=1 test_simple
```

### 调试
```powershell
# 查看可执行文件依赖
objdump -p bin\test_simple.exe | Select-String -Pattern "DLL Name"

# 检查符号
nm bin\test_simple.exe | Select-String -Pattern "console"

# 运行并捕获输出
.\bin\test_simple.exe 2>&1 | Tee-Object -Variable output
```

---

## 💡 开发技巧

### 1. 增量开发
每实现一个小功能就立即测试，不要等到全部完成

### 2. 使用 printf 调试
在开发过程中大量使用 printf，确保每一步都正确

### 3. 参考 QuickJS 示例
查看 `third_party/quickjs/` 中的示例代码

### 4. 错误处理
始终检查 `JS_IsException()` 并使用 `JS_GetException()` 获取错误信息

### 5. 内存管理
记住调用 `JS_FreeValue()` 和 `JS_FreeCString()`

---

## 🐛 已知问题和解决方案

### 问题 1: 控制台无输出
**症状**: 程序运行但没有任何输出  
**原因**: CMake 使用 UCRT 运行时库  
**解决**: 添加 `-static-libgcc -static-libstdc++` 链接选项

### 问题 2: QuickJS libc 函数崩溃
**症状**: 调用 `js_std_init_handlers()` 后程序崩溃  
**原因**: Windows 兼容性问题  
**解决**: 禁用这些函数，实现自定义标准库

### 问题 3: libbf 链接错误
**症状**: 找不到 `bf_*` 函数  
**原因**: QuickJS 构建缺少 libbf.c  
**解决**: 在 `third_party/CMakeLists.txt` 中添加 libbf.c 和 CONFIG_BIGNUM=1

---

## 📚 参考资源

### QuickJS 文档
- 官方网站: https://bellard.org/quickjs/
- API 文档: `third_party/quickjs/quickjs.h`
- 示例代码: `third_party/quickjs/examples/`

### nlohmann/json 文档
- GitHub: https://github.com/nlohmann/json
- 文档: https://json.nlohmann.me/

### 项目文档
- 架构设计: `docs/ARCHITECTURE.md`
- API 设计: `docs/API_DESIGN.md`
- 编码规范: `docs/CODING_STANDARDS.md`

---

## ✅ 检查清单

开始工作前：
- [ ] 阅读 `PHASE_2_1_PROGRESS.md` 了解详细进度
- [ ] 应用所有待应用的修复
- [ ] 运行现有测试确保环境正常
- [ ] 确定要实现的具体任务

开发过程中：
- [ ] 每完成一个小功能就测试
- [ ] 及时更新进度文件
- [ ] 编写清晰的注释
- [ ] 遵循项目编码规范

完成后：
- [ ] 运行所有测试
- [ ] 更新 `PHASE_2_1_PROGRESS.md`
- [ ] 更新 `PROJECT_PROGRESS.md`
- [ ] 提交代码（如果使用版本控制）

---

**祝开发顺利！** 🚀

