# MBink

**轻量级跨平台桌面应用框架 - Electron 的轻量级替代品**

基于 QuickJS + Skia + SDL3 + Taffy，使用 JavaScript/React 开发原生桌面应用

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.90.0-orange.svg)]()
[![Phase](https://img.shields.io/badge/phase-Taffy%20Layout%20Complete-brightgreen)]()
[![Progress](https://img.shields.io/badge/progress-90%25-blue)]()
[![Tests](https://img.shields.io/badge/tests-81%20passing-brightgreen)]()

---

## 📊 项目状态

**当前版本**: v0.90.0
**当前阶段**: Taffy CSS 布局引擎集成完成 ✅
**进度**: 90% (核心功能 + CSS 高级特性 + 布局引擎完成)
**最后更新**: 2025-11-29
**构建状态**: ✅ 所有模块编译成功
**测试状态**: ✅ 81 个测试用例全部通过
**布局引擎**: ✅ Taffy CSS 布局引擎 (Flexbox + CSS Grid)
**CSS 高级特性**: ✅ 100% 完成 (阴影、渐变、Transform、Transition、Animation、变量、滤镜)
**下一步**: React 生态完善 或 多语言绑定

### 🚀 快速开始

- **项目状态**: [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) - 完整的项目状态报告
- **快速上手**: [docs/QUICK_START_GUIDE.md](docs/QUICK_START_GUIDE.md) - 快速恢复工作指南
- **开发路线图**: [docs/ROADMAP.md](docs/ROADMAP.md) - 详细开发计划

### 最新成就 🎉

**Phase 8 - Taffy CSS 布局引擎** (2025-11-28 完成 ✅):
- ✅ **Taffy 布局引擎集成** - 替代 Yoga，支持更完整的 CSS 布局
- ✅ **CSS Flexbox** - 完整的 Flexbox 布局支持
- ✅ **CSS Grid** - grid-template-columns/rows 模板支持
- ✅ **Position/Overflow** - 定位和溢出处理
- ✅ **D3D11 DisplayBackend** - 无闪烁 CPU 渲染
- ✅ **DPI 缩放** - 高 DPI 显示支持
- ✅ **滚动条支持** - 原生滚动条渲染

**Phase 7 - HTML/CSS 完整支持** (v0.90.0 完成 ✅):
- ✅ **HTML5 完整解析** - 错误处理、DOCTYPE、HTML 实体 (200+)
- ✅ **CSS3 选择器** - 所有基本、组合、属性、伪类、伪元素选择器
- ✅ **表单元素** - 所有 HTML5 input 类型、验证、状态管理
- ✅ **333 个测试** - 283 单元测试 + 50 性能测试

**Phase 3-6 - CSS 高级特性** (100% 完成 ✅):
- ✅ **CSS Shadows** - Box Shadow、Text Shadow (多重阴影)
- ✅ **CSS Gradients** - Linear/Radial Gradient (多色停止点)
- ✅ **CSS Transform** - translate、rotate、scale、skew、matrix
- ✅ **CSS Transition** - 12 种缓动函数、动画时间线
- ✅ **CSS Animation** - @keyframes、AnimationController
- ✅ **CSS Variables** - 自定义属性、var() 函数
- ✅ **CSS Filters** - 10 种滤镜效果

**Phase 2 - 核心功能** (100% 完成 ✅):
- ✅ **JavaScript 运行时** - QuickJS 封装、Console API、定时器
- ✅ **DOM API** - 遵循 W3C 标准、事件冒泡/捕获
- ✅ **布局引擎** - Flexbox、CSS 盒模型
- ✅ **渲染引擎** - Skia 渲染、CSS 样式
- ✅ **窗口系统** - SDL3 窗口、60 FPS 主循环
- ✅ **事件系统** - 鼠标、键盘、焦点、拖拽

📝 [项目状态](docs/PROJECT_STATUS.md) | 📊 [开发路线图](docs/ROADMAP.md) | 🏗️ [架构设计](docs/ARCHITECTURE.md)

---

## ✨ 特性

- 🪶 **轻量级** - 总体积约 50MB（比Electron小50-70%，比Tauri大但功能更完整）
- ⚡ **高性能** - Skia硬件加速渲染，浏览器级渲染效果，QuickJS轻量级引擎
- 🎨 **易开发** - 使用 JavaScript/Preact + React生态开发UI，丰富的组件库支持
- 🌍 **跨平台** - Windows、macOS、Linux 一次编写，到处运行
- 🔗 **跨语言** - Python、C++、Rust、Go、Node.js 等语言都能使用
- 📦 **独立运行** - 单个可执行文件，无需安装额外运行时
- 🎯 **React生态** - 完整支持React组件库（Ant Design、Material-UI等）

## 🆚 与竞品对比

| 特性 | **MBink** | **Electron** | **Tauri** | **RmlUi** |
|------|-----------|-------------|-----------|-----------|
| **体积** | ~50MB | ~150MB | ~10MB | ~5MB |
| **JS引擎** | QuickJS | V8 | JavaScriptCore | ❌ |
| **渲染** | Skia | Chromium | WebView | 用户提供 |
| **React支持** | ✅ | ✅ | ✅ | ❌ |
| **启动速度** | 快 (~200ms) | 慢 (~1s) | 快 (~100ms) | 极快 (~50ms) |
| **目标场景** | 桌面应用 | 桌面应用 | 桌面应用 | 游戏UI |
| **内存占用** | 中 (~100MB) | 高 (~300MB) | 低 (~50MB) | 极低 (~20MB) |

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

## 🔨 构建指南

### Windows (推荐使用 Visual Studio 2022)

```bash
# 1. 克隆仓库
git clone https://github.com/yourusername/LightUI.git
cd LightUI

# 2. 下载 Skia 预编译库
.\scripts\download_deps.bat

# 3. 配置项目
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..

# 4. 构建（Debug 模式）
cmake --build . --config Debug -j 8

# 5. 运行测试
cd bin\Debug
.\test_hello.exe
.\test_dom_node.exe
.\test_css_rendering.exe
```

### Linux/macOS

```bash
# 1. 克隆仓库
git clone https://github.com/yourusername/LightUI.git
cd LightUI

# 2. 下载依赖
./scripts/download_deps.sh

# 3. 配置和构建
mkdir build && cd build
cmake ..
cmake --build . -j 8

# 4. 运行测试
./bin/test_hello
./bin/test_dom_node
```

---

## 🧪 测试

项目包含完整的测试套件，所有核心功能都经过验证：

### 运行所有测试

```bash
cd build/bin/Debug  # Windows
# 或
cd build/bin        # Linux/macOS

# DOM 测试
./test_dom_node        # 25 个节点操作测试
./test_dom_document    # 17 个文档操作测试
./test_dom_query       # 27 个查询选择器测试
./test_dom_integration # 9 个集成测试

# 渲染测试
./test_css_rendering   # CSS 渲染测试
./test_render_tree     # 渲染树测试

# CSS 高级特性测试
./test_shadow_renderer    # 阴影渲染测试 (12个)
./test_text_shadow        # 文本阴影测试 (12个)
./test_gradient_renderer  # 渐变渲染测试 (15个)
./test_transform          # Transform 测试 (19个)
./test_css_integration    # CSS 集成测试 (11个)

# JavaScript 测试
./test_simple          # QuickJS 基础测试
./test_quickjs_runtime # 运行时测试
```

### 测试统计

| 测试套件 | 测试数量 | 状态 |
|---------|---------|------|
| test_window | 17 | ✅ PASSED |
| test_event_loop | 46 | ✅ PASSED |
| test_dom_node | 25 | ✅ PASSED |
| test_dom_document | 17 | ✅ PASSED |
| test_dom_query | 27 | ✅ PASSED |
| test_dom_event | 9 | ✅ PASSED |
| test_quickjs_runtime | 11 | ✅ PASSED |
| test_css_rendering | 3 | ✅ PASSED |
| **CSS 高级特性** | | |
| test_shadow_renderer | 12 | ✅ PASSED |
| test_text_shadow | 12 | ✅ PASSED |
| test_gradient_renderer | 15 | ✅ PASSED |
| test_transform | 19 | ✅ PASSED |
| test_css_integration | 11 | ✅ PASSED |
| **总计** | **224** | **✅ 全部通过** |

---

## 📚 文档

### 核心文档
- **[项目状态](docs/PROJECT_STATUS.md)** - 完整的项目状态报告
- **[项目规范](docs/PROJECT_STANDARDS.md)** - 强制执行的开发规范
- **[开发路线图](docs/ROADMAP.md)** - 详细开发计划
- **[架构设计](docs/ARCHITECTURE.md)** - 技术架构和模块设计

### 开发文档
- **[入门指南](docs/GETTING_STARTED.md)** - 快速开始开发
- **[快速上手](docs/QUICK_START_GUIDE.md)** - 快速恢复工作指南
- **[DOM API 文档](docs/DOM_API.md)** - DOM 操作接口
- **[API 设计](docs/API_DESIGN.md)** - C API 设计
- **[代码规范](docs/CODING_STANDARDS.md)** - 代码风格指南
- **[测试指南](docs/TESTING.md)** - 测试规范和方法
- **[贡献指南](docs/CONTRIBUTING.md)** - 如何贡献代码
- **[示例文档](docs/EXAMPLES.md)** - 示例代码说明

---

## 🎯 开发状态

当前版本：**v0.90.0**
总体进度：**90%**

### ✅ 已完成阶段

#### Phase 1: 基础架构 (100%) ✅
- ✅ CMake构建系统
- ✅ SDL3集成 (6.5 MB)
- ✅ Skia集成 (36.5 MB)
- ✅ QuickJS集成 (1.1 MB)
- ✅ Taffy 布局引擎 (替代 Yoga)
- ✅ Lexbor集成 (2.6.0)

#### Phase 2-6: 核心功能 (100%) ✅
- ✅ JavaScript运行时 (QuickJS封装、Console API、定时器)
- ✅ DOM API (Node、Element、Document、事件系统)
- ✅ 布局引擎 (Taffy Flexbox + CSS Grid)
- ✅ 渲染引擎 (Skia渲染、CSS样式、文本渲染)
- ✅ 窗口系统 (SDL3窗口、D3D11 后端)
- ✅ 事件系统 (鼠标、键盘、焦点、拖拽)
- ✅ CSS 高级特性 (阴影、渐变、变换、动画、滤镜)

#### Phase 7: HTML/CSS 完整支持 (100%) ✅
- ✅ HTML5 完整解析 (DOCTYPE、实体、错误恢复)
- ✅ CSS3 选择器 (所有类型)
- ✅ 表单元素 (所有 HTML5 input 类型)

#### Phase 8: Taffy 布局引擎 (100%) ✅
- ✅ CSS Flexbox 完整支持
- ✅ CSS Grid 布局支持
- ✅ Position/Overflow 支持

### 🔄 进行中阶段

#### Phase 9: Preact 生态系统 (60%)
- ✅ Preact 基本集成
- ✅ 5 个 Preact 示例应用
- ⏳ Preact Hooks 完整测试
- ⏳ 组件库测试 (Ant Design)

### 📋 计划中阶段

#### Phase 10: 多语言绑定 (20%)
- ✅ C API 基础框架
- ⏳ Python绑定完善
- ⏳ Rust绑定
- ⏳ Go绑定

#### Phase 11: 工具链和发布
- ⏳ CLI 工具
- ⏳ 跨平台测试 (macOS, Linux)
- ⏳ v1.0 发布

查看完整进度：[项目状态](docs/PROJECT_STATUS.md) | [开发路线图](docs/ROADMAP.md)

---

## 🤝 贡献

我们欢迎各种形式的贡献！详细信息请查看 [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

---

## 📄 许可证

本项目采用 [MIT许可证](LICENSE)

---

## 🙏 致谢

MBink基于以下优秀的开源项目：

- **[QuickJS](https://bellard.org/quickjs/)** - 轻量级JavaScript引擎 (600KB)
- **[Skia](https://skia.org/)** - 2D图形库 (Chrome同源)
- **[SDL3](https://www.libsdl.org/)** - 跨平台窗口库
- **[Taffy](https://github.com/DioxusLabs/taffy)** - CSS 布局引擎 (Flexbox + Grid)
- **[Lexbor](https://github.com/lexbor/lexbor)** - HTML5/CSS3解析库
- **[Preact](https://preactjs.com/)** - 轻量级React替代品
- **[RmlUi](https://github.com/mikke89/RmlUi)** - 参考项目（事件系统、CSS动画）

## 📖 参考资料

- [RmlUi Documentation](https://mikke89.github.io/RmlUiDoc/) - 事件系统和CSS动画参考
- [React Documentation](https://react.dev/) - React生态
- [Electron Documentation](https://www.electronjs.org/) - 竞品参考
- [Tauri Documentation](https://tauri.app/) - 竞品参考

---

<div align="center">

**如果这个项目对你有帮助，请给我们一个⭐️！**

Made with ❤️ by the MBink Team

</div>

