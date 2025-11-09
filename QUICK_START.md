# 🚀 LightUI 快速开始指南

**5分钟了解项目，10分钟开始开发！**

---

## 📖 第一步：了解项目（5分钟）

### 1. 项目是什么？

**LightUI** 是一个轻量级跨语言UI框架，让你用 **JavaScript/Preact** 开发原生桌面应用。

```python
# Python示例
import lightui

app = lightui.Window("My App", 800, 600)

@app.bind("getData")
def get_data():
    return {"message": "Hello from Python!"}

app.load_ui("""
    import { render } from 'preact';
    function App() {
        return <h1>Hello World!</h1>;
    }
    render(<App />, document.body);
""")

app.run()
```

### 2. 核心特性

- 🪶 **体积小**: 10-15MB（vs Electron 100MB+）
- ⚡ **高性能**: Skia硬件加速，60fps
- 🎨 **易开发**: JavaScript/Preact + React生态
- 🌍 **跨平台**: Windows/macOS/Linux
- 🔗 **跨语言**: Python/C++/Rust/Go

### 3. 技术栈

| 组件 | 技术 | 大小 |
|------|------|------|
| JavaScript引擎 | QuickJS | ~600KB |
| 渲染引擎 | Skia | ~5-8MB |
| 窗口系统 | SDL3 | ~1-2MB |
| 布局引擎 | Yoga | ~1-2MB |
| UI框架 | Preact | ~5KB |

### 4. 项目状态

- **当前阶段**: Phase 0 - 项目结构创建 ✅
- **下一阶段**: Phase 1 - 核心框架（Week 1-12）
- **第一个里程碑**: M1 - 简单Preact应用可运行（12周后）

---

## 📁 第二步：浏览项目（2分钟）

### 重要文件

```bash
# 项目主页
README.md

# 项目状态（必读！）
PROJECT_STATUS.md

# 项目结构树
PROJECT_TREE.md

# 设置完成说明
SETUP_COMPLETE.md
```

### 重要目录

```bash
core/           # C++核心实现
├── window/     # 窗口管理 ✅
├── quickjs/    # JavaScript引擎 ✅
├── dom/        # DOM实现 🔄
├── event/      # 事件系统 ⏳
├── layout/     # 布局引擎 ⏳
├── render/     # 渲染引擎 ⏳
└── api/        # C API ✅

bindings/       # 语言绑定
└── python/     # Python绑定 ✅

examples/       # 示例应用
├── python/     # Python示例 ✅
└── cpp/        # C++示例 ✅

docs/           # 文档 ✅✅✅
```

### 核心文档

| 文档 | 内容 | 阅读时间 |
|------|------|----------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | 入门指南 | 10分钟 |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | 架构设计 | 20分钟 |
| [docs/ROADMAP.md](docs/ROADMAP.md) | 32周路线图 | 15分钟 |
| [docs/API_DESIGN.md](docs/API_DESIGN.md) | C API文档 | 15分钟 |
| [docs/PYTHON_API.md](docs/PYTHON_API.md) | Python API | 10分钟 |

---

## 💻 第三步：开始开发（10分钟）

### 选择你的角色

#### 🎯 角色1: 核心开发者（C++）

**任务**: 实现核心模块

```bash
# 1. 查看架构设计
cat docs/ARCHITECTURE.md

# 2. 选择一个模块
cd core/window/  # 或 quickjs/, dom/, event/, 等

# 3. 查看头文件
cat window.h

# 4. 实现TODO标记的功能
# 编辑 window.cpp

# 5. 编写测试
cd ../../tests/unit/
# 编辑 test_window.cpp

# 6. 运行测试
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

**推荐起点**:
- `core/window/window.cpp` - 窗口管理
- `core/quickjs/quickjs_runtime.cpp` - JavaScript运行时
- `core/dom/node.cpp` - DOM节点

#### 🐍 角色2: Python开发者

**任务**: 完善Python绑定和示例

```bash
# 1. 查看Python API文档
cat docs/PYTHON_API.md

# 2. 查看现有代码
cd bindings/python/lightui/
cat window.py

# 3. 完善实现
# 编辑 window.py，实现TODO标记的功能

# 4. 创建示例
cd ../../../examples/python/
# 创建新的示例应用

