# Architecture | 架构

## Core Modules | 核心模块

Modules currently included from `core/CMakeLists.txt` | 当前 `core/CMakeLists.txt` 已接入模块：

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

## Build-Level Structure | 构建层结构

| Area | Current structure | 说明 |
|---|---|---|
| Public C API | `core/api/mblink.h` | 对外 C API 入口 |
| C API implementation | `core/api/mblink.cpp` | C API 实现 |
| Shared library | `mblink_api` | 共享库 |
| Static libraries | `mblink_window`, `mblink_render`, `mblink_devtools`, `mblink_network`, etc. | 静态库聚合 |
| Python integration | `mblink_api` copied into Python package output | 会复制到 Python 输出目录 |
| Rust integration | `mblink_api` copied into Rust runtime package output | 会复制到 Rust 运行时目录 |
| Go integration | `mblink_api` DLL/import library copied into Go binding output | 会复制到 Go 绑定目录 |

## Layering | 分层

### API Layer | API 层

- `core/api/`
- unified C ABI for external consumers
  对外提供统一 C ABI
- entry header: `core/api/mblink.h`
  入口头文件：`core/api/mblink.h`

### Bridge and Binding Layer | 桥接与绑定层

- `core/bridge/`
- `bindings/python/`
- `bindings/rust/`
- `bindings/go/`
- Python, Rust, and Go should expose the same observable C API runtime behavior
  Python、Rust、Go 应暴露同一套可观察 C API 运行时行为

### Runtime and Document Model | 运行时与文档模型层

- `core/quickjs/`
- `core/dom/`
- `core/event/`
- `core/editing/`

### Layout and Rendering | 布局与渲染层

- `core/layout/`
- `core/render/`
- `core/compositor/`

### Platform and Windowing | 平台与窗口层

- `core/window/`
- current build links platform/windowing related dependencies through this area
  当前平台与窗口相关依赖通过该层接入

### Supporting Modules | 辅助模块

- `core/network/`
- `core/devtools/`
- `core/utils/`
- `core/lexbor/`

## Tooling | 工具链

Top-level tooling targets currently included | 顶层当前工具目标：

- `tools/app_bundler/`
- `tools/esm_loader/`

## Confirmed Architectural Properties | 当前可确认的架构特征

- modular structure exists under `core/`
  `core/` 下存在明确模块结构
- unified C API entry exists
  存在统一 C API 出口
- binding integration path exists
  存在绑定层接入路径
- test and tool targets are present in the repository
  仓库中存在测试与工具目标

## Non-Claims | 不应直接声称的内容

Current repository state does not justify claims that:
当前仓库状态不足以直接宣称：

- all subsystems are production-complete / 所有子系统已生产级完成
- all examples are runnable on all platforms / 所有示例在所有平台可运行
- all interfaces and naming are fully unified / 所有接口和命名都已完全统一
