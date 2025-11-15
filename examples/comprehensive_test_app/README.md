# MBink 综合功能测试应用

## 📖 简介

这是 MBink 框架的综合功能测试应用，用于验证所有核心 DOM API 的正确性。

**当前状态**: ✅ **100% 测试通过** (126/126)

## 🚀 快速开始

### 编译

```bash
# 从项目根目录
cmake --build build --config Release --target comprehensive_test_app
```

### 运行

```bash
# Windows
.\build\bin\Release\comprehensive_test_app.exe

# Linux/macOS
./build/bin/Release/comprehensive_test_app
```

### 预期输出

```
=== MBink 综合功能测试应用 ===
版本: v1.0
日期: 2025-11-15

[1/5] 创建 Document...
   ✓ Document 创建成功
[2/5] 创建 TaskScheduler...
   ✓ TaskScheduler 创建成功
[3/5] 创建 QuickJS 运行时...
   ✓ QuickJS 运行时创建成功
[4/5] 初始化 DOM 绑定...
   ✓ DOM 绑定初始化成功
[5/5] 加载测试框架...
   ✓ 测试框架加载成功

=== 开始运行测试 ===

✅ DOM 操作 > createElement - 创建元素
✅ DOM 操作 > createTextNode - 创建文本节点
... (126 个测试)

════════════════════════════════════════════════════════════
📊 测试结果
════════════════════════════════════════════════════════════
✅ 通过: 126
❌ 失败: 0
📝 总计: 126
📈 通过率: 100.00%
════════════════════════════════════════════════════════════
🎉 优秀！所有测试基本通过！
════════════════════════════════════════════════════════════
```

## 📁 文件结构

```
comprehensive_test_app/
├── README.md              # 本文件
├── main.cpp              # 主程序入口
├── test_framework.js     # 测试框架
├── CMakeLists.txt        # 构建配置
└── tests/                # 测试用例目录
    ├── dom_tests.js      # DOM 操作测试 (19 个)
    ├── attribute_tests.js # 属性操作测试 (15 个)
    ├── event_tests.js    # 事件系统测试 (20 个)
    ├── query_tests.js    # 查询选择器测试 (12 个)
    ├── timer_tests.js    # 定时器测试 (8 个)
    ├── form_tests.js     # 表单元素测试 (35 个)
    └── html_tests.js     # HTML 内容测试 (17 个)
```

## 📊 测试覆盖

| 测试类别 | 测试数量 | 通过率 | 状态 |
|---------|---------|--------|------|
| DOM 操作 | 19 | 100% | ✅ |
| 属性操作 | 15 | 100% | ✅ |
| 事件系统 | 20 | 100% | ✅ |
| 查询选择器 | 12 | 100% | ✅ |
| 定时器 | 8 | 100% | ✅ |
| 表单元素 | 35 | 100% | ✅ |
| HTML 内容 | 17 | 100% | ✅ |
| **总计** | **126** | **100%** | ✅ |

## 🧪 测试框架 API

### 测试套件和用例

```javascript
// 定义测试套件
describe('测试套件名称', () => {
    // 定义测试用例
    test('测试用例名称', () => {
        // 测试代码
        const element = document.createElement('div');
        assertEqual(element.tagName, 'DIV', '标签名应为 DIV');
    });
});
```

### 断言函数

```javascript
// 基本断言
assert(condition, message)

// 相等性断言
assertEqual(actual, expected, message)
assertNotEqual(actual, expected, message)

// 空值断言
assertNull(value, message)
assertNotNull(value, message)

// 布尔断言
assertTrue(value, message)
assertFalse(value, message)
```

### 辅助函数

```javascript
// 创建测试元素
const element = createTestElement('div', {
    id: 'test-div',
    className: 'test-class',
    textContent: 'Hello',
    attributes: {
        'data-test': 'value'
    },
    styles: {
        'color': 'red'
    }
});

// 清理测试环境
cleanup();
```

## 📝 添加新测试

### 1. 创建测试文件

在 `tests/` 目录下创建新的 `.js` 文件：

```javascript
// tests/my_tests.js
describe('我的测试套件', () => {
    test('测试用例 1', () => {
        const element = document.createElement('div');
        element.id = 'test';
        assertEqual(element.id, 'test', 'ID 应该被正确设置');
    });
    
    test('测试用例 2', () => {
        const text = document.createTextNode('Hello');
        assertEqual(text.textContent, 'Hello', '文本内容应该正确');
    });
});
```

