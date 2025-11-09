# LightUI

**轻量级跨语言UI框架**

基于 QuickJS + Skia + SDL3，使用 JavaScript/Preact 开发原生桌面应用

[![Build Status](https://github.com/lightui/lightui/workflows/build/badge.svg)](https://github.com/lightui/lightui/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)](https://github.com/lightui/lightui/releases)

---

## ✨ 特性

- 🪶 **轻量级** - 总体积仅 10-15MB（vs Electron 100MB+）
- ⚡ **高性能** - Skia硬件加速渲染，60fps流畅体验
- 🎨 **易开发** - 使用 JavaScript/Preact + React生态开发UI
- 🌍 **跨平台** - Windows、macOS、Linux 一次编写，到处运行
- 🔗 **跨语言** - Python、C++、Rust、Go 等语言都能使用
- 📦 **零依赖** - 单个可执行文件，无需安装运行时

---

## 🚀 快速开始

### Python示例

```python
import lightui

# 创建窗口
app = lightui.Window("Todo App", 600, 800)

# 绑定Python函数
@app.bind("getTodos")
def get_todos():
    return [
        {"id": 1, "text": "Learn LightUI", "done": False},
        {"id": 2, "text": "Build an app", "done": False}
    ]

# 加载UI（使用Preact + Ant Design）
app.load_ui("""
import { render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

function TodoApp() {
    const [todos, setTodos] = useState([]);
    
    useEffect(() => {
        setTodos(window.getTodos());
    }, []);
    
    return (
        <ul>
            {todos.map(todo => <li key={todo.id}>{todo.text}</li>)}
        </ul>
    );
}

render(<TodoApp />, document.body);
""")

# 运行
app.run()
```

---

## 📚 文档

- **[项目概述](docs/PROJECT_OVERVIEW.md)** - 了解项目目标和价值
- **[开发路线图](docs/ROADMAP.md)** - 32周详细开发计划
- **[架构设计](docs/ARCHITECTURE.md)** - 技术架构和模块设计
- **[入门指南](docs/GETTING_STARTED.md)** - 快速开始开发
- **[贡献指南](docs/CONTRIBUTING.md)** - 如何贡献代码
- **[文档索引](docs/DOCUMENTATION_INDEX.md)** - 完整文档导航

---

## 🎯 开发状态

当前版本：**0.1.0-alpha** (开发中)

### Phase 1: 核心框架 (Week 1-12) 🚧

- [ ] SDL3 + Skia集成 (Week 1-2)
- [ ] QuickJS集成 (Week 3-4)
- [ ] 基础DOM API (Week 5-6)
- [ ] Yoga布局引擎 (Week 7-8)
- [ ] Skia渲染 (Week 9-10)
- [ ] 事件系统 (Week 11-12)

查看完整路线图：[docs/ROADMAP.md](docs/ROADMAP.md)

---

## 🤝 贡献

我们欢迎各种形式的贡献！详细信息请查看 [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

---

## 📄 许可证

本项目采用 [MIT许可证](LICENSE)

---

## 🙏 致谢

LightUI基于以下优秀的开源项目：

- **[QuickJS](https://bellard.org/quickjs/)** - 轻量级JavaScript引擎
- **[Skia](https://skia.org/)** - 2D图形库
- **[SDL3](https://www.libsdl.org/)** - 跨平台窗口库
- **[Yoga](https://yogalayout.com/)** - Flexbox布局引擎
- **[Preact](https://preactjs.com/)** - 轻量级React替代品

---

<div align="center">

**如果这个项目对你有帮助，请给我们一个⭐️！**

Made with ❤️ by the LightUI Team

</div>

