# LightUI Python API 文档

## 1. 安装

### 1.1 使用pip安装

```bash
pip install lightui
```

### 1.2 从源码安装

```bash
git clone https://github.com/yourusername/lightui.git
cd lightui/bindings/python
pip install -e .
```

---

## 2. 快速开始

### 2.1 Hello World

```python
import lightui

# 初始化
lightui.init()

# 创建窗口
window = lightui.Window("Hello World", 400, 300)

# 加载UI
window.load_ui("""
import { render } from 'preact';

function App() {
    return <h1>Hello, LightUI!</h1>;
}

render(<App />, document.body);
""")

# 运行
window.run()

# 清理
lightui.cleanup()
```

### 2.2 使用上下文管理器

```python
import lightui

with lightui.App() as app:
    window = app.create_window("Hello World", 400, 300)
    window.load_ui("""
        import { render } from 'preact';
        render(<h1>Hello!</h1>, document.body);
    """)
    window.run()
```

---

## 3. 核心API

### 3.1 初始化和清理

```python
import lightui

# 初始化LightUI
lightui.init()

# 获取版本
version = lightui.get_version()
print(f"LightUI version: {version}")  # (1, 0, 0)

# 清理
lightui.cleanup()
```

### 3.2 Window类

#### 创建窗口

```python
# 基础创建
window = lightui.Window(title="My App", width=800, height=600)

# 高级配置
window = lightui.Window(
    title="My App",
    width=800,
    height=600,
    x=100,           # 窗口X位置
    y=100,           # 窗口Y位置
    resizable=True,  # 可调整大小
    fullscreen=False,
    borderless=False
)
```

#### 窗口操作

```python
# 显示/隐藏
window.show()
window.hide()

# 设置标题
window.set_title("New Title")

# 设置大小
window.set_size(1024, 768)

# 获取大小
width, height = window.get_size()

# 关闭窗口
window.close()
```

#### 加载UI

```python
# 从字符串加载
window.load_ui("""
import { render } from 'preact';
render(<h1>Hello</h1>, document.body);
""")

# 从文件加载
window.load_ui_file("ui/app.jsx")

# 重新加载（热重载）
window.reload()
```

#### 运行事件循环

```python
# 阻塞运行
window.run()

# 非阻塞运行（自定义事件循环）
while window.is_running():
    window.poll_events()
    # 做其他事情...
    time.sleep(0.016)  # ~60fps
```

---

## 4. 函数绑定

### 4.1 基础绑定

```python
import lightui

window = lightui.Window("App", 800, 600)

# 定义Python函数
def get_data():
    return {
        "users": [
            {"id": 1, "name": "Alice"},
            {"id": 2, "name": "Bob"}
        ]
    }

# 绑定函数（让JavaScript可以调用）
window.bind("getData", get_data)

# JavaScript中调用
window.load_ui("""
import { render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

function App() {
    const [users, setUsers] = useState([]);
    
    useEffect(() => {
        // 调用Python函数
        const data = window.getData();
        setUsers(data.users);
    }, []);
    
    return (
        <ul>
            {users.map(user => <li key={user.id}>{user.name}</li>)}
        </ul>
    );
}

render(<App />, document.body);
""")

window.run()
```

### 4.2 使用装饰器

```python
import lightui

window = lightui.Window("App", 800, 600)

# 使用装饰器绑定
@window.bind("getData")
def get_data():
    return {"message": "Hello from Python!"}

@window.bind("saveData")
def save_data(data):
    print(f"Saving: {data}")
    return {"success": True}

window.load_ui_file("ui/app.jsx")
window.run()
```

### 4.3 带参数的函数

```python
@window.bind("addUser")
def add_user(name, age):
    """添加用户"""
    user_id = db.insert_user(name, age)
    return {"id": user_id, "name": name, "age": age}

# JavaScript中调用
# const user = window.addUser("Alice", 30);
```

### 4.4 异步函数（未来支持）

```python
import asyncio

@window.bind("fetchData")
async def fetch_data(url):
    """异步获取数据"""
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            return await response.json()
```

---

## 5. JavaScript调用

### 5.1 调用JavaScript函数

```python
# Python调用JavaScript
result = window.call_js("updateData", [1, 2, 3])
print(result)

# 执行JavaScript代码
result = window.eval_js("2 + 2")
print(result)  # 4
```

### 5.2 双向通信示例

```python
import lightui
import time
import threading

window = lightui.Window("Real-time Monitor", 800, 600)

# Python -> JavaScript
def update_ui():
    while window.is_running():
        data = get_system_stats()
        window.call_js("updateStats", data)
        time.sleep(1)

# JavaScript -> Python
@window.bind("getHistory")
def get_history():
    return load_history_data()

# 启动更新线程
threading.Thread(target=update_ui, daemon=True).start()

window.load_ui_file("ui/monitor.jsx")
window.run()
```

