# Window Module | 窗口模块

## Overview | 概览

`core/window/` provides repository-visible window and window-manager infrastructure based on SDL3.  
`core/window/` 提供基于 SDL3 的窗口与窗口管理基础设施。

## Responsibilities | 职责

- create and destroy windows  
  创建与销毁窗口
- manage window properties and state  
  管理窗口属性与状态
- coordinate rendering surfaces and frame boundaries  
  协调渲染表面与帧边界
- expose document/runtime hosting entry points for UI content  
  为 UI 内容暴露文档与运行时宿主入口
- support multi-window management  
  支持多窗口管理

## Repository-Visible Files | 仓库可见文件

- `window.h` / `window.cpp` — window abstraction / 窗口抽象
- `window_manager.h` / `window_manager.cpp` — multi-window management / 多窗口管理
- `CMakeLists.txt` — build integration / 构建接入

## Dependencies | 依赖关系

Window-related code in the repository visibly interacts with:  
从仓库可见，窗口相关代码与以下模块存在交互：

- `SDL3`
- `Skia`
- `core/dom`
- `core/quickjs`
- `core/render`
- `core/event`

## Notes | 说明

- exact public methods and lifecycle rules must follow the source headers and implementations  
  具体公开方法与生命周期规则应以头文件和实现为准
- cross-platform behavior still requires validation on real targets  
  跨平台行为仍需在真实目标平台验证
- this document should not be used to imply every windowing feature is production-complete  
  本文档不应暗示所有窗口特性都已达到生产级

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/event/README.md`
- `core/render/README.md`
- `docs/KNOWN_LIMITATIONS.md`

