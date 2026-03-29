# Modern Desktop Demo

这是当前仓库推荐的演示示例，可用于展示 MBink 的桌面 UI 效果。

## 展示素材占位

> 这里预留给这个 demo 的截图和视频，你后续可直接替换。

### 截图占位

```md
![Modern Desktop Demo Screenshot](./assets/your-demo-shot.png)
```

### 视频占位

```md
[观看 Modern Desktop Demo 演示视频](https://your-video-url)
```

## 作用

该示例适合作为：
- 当前 UI 效果演示入口
- `esm_loader` 运行示例
- `app_bundler` 打包演示素材

## 展示素材占位

### 截图

> 你可以在这里放应用截图，例如首页、工作台、设置页、深色主题等效果图。

```md
![Demo Screenshot 1](./assets/demo-screenshot-1.png)
![Demo Screenshot 2](./assets/demo-screenshot-2.png)
```

### 视频

> 你可以在这里放录制视频链接、GIF 或平台页面链接。

```md
[观看演示视频](https://your-video-link)
```

## 当前运行方式

从仓库中的运行脚本和工具目标可确认，当前示例通过 `esm_loader` 运行：

```bash
cmake --build build --config Release --target esm_loader
build/bin/Release/esm_loader.exe examples/modern_desktop_demo/app.js
```

如果 Release 不可用，可按实际构建结果改为 Debug 路径。

## 当前可对外说明的特点

- UI 启动速度快，可用于展示秒开体验
- 可配合 `app_bundler` 进行应用打包
- 当前 DLL 编译产物体积约 13MB，配合 `UPX` 可压缩到约 3MB

## 目录结构

- `app.js`：示例入口
- `src/`：界面逻辑与组件
- `assets/`：静态资源
- `run_demo.bat`：本地运行脚本