---

## 6. 完整示例

### 6.1 Todo应用

```python
# todo_app.py
import lightui
import json
from pathlib import Path

class TodoApp:
    def __init__(self):
        self.todos = self.load_todos()
        self.window = lightui.Window("Todo App", 600, 800)
        
        # 绑定函数
        self.window.bind("getTodos", self.get_todos)
        self.window.bind("addTodo", self.add_todo)
        self.window.bind("toggleTodo", self.toggle_todo)
        self.window.bind("deleteTodo", self.delete_todo)
    
    def load_todos(self):
        """从文件加载todos"""
        path = Path("todos.json")
        if path.exists():
            return json.loads(path.read_text())
        return []
    
    def save_todos(self):
        """保存todos到文件"""
        Path("todos.json").write_text(json.dumps(self.todos))
    
    def get_todos(self):
        """获取所有todos"""
        return self.todos
    
    def add_todo(self, text):
        """添加todo"""
        todo = {
            "id": len(self.todos) + 1,
            "text": text,
            "done": False
        }
        self.todos.append(todo)
        self.save_todos()
        return todo
    
    def toggle_todo(self, todo_id):
        """切换todo完成状态"""
        for todo in self.todos:
            if todo["id"] == todo_id:
                todo["done"] = not todo["done"]
                self.save_todos()
                return {"success": True}
        return {"success": False}
    
    def delete_todo(self, todo_id):
        """删除todo"""
        self.todos = [t for t in self.todos if t["id"] != todo_id]
        self.save_todos()
        return {"success": True}
    
    def run(self):
        """运行应用"""
        self.window.load_ui_file("ui/todo.jsx")
        self.window.run()

if __name__ == "__main__":
    lightui.init()
    app = TodoApp()
    app.run()
    lightui.cleanup()
```

### 6.2 数据可视化

```python
# data_viewer.py
import lightui
import pandas as pd
import json

class DataViewer:
    def __init__(self):
        self.window = lightui.Window("Data Viewer", 1200, 800)
        self.df = None
        
        self.window.bind("loadFile", self.load_file)
        self.window.bind("getData", self.get_data)
        self.window.bind("getColumns", self.get_columns)
        self.window.bind("filterData", self.filter_data)
    
    def load_file(self, filepath):
        """加载CSV文件"""
        try:
            self.df = pd.read_csv(filepath)
            return {
                "success": True,
                "rows": len(self.df),
                "columns": list(self.df.columns)
            }
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def get_data(self, page=0, page_size=100):
        """获取数据（分页）"""
        if self.df is None:
            return []
        
        start = page * page_size
        end = start + page_size
        return self.df.iloc[start:end].to_dict('records')
    
    def get_columns(self):
        """获取列信息"""
        if self.df is None:
            return []
        
        return [
            {
                "name": col,
                "type": str(self.df[col].dtype)
            }
            for col in self.df.columns
        ]
    
    def filter_data(self, column, value):
        """过滤数据"""
        if self.df is None:
            return []
        
        filtered = self.df[self.df[column] == value]
        return filtered.to_dict('records')
    
    def run(self):
        self.window.load_ui_file("ui/data_viewer.jsx")
        self.window.run()

if __name__ == "__main__":
    lightui.init()
    viewer = DataViewer()
    viewer.run()
    lightui.cleanup()
```

### 6.3 系统监控

```python
# system_monitor.py
import lightui
import psutil
import time
import threading

class SystemMonitor:
    def __init__(self):
        self.window = lightui.Window("System Monitor", 800, 600)
        self.running = True
        
        self.window.bind("getSystemInfo", self.get_system_info)
        self.window.bind("getProcesses", self.get_processes)
    
    def get_system_info(self):
        """获取系统信息"""
        return {
            "cpu": psutil.cpu_percent(interval=0.1),
            "memory": psutil.virtual_memory().percent,
            "disk": psutil.disk_usage('/').percent,
            "network": {
                "sent": psutil.net_io_counters().bytes_sent,
                "recv": psutil.net_io_counters().bytes_recv
            }
        }
    
    def get_processes(self):
        """获取进程列表"""
        processes = []
        for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_percent']):
            try:
                processes.append(proc.info)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass
        
        # 按CPU使用率排序
        processes.sort(key=lambda p: p['cpu_percent'], reverse=True)
        return processes[:20]  # 返回前20个
    
    def update_loop(self):
        """定期更新UI"""
        while self.running and self.window.is_running():
            info = self.get_system_info()
            self.window.call_js("updateSystemInfo", info)
            time.sleep(1)
    
    def run(self):
        # 启动更新线程
        thread = threading.Thread(target=self.update_loop, daemon=True)
        thread.start()
        
        self.window.load_ui_file("ui/monitor.jsx")
        self.window.run()
        
        self.running = False

if __name__ == "__main__":
    lightui.init()
    monitor = SystemMonitor()
    monitor.run()
    lightui.cleanup()
```

