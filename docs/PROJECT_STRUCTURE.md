# LightUI 项目结构

## 目录结构

```
lightui/
├── CMakeLists.txt                 # 根CMake配置
├── README.md                      # 项目说明
├── LICENSE                        # MIT许可证
├── .gitignore                     # Git忽略文件
├── .clang-format                  # C++代码格式配置
│
├── docs/                          # 文档目录
│   ├── PROJECT_OVERVIEW.md        # 项目概述
│   ├── ROADMAP.md                 # 开发路线图
│   ├── ARCHITECTURE.md            # 架构设计
│   ├── CODING_STANDARDS.md        # 编码规范
│   ├── API_REFERENCE.md           # API参考
│   ├── CONTRIBUTING.md            # 贡献指南
│   └── CHANGELOG.md               # 更新日志
│
├── core/                          # C++核心代码
│   ├── CMakeLists.txt
│   │
│   ├── window/                    # 窗口管理模块
│   │   ├── window.h
│   │   ├── window.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── quickjs/                   # QuickJS集成
│   │   ├── quickjs_runtime.h
│   │   ├── quickjs_runtime.cpp
│   │   ├── js_bindings.h
│   │   ├── js_bindings.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── dom/                       # DOM实现
│   │   ├── node.h
│   │   ├── node.cpp
│   │   ├── element.h
│   │   ├── element.cpp
│   │   ├── text.h
│   │   ├── text.cpp
│   │   ├── document.h
│   │   ├── document.cpp
│   │   ├── dom_bindings.h
│   │   ├── dom_bindings.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── event/                     # 事件系统
│   │   ├── event.h
│   │   ├── event.cpp
│   │   ├── mouse_event.h
│   │   ├── mouse_event.cpp
│   │   ├── keyboard_event.h
│   │   ├── keyboard_event.cpp
│   │   ├── event_system.h
│   │   ├── event_system.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── layout/                    # 布局引擎
│   │   ├── layout_engine.h
│   │   ├── layout_engine.cpp
│   │   ├── style_parser.h
│   │   ├── style_parser.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── render/                    # 渲染引擎
│   │   ├── renderer.h
│   │   ├── renderer.cpp
│   │   ├── text_renderer.h
│   │   ├── text_renderer.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── bridge/                    # 语言桥接
│   │   ├── bridge.h
│   │   ├── bridge.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── api/                       # C API
│   │   ├── lightui.h              # 公开的C API头文件
│   │   ├── lightui.cpp
│   │   └── CMakeLists.txt
│   │
│   └── utils/                     # 工具函数
│       ├── logger.h
│       ├── logger.cpp
│       ├── json.h
│       ├── json.cpp
│       └── CMakeLists.txt
│
├── third_party/                   # 第三方库
│   ├── CMakeLists.txt
│   ├── quickjs/                   # QuickJS源码
│   ├── skia/                      # Skia库
│   ├── sdl3/                      # SDL3库
│   ├── yoga/                      # Yoga布局引擎
│   └── json/                      # JSON库（nlohmann/json）
│
├── js/                            # JavaScript运行时
│   ├── runtime/                   # 运行时库
│   │   ├── console.js             # console API
│   │   ├── timers.js              # setTimeout/setInterval
│   │   └── fetch.js               # fetch API（可选）
│   │
│   ├── preact/                    # Preact集成
│   │   ├── preact.js              # Preact核心
│   │   ├── hooks.js               # Preact Hooks
│   │   └── compat.js              # React兼容层
│   │
│   └── polyfills/                 # Polyfills
│       ├── promise.js
│       └── array.js
│
├── bindings/                      # 语言绑定
│   ├── python/                    # Python绑定
│   │   ├── setup.py
│   │   ├── pyproject.toml
│   │   ├── lightui/
│   │   │   ├── __init__.py
│   │   │   ├── window.py          # ctypes版本
│   │   │   └── _lightui.cpp       # pybind11版本
│   │   ├── examples/
│   │   │   ├── hello_world.py
│   │   │   ├── todo_app.py
│   │   │   └── data_viewer.py
│   │   └── tests/
│   │       └── test_window.py
│   │
│   ├── rust/                      # Rust绑定
│   │   ├── Cargo.toml
│   │   ├── build.rs
│   │   ├── src/
│   │   │   └── lib.rs
│   │   └── examples/
│   │       └── hello_world.rs
│   │
│   ├── go/                        # Go绑定
│   │   ├── go.mod
│   │   ├── lightui.go
│   │   └── examples/
│   │       └── hello_world.go
│   │
│   └── nodejs/                    # Node.js绑定
│       ├── package.json
│       ├── binding.gyp
│       ├── src/
│       │   └── binding.cpp
│       └── examples/
│           └── hello_world.js
│
├── tools/                         # 工具
│   ├── cli/                       # 命令行工具
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── commands/
│   │       ├── create.cpp         # 创建项目
│   │       ├── build.cpp          # 构建项目
│   │       └── run.cpp            # 运行项目
│   │
│   ├── bundler/                   # 打包工具
│   │   ├── package.json
│   │   └── src/
│   │       └── index.js
│   │
│   └── devtools/                  # 开发者工具
│       └── inspector/             # 调试器
│
├── examples/                      # 示例项目
│   ├── cpp/                       # C++示例
│   │   ├── hello_world/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   └── ui/
│   │   │       └── app.jsx
│   │   ├── todo_app/
│   │   └── data_viewer/
│   │
│   ├── python/                    # Python示例
│   │   ├── hello_world/
│   │   │   ├── main.py
│   │   │   └── ui/
│   │   │       └── app.jsx
│   │   ├── todo_app/
│   │   ├── system_monitor/
│   │   └── db_manager/
│   │
│   ├── rust/                      # Rust示例
│   │   └── hello_world/
│   │
│   └── go/                        # Go示例
│       └── hello_world/
│
├── tests/                         # 测试
│   ├── unit/                      # 单元测试
│   │   ├── dom/
│   │   │   ├── test_element.cpp
│   │   │   └── test_document.cpp
│   │   ├── event/
│   │   │   └── test_event_system.cpp
│   │   └── layout/
│   │       └── test_layout_engine.cpp
│   │
│   ├── integration/               # 集成测试
│   │   ├── test_preact.cpp
│   │   └── test_rendering.cpp
│   │
│   └── benchmarks/                # 性能测试
│       ├── bench_rendering.cpp
│       └── bench_layout.cpp
│
├── scripts/                       # 构建脚本
│   ├── build.sh                   # Linux/macOS构建
│   ├── build.bat                  # Windows构建
│   ├── setup_deps.sh              # 安装依赖
│   └── run_tests.sh               # 运行测试
│
└── .github/                       # GitHub配置
    ├── workflows/                 # CI/CD
    │   ├── build.yml              # 构建工作流
    │   ├── test.yml               # 测试工作流
    │   └── release.yml            # 发布工作流
    │
    ├── ISSUE_TEMPLATE/            # Issue模板
    │   ├── bug_report.md
    │   └── feature_request.md
    │
    └── PULL_REQUEST_TEMPLATE.md   # PR模板
```

