# Bridge Module | 桥接模块

## Overview | 概览

`core/bridge/` is the bridge layer between the public C API and internal C++ objects.  
`core/bridge/` 是公开 C API 与内部 C++ 对象之间的桥接层。

## Responsibilities | 职责

- convert between C handles and C++ objects  
  在 C 句柄与 C++ 对象之间转换
- capture exceptions before they cross the C ABI boundary  
  在异常跨越 C ABI 边界前进行捕获
- manage object lifetime around exported handles  
  管理导出句柄相关的对象生命周期
- provide helper glue for callbacks and ownership transfer  
  为回调和所有权转移提供辅助胶水层

## Repository-Visible Files | 仓库可见文件

- `bridge.h` — bridge declarations / 桥接声明
- `bridge.cpp` — bridge implementation / 桥接实现
- `host_bridge.h` / `host_bridge.cpp` — host-side bridge helpers / 宿主桥接辅助
- `state_manager.h` / `state_manager.cpp` — state coordination / 状态协调
- `CMakeLists.txt` — build integration / 构建接入

## Typical Patterns | 常见模式

- wrapping `std::shared_ptr` inside opaque handles  
  用不透明句柄包装 `std::shared_ptr`
- translating C++ exceptions into error codes  
  将 C++ 异常转换为错误码
- releasing resources through explicit destroy paths  
  通过显式销毁路径释放资源

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/utils`
- `core/window`
- `core/dom`
- `core/quickjs`

Used by | 被依赖：

- `core/api`

## Notes | 说明

- handle ownership rules must follow the actual implementation  
  句柄所有权规则应以实际实现为准
- this layer is implementation-critical and should stay conservative  
  该层属于关键基础设施，应保持保守演进
- thread-safety should not be assumed unless the source explicitly guarantees it  
  除非源码明确保证，否则不应假设线程安全

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/api/README.md`
- `docs/KNOWN_LIMITATIONS.md`

