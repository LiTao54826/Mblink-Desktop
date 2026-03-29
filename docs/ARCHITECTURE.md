# 架构概览

本文档只描述当前能从源码结构和 CMake 接入关系确认的架构事实。

## 总体结构

顶层 `core/CMakeLists.txt` 当前接入的模块包括：

- `window`
- `quickjs`
- `dom`
- `editing`
- `event`
- `layout`
- `render`
- `bridge`
- `api`
- `utils`
- `lexbor`
- `network`
- `devtools`
- `compositor`

这些模块通过 `lightui` 接口库统一聚合。

从当前 CMake 目标还可以确认：
- `lightui_api` 以共享库形式构建
- `lightui_window`、`lightui_render`、`lightui_devtools`、`lightui_network` 等以静态库形式参与聚合
- 构建后的 `lightui_api` 会复制到 Python 包输出目录，供绑定层使用

## 分层理解

### 1. 对外接口层

- `core/api/`
  - 提供统一 C ABI
  - 头文件入口：`core/api/lightui.h`
  - 实现文件：`core/api/lightui.cpp`

这层是跨语言接入的统一入口，面向 Python、Go、Rust、Node.js 等绑定场景。

### 2. 绑定与桥接层

- `core/bridge/`
- `bindings/python/`

其中 Python 绑定当前是唯一能从代码中确认已经落地的绑定实现。

### 3. 运行时与文档模型层

- `core/quickjs/`
- `core/dom/`
- `core/event/`
- `core/editing/`

这一层负责 JavaScript 执行、DOM 数据模型、事件调度以及编辑相关能力。

### 4. 布局与渲染层

- `core/layout/`
- `core/render/`
- `core/compositor/`

这一层负责布局计算、绘制和图层合成。

### 5. 平台与窗口层

- `core/window/`

从 CMake 可见，该模块会链接布局、合成、SDL3、Skia 以及 OpenGL / Windows 图形相关依赖。

### 6. 辅助与扩展层

- `core/network/`
- `core/devtools/`
- `core/utils/`
- `core/lexbor/`

## 工具链

顶层当前还接入了两个工具目录与目标：

- `tools/app_bundler/`
- `tools/esm_loader/`

说明仓库不仅包含运行时核心，也包含应用打包和资源/模块加载相关工具。

## 当前可以确认的架构特点

- 有明确的模块边界
- 有统一 C API 出口
- 有绑定层接入路径
- 有测试与工具链配套结构

## 当前仍需谨慎表述的部分

以下内容虽然在代码中存在大量实现痕迹，但对外文档中仍不应做过度承诺：

- 全平台成熟状态
- 所有子系统已稳定完成
- 所有示例与测试可直接运行
- 命名和接口已完全收口

更准确的做法是：按模块存在性、构建接入情况和实际可运行性逐步补充文档。