---

## 核心模块说明

### 1. core/window/
窗口管理模块，负责创建和管理应用窗口。

**主要文件**:
- `window.h/cpp`: Window类，封装SDL3窗口

### 2. core/quickjs/
QuickJS引擎集成，负责JavaScript代码执行。

**主要文件**:
- `quickjs_runtime.h/cpp`: QuickJS运行时封装
- `js_bindings.h/cpp`: JavaScript绑定工具

### 3. core/dom/
DOM API实现，提供轻量级DOM树。

**主要文件**:
- `node.h/cpp`: 基础Node类
- `element.h/cpp`: Element类
- `text.h/cpp`: Text节点
- `document.h/cpp`: Document类
- `dom_bindings.h/cpp`: DOM API的JavaScript绑定

### 4. core/event/
事件系统，处理用户输入事件。

**主要文件**:
- `event.h/cpp`: 基础Event类
- `mouse_event.h/cpp`: 鼠标事件
- `keyboard_event.h/cpp`: 键盘事件
- `event_system.h/cpp`: 事件系统管理器

### 5. core/layout/
布局引擎，使用Yoga计算布局。

**主要文件**:
- `layout_engine.h/cpp`: 布局引擎
- `style_parser.h/cpp`: CSS样式解析器

### 6. core/render/
渲染引擎，使用Skia渲染DOM树。

**主要文件**:
- `renderer.h/cpp`: 主渲染器
- `text_renderer.h/cpp`: 文本渲染器

### 7. core/bridge/
语言桥接层，连接宿主语言和JavaScript。

**主要文件**:
- `bridge.h/cpp`: 桥接管理器

### 8. core/api/
C API层，提供统一的C接口。

