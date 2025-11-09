# LightUI 编码规范

## 1. C++ 编码规范

### 1.1 命名约定

#### 类名
- 使用 PascalCase（大驼峰）
- 清晰描述类的职责

```cpp
// ✅ 好的命名
class QuickJSRuntime { };
class DOMElement { };
class EventSystem { };

// ❌ 不好的命名
class runtime { };
class elem { };
class ES { };
```

#### 函数名
- 使用 PascalCase（大驼峰）
- 动词开头，描述动作

```cpp
// ✅ 好的命名
void CreateWindow();
Element* GetElementById(const std::string& id);
void SetAttribute(const std::string& name, const std::string& value);

// ❌ 不好的命名
void window();
Element* element(const std::string& id);
void attr(const std::string& n, const std::string& v);
```

#### 变量名
- 使用 camelCase（小驼峰）
- 成员变量以下划线结尾

```cpp
class Window {
public:
    void SetTitle(const std::string& title) {
        // 局部变量：camelCase
        std::string formattedTitle = FormatTitle(title);
        title_ = formattedTitle;  // 成员变量：下划线结尾
    }
    
private:
    std::string title_;
    int width_;
    int height_;
    SDL_Window* sdlWindow_;  // 指针也用下划线
};
```

#### 常量
- 使用 kPascalCase（k前缀 + 大驼峰）

```cpp
// ✅ 好的命名
const int kDefaultWidth = 800;
const int kDefaultHeight = 600;
const char* kDefaultTitle = "LightUI App";

// ❌ 不好的命名
const int DEFAULT_WIDTH = 800;  // 不要用全大写
const int default_width = 800;  // 不要用下划线
```

#### 枚举
- 枚举类型使用 PascalCase
- 枚举值使用 UPPER_CASE

```cpp
// ✅ 好的命名
enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    COMMENT_NODE = 8,
    DOCUMENT_NODE = 9
};

// 使用
NodeType type = NodeType::ELEMENT_NODE;
```

#### 命名空间
- 使用小写，单词用下划线分隔

```cpp
namespace lightui {
namespace dom {

class Element { };

}  // namespace dom
}  // namespace lightui
```

---

### 1.2 文件组织

#### 头文件
- 使用 `.h` 扩展名
- 包含保护使用 `#pragma once`

```cpp
// dom/element.h
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace lightui {
namespace dom {

class Element {
public:
    Element(const std::string& tagName);
    ~Element();
    
    // ... 声明
    
private:
    std::string tagName_;
};

}  // namespace dom
}  // namespace lightui
```

#### 实现文件
- 使用 `.cpp` 扩展名
- 首先包含对应的头文件

```cpp
// dom/element.cpp
#include "dom/element.h"

#include <algorithm>
#include "dom/node.h"
#include "dom/document.h"

namespace lightui {
namespace dom {

Element::Element(const std::string& tagName)
    : tagName_(tagName) {
    // 实现
}

Element::~Element() {
    // 清理
}

}  // namespace dom
}  // namespace lightui
```

#### 包含顺序
1. 对应的头文件
2. C系统头文件
3. C++标准库头文件
4. 第三方库头文件
5. 项目内部头文件

```cpp
#include "dom/element.h"  // 1. 对应头文件

#include <stdio.h>         // 2. C系统头文件

#include <string>          // 3. C++标准库
#include <vector>
#include <memory>

#include <SDL3/SDL.h>      // 4. 第三方库
#include <quickjs.h>

#include "dom/node.h"      // 5. 项目内部
#include "dom/document.h"
```

---

### 1.3 代码格式

#### 缩进
- 使用 4 个空格，不使用 Tab

```cpp
class Window {
public:
    void Show() {
        if (visible_) {
            return;
        }
        visible_ = true;
    }
};
```

#### 大括号
- 左大括号不换行（K&R风格）
- 右大括号单独一行

```cpp
// ✅ 正确
if (condition) {
    DoSomething();
} else {
    DoOtherThing();
}

// ❌ 错误
if (condition)
{
    DoSomething();
}
```

#### 行长度
- 每行不超过 100 字符
- 超过时合理换行

```cpp
// ✅ 好的换行
void LongFunctionName(
    const std::string& parameter1,
    const std::string& parameter2,
    int parameter3) {
    // ...
}

// 链式调用换行
element->SetAttribute("class", "container")
       ->SetAttribute("id", "main")
       ->AppendChild(child);
```