# 5. 测试
python hello_world.py
```

**推荐起点**:
- `bindings/python/lightui/window.py` - Window类实现
- `examples/python/` - 创建新示例

#### 📚 角色3: 文档贡献者

**任务**: 完善文档和教程

```bash
# 1. 查看文档索引
cat docs/DOCUMENTATION_INDEX.md

# 2. 选择要完善的文档
cd docs/

# 3. 添加内容
# - 教程
# - API文档
# - 最佳实践
# - 常见问题

# 4. 创建示例
cd ../examples/
# 创建示例并添加详细注释
```

#### 🧪 角色4: 测试工程师

**任务**: 编写测试用例

```bash
# 1. 查看现有测试
cd tests/unit/
cat test_window.cpp
cat test_quickjs.cpp

# 2. 实现测试用例
# 编辑测试文件，实现TODO标记的测试

# 3. 添加新测试
# 创建新的测试文件

# 4. 运行测试
cd ../../build/
ctest --output-on-failure
```

---

## 🎯 第四步：当前优先任务

### Phase 1: Week 1-2（当前）

**任务**: SDL3 + Skia集成

```bash
# 1. 下载SDL3
cd third_party/
git clone https://github.com/libsdl-org/SDL SDL3

# 2. 下载Skia
git clone https://github.com/google/skia
cd skia
python3 tools/git-sync-deps

# 3. 配置CMake
cd ../../
mkdir build && cd build
cmake ..

# 4. 实现window.cpp
cd ../core/window/
# 实现InitSDL(), CreateSDLWindow(), InitOpenGL(), InitSkia()

# 5. 测试
cd ../../build/
cmake --build .
./bin/test_window
```

### Phase 1: Week 3-4（下一步）

**任务**: QuickJS集成

```bash
# 1. 下载QuickJS
cd third_party/
wget https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz
tar xf quickjs-2024-01-13.tar.xz

# 2. 实现quickjs_runtime.cpp
cd ../core/quickjs/
# 实现InitRuntime(), Eval(), RegisterFunction()

# 3. 测试
cd ../../build/
./bin/test_quickjs
```

---

## 📋 开发检查清单

### 开始开发前

- [ ] 阅读 `PROJECT_STATUS.md`
- [ ] 阅读 `docs/GETTING_STARTED.md`
- [ ] 阅读 `docs/ARCHITECTURE.md`
- [ ] 阅读 `docs/CODING_STANDARDS.md`
- [ ] 配置开发环境

### 实现功能时

- [ ] 查看对应的头文件
- [ ] 理解TODO列表
- [ ] 遵循编码规范
- [ ] 添加注释
- [ ] 编写测试

### 提交代码前

- [ ] 运行测试
- [ ] 检查代码格式（clang-format）
- [ ] 更新文档
- [ ] 编写提交信息（遵循Conventional Commits）
- [ ] 创建PR

---

## 🛠️ 开发环境配置

### Linux

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y \
    cmake \
    g++ \
    libsdl3-dev \
    python3-dev \
    git \
    clang-format

# 克隆项目
git clone https://github.com/lightui/lightui
cd lightui

# 构建
mkdir build && cd build
cmake ..
cmake --build .
```

### macOS

```bash
# 安装依赖
brew install cmake sdl3 python3

# 克隆项目
git clone https://github.com/lightui/lightui
cd lightui

# 构建
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows

```powershell
# 安装依赖
choco install cmake visualstudio2022buildtools python3

# 克隆项目
git clone https://github.com/lightui/lightui
cd lightui

# 构建
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 📞 获取帮助

### 文档

- **入门**: [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)
- **架构**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **API**: [docs/API_DESIGN.md](docs/API_DESIGN.md)
- **贡献**: [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

### 社区

- **GitHub Issues**: 报告bug和提问
- **GitHub Discussions**: 讨论和交流
- **Discord**: (待创建)

---

## 🎉 开始你的贡献之旅！

1. **Fork项目**
2. **选择任务**（查看 `PROJECT_STATUS.md`）
3. **实现功能**
4. **提交PR**

**Let's build something amazing together!** 🚀

---

<div align="center">

**Made with ❤️ by the LightUI Team**

[GitHub](https://github.com/lightui/lightui) • [Docs](docs/) • [Examples](examples/)

</div>

