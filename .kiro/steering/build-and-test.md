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

### 运行测试命令

```cmd
# 运行测试脚本，日志追加到文件
build\bin\Release\esm_loader.exe tests\js\test_xxx.js >> debuglog.txt

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

## 参考文档

- 测试脚本示例：`tests/js/`
- esm_loader 使用：`tools/esm_loader/`
