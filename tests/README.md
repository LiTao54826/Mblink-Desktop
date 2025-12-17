# LightUI 测试套件

基于历史测试清单重新编写的全面测试套件，覆盖 DOM、事件、布局、渲染、JavaScript 集成等核心功能。

## 测试分类

| 类别 | 目录 | 说明 |
|------|------|------|
| 单元测试 | `unit/` | 核心功能单元测试 |
| 渲染测试 | `render/` | 渲染相关测试 |
| 集成测试 | `integration/` | 模块集成测试 |
| 性能测试 | `performance/` | 性能基准测试 |

## 单元测试详情

### DOM 测试 (`unit/dom/`)
- `test_node.cpp` - Node 基类测试（节点类型、父子关系、脏标记）
- `test_element.cpp` - Element 测试（属性、样式、事件、选择器）
- `test_document.cpp` - Document 测试（工厂方法、查询、HTML 加载）
- `test_text.cpp` - Text 节点测试
- `test_selector_engine.cpp` - CSS 选择器引擎测试
- `test_dom_token_list.cpp` - classList 测试
- `test_css_style_declaration.cpp` - style 对象测试

### 事件测试 (`unit/event/`)
- `test_event.cpp` - 事件创建、传播、默认行为
- `test_event_loop.cpp` - 事件循环测试
- `test_task_scheduler.cpp` - 任务调度器测试
- `test_input_handler.cpp` - 输入处理器测试

### 布局测试 (`unit/layout/`)
- `test_native_layout_engine.cpp` - 布局引擎测试
- `test_flex_layout.cpp` - Flexbox 布局测试
- `test_block_layout.cpp` - Block 布局测试

### 渲染测试 (`unit/render/`)
- `test_css_value.cpp` - CSS 值解析测试
- `test_css_variables.cpp` - CSS 变量测试
- `test_color.cpp` - 颜色处理测试
- `test_transform.cpp` - 变换测试

### QuickJS 测试 (`unit/quickjs/`)
- `test_quickjs_runtime.cpp` - QuickJS 运行时测试
- `test_dom_bindings.cpp` - DOM JavaScript 绑定测试

### Lexbor 测试 (`unit/lexbor/`)
- `test_lexbor_document.cpp` - HTML 解析测试
- `test_style_manager.cpp` - 样式管理器测试

### 工具测试 (`unit/utils/`)
- `test_json.cpp` - JSON 工具测试
- `test_encoding.cpp` - 编码工具测试

## 构建测试

```bash
# 配置（启用测试）
cmake -B build -DLIGHTUI_BUILD_TESTS=ON

# 构建
cmake --build build

# 运行所有测试
cd build
ctest --output-on-failure

# 或运行特定测试
./bin/lightui_unit_tests
./bin/lightui_integration_tests
./bin/lightui_render_tests
./bin/lightui_performance_tests
```

## 运行特定测试

```bash
# 运行特定测试用例
./bin/lightui_unit_tests --gtest_filter="NodeTest.*"
./bin/lightui_unit_tests --gtest_filter="ElementTest.SetAttribute"

# 列出所有测试
./bin/lightui_unit_tests --gtest_list_tests
```

## 性能测试

性能测试默认不构建，需要显式启用：

```bash
cmake -B build -DLIGHTUI_BUILD_TESTS=ON -DLIGHTUI_BUILD_PERF_TESTS=ON
cmake --build build
./bin/lightui_performance_tests
```

## 测试覆盖率

测试覆盖以下核心模块：

- ✅ DOM 操作（Node, Element, Document, Text）
- ✅ CSS 选择器（ID, Class, Tag, 属性, 伪类, 组合选择器）
- ✅ 样式系统（classList, style, CSS 变量）
- ✅ 事件系统（监听器、冒泡、捕获、委托）
- ✅ 布局引擎（Block, Flexbox）
- ✅ 渲染（颜色、渐变、阴影、变换、动画）
- ✅ JavaScript 集成（QuickJS, DOM 绑定）
- ✅ HTML 解析（Lexbor）
- ✅ 性能基准

## 添加新测试

1. 在相应目录创建测试文件
2. 更新对应的 `CMakeLists.txt`
3. 使用 `DOMTestBase` 基类获取测试环境
4. 使用 `MockEventListener` 等 mock 对象

示例：

```cpp
#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"

namespace lightui {
namespace test {

class MyFeatureTest : public DOMTestBase {};

TEST_F(MyFeatureTest, BasicTest) {
    auto elem = CreateElement("div");
    EXPECT_NE(elem, nullptr);
}

} // namespace test
} // namespace lightui
```
