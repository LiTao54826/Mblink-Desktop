# API 模块

`core/api/` 提供当前仓库中的统一 C ABI 入口。

## 目录内容

- `mbink.h`：公开头文件
- `mbink.cpp`：实现文件
- `CMakeLists.txt`：构建接入

## 作用

这一层的职责是：
- 将内部 C++ 模块能力整理为 C 风格入口
- 为 Python 等语言绑定提供统一调用面
- 作为外部工具和宿主接入核心运行时的边界层

## 当前可确认的导出接口

从 `core/api/mbink.cpp` 可直接确认至少存在以下导出函数：
- `mbink_init()`
- `mbink_cleanup()`
- `mbink_version()`
- `mbink_create()`
- `mbink_create_ex()`
- `mbink_default_config()`
- `mbink_destroy()`
- `mbink_run()`
- `mbink_stop()`

## 当前限制

- 具体 API 形态仍以 `mbink.h` 与 `mbink.cpp` 的实际声明和实现为准
- 文档不应假设 Rust / Go / Node.js 绑定已可用
- 命名仍处于 `MBink` / `MBink` 混合状态

## 相关文档

- `docs/ARCHITECTURE.md`
- `docs/BINDINGS.md`
- `docs/KNOWN_LIMITATIONS.md`