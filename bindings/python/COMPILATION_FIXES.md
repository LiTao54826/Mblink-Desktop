# Python 绑定编译修复

## 修复的编译错误

### 1. ❌ 错误：找不到 `dom_binding_map.h`

**错误信息：**
```
error C1083: 无法打开包括文件: "core/dom/bindings/dom_binding_map.h": No such file or directory
```

**原因：**
- `dom_binding_map.h` 文件不存在
- `DOMBindingMap` 类不存在

**修复：**
- 移除了 `#include "core/dom/bindings/dom_binding_map.h"` 引用
- 移除了对 `DOMBindingMap::GetInstance().Clear()` 的调用
- `DOMBindings::Cleanup()` 已经处理了所有缓存清理

**修改文件：**
- `bindings/python/src/bindings.cpp` (第 24 行，第 925 行，第 1346 行)

---

### 2. ❌ 错误：`Window::MarkDirty()` 方法不存在

**错误信息：**
```
error C2039: "MarkDirty": 不是 "lightui::Window" 的成员
```

**原因：**
- `Window` 类使用的是 `SetNeedsRepaint()` 而不是 `MarkDirty()`

**修复：**
- 将 `window_->MarkDirty()` 改为 `window_->SetNeedsRepaint()`

**修改文件：**
- `bindings/python/src/bindings.cpp` (第 737 行)

---

### 3. ❌ 错误：`DevToolsManager` 析构函数不可访问

**错误信息：**
```
error C2248: "lightui::DevToolsManager::~DevToolsManager": 无法访问 private 成员
```

**原因：**
- `DevToolsManager` 是单例模式，析构函数是私有的
- pybind11 默认会尝试创建 `std::unique_ptr`，但这会失败

**修复：**
- 使用 `py::class_<DevToolsManager, std::shared_ptr<DevToolsManager>>` 声明
- 使用 `py::return_value_policy::reference` 返回单例引用
- 修改 `get_instance()` 返回裸指针而不是引用

**修改文件：**
- `bindings/python/src/bindings.cpp` (第 1318-1334 行)

**修复代码：**
```cpp
py::class_<DevToolsManager, std::shared_ptr<DevToolsManager>>(m, "DevToolsManager")
    .def_static("get_instance", []() -> DevToolsManager* {
        return &DevToolsManager::GetInstance();
    }, py::return_value_policy::reference,
       "Get the singleton DevTools manager instance")
    // ...
```

---

## 编译命令

### 完整重新编译

```bash
# 1. 清理旧的构建文件（可选）
cmake --build build --config Release --target clean

# 2. 重新编译 Python 绑定
cmake --build build --config Release --target lightui_core
```

### 仅编译 Python 绑定

```bash
cmake --build build --config Release --target lightui_core
```

---

## 测试编译结果

### 1. 运行编译测试脚本

```bash
python bindings/python/test_compile.py
```

**预期输出：**
```
✅ lightui_core 模块加载成功
   版本: 0.5.0

检查 EventLoop 类...
✅ EventLoop.set_window() 方法存在
✅ EventLoop.set_quickjs_runtime() 方法存在

检查 Window 类...
✅ Window.needs_repaint() 方法存在
✅ Window.mark_dirty() 方法存在
✅ Window.get_document_ptr() 方法存在

检查 Runtime 类...
✅ Runtime.register_module() 方法存在
✅ Runtime.set_base_module_path() 方法存在
✅ Runtime.init_fetch_bindings() 方法存在

检查 DevToolsManager 类...
✅ DevToolsManager 类存在
✅ DevToolsManager.get_instance() 成功

检查全局函数...
✅ set_image_base_path() 函数存在
✅ clear_font_cache() 函数存在
✅ cleanup_dom_bindings() 函数存在

==================================================
编译测试完成！
==================================================
```

### 2. 运行增量渲染测试

```bash
python bindings/python/examples/test_incremental_rendering.py
```

**预期行为：**
- 窗口打开并显示计数器界面
- 点击按钮时计数器增加
- 渲染次数远少于点击次数（增量渲染生效）
- 按 F12 可以打开开发者工具

### 3. 运行其他示例

```bash
# Preact 计数器
python bindings/python/examples/preact_counter.py

# Preact 待办应用
python bindings/python/examples/preact_todo_app.py

# 基础用法
python bindings/python/examples/basic_usage.py
```

---

## 验证修复的功能

### 1. 增量渲染

```python
from lightui import LightUIApp

with LightUIApp("Test", 800, 600) as app:
    # EventLoop 自动设置 Window
    # 只在需要时重绘
    app.load_html("<h1>Hello</h1>")
    app.run()  # 自动增量渲染
```

### 2. DevTools

```python
with LightUIApp("Test", enable_devtools=True) as app:
    app.load_html("<h1>Hello</h1>")
    app.open_devtools()  # 打开开发者工具
    app.run()
```

### 3. ES 模块

```python
with LightUIApp("Test") as app:
    # 注册模块
    app.register_module('my-utils', '''
        export function add(a, b) { return a + b; }
    ''')
    
    # 加载模块
    app.load_module('''
        import { add } from 'my-utils';
        console.log(add(1, 2));
    ''')
    
    app.run()
```

### 4. Fetch API

```python
with LightUIApp("Test") as app:
    app.load_html('''
        <script>
            fetch('https://api.github.com/users/github')
                .then(r => r.json())
                .then(data => console.log(data));
        </script>
    ''')
    app.run()
```

---

## 故障排除

### 问题：编译失败

**解决方案：**
1. 确保已安装所有依赖：`pip install pybind11`
2. 确保 CMake 配置正确：`cmake -B build -DLIGHTUI_BUILD_PYTHON_BINDING=ON`
3. 清理并重新编译：`cmake --build build --config Release --target clean && cmake --build build --config Release --target lightui_core`

### 问题：找不到 lightui_core 模块

**解决方案：**
1. 检查 `bindings/python/lightui/bin/lightui_core.pyd` 是否存在
2. 确保 Python 路径正确：`sys.path.insert(0, 'bindings/python')`
3. 使用虚拟环境：`.venv/Scripts/python.exe`

### 问题：运行时崩溃

**解决方案：**
1. 确保使用 `with` 语句或手动调用 `cleanup()`
2. 确保在退出前停止事件循环
3. 检查是否正确清理资源

---

## 下一步

1. ✅ 编译 Python 绑定
2. ✅ 运行测试脚本验证
3. ✅ 测试增量渲染
4. ✅ 测试 DevTools
5. ⏳ 添加更多示例
6. ⏳ 完善文档

