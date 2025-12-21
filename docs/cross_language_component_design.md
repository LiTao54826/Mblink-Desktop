# LightUI 跨语言组件设计

## 设计原则

1. **统一的组件模型** - 所有语言使用相同的概念和生命周期
2. **JSON 作为通信协议** - 跨语言边界使用 JSON 传递数据
3. **声明式 UI** - 组件返回 UI 描述，框架负责渲染
4. **响应式状态** - 状态变化自动触发 UI 更新

## 核心概念

### 1. 虚拟 DOM 节点 (VNode)

所有语言使用统一的 VNode 结构：

```json
{
  "type": "div",                    // 标签名或组件名
  "props": {                        // 属性
    "class": "container",
    "style": {"color": "red"},
    "onClick": "__callback_123__"   // 回调引用
  },
  "children": [                     // 子节点
    {"type": "text", "value": "Hello"},
    {"type": "span", "props": {}, "children": []}
  ]
}
```

### 2. 组件接口

每个组件必须实现：

```
interface Component {
    // 渲染方法：返回 VNode
    render() -> VNode
    
    // 可选生命周期
    on_mount()      // 组件挂载后
    on_unmount()    // 组件卸载前
    on_update()     // 组件更新后
}
```

### 3. 状态管理

```
interface State<T> {
    get() -> T           // 获取当前值
    set(value: T)        // 设置新值（触发重渲染）
    update(fn: T -> T)   // 函数式更新
}
```

## 各语言实现

### Python

```python
import lightui as ui

@ui.component
def Counter():
    count = ui.state(0)
    
    def increment():
        count.set(count.get() + 1)
    
    return ui.h("div", [
        ui.h("span", f"Count: {count.get()}"),
        ui.h("button", {"onClick": increment}, "+1")
    ])

ui.run(Counter, title="Counter")
```

### Rust

```rust
use lightui::prelude::*;

#[component]
fn Counter() -> VNode {
    let count = use_state(0);
    
    let increment = {
        let count = count.clone();
        move |_| count.set(*count.get() + 1)
    };
    
    h!("div", [
        h!("span", format!("Count: {}", count.get())),
        h!("button", on_click = increment, "+1")
    ])
}

fn main() {
    lightui::run(Counter, "Counter", 400, 300);
}
```

### Go

```go
package main

import ui "github.com/user/lightui"

func Counter() ui.VNode {
    count := ui.UseState(0)
    
    increment := func() {
        count.Set(count.Get().(int) + 1)
    }
    
    return ui.H("div", nil,
        ui.H("span", nil, fmt.Sprintf("Count: %d", count.Get())),
        ui.H("button", ui.Props{"onClick": increment}, "+1"),
    )
}

func main() {
    ui.Run(Counter, "Counter", 400, 300)
}
```

### Node.js

```javascript
const ui = require('lightui');

function Counter() {
    const [count, setCount] = ui.useState(0);
    
    return ui.h('div', [
        ui.h('span', `Count: ${count}`),
        ui.h('button', { onClick: () => setCount(count + 1) }, '+1')
    ]);
}

ui.run(Counter, { title: 'Counter', width: 400, height: 300 });
```

### C++

```cpp
#include <lightui/lightui.hpp>

using namespace lightui;

VNode Counter() {
    auto [count, setCount] = useState(0);
    
    return h("div", {
        h("span", "Count: " + std::to_string(count)),
        h("button", {{"onClick", [=]{ setCount(count + 1); }}}, "+1")
    });
}

int main() {
    run(Counter, "Counter", 400, 300);
}
```

## 通用 API 规范

### 1. 创建元素

```
h(type, props?, ...children) -> VNode
h(type, children) -> VNode  // props 可省略
```

| 参数 | 类型 | 说明 |
|------|------|------|
| type | string \| Component | 标签名或组件 |
| props | object | 属性对象 |
| children | VNode[] \| string | 子节点 |

### 2. 状态 Hook

```
useState(initialValue) -> [value, setValue]
// 或
state(initialValue) -> State<T>
```

### 3. 副作用 Hook

```
useEffect(callback, dependencies?)
// callback 可返回清理函数
```

### 4. 运行应用

```
run(RootComponent, options)

options = {
    title: string,
    width: number,
    height: number,
    resizable?: boolean,
    icon?: string
}
```

## C 核心 API

所有语言绑定都调用同一套 C API：

```c
// ========== 应用生命周期 ==========
int lightui_init();
void lightui_cleanup();
void* lightui_create_window(const char* title, int width, int height);
void lightui_destroy_window(void* window);
void lightui_run(void* window);

// ========== 虚拟 DOM ==========
// 接收 JSON 格式的 VNode 树
int lightui_render(void* window, const char* vnode_json);

// ========== 回调注册 ==========
typedef char* (*LightUICallback)(const char* args_json, void* user_data);
int lightui_register_callback(void* window, const char* id, LightUICallback cb, void* user_data);
void lightui_unregister_callback(void* window, const char* id);

// ========== 状态通知 ==========
// 当宿主语言状态变化时调用，触发重渲染
void lightui_state_changed(void* window);

// ========== 事件分发 ==========
// 框架调用此函数通知宿主语言
typedef void (*LightUIEventHandler)(const char* event_json, void* user_data);
void lightui_set_event_handler(void* window, LightUIEventHandler handler, void* user_data);
```

## 数据流