#### 空格
- 运算符两边加空格
- 逗号后加空格
- 关键字后加空格

```cpp
// ✅ 正确
int result = a + b * c;
Function(arg1, arg2, arg3);
if (condition) { }
for (int i = 0; i < n; i++) { }

// ❌ 错误
int result=a+b*c;
Function(arg1,arg2,arg3);
if(condition){ }
```

---

### 1.4 注释

#### 文件注释
```cpp
// lightui/dom/element.h
//
// DOM Element类的实现
// 提供元素节点的基本操作和属性管理
//
// Author: [Your Name]
// Date: 2025-01-01
```

#### 类注释
```cpp
// Element类表示DOM树中的元素节点
// 
// 示例:
//   Element* div = document->CreateElement("div");
//   div->SetAttribute("class", "container");
//   div->AppendChild(child);
class Element : public Node {
    // ...
};
```

#### 函数注释
```cpp
// 设置元素的属性
//
// @param name 属性名称
// @param value 属性值
// 
// 示例:
//   element->SetAttribute("id", "main");
void SetAttribute(const std::string& name, const std::string& value);
```

#### 行内注释
```cpp
// 计算布局（使用Yoga引擎）
void Layout() {
    // TODO: 优化性能
    // FIXME: 处理边界情况
    // NOTE: 这里需要特别注意内存管理
}
```

---

### 1.5 内存管理

#### 使用智能指针
```cpp
// ✅ 推荐：使用智能指针
class Window {
private:
    std::unique_ptr<Document> document_;
    std::shared_ptr<Renderer> renderer_;
};

// ❌ 避免：裸指针（除非必要）
class Window {
private:
    Document* document_;  // 需要手动delete
};
```

#### RAII原则
```cpp
// ✅ 好的设计
class Window {
public:
    Window() {
        sdlWindow_ = SDL_CreateWindow(...);
    }
    
    ~Window() {
        if (sdlWindow_) {
            SDL_DestroyWindow(sdlWindow_);
        }
    }
    
    // 禁止拷贝
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
private:
    SDL_Window* sdlWindow_;
};
```

---

### 1.6 错误处理

#### 使用异常处理关键错误
```cpp
class QuickJSRuntime {
public:
    JSValue Eval(const std::string& code) {
        JSValue result = JS_Eval(ctx_, code.c_str(), code.size(), 
                                 "<eval>", JS_EVAL_TYPE_GLOBAL);
        
        if (JS_IsException(result)) {
            std::string error = GetLastError();
            throw JSException(error);
        }
        
        return result;
    }
};
```

#### 返回错误码处理预期错误
```cpp
// 返回bool表示成功/失败
bool LoadUIFile(const std::string& filepath) {
    if (!FileExists(filepath)) {
        return false;
    }
    // ...
    return true;
}

// 使用std::optional表示可能失败
std::optional<Element*> GetElementById(const std::string& id) {
    auto it = idMap_.find(id);
    if (it == idMap_.end()) {
        return std::nullopt;
    }
    return it->second;
}
```

---

### 1.7 性能考虑

#### 避免不必要的拷贝
```cpp
// ✅ 使用const引用
void SetTitle(const std::string& title) {
    title_ = title;
}

// ❌ 值传递（会拷贝）
void SetTitle(std::string title) {
    title_ = title;
}
```

#### 使用移动语义
```cpp
// ✅ 支持移动
void SetChildren(std::vector<Element*>&& children) {
    children_ = std::move(children);
}
```

#### 预分配容器大小
```cpp
// ✅ 预分配
std::vector<Element*> elements;
elements.reserve(100);  // 避免多次重新分配

// 循环中添加元素
for (int i = 0; i < 100; i++) {
    elements.push_back(CreateElement());
}
```

---

## 2. JavaScript 编码规范

### 2.1 命名约定

```javascript
// 变量和函数：camelCase
const userName = "Alice";
function getUserData() { }

// 类：PascalCase
class UserManager { }

// 常量：UPPER_CASE
const MAX_RETRY_COUNT = 3;
const API_BASE_URL = "https://api.example.com";

// 私有成员：下划线前缀
class MyClass {
    _privateMethod() { }
}
```

