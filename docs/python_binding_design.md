# LightUI Python 绑定设计方案

## 设计目标

1. **Pythonic** - 符合 Python 习惯，简洁优雅
2. **声明式 UI** - 类似 React/Vue 的组件化开发
3. **双向绑定** - Python 数据自动同步到 UI
4. **类型安全** - 完整的类型提示支持
5. **热重载** - 开发时支持代码热更新

## API 设计

### 方案一：装饰器 + JSX 字符串（简单直接）

```python
import lightui as ui

app = ui.App("My App", 800, 600)

# 绑定 Python 函数供 JS 调用
@app.bind
def get_users():
    return [{"name": "Alice"}, {"name": "Bob"}]

@app.bind
def add_user(name: str):
    print(f"Adding user: {name}")
    return {"success": True}

# 加载 Preact UI
app.load("""
const { h, render } = preact;
const { useState, useEffect } = preactHooks;

function App() {
    const [users, setUsers] = useState([]);
    
    useEffect(() => {
        setUsers(py.get_users());
    }, []);
    
    return h('div', null, [
        h('h1', null, 'User List'),
        h('ul', null, users.map(u => h('li', null, u.name))),
        h('button', { onClick: () => py.add_user('New User') }, 'Add')
    ]);
}

render(h(App), document.body);
""")

app.run()
```

### 方案二：Python 组件（推荐）

```python
import lightui as ui
from lightui import Component, State, html

class UserList(Component):
    users: State[list] = []
    
    def on_mount(self):
        self.users = self.fetch_users()
    
    def fetch_users(self):
        return [{"name": "Alice"}, {"name": "Bob"}]
    
    def add_user(self, name: str):
        self.users = [*self.users, {"name": name}]
    
    def render(self):
        return html("""
            <div class="container">
                <h1>User List ({len(self.users)})</h1>
                <ul>
                    {[html('<li>{u["name"]}</li>') for u in self.users]}
                </ul>
                <button @click="self.add_user('New')">Add User</button>
            </div>
        """)

app = ui.App("My App", 800, 600)
app.mount(UserList())
app.run()
```

### 方案三：函数式组件 + Hooks（最 Pythonic）

```python
import lightui as ui
from lightui import use_state, use_effect, h

@ui.component
def UserList():
    users, set_users = use_state([])
    loading, set_loading = use_state(True)
    
    @use_effect([])
    def fetch_data():
        set_users([{"name": "Alice"}, {"name": "Bob"}])
        set_loading(False)
    
    def add_user():
        set_users([*users, {"name": f"User {len(users) + 1}"}])
    
    if loading:
        return h("div", "Loading...")
    
    return h("div", {"class": "container"}, [
        h("h1", f"Users ({len(users)})"),
        h("ul", [h("li", {"key": i}, u["name"]) for i, u in enumerate(users)]),
        h("button", {"onClick": add_user}, "Add User")
    ])

@ui.component  
def App():
    return h("div", [
        h("header", h("h1", "My App")),
        h(UserList),
    ])

ui.run(App, title="My App", width=800, height=600)
```

### 方案四：模板语法（类 Vue）

```python
import lightui as ui

@ui.component
class Counter:
    count: int = 0
    
    def increment(self):
        self.count += 1
    
    template = """
    <div class="counter">
        <span>Count: {{ count }}</span>
        <button @click="increment">+1</button>
    </div>
    """

@ui.component
class App:
    title: str = "My App"
    
    template = """
    <div>
        <h1>{{ title }}</h1>
        <Counter />
        <Counter />
    </div>
    """

ui.run(App, width=800, height=600)
```

## 推荐方案：混合模式

结合方案一和方案三的优点：

```python
import lightui as ui
from lightui import h, use_state, use_effect
from dataclasses import dataclass
from typing import List

# 数据模型
@dataclass
class User:
    id: int
    name: str
    email: str

# 函数式组件
@ui.component
def UserCard(user: User, on_delete):
    return h("div", {"class": "card"}, [
        h("h3", user.name),
        h("p", user.email),
        h("button", {"onClick": lambda: on_delete(user.id)}, "Delete")
    ])

@ui.component
def UserList():
    users, set_users = use_state([
        User(1, "Alice", "alice@example.com"),
        User(2, "Bob", "bob@example.com"),
    ])
    
    def delete_user(user_id: int):
        set_users([u for u in users if u.id != user_id])
    
    def add_user():
        new_id = max(u.id for u in users) + 1 if users else 1
        set_users([*users, User(new_id, f"User {new_id}", f"user{new_id}@example.com")])
    
    return h("div", {"class": "user-list"}, [
        h("h2", f"Users ({len(users)})"),
        h("div", {"class": "cards"}, [
            h(UserCard, {"key": u.id, "user": u, "on_delete": delete_user})
            for u in users
        ]),
        h("button", {"onClick": add_user, "class": "btn-primary"}, "Add User")
    ])

@ui.component
def App():
    theme, set_theme = use_state("light")
    
    return h("div", {"class": f"app theme-{theme}"}, [
        h("header", [
            h("h1", "User Management"),
            h("button", {"onClick": lambda: set_theme("dark" if theme == "light" else "light")},
              f"Switch to {'Dark' if theme == 'light' else 'Light'}")
        ]),
        h("main", h(UserList)),
    ])

# 运行应用
if __name__ == "__main__":
    ui.run(App, title="User Management", width=1024, height=768)
```

## 核心 API 设计

### 1. 应用入口

