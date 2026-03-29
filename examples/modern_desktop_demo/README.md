# Modern Desktop Demo | 现代桌面示例

## Overview | 概览

Recommended showcase demo for the current repository.
当前仓库中推荐的展示型 demo。

## Purpose | 用途

This demo is suitable as:
该示例适合作为：

- a desktop UI showcase / 桌面 UI 效果展示
- an `esm_loader` runtime sample / `esm_loader` 运行样例
- an `app_bundler` packaging demo / `app_bundler` 打包示例

## Run | 运行方式

```bash
cmake --build build --config Release --target esm_loader
build/bin/Release/esm_loader.exe examples/modern_desktop_demo/app.js
```

If `Release` is unavailable, use the actual build output path.
如果没有 `Release` 目录，请按实际构建结果调整路径。

## Directory Layout | 目录结构

- `app.js` — entry / 入口
- `src/` — UI logic and components / 界面逻辑与组件
- `assets/` — static assets / 静态资源
- `run_demo.bat` — local run script / 本地运行脚本

## Notes | 说明

- media placeholders can be replaced with real screenshots or videos
  其中的展示素材占位可替换为真实截图或视频
- runnable status on all platforms still requires verification
  是否在所有平台可运行仍需验证