```
┌─────────────────────────────────────────────────────────────┐
│                     Host Language                            │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                    Component                         │    │
│  │  ┌─────────┐    ┌─────────┐    ┌─────────────┐     │    │
│  │  │  State  │───▶│ render()│───▶│   VNode     │     │    │
│  │  └────▲────┘    └─────────┘    └──────┬──────┘     │    │
│  │       │                               │             │    │
│  │       │ set()                         │ JSON        │    │
│  │       │                               ▼             │    │
│  └───────┼───────────────────────────────┼─────────────┘    │
│          │                               │                   │
│          │ Event                         │ lightui_render()  │
│          │                               │                   │
│  ┌───────┴───────────────────────────────▼─────────────┐    │
│  │                  LightUI Core (C++)                  │    │
│  │  ┌─────────┐    ┌─────────┐    ┌─────────────┐     │    │
│  │  │  Event  │───▶│  Diff   │───▶│   Render    │     │    │
│  │  │ Handler │    │ Engine  │    │   (Skia)    │     │    │
│  │  └─────────┘    └─────────┘    └─────────────┘     │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## 完整示例：Todo App

### Python

```python
import lightui as ui
from dataclasses import dataclass
from typing import List

@dataclass
class Todo:
    id: int
    text: str
    done: bool = False

@ui.component
def TodoItem(todo: Todo, on_toggle, on_delete):
    style = {"textDecoration": "line-through"} if todo.done else {}
    
    return ui.h("li", {"class": "todo-item"}, [
        ui.h("input", {
            "type": "checkbox",
            "checked": todo.done,
            "onChange": lambda: on_toggle(todo.id)
        }),
        ui.h("span", {"style": style}, todo.text),
        ui.h("button", {"onClick": lambda: on_delete(todo.id)}, "×")
    ])

@ui.component
def TodoApp():
    todos = ui.state([])
    input_text = ui.state("")
    next_id = ui.state(1)
    
    def add_todo():
        if input_text.get().strip():
            todos.set([*todos.get(), Todo(next_id.get(), input_text.get())])
            next_id.set(next_id.get() + 1)
            input_text.set("")
    
    def toggle_todo(id: int):
        todos.set([
            Todo(t.id, t.text, not t.done) if t.id == id else t
            for t in todos.get()
        ])
    
    def delete_todo(id: int):
        todos.set([t for t in todos.get() if t.id != id])
    
    return ui.h("div", {"class": "todo-app"}, [
        ui.h("h1", "Todo List"),
        ui.h("div", {"class": "input-row"}, [
            ui.h("input", {
                "value": input_text.get(),
                "onInput": lambda e: input_text.set(e.target.value),
                "placeholder": "What needs to be done?"
            }),
            ui.h("button", {"onClick": add_todo}, "Add")
        ]),
        ui.h("ul", {"class": "todo-list"}, [
            ui.h(TodoItem, {
                "key": t.id,
                "todo": t,
                "on_toggle": toggle_todo,
                "on_delete": delete_todo
            })
            for t in todos.get()
        ]),
        ui.h("p", f"Total: {len(todos.get())} | Done: {sum(1 for t in todos.get() if t.done)}")
    ])

if __name__ == "__main__":
    ui.run(TodoApp, title="Todo App", width=500, height=600)
```

### Rust

```rust
use lightui::prelude::*;

#[derive(Clone)]
struct Todo {
    id: u32,
    text: String,
    done: bool,
}

#[component]
fn TodoItem(todo: Todo, on_toggle: impl Fn(u32), on_delete: impl Fn(u32)) -> VNode {
    let style = if todo.done { 
        Some(("textDecoration", "line-through")) 
    } else { 
        None 
    };
    
    h!("li", class = "todo-item", [
        h!("input", 
            type = "checkbox", 
            checked = todo.done,
            on_change = move |_| on_toggle(todo.id)
        ),
        h!("span", style = style, &todo.text),
        h!("button", on_click = move |_| on_delete(todo.id), "×")
    ])
}

#[component]
fn TodoApp() -> VNode {
    let todos = use_state(Vec::<Todo>::new());
    let input_text = use_state(String::new());
    let next_id = use_state(1u32);
    
    let add_todo = {
        let todos = todos.clone();
        let input_text = input_text.clone();
        let next_id = next_id.clone();
        move |_| {
            if !input_text.get().trim().is_empty() {
                let mut new_todos = todos.get().clone();
                new_todos.push(Todo {
                    id: *next_id.get(),
                    text: input_text.get().clone(),
                    done: false,
                });
                todos.set(new_todos);
                next_id.set(*next_id.get() + 1);
                input_text.set(String::new());
            }
        }
    };
    
    h!("div", class = "todo-app", [
        h!("h1", "Todo List"),
        h!("div", class = "input-row", [
            h!("input",
                value = input_text.get(),
                on_input = move |e| input_text.set(e.value()),
                placeholder = "What needs to be done?"
            ),
            h!("button", on_click = add_todo, "Add")
        ]),
        h!("ul", class = "todo-list", 
            todos.get().iter().map(|t| {
                h!(TodoItem, key = t.id, todo = t.clone(), 
                   on_toggle = toggle_todo, on_delete = delete_todo)
            })
        ),
        h!("p", format!("Total: {} | Done: {}", 
            todos.get().len(),
            todos.get().iter().filter(|t| t.done).count()
        ))
    ])
}

fn main() {
    lightui::run(TodoApp, "Todo App", 500, 600);
}
```

## 实现路线图

### Phase 1: 核心 C API
- [ ] VNode JSON 解析器
- [ ] Diff 算法
- [ ] 回调注册机制
- [ ] 事件分发

### Phase 2: Python 绑定
- [ ] pybind11 封装
- [ ] State 类实现
- [ ] h() 函数
- [ ] @component 装饰器

### Phase 3: 其他语言
- [ ] Rust (通过 FFI)
- [ ] Go (通过 cgo)
- [ ] Node.js (通过 N-API)

### Phase 4: 高级功能
- [ ] 组件缓存
- [ ] 异步渲染
- [ ] 开发者工具
- [ ] 热重载
