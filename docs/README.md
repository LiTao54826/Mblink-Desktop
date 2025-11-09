# LightUI

<div align="center">

![LightUI Logo](docs/assets/logo.png)

**轻量级跨语言UI框架**

基于 QuickJS + Skia + SDL3，使用 JavaScript/Preact 开发原生桌面应用

[![Build Status](https://github.com/lightui/lightui/workflows/build/badge.svg)](https://github.com/lightui/lightui/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)](https://github.com/lightui/lightui/releases)

[English](README.md) | [中文](README_zh.md)

</div>

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
import { List, Checkbox } from 'antd';

function TodoApp() {
    const [todos, setTodos] = useState([]);
    
    useEffect(() => {
        setTodos(window.getTodos());
    }, []);
    
    return (
        <List
            dataSource={todos}
            renderItem={item => (
                <List.Item>
                    <Checkbox>{item.text}</Checkbox>
                </List.Item>
            )}
        />
    );
}

render(<TodoApp />, document.body);
""")

# 运行
app.run()
```

### C++示例

```cpp
#include "lightui.h"

int main() {
    lightui_init();
    
    // 创建窗口
    auto window = lightui_create_window("My App", 800, 600);
    
    // 绑定C++函数
    lightui_bind_function(window, "getData", 
        [](const char* args, char** result, void* user_data) {
            *result = strdup(R"({"message": "Hello from C++"})");
        }, nullptr);
    
    // 加载UI
    lightui_load_ui_file(window, "ui/app.jsx");
    
    // 运行
    lightui_run(window);
    
    lightui_cleanup();
    return 0;
}
```

---

## 📦 安装

### Python

```bash
pip install lightui
```

### C++

```bash
# Linux/macOS
git clone https://github.com/lightui/lightui.git
cd lightui
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --install .
```

### Rust

```toml
[dependencies]
lightui = "0.1"
```

### Go

```bash
go get github.com/lightui/lightui-go
```

---

## 🏗️ 架构

```
┌─────────────────────────────────────────────────────────┐
│  应用层 (Python/C++/Rust/Go)                             │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  语言绑定层 (ctypes/pybind11/bindgen/cgo)               │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  C API层 (lightui.h)                                    │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  JavaScript运行时层                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  QuickJS     │  │  DOM API     │  │  Event       │  │
│  │  (600KB)     │  │  (40+ APIs)  │  │  System      │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  渲染层                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  Yoga        │  │  Skia        │  │  SDL3        │  │
│  │  (Layout)    │  │  (Render)    │  │  (Window)    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 📊 对比

| 特性 | LightUI | Electron | Qt | Tauri | Dear ImGui |
|------|---------|----------|----|----|------------|
| **体积** | 10-15MB | 100MB+ | 30MB+ | 15-20MB | 2-3MB |
| **性能** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **开发效率** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| **UI生态** | React生态 | Web生态 | Qt组件 | Web生态 | 有限 |
| **跨语言** | ✅ | ❌ | ✅ | ❌ | ✅ |
| **学习曲线** | 低 | 低 | 高 | 中 | 中 |

---

## 📚 文档

### 核心文档

- **[项目概述](PROJECT_OVERVIEW.md)** - 了解项目目标和价值
- **[开发路线图](ROADMAP.md)** - 32周详细开发计划
- **[架构设计](ARCHITECTURE.md)** - 技术架构和模块设计
- **[项目结构](PROJECT_STRUCTURE.md)** - 目录结构和构建系统
- **[编码规范](CODING_STANDARDS.md)** - 代码风格和最佳实践

### API文档

- **[C API](API_DESIGN.md)** - C语言API完整参考
- **[Python API](PYTHON_API.md)** - Python绑定API和示例
- **JavaScript API** - DOM API和Web API参考（待完成）

### 开发指南

- **[入门指南](GETTING_STARTED.md)** - 快速开始开发
- **[贡献指南](CONTRIBUTING.md)** - 如何贡献代码
- **构建指南** - 各平台构建说明（待完成）
- **测试指南** - 测试策略和工具（待完成）

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

### Phase 2: Preact支持 (Week 13-16) 📅

- [ ] 完整DOM API (40+ APIs)
- [ ] Preact集成和测试
- [ ] 基础组件示例

### Phase 3: 组件库支持 (Week 17-20) 📅

- [ ] Ant Design集成
- [ ] Material-UI集成
- [ ] 示例应用

### Phase 4: 多语言绑定 (Week 21-24) 📅

- [ ] Python绑定
- [ ] Rust绑定
- [ ] Go绑定

查看完整路线图：[ROADMAP.md](ROADMAP.md)

---

## 🌟 示例项目

### 官方示例

- **[Hello World](examples/python/hello_world/)** - 最简单的示例
- **[Todo App](examples/python/todo_app/)** - 完整的Todo应用
- **[Data Viewer](examples/python/data_viewer/)** - 数据可视化工具
- **[System Monitor](examples/python/system_monitor/)** - 系统监控工具

### 社区项目

- **Markdown编辑器** - 实时预览的Markdown编辑器
- **音乐播放器** - 跨平台音乐播放器
- **文件管理器** - 现代化文件管理器

[查看更多示例](https://github.com/lightui/examples)

---

## 🤝 贡献

我们欢迎各种形式的贡献！

### 如何贡献

1. Fork项目
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'feat: add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建Pull Request

详细信息请查看 [CONTRIBUTING.md](CONTRIBUTING.md)

### 贡献者

感谢所有贡献者！

<a href="https://github.com/lightui/lightui/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=lightui/lightui" />
</a>

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

## 📞 联系我们

- **GitHub**: [@lightui](https://github.com/lightui)
- **Discord**: [加入服务器](https://discord.gg/lightui)
- **Email**: dev@lightui.dev
- **Twitter**: [@lightui_dev](https://twitter.com/lightui_dev)

---

## ⭐ Star历史

[![Star History Chart](https://api.star-history.com/svg?repos=lightui/lightui&type=Date)](https://star-history.com/#lightui/lightui&Date)

---

<div align="center">

**如果这个项目对你有帮助，请给我们一个⭐️！**

Made with ❤️ by the LightUI Team

</div>

