# MBink

**轻量高效的企业级桌面应用框架**

基于 QuickJS + Skia + SDL3 + NativeLayoutEngine，使用 JavaScript/Preact 开发高性能桌面应用

> 🎯 **定位**: 轻量核心 + 可选企业级扩展（混合策略）

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.91.0-orange.svg)]()
[![Phase](https://img.shields.io/badge/phase-Native%20Layout%20Engine-yellow)]()
[![Progress](https://img.shields.io/badge/progress-91%25-blue)]()
[![Tests](https://img.shields.io/badge/tests-541%20passing-brightgreen)]()
[![Strategy](https://img.shields.io/badge/strategy-Hybrid-purple)]()

---

## 📊 项目状态

**当前版本**: v0.91.0
**当前阶段**: UI 组件库开发 🔄
**进度**: 91% (核心功能完成，组件库开发中)
**最后更新**: 2025-12-12
**构建状态**: ✅ 核心模块编译成功
**测试状态**: ✅ 541 个测试用例通过
**Preact 生态**: ✅ 90% 完成 (完整 Virtual DOM Diffing)
**战略定位**: 轻量核心 + 可选企业级扩展
**下一步**: UI 组件库 / 状态管理 / 路由系统

### 🚀 快速开始

- **项目状态**: [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) - 完整的项目状态报告
- **快速上手**: [docs/QUICK_START_GUIDE.md](docs/QUICK_START_GUIDE.md) - 快速恢复工作指南
- **开发路线图**: [docs/ROADMAP.md](docs/ROADMAP.md) - 详细开发计划

### 最新成就 🎉

**Phase 10 - 原生布局引擎** (完成 ✅):
- ✅ **Native Layout Engine** - 完全原生的 C++ 布局引擎
- ✅ **Block 布局** - 标准块级元素布局
- ✅ **IFC 布局** - 行内格式化上下文 (Inline Formatting Context)
- ✅ **text-align** - left/center/right/justify 文本对齐
- ✅ **vertical-align** - 行内元素垂直对齐
- ✅ **inline-block** - 行内块级元素支持
- ✅ **换行算法** - 支持 CJK 字符、连字符断行
- ✅ **布局测试** - 312 个布局比较测试 100% 通过

**Phase 9 - Preact 生态集成** (进行中 🔄):
- ✅ **Preact 核心库** - 纯 JS 实现 `js/preact/preact.js`
- ✅ **Hooks 支持** - useState, useEffect, useRef 等完整 Hooks
- ✅ **示例应用** - preact_counter, preact_todo_app 可运行
- 🔄 **C++ 绑定** - PreactRenderer/PreactBindings 待实现

**Phase 8 - 原生布局引擎** (完成 ✅):
- ✅ **Flexbox/Grid** - 通过 NativeLayoutEngine 实现完整布局支持
- ✅ **D3D11 DisplayBackend** - 无闪烁 CPU 渲染

**Phase 3-7 - CSS 高级特性与 HTML 支持** (100% 完成 ✅):
- ✅ **CSS Shadows/Gradients** - 阴影、渐变
- ✅ **CSS Transform/Transition/Animation** - 变换、过渡、动画
- ✅ **CSS Variables/Filters** - 变量、滤镜
- ✅ **HTML5 完整解析** - 错误处理、表单元素

**Phase 2 - 核心功能** (100% 完成 ✅):
- ✅ **JavaScript 运行时** - QuickJS 封装、Console API
- ✅ **DOM API** - W3C 标准、事件系统
- ✅ **渲染/窗口系统** - Skia + SDL3

📝 [项目状态](docs/PROJECT_STATUS.md) | 📊 [开发路线图](docs/ROADMAP.md) | 🏗️ [架构设计](docs/ARCHITECTURE.md)

---

## ✨ 特性

### 核心优势
- 🪶 **轻量高效** - 启动 ~100ms，内存 ~50MB（比 Electron 显著优秀）
- ⚡ **原生性能** - Skia 硬件加速，QuickJS 轻量引擎
- 🎨 **Preact 生态** - 完整 Virtual DOM，支持所有 React Hooks
- 🌍 **跨平台** - Windows、macOS、Linux 一次编写
- 📦 **独立部署** - 单文件运行，无需额外运行时

### 混合策略（企业级能力）
- 🔧 **轻量核心** - 20个基础组件 (~50KB)，满足 80% 场景
- 🔌 **可选扩展** - 按需加载高级组件和第三方库
- 🏢 **企业支持** - 完整组件库、可视化设计器（规划中）

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
| **核心模块** | | |
| test_window | 17 | ✅ PASSED |
| test_event_loop | 46 | ✅ PASSED |
| test_dom_* | 78 | ✅ PASSED |
| test_quickjs_runtime | 11 | ✅ PASSED |
| **CSS 高级特性** | | |
| test_shadow_renderer | 12 | ✅ PASSED |
| test_text_shadow | 12 | ✅ PASSED |
| test_gradient_renderer | 15 | ✅ PASSED |
| test_transform | 19 | ✅ PASSED |
| test_css_integration | 11 | ✅ PASSED |
| **布局引擎测试** | | |
| test_ifc (IFC 单元测试) | 32 | ✅ PASSED |
| layout_compare (基础布局) | 121 | ✅ PASSED |
| layout_compare (高级布局) | 159 | ✅ PASSED |
| test_layout_performance | 8 | ✅ PASSED |
| **总计** | **541** | **✅ 全部通过** |

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

当前版本：**v0.91.0**
总体进度：**90%**

### ✅ 已完成阶段

#### Phase 1: 基础架构 (100%) ✅
- ✅ CMake构建系统
- ✅ SDL3集成 (6.5 MB)
- ✅ Skia集成 (36.5 MB)
- ✅ QuickJS集成 (1.1 MB)
- ✅ NativeLayoutEngine (Block + IFC + Flexbox + Grid)
- ✅ Lexbor集成 (2.6.0)

#### Phase 2-6: 核心功能 (100%) ✅
- ✅ JavaScript运行时 (QuickJS封装、Console API、定时器)
- ✅ DOM API (Node、Element、Document、事件系统)
- ✅ 布局引擎 (NativeLayoutEngine Flexbox + CSS Grid)
- ✅ 渲染引擎 (Skia渲染、CSS样式、文本渲染)
- ✅ 窗口系统 (SDL3窗口、D3D11 后端)
- ✅ 事件系统 (鼠标、键盘、焦点、拖拽)
- ✅ CSS 高级特性 (阴影、渐变、变换、动画、滤镜)

#### Phase 7: HTML/CSS 完整支持 (100%) ✅
- ✅ HTML5 完整解析 (DOCTYPE、实体、错误恢复)
- ✅ CSS3 选择器 (所有类型)
- ✅ 表单元素 (所有 HTML5 input 类型)

#### Phase 8: 原生布局引擎 (100%) ✅
- ✅ CSS Flexbox 完整支持 (NativeLayoutEngine)
- ✅ CSS Grid 布局支持
- ✅ Position/Overflow 支持

#### Phase 9: 原生布局引擎 (100%) ✅
- ✅ Native Layout Engine (Block + IFC)
- ✅ IFC 行内格式化上下文
- ✅ text-align / vertical-align
- ✅ 312 个布局测试 100% 通过

### 🔄 进行中阶段

#### Phase 10: Preact 生态系统 (60%)
- ✅ Preact 基本集成
- ✅ 5 个 Preact 示例应用
- ⏳ Preact Hooks 完整测试
- ⏳ 组件库测试 (Ant Design)

### 📋 计划中阶段

#### Phase 11: 多语言绑定 (20%)
- ✅ C API 基础框架
- ⏳ Python绑定完善
- ⏳ Rust绑定
- ⏳ Go绑定

#### Phase 12: 工具链和发布
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
- **NativeLayoutEngine** - 原生 C++ 布局引擎 (Block + IFC + Flexbox + Grid)
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

