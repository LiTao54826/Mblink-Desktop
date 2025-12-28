# LightUI 代码结构规范

本文档定义了 LightUI 项目的代码结构强制规范，所有开发者必须遵循。

## 1. 文件大小指导原则

### 1.1 源文件 (.cpp)

| 行数范围 | 状态 | 要求 |
|---------|------|------|
| < 1000 行 | ✅ 正常 | 无特殊要求 |
| 1000-1500 行 | ⚠️ 关注 | 考虑是否可以拆分 |
| 1500-2000 行 | 🔶 评审 | 需要在代码审查中说明理由 |
| > 2000 行 | 🔴 需文档 | 必须在文件头部或 README 中说明为何保持统一 |

### 1.2 头文件 (.h)

| 行数范围 | 状态 | 要求 |
|---------|------|------|
| < 200 行 | ✅ 正常 | 无特殊要求 |
| 200-300 行 | ⚠️ 关注 | 检查是否有实现代码应移至 .cpp |
| > 300 行 | 🔶 评审 | 需要评审是否应该拆分 |

### 1.3 函数大小

| 行数范围 | 状态 | 要求 |
|---------|------|------|
| < 50 行 | ✅ 理想 | 推荐的函数大小 |
| 50-100 行 | ⚠️ 关注 | 考虑是否可以分解 |
| 100-150 行 | 🔶 评审 | 需要在代码审查中说明 |
| > 150 行 | 🔴 需重构 | 强烈建议分解为更小的函数 |

### 1.4 例外情况

以下情况允许较大的文件：

- **状态机实现**: 包含大量 case 分支的状态机
- **查找表**: 包含大量静态数据的文件
- **紧密耦合的功能**: 拆分会导致过多的跨文件调用

## 2. 目录结构规范

### 2.1 目录文件数量

| 文件数量 | 状态 | 要求 |
|---------|------|------|
| ≤ 10 | ✅ 正常 | 无特殊要求 |
| 11-15 | ⚠️ 关注 | 考虑是否需要子目录 |
| > 15 | 🔴 需重组 | 必须创建子目录进行逻辑分组 |

### 2.2 标准目录结构

```
core/
├── {subsystem}/
│   ├── CMakeLists.txt      # 必须：子系统构建配置
│   ├── README.md           # 推荐：子系统说明文档
│   ├── {module}.h          # 模块头文件
│   ├── {module}.cpp        # 模块实现
│   ├── {subdir}/           # 可选：逻辑分组子目录
│   │   ├── CMakeLists.txt
│   │   └── ...
│   ├── platform/           # 可选：平台特定代码
│   │   ├── win32_{module}.cpp
│   │   ├── linux_{module}.cpp
│   │   └── macos_{module}.cpp
│   └── tests/              # 可选：测试文件
│       └── {module}_test.cpp
```

### 2.3 子系统划分

| 子系统 | 目录 | 职责 |
|-------|------|------|
| DOM | `core/dom/` | DOM 树结构和元素 |
| 渲染 | `core/render/` | 渲染对象和绘制 |
| 布局 | `core/layout/` | 布局算法 |
| 事件 | `core/event/` | 事件循环和分发 |
| 编辑 | `core/editing/` | 文本编辑和选择 |
| 窗口 | `core/window/` | 窗口管理 |
| CSS | `core/css/` | CSS 解析和计算 |

## 3. 命名规范

### 3.1 文件命名

```
# 源文件：snake_case
render_object.cpp
event_loop.cpp
html_input_element.cpp

# 测试文件：{module}_test.cpp
render_object_test.cpp
event_loop_test.cpp
```

### 3.2 类命名

```cpp
// PascalCase，使用描述性名词短语
class RenderObject;
class EventLoop;
class HTMLInputElement;
class MouseEventDispatcher;
```

### 3.3 函数命名

```cpp
// 公共方法：PascalCase
void RenderObject::Paint(SkCanvas* canvas);
void EventLoop::ProcessEvents();

// 私有辅助函数：可以使用 snake_case
void calculate_layout_bounds();
bool is_valid_input();
```

### 3.4 变量命名

```cpp
// 成员变量：snake_case 带下划线后缀
class MyClass {
    int member_variable_;
    std::string name_;
};

// 局部变量：snake_case
int local_variable = 0;
std::string user_name;

// 常量：kPascalCase 或 UPPER_SNAKE_CASE
static const int kMaxBufferSize = 1024;
constexpr int MAX_RETRY_COUNT = 3;
```

### 3.5 目录命名

```
# 全小写，多词用下划线
core/render/
core/event/loop/
core/dom/elements/form/
```

## 4. 头文件规范

### 4.1 Include 顺序

