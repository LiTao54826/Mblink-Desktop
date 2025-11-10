# LightUI

**轻量级跨语言UI框架**

基于 QuickJS + Skia + SDL3，使用 JavaScript/Preact 开发原生桌面应用

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)]()
[![Phase](https://img.shields.io/badge/phase-2.3%20nearly%20complete-green)]()
[![Progress](https://img.shields.io/badge/progress-78%25-blue)]()

---

## 📊 项目状态

**当前阶段**: Phase 2.3 完成 ✅ - 渲染引擎实现与测试
**进度**: 85% (Phase 1 + Phase 2.1 + Phase 2.2 + Phase 2.3 完成 ✅)
**最后更新**: 2025-11-10
**构建状态**: ✅ 所有核心模块编译成功
**测试状态**: ✅ 83+ 个测试用例全部通过

### 最新成就 🎉

**Phase 2.3 - 渲染引擎** (100% 完成):
- ✅ **Skia 渲染器** - 完整的 2D 图形渲染引擎，已启用并测试
- ✅ **文本渲染** - 字体管理、文本布局、多语言支持
- ✅ **图片渲染** - 图片加载、缓存、多格式支持
- ✅ **CSS 样式渲染** - 完整的盒模型、圆角、阴影、渐变
- ✅ **DOM 渲染树** - 样式计算、渲染树构建、布局系统
- ✅ **渲染优化** - 脏区域、层级系统、缓存、批量渲染、裁剪优化
- ✅ **83+ 测试全部通过** - 完整的单元测试覆盖
- ✅ **构建系统优化** - CMake 配置完善，支持 Debug/Release 构建

**Phase 2.2 - DOM API** (100% 完成):
- ✅ **完整的 DOM API** - 遵循 W3C 标准
- ✅ **事件系统** - 完整的事件冒泡、捕获
- ✅ **CSS 选择器** - QuerySelector/QuerySelectorAll
- ✅ **QuickJS 绑定** - JavaScript 可直接操作 DOM
- ✅ **所有 DOM 测试通过**

📝 [Phase 2.3 计划](PHASE_2_3_PLAN.md) | 📚 [文档索引](docs/DOCUMENTATION_INDEX.md) | 🏗️ [架构设计](docs/ARCHITECTURE.md)

---

## ✨ 特性

- 🪶 **轻量级** - 总体积约 50MB（比Electron小50-70%）
- ⚡ **高性能** - Skia硬件加速渲染，浏览器级渲染效果
- 🎨 **易开发** - 使用 JavaScript/Preact + React生态开发UI
- 🌍 **跨平台** - Windows、macOS、Linux 一次编写，到处运行
- 🔗 **跨语言** - Python、C++、Rust、Go 等语言都能使用
- 📦 **独立运行** - 单个可执行文件，无需安装额外运行时

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

# JavaScript 测试
./test_simple          # QuickJS 基础测试
./test_quickjs_runtime # 运行时测试
```

### 测试统计

| 测试套件 | 测试数量 | 状态 |
|---------|---------|------|
| test_hello | 1 | ✅ PASSED |
| test_simple | 1 | ✅ PASSED |
| test_dom_node | 25 | ✅ PASSED |
| test_dom_document | 17 | ✅ PASSED |
| test_dom_query | 27 | ✅ PASSED |
| test_dom_integration | 9 | ✅ PASSED |
| test_css_rendering | 3 | ✅ PASSED |
| **总计** | **83+** | **✅ 全部通过** |

---

## 📚 文档

- **[项目概述](docs/PROJECT_OVERVIEW.md)** - 了解项目目标和价值
- **[开发路线图](docs/ROADMAP.md)** - 32周详细开发计划
- **[架构设计](docs/ARCHITECTURE.md)** - 技术架构和模块设计
- **[入门指南](docs/GETTING_STARTED.md)** - 快速开始开发
- **[DOM API 文档](docs/DOM_API.md)** - DOM 操作接口
- **[性能优化](docs/PERFORMANCE.md)** - 性能优化指南
- **[贡献指南](docs/CONTRIBUTING.md)** - 如何贡献代码
- **[文档索引](docs/DOCUMENTATION_INDEX.md)** - 完整文档导航

---

## 🎯 开发状态

当前版本：**0.1.0-alpha**

### Phase 1: 基础架构 ✅ 已完成

- [x] 开发环境搭建 (MSVC 2022 / GCC 11+ / Clang 14+)
- [x] CMake 构建系统配置
- [x] SDL3 集成 (6.5 MB)
- [x] QuickJS 集成 (1.1 MB)
- [x] Yoga 布局引擎集成 (2.1 MB)
- [x] Skia 渲染引擎集成 (36.5 MB) - **已启用并测试**
- [x] GoogleTest 测试框架集成
- [x] nlohmann/json 库集成
- [x] 9 个核心模块编译成功

### Phase 2: 核心功能 ✅ 已完成 (85%)

- [x] JavaScript 运行时实现 ✅
- [x] DOM API 实现 ✅
- [x] 事件系统实现 ✅
- [x] 渲染引擎实现 ✅
- [x] CSS 样式渲染 ✅
- [x] 布局引擎集成 ✅
- [x] 83+ 个测试全部通过 ✅

### Phase 3: 高级功能 🚧 计划中

- [ ] 动画系统
- [ ] 网络请求 (Fetch API)
- [ ] 多窗口支持
- [ ] WebGL 支持
- [ ] 更多语言绑定

查看完整进度：[PROJECT_STATUS.md](PROJECT_STATUS.md) | [docs/ROADMAP.md](docs/ROADMAP.md)

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

