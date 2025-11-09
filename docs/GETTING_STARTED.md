# LightUI 开发入门指南

欢迎加入LightUI项目！本文档将帮助你快速开始开发。

## 📚 文档导航

在开始之前，请先了解项目的核心文档：

### 核心文档

1. **[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)** - 项目概述
   - 了解项目目标和核心价值
   - 技术栈概览
   - 与其他框架的对比
   - 快速示例

2. **[ROADMAP.md](ROADMAP.md)** - 开发路线图
   - 32周详细开发计划
   - 6个主要阶段
   - 关键里程碑
   - 当前进度

3. **[ARCHITECTURE.md](ARCHITECTURE.md)** - 架构设计
   - 5层架构详解
   - 核心模块设计
   - 数据流和渲染流程
   - 性能优化策略

4. **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - 项目结构
   - 完整目录结构
   - 模块说明
   - 构建系统
   - 开发工作流

5. **[CODING_STANDARDS.md](CODING_STANDARDS.md)** - 编码规范
   - C++/JavaScript/Python规范
   - 命名约定
   - 代码格式
   - 最佳实践

### API文档

6. **[API_DESIGN.md](API_DESIGN.md)** - C API设计
   - 完整C API规范
   - 函数签名和用法
   - 错误处理
   - 使用示例

7. **[PYTHON_API.md](PYTHON_API.md)** - Python API文档
   - Python绑定API
   - 完整示例项目
   - 最佳实践
   - 常见问题

### 贡献指南

8. **[CONTRIBUTING.md](CONTRIBUTING.md)** - 贡献指南
   - 如何贡献代码
   - 开发环境设置
   - 提交流程
   - 代码审查

---

## 🚀 快速开始

### 第一步：了解项目

**花费时间：30分钟**

1. 阅读 [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) 了解项目目标
2. 查看 [ROADMAP.md](ROADMAP.md) 了解当前进度
3. 浏览 [ARCHITECTURE.md](ARCHITECTURE.md) 理解技术架构

### 第二步：设置开发环境

**花费时间：1-2小时**

#### Linux (Ubuntu/Debian)

```bash
# 1. 安装基础工具
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    clang-format \
    clang-tidy \
    libgl1-mesa-dev \
    libglu1-mesa-dev

# 2. 克隆仓库
git clone https://github.com/yourusername/lightui.git
cd lightui

# 3. 初始化子模块
git submodule update --init --recursive

# 4. 安装Python依赖
pip3 install -r requirements-dev.txt

# 5. 构建项目
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 6. 运行测试
ctest --output-on-failure
```

#### macOS

```bash
# 1. 安装Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. 安装依赖
brew install cmake sdl3 clang-format

# 3. 克隆和构建（同Linux）
git clone https://github.com/yourusername/lightui.git
cd lightui
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(sysctl -n hw.ncpu)
ctest
```

#### Windows

```powershell
# 1. 安装Visual Studio 2022（包含C++工具）
# 2. 安装CMake（从cmake.org下载）
# 3. 安装Git

# 4. 克隆仓库
git clone https://github.com/yourusername/lightui.git
cd lightui
git submodule update --init --recursive

# 5. 构建
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug

# 6. 运行测试
ctest -C Debug
```

### 第三步：运行示例

**花费时间：15分钟**

```bash
# C++示例
cd build/examples/cpp/hello_world
./hello_world

# Python示例
cd examples/python/hello_world
python main.py
```

---

## 🎯 选择你的贡献方向

根据你的兴趣和技能，选择合适的贡献方向：

### 1. 核心引擎开发（C++）

**适合：** 有C++经验，熟悉图形编程

**任务示例：**
- 实现DOM API
- 优化渲染性能
- 集成Yoga布局引擎
- 实现事件系统

**开始：**
1. 阅读 [ARCHITECTURE.md](ARCHITECTURE.md) 的核心模块部分
2. 查看 `core/` 目录下的代码
3. 选择一个模块开始贡献

**推荐任务（按难度）：**
- 🟢 简单：实现基础DOM API（createElement, appendChild）
- 🟡 中等：实现事件系统
- 🔴 困难：优化渲染性能

### 2. JavaScript运行时（QuickJS）

**适合：** 熟悉JavaScript引擎和C语言

**任务示例：**
- 实现JavaScript绑定
- 添加Web API（console, setTimeout）
- 优化JS-C++互操作
- 实现模块系统

**开始：**
1. 阅读 [ARCHITECTURE.md](ARCHITECTURE.md) 的QuickJS模块部分
2. 查看 `core/quickjs/` 目录
3. 研究QuickJS文档

### 3. 语言绑定（Python/Rust/Go）

**适合：** 熟悉多语言开发和FFI

**任务示例：**
- 完善Python绑定
- 实现Rust绑定
- 实现Go绑定
- 编写语言特定的示例

**开始：**
1. 阅读 [API_DESIGN.md](API_DESIGN.md) 了解C API
2. 阅读 [PYTHON_API.md](PYTHON_API.md) 了解Python绑定
3. 查看 `bindings/` 目录

### 4. UI框架集成（Preact/React）

**适合：** 熟悉前端开发和React生态

**任务示例：**
- 测试Preact兼容性
- 集成Ant Design
- 集成Material-UI
- 编写UI组件示例