---

## 7. 高级特性

### 7.1 自定义事件循环

```python
import lightui
import asyncio

async def main():
    window = lightui.Window("Async App", 800, 600)
    window.load_ui_file("ui/app.jsx")
    
    while window.is_running():
        # 处理UI事件
        window.poll_events()
        
        # 处理异步任务
        await asyncio.sleep(0.016)  # ~60fps

if __name__ == "__main__":
    lightui.init()
    asyncio.run(main())
    lightui.cleanup()
```

### 7.2 多窗口

```python
import lightui

lightui.init()

# 创建多个窗口
window1 = lightui.Window("Window 1", 400, 300)
window2 = lightui.Window("Window 2", 400, 300)

window1.load_ui("...")
window2.load_ui("...")

# 同时运行（需要自定义事件循环）
while window1.is_running() or window2.is_running():
    if window1.is_running():
        window1.poll_events()
    if window2.is_running():
        window2.poll_events()

lightui.cleanup()
```

### 7.3 错误处理

```python
import lightui

try:
    window = lightui.Window("App", 800, 600)
    window.load_ui_file("ui/app.jsx")
    window.run()
except lightui.LightUIError as e:
    print(f"LightUI Error: {e}")
except FileNotFoundError as e:
    print(f"File not found: {e}")
finally:
    lightui.cleanup()
```

---

## 8. 配置和选项

### 8.1 日志配置

```python
import lightui

# 设置日志级别
lightui.set_log_level(lightui.LOG_DEBUG)

# 自定义日志处理
def log_handler(level, message):
    print(f"[{level}] {message}")

lightui.set_log_callback(log_handler)
```

### 8.2 性能选项

```python
# 设置帧率限制
window.set_fps_limit(60)

# 启用/禁用VSync
window.set_vsync(True)

# 设置渲染质量
window.set_render_quality(lightui.QUALITY_HIGH)
```

---

## 9. 调试

### 9.1 开发者工具

```python
# 启用开发者工具（未来功能）
window.enable_devtools()
```

### 9.2 热重载

```python
import lightui
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class UIReloader(FileSystemEventHandler):
    def __init__(self, window):
        self.window = window
    
    def on_modified(self, event):
        if event.src_path.endswith('.jsx'):
            print(f"Reloading {event.src_path}")
            self.window.reload()

window = lightui.Window("App", 800, 600)
window.load_ui_file("ui/app.jsx")

# 监听文件变化
observer = Observer()
observer.schedule(UIReloader(window), "ui/", recursive=True)
observer.start()

window.run()
observer.stop()
```

---

## 10. API参考

### 10.1 模块级函数

```python
lightui.init() -> None
lightui.cleanup() -> None
lightui.get_version() -> Tuple[int, int, int]
lightui.set_log_level(level: int) -> None
lightui.set_log_callback(callback: Callable) -> None
```

### 10.2 Window类

```python
class Window:
    def __init__(self, title: str, width: int, height: int, **kwargs)
    def show(self) -> None
    def hide(self) -> None
    def close(self) -> None
    def set_title(self, title: str) -> None
    def set_size(self, width: int, height: int) -> None
    def get_size(self) -> Tuple[int, int]
    def load_ui(self, code: str) -> None
    def load_ui_file(self, filepath: str) -> None
    def reload(self) -> None
    def bind(self, name: str, func: Callable) -> None
    def unbind(self, name: str) -> None
    def call_js(self, func_name: str, *args) -> Any
    def eval_js(self, code: str) -> Any
    def run(self) -> None
    def stop(self) -> None
    def poll_events(self) -> bool
    def is_running(self) -> bool
```

### 10.3 异常

```python
class LightUIError(Exception):
    """LightUI基础异常"""
    pass

class InitError(LightUIError):
    """初始化错误"""
    pass

class WindowError(LightUIError):
    """窗口错误"""
    pass

class JSError(LightUIError):
    """JavaScript错误"""
    pass
```

---

## 11. 最佳实践

1. **总是使用上下文管理器或try-finally确保cleanup**
2. **在绑定函数中避免耗时操作**
3. **使用线程处理长时间任务**
4. **合理使用缓存减少Python-JS调用**
5. **使用类型注解提高代码可读性**

---

## 12. 常见问题

### Q: 如何在Python中更新UI？
A: 使用`window.call_js()`调用JavaScript函数更新UI。

### Q: 如何处理大量数据？
A: 使用分页或虚拟滚动，不要一次性传递大量数据。

### Q: 如何调试JavaScript代码？
A: 使用`console.log()`，输出会显示在Python控制台。

---

## 13. 更多资源

- [完整API文档](https://lightui.dev/docs/python)
- [示例项目](https://github.com/lightui/examples)
- [社区论坛](https://forum.lightui.dev)

