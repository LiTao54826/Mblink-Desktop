<p align="center">
  <img src="./assets/logo.svg" alt="MBink Logo" width="140" />
</p>

# MBink

MBink 是一个基于 C++ 的桌面 UI / 应用框架仓库，当前代码中可确认接入了 QuickJS、SDL3、Skia、Lexbor，以及一套自有的 DOM、事件、布局、渲染、合成与工具链模块。

> 当前仓库仍处于开源整理阶段。以下说明以源码、CMake、示例和测试结构为准，不沿用旧阶段文档中的完成度口径。

## 当前可以确认的内容

- 使用 `CMake` 作为主构建系统
- `core/` 下已接入 window、dom、event、layout、render、quickjs、network、devtools、compositor 等模块
- 存在统一 C ABI：`core/api/mbink.h`（兼容头），底层实现仍为 `core/api/mbink.h`
- 顶层当前只接入 `bindings/python`
- 存在真实工具目标：`app_bundler`、`esm_loader`
- `app_loader` 当前未接入顶层构建
- `app_bundler` 可用于打包应用
- 编译后的 DLL 体积约 13MB，配合 `UPX` 可压到约 3MB
- UI 启动速度快，可做到秒开体验
- `modern_desktop_demo` 可作为当前演示示例
- 存在分层测试工程：unit / render / integration / property / performance
- 仓库内有较多示例与实验性 demo

## 当前不能直接承诺的内容

- Rust / Go / Node.js 绑定可用（当前仅能确认这些目录为预留空目录）
- 所有测试稳定通过
- 所有平台均已验证
- 项目对外命名已迁移到 `MBink`，并保留 `MBink / mbink` 兼容层

## 仓库现状说明

当前仓库内部仍保留部分 `MBink` / `mbink` 命名作为兼容层，例如：
- CMake 项目名
- target 名称
- C API 命名
- Python 包名

这表示仓库仍处于品牌和对外接口整理阶段。

## 目录概览

```text
core/        核心模块
bindings/    语言绑定（当前仅 Python 可确认有实现）
examples/    示例与实验 demo
tests/       测试工程
tools/       工具链目标
scripts/     下载、测试、辅助脚本
docs/        开源整理后的文档
```

## 构建

基础构建：

```bash
cmake -B build
cmake --build build --config Release
```

启用测试：

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

更多说明见：
- [docs/BUILD.md](docs/BUILD.md)
- [docs/TESTING.md](docs/TESTING.md)

## 演示展示

> 这里预留给项目展示图和演示视频，你后续可直接替换为自己的素材。

### 截图占位

```md
![MBink Screenshot 1](./assets/your-screenshot-1.png)
![MBink Screenshot 2](./assets/your-screenshot-2.png)
```

### 视频占位

```md
[观看演示视频](./assets/your-demo-video.mp4)
```

或直接放外链：

```md
[观看演示视频](https://your-video-url)
```

## 演示与打包

当前建议优先参考：
- `examples/modern_desktop_demo/`

该示例可作为仓库当前效果演示入口。

### 截图展示

> 这里预留项目截图位置，你可以后续直接替换为实际图片。

```md
![Modern Desktop Demo Screenshot 1](./assets/demo-screenshot-1.png)
![Modern Desktop Demo Screenshot 2](./assets/demo-screenshot-2.png)
```

### 视频展示

> 这里预留演示视频位置，你可以后续直接放视频链接、Bilibili 链接、YouTube 链接或 GIF。

```md
[演示视频：Modern Desktop Demo](https://your-video-link)
```

关于当前工具链，可明确说明：
- `esm_loader` 可用于加载和运行示例应用
- `app_bundler` 可用于应用打包
- 当前 DLL 编译产物体积约 13MB，配合 `UPX` 可压缩到约 3MB
- UI 启动速度快，可作为桌面应用快速启动方案

如果后续补充演示素材，可优先放在这一节。

## 绑定状态

- Python：已接入构建，存在真实源码与包结构
- Go：目录预留，当前为空
- Rust：目录预留，当前为空
- Node.js：目录预留，当前为空

更多说明见：[docs/BINDINGS.md](docs/BINDINGS.md)

## 架构与限制

- 架构概览见：[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- 当前限制见：[docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md)
- 贡献方式见：[docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## 开源整理原则

当前文档遵循以下原则：
- 只写代码、构建脚本、示例、测试能证明的事实
- 不再使用旧文档中的阶段进度、完成率和宣传性口径
- 逐步清理历史计划稿、总结稿和构建产物