```python
# 简单方式
ui.run(App, title="My App", width=800, height=600)

# 完整配置
app = ui.Application(
    title="My App",
    width=800,
    height=600,
    resizable=True,
    icon="icon.png",
    theme="light",
)
app.mount(App)
app.run()
```

### 2. 组件定义

```python
# 函数式组件
@ui.component
def MyComponent(props):
    return h("div", props.children)

# 类组件
@ui.component
class MyComponent:
    def render(self):
        return h("div", self.props.children)
```

### 3. 状态管理

```python
# 局部状态
count, set_count = use_state(0)

# 全局状态（类似 Redux）
store = ui.create_store({
    "user": None,
    "theme": "light",
})

@ui.component
def App():
    user = ui.use_store("user")
    
    def login(username):
        store.dispatch("SET_USER", {"name": username})
    
    return h("div", user.name if user else "Not logged in")
```

### 4. 副作用

```python
# 组件挂载时执行
@use_effect([])
def on_mount():
    print("Component mounted")
    return lambda: print("Component unmounted")  # cleanup

# 依赖变化时执行
@use_effect([count])
def on_count_change():
    print(f"Count changed to {count}")
```

### 5. 样式

```python
# 内联样式
h("div", {"style": {"color": "red", "fontSize": 16}})

# CSS 类
h("div", {"class": "container primary"})

# CSS-in-Python
styles = ui.css({
    ".container": {
        "padding": "20px",
        "background": "#f5f5f5",
    },
    ".btn": {
        "padding": "8px 16px",
        "&:hover": {
            "background": "#e0e0e0",
        }
    }
})

@ui.component
def App():
    return h("div", {"class": styles.container}, [
        h("button", {"class": styles.btn}, "Click me")
    ])
```

### 6. 事件处理

```python
# 基本事件
h("button", {"onClick": lambda e: print("clicked")})

# 带参数
h("button", {"onClick": lambda: handle_click(item.id)})

# 表单
h("input", {
    "value": text,
    "onInput": lambda e: set_text(e.target.value)
})
```

### 7. 条件渲染和列表

```python
# 条件渲染
h("div", [
    h("p", "Logged in") if user else h("p", "Please login"),
    user and h("button", "Logout"),  # 短路求值
])

# 列表渲染
h("ul", [
    h("li", {"key": item.id}, item.name)
    for item in items
])
```

### 8. 与 Python 后端集成

```python
import lightui as ui
from lightui import h, use_state, use_effect
import asyncio
import aiohttp

@ui.component
def DataFetcher():
    data, set_data = use_state(None)
    loading, set_loading = use_state(True)
    error, set_error = use_state(None)
    
    @use_effect([])
    async def fetch():
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get("https://api.example.com/data") as resp:
                    set_data(await resp.json())
        except Exception as e:
            set_error(str(e))
        finally:
            set_loading(False)
    
    if loading:
        return h("div", "Loading...")
    if error:
        return h("div", {"class": "error"}, f"Error: {error}")
    
    return h("div", [
        h("pre", json.dumps(data, indent=2))
    ])
```

## 实现架构

```
┌─────────────────────────────────────────────────────────┐
│                    Python Application                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │              lightui Python Package              │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │    │
│  │  │Component│ │  Hooks  │ │  State Manager  │   │    │
│  │  └────┬────┘ └────┬────┘ └────────┬────────┘   │    │
│  │       │           │               │            │    │
│  │  ┌────▼───────────▼───────────────▼────────┐   │    │
│  │  │           Virtual DOM (Python)           │   │    │
│  │  └────────────────────┬────────────────────┘   │    │
│  └───────────────────────┼────────────────────────┘    │
│                          │ pybind11                     │
│  ┌───────────────────────▼────────────────────────┐    │
│  │              LightUI C++ Core                   │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │    │
│  │  │ Window  │ │Document │ │   QuickJS       │   │    │
│  │  └────┬────┘ └────┬────┘ └────────┬────────┘   │    │
│  │       │           │               │            │    │
│  │  ┌────▼───────────▼───────────────▼────────┐   │    │
│  │  │         Render Pipeline (Skia)           │   │    │
│  │  └─────────────────────────────────────────┘   │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

## 文件结构

```
bindings/python/
├── lightui/
│   ├── __init__.py          # 主入口
│   ├── _core.pyd             # pybind11 编译的扩展模块
│   ├── app.py               # Application 类
│   ├── component.py         # 组件基类和装饰器
│   ├── hooks.py             # use_state, use_effect 等
│   ├── vdom.py              # 虚拟 DOM 实现
│   ├── reconciler.py        # Diff 算法
│   ├── store.py             # 全局状态管理
│   ├── css.py               # CSS-in-Python
│   └── types.py             # 类型定义
├── examples/
│   ├── hello_world.py
│   ├── counter.py
│   ├── todo_app.py
│   └── data_dashboard.py
├── tests/
│   └── ...
├── setup.py
└── pyproject.toml
```

## 实现优先级

### Phase 1: 基础功能
1. pybind11 绑定核心类 (Window, Document)
2. 基本的 `h()` 函数和 VDOM
3. `use_state` hook
4. 简单的事件处理

### Phase 2: 完整组件系统
1. `@ui.component` 装饰器
2. `use_effect` hook
3. 组件生命周期
4. Props 传递

### Phase 3: 高级功能
1. 全局状态管理
2. CSS-in-Python
3. 异步支持
4. 热重载

### Phase 4: 开发体验
1. 开发者工具
2. 错误边界
3. 性能优化
4. 文档和示例