**主要文件**:
- `lightui.h`: 公开的C API头文件
- `lightui.cpp`: C API实现

---

## 构建系统

### CMake结构

```cmake
# 根CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(LightUI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(LIGHTUI_BUILD_TESTS "Build tests" ON)
option(LIGHTUI_BUILD_EXAMPLES "Build examples" ON)
option(LIGHTUI_BUILD_PYTHON_BINDING "Build Python binding" ON)

# 第三方库
add_subdirectory(third_party)

# 核心库
add_subdirectory(core)

# 语言绑定
if(LIGHTUI_BUILD_PYTHON_BINDING)
    add_subdirectory(bindings/python)
endif()

# 测试
if(LIGHTUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
if(LIGHTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

---

## 依赖管理

### 第三方库版本

```
QuickJS: 2024-01-13
Skia: m120 (2024)
SDL3: 3.0.0 (preview)
Yoga: 2.0.1
nlohmann/json: 3.11.3
Google Test: 1.14.0 (测试)
```

### 安装依赖

```bash
# Linux (Ubuntu/Debian)
sudo apt-get install cmake build-essential
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev

# macOS
brew install cmake
brew install sdl3

# Windows
# 使用vcpkg或手动下载
```

---

## 构建流程

### Linux/macOS

```bash
# 1. 克隆仓库
git clone https://github.com/yourusername/lightui.git
cd lightui

# 2. 初始化子模块
git submodule update --init --recursive

# 3. 创建构建目录
mkdir build
cd build

# 4. 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 5. 编译
cmake --build . -j8

# 6. 运行测试
ctest

# 7. 安装
sudo cmake --install .
```

### Windows

```powershell
# 使用Visual Studio
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# 或使用Ninja
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

---

## 输出产物

### 库文件

```
build/
├── lib/
│   ├── liblightui.so          # Linux共享库
│   ├── liblightui.dylib       # macOS共享库
│   ├── lightui.dll            # Windows动态库
│   └── liblightui.a           # 静态库
│
└── bin/
    ├── lightui-cli            # 命令行工具
    └── examples/              # 示例程序
```

### Python包

```
bindings/python/dist/
└── lightui-1.0.0-py3-none-any.whl
```

---

## 开发工作流

### 1. 创建新功能分支

```bash
git checkout -b feature/new-feature
```

### 2. 开发和测试

```bash
# 编译
cmake --build build

# 运行测试
cd build
ctest

# 运行示例
./build/examples/cpp/hello_world/hello_world
```

### 3. 代码格式化

```bash
# C++代码格式化
find core -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Python代码格式化
black bindings/python
```

### 4. 提交代码

```bash
git add .
git commit -m "feat(dom): add querySelector support"
git push origin feature/new-feature
```

### 5. 创建Pull Request

在GitHub上创建PR，等待代码审查。

---

## 发布流程

### 1. 更新版本号

```cmake
# CMakeLists.txt
project(LightUI VERSION 1.1.0)
```

### 2. 更新CHANGELOG.md

```markdown
## [1.1.0] - 2025-02-01

### Added
- querySelector support
- New event types

### Fixed
- Memory leak in event system
```

### 3. 创建Git标签

```bash
git tag -a v1.1.0 -m "Release v1.1.0"
git push origin v1.1.0
```

### 4. 构建发布包

```bash
# 构建所有平台
./scripts/build_release.sh

# 生成发布包
cpack
```

### 5. 发布到GitHub Releases

上传构建产物到GitHub Releases。

### 6. 发布Python包到PyPI

```bash
cd bindings/python
python setup.py sdist bdist_wheel
twine upload dist/*
```

---

## 文档生成

### C++ API文档 (Doxygen)

```bash
# 安装Doxygen
sudo apt-get install doxygen graphviz

# 生成文档
cd docs
doxygen Doxyfile

# 查看文档
open html/index.html
```

### Python API文档 (Sphinx)

```bash
# 安装Sphinx
pip install sphinx sphinx-rtd-theme

# 生成文档
cd bindings/python/docs
make html

# 查看文档
open _build/html/index.html
```

---

## 持续集成

### GitHub Actions工作流

- **build.yml**: 多平台构建
- **test.yml**: 运行测试
- **release.yml**: 自动发布

### 代码质量检查

- clang-tidy (C++)
- cppcheck (C++)
- ESLint (JavaScript)
- pylint (Python)

---

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