### 2.2 代码格式

```javascript
// 使用2个空格缩进
function example() {
  if (condition) {
    doSomething();
  }
}

// 使用单引号
const message = 'Hello World';

// 对象和数组
const user = {
  name: 'Alice',
  age: 30
};

const items = [1, 2, 3];
```

### 2.3 现代JavaScript特性

```javascript
// ✅ 使用const/let，不用var
const name = 'Alice';
let count = 0;

// ✅ 使用箭头函数
const add = (a, b) => a + b;

// ✅ 使用解构
const { name, age } = user;
const [first, second] = array;

// ✅ 使用模板字符串
const message = `Hello, ${name}!`;

// ✅ 使用async/await
async function fetchData() {
  const response = await fetch(url);
  return response.json();
}
```

---

## 3. Python 编码规范

### 3.1 遵循PEP 8

```python
# 类名：PascalCase
class WindowManager:
    pass

# 函数和变量：snake_case
def get_user_data():
    user_name = "Alice"
    return user_name

# 常量：UPPER_CASE
MAX_RETRY_COUNT = 3
API_BASE_URL = "https://api.example.com"

# 私有成员：下划线前缀
class MyClass:
    def __init__(self):
        self._private_var = 0
    
    def _private_method(self):
        pass
```

### 3.2 类型注解

```python
from typing import List, Dict, Optional

def get_users(limit: int = 10) -> List[Dict[str, str]]:
    """获取用户列表"""
    return []

def find_user(user_id: int) -> Optional[Dict[str, str]]:
    """查找用户，可能返回None"""
    return None
```

---

## 4. 文档规范

### 4.1 README.md
- 项目简介
- 快速开始
- 安装说明
- 使用示例
- API文档链接

### 4.2 API文档
- 使用Doxygen（C++）
- 使用JSDoc（JavaScript）
- 使用Sphinx（Python）

### 4.3 注释要求
- 所有公开API必须有文档注释
- 复杂逻辑必须有解释注释
- TODO/FIXME必须有负责人和日期

---

## 5. Git 提交规范

### 5.1 提交信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 5.2 Type类型

- `feat`: 新功能
- `fix`: 修复bug
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

### 5.3 示例

```
feat(dom): 实现Element.querySelector方法

添加了querySelector和querySelectorAll方法，
支持基础的CSS选择器语法。

Closes #123
```

---

## 6. 测试规范

### 6.1 单元测试

```cpp
// 使用Google Test
TEST(ElementTest, SetAttribute) {
    Element element("div");
    element.SetAttribute("id", "main");
    
    EXPECT_EQ(element.GetAttribute("id"), "main");
}
```

### 6.2 测试覆盖率
- 核心模块：> 80%
- 工具函数：> 90%
- 边界情况必须测试

---

## 7. 代码审查清单

### 7.1 功能性
- [ ] 代码实现了需求
- [ ] 边界情况已处理
- [ ] 错误处理完善

### 7.2 可读性
- [ ] 命名清晰
- [ ] 注释充分
- [ ] 逻辑清晰

### 7.3 性能
- [ ] 无明显性能问题
- [ ] 内存管理正确
- [ ] 无资源泄漏

### 7.4 安全性
- [ ] 输入验证
- [ ] 无安全漏洞
- [ ] 权限检查

### 7.5 测试
- [ ] 有单元测试
- [ ] 测试覆盖充分
- [ ] 测试通过

---

## 8. 工具配置

### 8.1 clang-format (C++)

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
```

### 8.2 ESLint (JavaScript)

```json
{
  "extends": "eslint:recommended",
  "rules": {
    "indent": ["error", 2],
    "quotes": ["error", "single"],
    "semi": ["error", "always"]
  }
}
```

### 8.3 Black (Python)

```toml
# pyproject.toml
[tool.black]
line-length = 100
target-version = ['py38']
```

---

## 9. 持续集成

### 9.1 自动检查
- 代码格式检查
- 静态分析
- 单元测试
- 代码覆盖率

### 9.2 构建检查
- 多平台编译
- 依赖检查
- 性能测试

---

## 10. 参考资源

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Airbnb JavaScript Style Guide](https://github.com/airbnb/javascript)
- [PEP 8 -- Style Guide for Python Code](https://www.python.org/dev/peps/pep-0008/)