```cpp
// 1. 对应的头文件（如果是 .cpp 文件）
#include "render_object.h"

// 2. 项目内部头文件（按字母顺序）
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/render/style_resolver.h"

// 3. 第三方库头文件
#include <SDL3/SDL.h>
#include "include/core/SkCanvas.h"

// 4. 系统头文件
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
```

### 4.2 前向声明

优先使用前向声明减少编译依赖：

```cpp
// 好：使用前向声明
class Element;
class RenderObject;

class MyClass {
    std::shared_ptr<Element> element_;  // 只需要指针/引用
};

// 避免：不必要的 include
#include "core/dom/element.h"  // 如果只需要指针，不要 include
```

### 4.3 Include Guard

使用 `#pragma once`（现代编译器都支持）：

```cpp
#pragma once

// 或者传统方式
#ifndef LIGHTUI_CORE_RENDER_RENDER_OBJECT_H_
#define LIGHTUI_CORE_RENDER_RENDER_OBJECT_H_
// ...
#endif  // LIGHTUI_CORE_RENDER_RENDER_OBJECT_H_
```

### 4.4 内联函数

头文件中的内联函数应保持简短：

```cpp
// 好：简短的 getter/setter
inline int GetWidth() const { return width_; }
inline void SetWidth(int w) { width_ = w; }

// 避免：复杂的内联函数（应移至 .cpp）
inline void ComplexOperation() {
    // 超过 10 行的实现应该放在 .cpp 文件中
}
```

## 5. 依赖管理规范

### 5.1 依赖方向

```
高层模块 → 低层模块

window → event → dom → core
         ↓
       render → layout
```

### 5.2 禁止循环依赖

```cpp
// 错误：循环依赖
// a.h includes b.h
// b.h includes a.h

// 正确：使用前向声明或接口打破循环
// a.h
class B;  // 前向声明
class A {
    B* b_;  // 使用指针
};
```

### 5.3 依赖数量限制

| Include 数量 | 状态 | 要求 |
|-------------|------|------|
| ≤ 10 | ✅ 正常 | 无特殊要求 |
| 11-15 | ⚠️ 关注 | 考虑是否可以减少依赖 |
| > 15 | 🔶 评审 | 需要评审是否应该拆分模块 |

## 6. 文档规范

### 6.1 文件头注释

```cpp
/**
 * @file render_object.cpp
 * @brief 渲染对象基类实现
 * 
 * 本文件实现了渲染树的基础节点类，包括：
 * - 树结构管理
 * - 布局计算
 * - 绘制接口
 */
```

### 6.2 类注释

```cpp
/**
 * @brief 渲染对象基类
 * 
 * RenderObject 是渲染树的基础节点，负责：
 * - 管理子节点
 * - 存储计算后的样式
 * - 执行布局和绘制
 * 
 * @note 这是一个抽象基类，具体渲染行为由子类实现
 */
class RenderObject {
    // ...
};
```

### 6.3 函数注释

```cpp
/**
 * @brief 执行布局计算
 * 
 * @param parent_width 父容器宽度
 * @param parent_height 父容器高度
 * @return 布局是否发生变化
 * 
 * @note 此方法会递归布局所有子节点
 */
bool Layout(float parent_width, float parent_height);
```

### 6.4 目录 README

每个包含 3 个以上源文件的目录应有 README.md：

```markdown
# core/event/dispatch/

## 概述
事件分发子系统，负责将 SDL 事件转换并分发到 DOM 元素。

## 模块列表
- `mouse_event_dispatcher` - 鼠标事件分发
- `keyboard_event_dispatcher` - 键盘事件分发

## 依赖关系
- 依赖: core/dom, core/render
- 被依赖: core/event/loop

## 使用示例
```cpp
auto dispatcher = std::make_unique<MouseEventDispatcher>(event_loop);
dispatcher->HandleEvent(sdl_event, window);
```
```

## 7. 代码审查检查清单

在提交代码前，请确认：

- [ ] 文件大小是否在合理范围内？
- [ ] 函数大小是否在合理范围内？
- [ ] 是否遵循命名规范？
- [ ] Include 顺序是否正确？
- [ ] 是否使用了前向声明减少依赖？
- [ ] 是否有循环依赖？
- [ ] 公共 API 是否有文档注释？
- [ ] 新目录是否有 README.md？
- [ ] CMakeLists.txt 是否已更新？

## 8. 违规处理

| 违规级别 | 处理方式 |
|---------|---------|
| 建议 (⚠️) | 代码审查中讨论，可以接受但建议改进 |
| 需评审 (🔶) | 必须在 PR 中说明理由，审查者决定是否接受 |
| 需修复 (🔴) | 必须修复后才能合并 |

---

*最后更新: 2024-12*
*版本: 1.0*
