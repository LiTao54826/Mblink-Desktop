# Examples UI 统一改造计划

## 目标

统一不同语言绑定的示例体系，减少零散 demo，建立一套跨语言共享的 `examples-ui` 展示层，并由各语言绑定提供各自宿主入口与 backend 适配。

最终目标：
- 共享一套示例 UI 内容：`html / js / css / assets / specs`
- 各语言绑定只保留运行入口与能力适配
- 形成 `1 个 showcase + 少量最小示例` 的统一结构
- 降低示例维护成本，提升跨语言一致性与可对照性

## 命名与定位

目录名确定为：`bindings/examples-ui`

命名原因：
- `examples-assets` 语义过窄，容易被理解为仅静态资源
- `examples-ui` 更准确表达“共享示例前端展示层”
- 便于与各语言 binding 宿主代码职责分离

## 设计原则

1. 统一功能矩阵，不强制统一内部实现细节
2. 统一页面结构、命名、交互文案、返回数据格式
3. 展示层共享，宿主层分语言实现
4. 主示例负责完整展示，最小示例负责 smoke / 排错 / 快速上手
5. 先 Python / Rust 对齐，后续 Go 等绑定按相同结构接入
6. 优先最小改动迁移，避免一次性重构过大

## 目标目录结构

```text
bindings/
  examples-ui/
    showcase/
      index.html
      app.js
      pages/
      components/
      styles/
    hello/
      index.html
    bind/
      index.html
    shared_state/
      index.html
    shared/
      assets/
      specs/
      data/

  python/
    examples/
      showcase/
        main.py
      hello/
        main.py
      bind/
        main.py
      shared_state/
        main.py

  rust/
    mbink/
      examples/
        showcase.rs
        hello.rs
        bind.rs
        shared_state.rs
```

## 示例体系

### 主示例

`showcase`

按页面或模块展示不同能力，第一阶段建议包含：
- Overview
- Bind / Callback
- Shared State
- Native Controls
- Window
- Resources
- Tray / Events

其中第一批优先实现：
- Overview
- Bind / Callback
- Shared State
- Native Controls

### 最小示例

保留以下最小示例：
- `hello`
- `bind`
- `shared_state`

说明：
- `hello`：验证窗口创建与基础加载
- `bind`：验证 JS -> backend 回调链路
- `shared_state`：验证状态同步能力

如后续确有必要，再追加：
- `resources`
- `tray`

## 职责划分

### `bindings/examples-ui`
负责：
- 页面结构
- UI 组件
- JS 交互
- CSS 样式
- 示例文案
- 公共资源与规格定义

不负责：
- 具体 binding 启动代码
- 具体语言回调注册逻辑
- 具体宿主生命周期管理

### 各语言 binding 目录
负责：
- App 启动
- 加载 `examples-ui` 中的页面资源
- 注册 bind / callback
- 注入 shared state
- 对接 window / tray / resources / native controls 等能力

## 统一规范

1. 示例名统一：`showcase`、`hello`、`bind`、`shared_state`
2. 页面 ID、按钮文案、返回结构跨语言统一
3. 能力说明与 README 结构统一
4. showcase 页面导航与顺序统一
5. 同一功能使用同一套交互流程与字段名

## 实施阶段

### Phase 1：盘点与映射
- 盘点 Python 现有示例
- 盘点 Rust 现有示例
- 建立“现有示例 -> 新结构”的迁移映射
- 确定首批 showcase 页面与最小示例集

### Phase 2：搭建 `bindings/examples-ui`
- 建立 `showcase / hello / bind / shared_state` 基础目录
- 抽出共享 `html / js / css`
- 建立 `shared/assets` 与 `shared/specs`
- 定义前后端交互协议与页面约定

### Phase 3：Python 对齐
- 新建 Python 宿主入口：`showcase / hello / bind / shared_state`
- 把现有 Python 示例能力迁入新结构
- 用 Python 先跑通统一 UI 协议与主流程

### Phase 4：Rust 对齐
- 新建 Rust 宿主入口：`showcase / hello / bind / shared_state`
- 对齐 Python 的页面结构、按钮文案、返回格式与行为
- 补齐 Rust 对共享 UI 的加载与 backend 适配

### Phase 5：清理旧示例与文档
- 标记旧示例为迁移或废弃
- 清理重复价值低的旧 demo
- 更新 `docs/BINDINGS.md` 与各 binding README
- 统一示例入口说明与运行方式

## 迁移策略

采用“先并行、后替换”的方式：
- 先新增 `examples-ui` 与新入口
- 待 Python / Rust 跑通后，再迁移文档入口
- 最后清理旧目录与重复示例

避免一开始直接删除旧示例，降低迁移风险。

## 验收标准

1. Python / Rust 都有统一命名的 `showcase / hello / bind / shared_state`
2. `bindings/examples-ui` 成为共享 UI 唯一来源
3. showcase 首批页面可在 Python / Rust 下运行
4. README 与文档不再出现示例结构混乱或状态描述错误
5. 新增绑定时可以复用同一套 `examples-ui` 协议与页面资源
