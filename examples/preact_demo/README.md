# Preact 桌面应用示例

这是一个使用 Preact 构建的 MBink 示例应用。

## 当前运行方式

从仓库中的运行脚本可确认，当前示例通过 `esm_loader` 运行，而不是 `app_loader`。

```bash
cmake --build build --config Release --target esm_loader
build/bin/Release/esm_loader.exe examples/preact_demo/app.js
```

如果 Release 不可用，可按实际构建结果改为 Debug 路径。

## 示例内容

该示例用于展示：
- 基于 Preact 的组件组织方式
- 状态更新与交互
- 在 MBink 运行时中加载 JS 应用

## 说明

- 示例是否在所有平台都可直接运行，仍需验证
- 实际可用命令以仓库中的构建目标和运行脚本为准
- 如果后续示例运行方式变化，应同步更新此文档

