---
inclusion: always
---

# 构建和测试规范

本文档定义了 LightUI 项目的构建规范和自动化测试流程。

## 构建规范

### CMake 配置

```bash
# 标准开发构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### 构建命令

```bash
# 仅构建 esm_loader（用于测试）
cmake --build build --config Release --target esm_loader
```

### 编译检查清单

每次提交代码前，必须确保：
- [ ] 代码能够成功编译（无错误）
- [ ] 无编译警告（或已记录原因）
- [ ] 所有 JS 自动化测试通过
- [ ] 新增代码有对应测试

## 测试规范

### 测试方式

使用 `esm_loader` 运行 JavaScript 测试脚本，配合日志输出进行自动化验证。
不需要认为干预的加上 -q 3 自动退出参数

修复bug时，每排除一个地方就记录已经排除的方向，不要反复在已经排除的问题上来回的排查
### 运行测试命令

```cmd
# 运行测试脚本，日志追加到文件
build\bin\Release\esm_loader.exe tests\js\test_xxx.js >> debuglog.txt
不要人工干预的情况下加上 -q 5 自动退出避免卡死

# 搜索过滤需要的日志（不要读取全部日志）
findstr "TEST_PASS TEST_FAIL ERROR" debuglog.txt

# 测试完成后删除日志文件
del debuglog.txt
```

### JS 测试脚本规范

#### 测试文件位置
- 测试脚本放在 `tests/js/` 目录
- 文件名格式：`test_{feature}.js`

#### 测试脚本模板

```javascript
// tests/js/test_example.js

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName) {
    const passed = actual === expected;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

// 测试用例
function testExample() {
    // 测试逻辑
    const result = someFunction();
    assertEqual(result, expectedValue, "Example test case");
}

// 运行测试
console.log("[TEST_START] Example Tests");
testExample();
console.log("[TEST_END]");
```

#### 日志标记规范

测试脚本必须使用以下标记输出日志：
- `[TEST_START]` - 测试开始
- `[TEST_PASS]` - 测试通过
- `[TEST_FAIL]` - 测试失败
- `[TEST_END]` - 测试结束
- `[ERROR]` - 错误信息

### 日志处理规范

1. **日志输出到文件**：运行时将输出重定向到临时日志文件
2. **过滤搜索**：只搜索过滤需要的日志内容，不要读取全部日志
3. **及时清理**：测试完成后删除日志文件

```cmd
# Windows CMD 示例
build\bin\Release\esm_loader.exe tests\js\test_xxx.js >> debuglog.txt
不需要人工干预的情况下加上 -q 5 自动退出避免卡住
# 检查测试结果（过滤搜索，不读取全部日志）
findstr "TEST_FAIL" debuglog.txt
findstr "TEST_PASS" debuglog.txt

# 清理日志文件
del debuglog.txt
```

### 属性测试规范

属性测试用于验证设计文档中定义的正确性属性。

#### 属性测试注释格式
每个属性测试必须包含以下注释：

```javascript
/**
 * Property Test: {属性名称}
 * 
 * Feature: {feature_name}, Property {number}: {property_text}
 * Validates: Requirements {requirement_numbers}
 */
function testPropertyXxx() {
    // 属性测试实现
}
```

## 测试开发规范

### 新增测试检查清单

创建新测试时，确保：
- [ ] 测试文件放在 `tests/js/` 目录
- [ ] 使用标准日志标记
- [ ] 测试能够独立运行
- [ ] 属性测试包含正确的注释格式

### 测试命名规范

- 测试文件名：`test_{feature}.js`
- 测试函数名：`test{Feature}{Case}`
- 示例：`testFocusControllerSetFocus`

## 任务执行规范

### 自动任务流程

执行 spec 任务时，遵循以下规则：

1. **自动继续**：完成一个任务后，自动开始下一个任务，无需用户确认
2. **减少询问**：除非遇到以下情况，否则不要询问用户：
   - 任务需求不明确或有歧义
   - 需要用户做出设计决策
   - 遇到无法自动解决的错误
   - 任务涉及破坏性变更
3. **进度报告**：每完成一个任务，简短报告完成状态，然后继续下一个

### 任务前置测试要求

开始每个任务前，必须：

1. **编译验证**：确保当前代码能够编译通过
   ```cmd
   cmake --build build --config Release --target esm_loader
   ```

2. **运行现有测试**：确保不破坏现有功能
   ```cmd
   build\bin\Release\esm_loader.exe tests\js\test_xxx.js -q 5 >> debuglog.txt
   findstr "TEST_FAIL" debuglog.txt
   del debuglog.txt
   ```

3. **如果测试失败**：先修复失败的测试，再开始新任务

### 任务完成检查清单

完成每个任务后，必须：
- [ ] 代码编译通过
- [ ] 相关测试通过
- [ ] 更新 tasks.md 中的任务状态为 `[x]`
- [ ] 自动开始下一个任务

### 错误处理

- 遇到编译错误：立即修复，不要跳过
- 遇到测试失败：分析原因，修复后继续
- 遇到设计问题：记录问题，尝试合理解决方案，只有无法决策时才询问用户

## 参考文档

- 测试脚本示例：`tests/js/`
- esm_loader 使用：`tools/esm_loader/`