**开始：**
1. 了解Preact和React差异
2. 查看 `js/` 目录
3. 测试现有组件库

### 5. 文档和示例

**适合：** 善于写作和教学

**任务示例：**
- 编写教程
- 完善API文档
- 创建示例项目
- 翻译文档

**开始：**
1. 阅读现有文档
2. 找出不清楚的地方
3. 提交改进建议

### 6. 测试和质量保证

**适合：** 注重细节，喜欢测试

**任务示例：**
- 编写单元测试
- 编写集成测试
- 性能测试
- Bug修复

**开始：**
1. 查看 `tests/` 目录
2. 运行现有测试
3. 添加缺失的测试

---

## 📋 当前优先级任务

根据 [ROADMAP.md](ROADMAP.md)，当前处于 **Phase 1: 核心框架开发**

### 高优先级（P0）

1. **SDL3 + Skia集成** (Week 1-2)
   - [ ] 创建基础窗口
   - [ ] 初始化Skia渲染上下文
   - [ ] 实现基础渲染循环

2. **QuickJS集成** (Week 3-4)
   - [ ] 集成QuickJS引擎
   - [ ] 实现基础JavaScript执行
   - [ ] 实现console API

3. **基础DOM API** (Week 5-6)
   - [ ] 实现Node/Element/Text类
   - [ ] 实现createElement/appendChild等9个核心API
   - [ ] 编写单元测试

### 中优先级（P1）

4. **Yoga布局引擎** (Week 7-8)
   - [ ] 集成Yoga
   - [ ] 实现CSS样式解析
   - [ ] 实现布局计算

5. **Skia渲染** (Week 9-10)
   - [ ] 实现DOM树渲染
   - [ ] 实现文本渲染
   - [ ] 实现基础样式（颜色、边框）

6. **事件系统** (Week 11-12)
   - [ ] 实现事件分发
   - [ ] 实现鼠标事件
   - [ ] 实现键盘事件

---

## 🛠️ 开发工具

### 推荐IDE

- **C++**: CLion, Visual Studio Code, Visual Studio
- **Python**: PyCharm, Visual Studio Code
- **JavaScript**: Visual Studio Code, WebStorm

### VS Code扩展

```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-python.python",
    "dbaeumer.vscode-eslint",
    "esbenp.prettier-vscode",
    "ms-vscode.cmake-tools"
  ]
}
```

### 调试配置

#### C++ (launch.json)

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug LightUI",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/examples/cpp/hello_world/hello_world",
      "args": [],
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb"
    }
  ]
}
```

---

## 📖 学习资源

### 核心技术

1. **QuickJS**
   - [官方文档](https://bellard.org/quickjs/)
   - [QuickJS源码](https://github.com/bellard/quickjs)

2. **Skia**
   - [官方文档](https://skia.org/docs/)
   - [Skia教程](https://skia.org/docs/user/sample/)

3. **SDL3**
   - [官方文档](https://wiki.libsdl.org/SDL3/FrontPage)
   - [SDL教程](https://lazyfoo.net/tutorials/SDL/)

4. **Yoga**
   - [官方文档](https://yogalayout.com/)
   - [Flexbox指南](https://css-tricks.com/snippets/css/a-guide-to-flexbox/)

5. **Preact**
   - [官方文档](https://preactjs.com/)
   - [Preact vs React](https://preactjs.com/guide/v10/differences-to-react/)

### 相关项目

- **Electron**: 了解跨平台桌面应用开发
- **Tauri**: 了解轻量级桌面应用框架
- **Flutter**: 了解现代UI框架设计
- **React Native**: 了解原生UI渲染

---

## 💬 获取帮助

### 遇到问题？

1. **查看文档**
   - 先查看相关文档
   - 搜索已有Issue

2. **提问**
   - [GitHub Discussions](https://github.com/lightui/lightui/discussions)
   - [Discord服务器](https://discord.gg/lightui)

3. **报告Bug**
   - 使用[Bug报告模板](https://github.com/lightui/lightui/issues/new?template=bug_report.md)

### 联系方式

- **GitHub**: [@lightui](https://github.com/lightui)
- **Email**: dev@lightui.dev
- **Discord**: [加入服务器](https://discord.gg/lightui)

---

## ✅ 检查清单

在开始贡献之前，确保：

- [ ] 阅读了 [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
- [ ] 了解了 [ROADMAP.md](ROADMAP.md) 的当前阶段
- [ ] 理解了 [ARCHITECTURE.md](ARCHITECTURE.md) 的核心设计
- [ ] 熟悉了 [CODING_STANDARDS.md](CODING_STANDARDS.md)
- [ ] 设置好了开发环境
- [ ] 成功构建并运行了测试
- [ ] 运行了示例项目
- [ ] 阅读了 [CONTRIBUTING.md](CONTRIBUTING.md)
- [ ] 选择了贡献方向
- [ ] 加入了社区讨论

---

## 🎉 欢迎贡献！

感谢你对LightUI项目的兴趣！我们期待你的贡献。

**下一步：**

1. 选择一个任务开始
2. 创建功能分支
3. 编写代码和测试
4. 提交Pull Request

**记住：**
- 不要害怕提问
- 从小任务开始
- 遵循编码规范
- 编写测试
- 保持沟通

祝你编码愉快！🚀

