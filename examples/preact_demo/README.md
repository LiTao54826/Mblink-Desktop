# Preact Demo | Preact 示例

## Overview | 概览

MBlink example application built with Preact.
一个基于 Preact 构建的 MBlink 示例应用。

## Run | 运行方式

The repository currently shows this demo running through `esm_loader`:
从当前仓库可见，该示例通过 `esm_loader` 运行：

```bash
cmake --build build --config Release --target esm_loader
build/bin/Release/esm_loader.exe examples/preact_demo/app.js
```

If `Release` is unavailable, use the actual build output path.
如果没有 `Release` 目录，请按实际构建结果调整路径。

## What It Demonstrates | 示例内容

- Preact-based component structure / 基于 Preact 的组件组织
- state updates and interaction / 状态更新与交互
- loading a JS app inside the MBlink runtime / 在 MBlink 运行时中加载 JS 应用

## Notes | 说明

- cross-platform runnable status still requires verification
  是否跨平台可运行仍需验证
- actual commands should follow repository build targets and scripts
  实际命令以仓库构建目标和脚本为准

