# Design Document: Project Structure Cleanup

## Overview

本设计文档描述了 LightUI 项目结构优化的技术方案。目标是将项目重构为符合企业级 C++ 项目规范的结构，提高代码可维护性和开发效率。

## Architecture

项目结构优化后的目录布局：

```
LightUI/
├── .github/                    # GitHub 配置
├── .kiro/                      # Kiro 配置
├── .vscode/                    # VS Code 配置
├── bindings/                   # 语言绑定
├── core/                       # 核心库
│   ├── api/                    # 公共 API
│   ├── bridge/                 # JS 桥接
│   ├── compositor/             # 合成器
│   ├── devtools/               # 开发工具
│   ├── dom/                    # DOM 实现
│   ├── event/                  # 事件系统
│   ├── layout/                 # 布局引擎
│   ├── lexbor/                 # HTML/CSS 解析
│   ├── network/                # 网络模块
│   ├── quickjs/                # QuickJS 集成
│   ├── render/                 # 渲染模块
│   │   ├── animation/          # 动画相关 (新增)
│   │   ├── canvas/             # Canvas 实现
│   │   ├── css/                # CSS 相关 (新增)
│   │   ├── image/              # 图片处理
│   │   └── text/               # 文本渲染
│   ├── utils/                  # 工具函数
│   └── window/                 # 窗口管理
├── css/                        # 默认样式
├── docs/                       # 文档
├── examples/                   # 示例
├── js/                         # JavaScript 运行时
├── scripts/                    # 构建脚本
├── tests/                      # 测试
│   ├── integration/
│   ├── performance/
│   ├── property/
│   ├── render/
│   ├── test_utils/
│   └── unit/
├── third_party/                # 第三方依赖
├── tools/                      # 工具程序
├── .clang-format               # 代码格式化配置
├── .gitignore                  # Git 忽略配置
├── CMakeLists.txt              # 主构建配置
├── LICENSE                     # 许可证
└── README.md                   # 项目说明
```

## Components and Interfaces

### 1. 根目录清理

需要清理的文件：
- `*_test_results.xml` - 测试结果文件
- `test_*.exe`, `test_*.bat` - 临时测试文件
- `.devtools_state.json` - 开发工具状态

### 2. render 模块重组

将 render 模块中的文件按功能分组：

**animation/ 子目录：**
- animation.cpp/h
- animation_controller.cpp/h
- animation_applicator.cpp/h
- animation_optimizer.cpp/h
- animation_timeline.cpp/h
- keyframes.cpp/h
- easing_functions.cpp/h
- property_interpolation.cpp/h
- transition.cpp/h

**css/ 子目录：**
- css_value.cpp/h
- css_variables.cpp/h
- css_filters.cpp/h
- css_clip_path.cpp/h

### 3. CMakeLists.txt 修复

修复根目录 CMakeLists.txt 的编码问题，使用正确的 UTF-8 编码。

## Data Models

无新增数据模型，本次重构主要涉及文件组织和构建配置。

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

Property 1: File naming convention consistency
*For any* source file in the core/ directory, the file name should follow snake_case convention (lowercase letters, numbers, and underscores only)
**Validates: Requirements 3.1**

Property 2: CMake target naming consistency
*For any* CMake library target defined in core/ modules, the target name should follow the pattern lightui_{module_name}
**Validates: Requirements 3.2, 3.5**

## Error Handling

- 如果文件移动失败，构建系统应报告清晰的错误信息
- 如果 CMakeLists.txt 配置错误，cmake 配置阶段应失败并提示

## Testing Strategy

### Unit Tests
- 验证 .gitignore 包含所需的忽略模式
- 验证 CMakeLists.txt 编码正确

### Property-Based Tests
- 使用 Python 脚本验证文件命名规范
- 使用脚本验证 CMake target 命名规范

### Integration Tests
- 完整构建测试，确保重构后项目可正常编译
- 运行现有测试套件，确保功能不受影响