### 2. 注册测试文件

在 `main.cpp` 中添加文件路径：

```cpp
std::vector<std::string> test_files = {
    "examples/comprehensive_test_app/tests/dom_tests.js",
    "examples/comprehensive_test_app/tests/attribute_tests.js",
    // ... 其他测试文件 ...
    "examples/comprehensive_test_app/tests/my_tests.js"  // 添加新文件
};
```

### 3. 重新编译和运行

```bash
cmake --build build --config Release --target comprehensive_test_app
./build/bin/Release/comprehensive_test_app.exe
```

## 🔍 测试示例

### DOM 操作测试

```javascript
describe('DOM 操作', () => {
    test('createElement - 创建元素', () => {
        const div = document.createElement('div');
        assertNotNull(div, 'createElement 应该返回元素');
        assertEqual(div.tagName, 'DIV', '标签名应该正确');
    });
    
    test('appendChild - 添加子节点', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        parent.appendChild(child);
        assertEqual(parent.childNodes.length, 1, '应该有 1 个子节点');
        assertEqual(parent.firstChild, child, '第一个子节点应该是 span');
    });
});
```

### 属性操作测试

```javascript
describe('属性操作', () => {
    test('setAttribute - 设置属性', () => {
        const div = document.createElement('div');
        div.setAttribute('data-test', 'value');
        assertEqual(div.getAttribute('data-test'), 'value', '属性值应该正确');
    });
    
    test('classList - 类名操作', () => {
        const div = document.createElement('div');
        div.classList.add('class1');
        assertTrue(div.classList.contains('class1'), '应该包含 class1');
    });
});
```

### 事件系统测试

```javascript
describe('事件系统', () => {
    test('addEventListener - 添加事件监听器', () => {
        const div = document.createElement('div');
        let clicked = false;
        
        div.addEventListener('click', () => {
            clicked = true;
        });
        
        div.dispatchEvent(new Event('click'));
        assertTrue(clicked, '事件应该被触发');
    });
});
```

### 查询选择器测试

```javascript
describe('查询选择器', () => {
    test('querySelector - 通过 class 查询', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        child.className = 'test-class';
        parent.appendChild(child);
        
        const result = parent.querySelector('.test-class');
        assertEqual(result, child, '应该找到正确的元素');
    });
});
```

## 🐛 调试技巧

### 1. 查看失败的测试

```bash
# 只显示失败的测试
./build/bin/Release/comprehensive_test_app.exe 2>&1 | grep "❌" -A 2
```

### 2. 添加调试输出

```javascript
test('调试示例', () => {
    const element = document.createElement('div');
    console.log('Element:', element);
    console.log('TagName:', element.tagName);
    assertEqual(element.tagName, 'DIV', '标签名应该正确');
});
```

### 3. 使用 cleanup()

```javascript
describe('测试套件', () => {
    test('测试 1', () => {
        const div = document.createElement('div');
        document.body.appendChild(div);
        // 测试代码...
        cleanup(); // 清理测试环境
    });
});
```

## 📚 相关文档

- [完整测试报告](../../docs/COMPREHENSIVE_TEST_APP_REPORT.md)
- [框架完成总结](../../docs/FINAL_COMPLETION_SUMMARY.md)
- [DOM API 文档](../../core/dom/README.md)
- [事件系统文档](../../core/event/README.md)

## ✅ 测试清单

- [x] DOM 操作 API (19 个测试)
- [x] 属性操作 API (15 个测试)
- [x] 事件系统 API (20 个测试)
- [x] 查询选择器 API (12 个测试)
- [x] 定时器 API (8 个测试)
- [x] 表单元素 API (35 个测试)
- [x] HTML 内容 API (17 个测试)

## 🎯 总结

Comprehensive Test App 是一个完善的测试应用，具有：

- ✅ **100% 测试通过率** - 所有 126 个测试全部通过
- ✅ **完整的 API 覆盖** - 覆盖所有核心 DOM API
- ✅ **简洁的测试框架** - 易于编写和维护测试
- ✅ **详细的文档** - 完整的使用说明和示例

该应用验证了 MBink 框架的功能完整性和稳定性，可以作为未来开发的可靠基础。

---

**版本**: v1.0  
**日期**: 2025-11-15  
**状态**: ✅ 完成  
**测试通过率**: 100.00% (126/126)

