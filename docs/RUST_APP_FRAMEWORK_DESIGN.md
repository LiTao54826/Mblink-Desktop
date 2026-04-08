# MBink Rust 资源打包与应用框架分层设计草案

## 1. 背景

当前 `bindings/rust/mbink-sys` 的职责是原生库绑定与链接控制，例如通过 `build.rs` 处理：

- `MBINK_LIB_DIR`
- `MBINK_LIB_NAME`
- `MBINK_DLL_PATH`
- `.lib` / `.dll` 检测
- 运行时动态加载 cfg 切换

这一层本质上是 **sys crate**，职责清晰，应该尽量保持纯粹。

与此同时，桌面应用通常还需要：

- 将 `js/html/css/image/font` 等静态资源打包进最终 exe
- 运行时按路径提供资源
- 支持 ESM 模块加载与相对路径解析
- 提供类似 `app://` 的统一资源访问方式
- 提供开发期与生产期一致的资源组织方式

这类能力更接近 **应用框架层**，而不是 FFI 层。

因此需要一个清晰的设计，避免将上层资源系统错误塞入 `mbink-sys`，同时为后续扩展到类似 Tauri 的使用体验预留空间。

## 2. 设计目标

### 2.1 主要目标

1. 保持 `mbink-sys` 的职责单一，不引入前端资源打包逻辑
2. 为上层应用提供统一的资源嵌入与加载范式
3. 支持未来扩展出类似 Tauri 的完整使用体验
4. 在“统一体验”和“开发者自由度”之间保持平衡
5. 允许逐步演进，先从模板/辅助工具开始，而不是一步做成重量级框架

### 2.2 非目标

当前阶段不追求：

- 在 `mbink-sys` 中直接内建前端 bundler
- 自研完整 JS bundler 替代 esbuild/vite/rollup
- 强制统一所有应用的前端工程结构
- 一次性完成 CLI、配置系统、完整工程化工具链

## 3. 核心判断

### 3.1 为什么不能放进 `mbink-sys`

`mbink-sys` 当前适合承载的内容是：

- FFI 绑定
- native 库发现
- 链接与运行时加载策略
- 低层 ABI 对接

它不适合承载的内容包括：

- 前端资源扫描
- `js/html/css` 打包
- ESM 模块解析
- app 级资源协议
- dev/prod 资源切换
- 目录结构约定

如果将这些逻辑加入 `mbink-sys`，会带来以下问题：

1. **职责污染**
   - sys crate 不再纯粹
   - 底层绑定和应用框架逻辑耦合

2. **构建复杂度上升**
   - `build.rs` 需要处理资源目录、MIME、hash、manifest、压缩等
   - 用户在只想用 FFI 时也要承担额外复杂度

3. **场景不统一**
   - 不同项目使用不同前端工具链
   - 不同项目对资源组织方式要求不同
   - 不适合在 sys 层做强约束

### 3.2 为什么也不能完全放任开发者自实现

如果完全不提供统一方案，也会出现问题：

- 每个应用都要重复写一套 `build.rs`
- 重复写资源映射、MIME 推断、路径解析
- 缺少官方推荐结构，接入成本高
- 生态使用方式发散，不利于推广

因此更合理的方向是：

> **底层保持纯粹，上层提供统一脚手架、模板、抽象接口和可选辅助 crate。**

## 4. 总体设计原则

参考 Tauri 的经验，但不照搬其实现细节。重点学习其 **分层设计**：

- 底层 runtime/sys 保持清晰
- 上层 build/codegen/cli 分离
- 统一体验通过上层框架完成，而非污染底层

因此 MBink Rust 侧建议采用如下原则：

1. **sys 层只处理 native binding**
2. **资源系统放到 app/runtime 层**
3. **优先统一接口，不强制统一具体实现**
4. **先提供模板和 build helper，再考虑 CLI**
5. **默认支持资源嵌入，但不强制所有项目都使用该模式**

## 5. 分层架构设计

建议逐步形成如下 crate 分层：

```text
mbink-sys
mbink
mbink-build        (可选)
mbink-cli          (后续可选)
examples/
templates/
```

## 6. 各层职责定义

### 6.1 `mbink-sys`

职责：

- FFI 绑定
- 原生类型映射
- 链接与动态加载控制
- `build.rs` 中处理 native lib 搜索与 cfg

约束：

- 不处理前端资源
- 不依赖 app 目录结构
- 不提供 ESM / bundler 能力
- 不引入前端工具链假设

当前 `build.rs` 保持此方向是正确的。

### 6.2 `mbink`

定位：上层应用运行时封装。

职责建议：

- 对 `mbink-sys` 提供更易用的 Rust API
- 窗口、页面、生命周期、事件等高层封装
- scheme handler / resource handler 接口
- JS 注入、页面加载等高层能力
- 资源提供器抽象接口

这一层可以定义资源系统的统一接口，但不强制绑定唯一实现。

建议抽象：

```rust
pub struct Asset {
    pub mime: &'static str,
    pub bytes: &'static [u8],
}

pub trait AssetProvider {
    fn get(&self, path: &str) -> Option<Asset>;
}
```

如有 ESM 场景，还可以进一步定义：

```rust
pub struct ModuleSource {
    pub code: &'static str,
    pub mime: &'static str,
}

pub trait ModuleLoader {
    fn resolve(&self, specifier: &str, referrer: Option<&str>) -> Option<String>;
    fn load(&self, path: &str) -> Option<ModuleSource>;
}
```

这样可以统一接入面，但不统一具体工程实现。

### 6.3 `mbink-build`（可选）

定位：构建期辅助 crate。

职责建议：

- 扫描前端资源目录
- 生成 Rust 代码（如 `assets.rs`）
- 提供资源路径到字节内容的静态映射
- 可选生成 MIME 信息
- 可选支持简单 ESM manifest
- 可选预留 hash / compression / import-map 支持

这一层不负责真正的 JS bundling，只负责：

> **将“前端构建产物”嵌入 Rust 项目并生成可用映射代码。**

即推荐工作流为：

1. 前端使用 vite / esbuild / rollup 输出 `dist/`
2. `mbink-build` 将 `dist/` 嵌入 exe
3. `mbink` 运行时提供给 WebView / MBink

这样职责清晰、可组合性强。

### 6.4 `mbink-cli`（后续可选）

定位：面向开发者的脚手架与工程工具。

未来可选职责：

- 创建项目模板
- dev / prod 构建流程编排
- 前端构建调用
- Rust 编译协调
- 资源生成与嵌入
- 配置文件管理

注意：

- 这一步应放在后续
- 在上层模式和目录约定稳定前，不建议过早引入 CLI

## 7. 推荐演进路径

### 阶段一：先做文档与示例

目标：

- 明确推荐目录结构
- 提供最小可运行示例
- 验证资源打包与加载方案

建议内容：

- `examples/embedded-assets`
- `examples/esm-app`
- 文档说明如何将 `frontend/dist` 嵌入到 exe

这是成本最低、收益最高的阶段。

### 阶段二：提炼 `mbink-build`

当示例中的资源生成逻辑逐渐稳定后，将重复代码抽取为：

- `mbink-build`

目标：

- 降低开发者重复造轮子成本
- 固化推荐工作流
- 保持上层足够灵活

### 阶段三：在 `mbink` 中补统一接口

在确认典型场景稳定后，在 `mbink` 中提供：

- `AssetProvider`
- `ModuleLoader`
- scheme handler glue
- 默认嵌入式资源实现

此时统一的是接口与接入方式，而不是强制 bundler 细节。

### 阶段四：评估是否需要 `mbink-cli`

只有在以下条件满足时再考虑：

- 模板已稳定
- 构建流程已有共识
- 用户希望开箱即用
- 有足够维护资源长期维护 CLI

否则不建议过早投入。

## 8. 推荐目录结构

建议应用项目采用如下结构：

```text
bindings/rust/
  mbink-sys/
    build.rs
    src/
  mbink/
    src/
  mbink-build/          # 可选，后续引入
    src/
  examples/
    embedded-assets/
      build.rs
      src/
      frontend/
        dist/
```

对于实际 app 项目，推荐：

```text
app/
  Cargo.toml
  build.rs
  src/
    main.rs
    assets.rs
    esm_loader.rs
  frontend/
    dist/
      index.html
      app.js
      style.css
```

其中：

- `frontend/dist`：前端构建产物
- `build.rs`：扫描并生成资源映射
- `src/assets.rs`：包含由 `build.rs` 生成的资源访问代码
- `src/esm_loader.rs`：可选 ESM 模块解析逻辑

## 9. 资源打包推荐工作流

### 9.1 推荐方案

采用“前端 bundler + Rust 嵌入”的组合方案：

1. 使用 `vite/esbuild/rollup` 生成 `frontend/dist`
2. Rust `build.rs` 扫描 `dist`
3. 生成资源索引代码
4. 将内容通过 `include_bytes!` / `include_str!` 嵌入最终 exe
5. 运行时通过统一资源接口提供给 MBink

### 9.2 不推荐方案

当前阶段不推荐：

- 直接在 Rust 中实现完整 JS bundler
- 在 `mbink-sys/build.rs` 中处理前端资源
- 强依赖某个特定前端工程工具

## 10. 统一什么，不统一什么

### 10.1 建议统一的内容

1. 资源获取抽象接口
2. 运行时资源提供范式
3. 推荐目录结构
4. 示例项目组织方式
5. `build.rs` 的基本模式
6. scheme handler 的接入方式

### 10.2 不建议统一死的内容

1. 前端使用 vite 还是 esbuild
2. 是否使用 ESM
3. 是否使用 hash 文件名
4. 是否需要 import-map
5. 是否嵌入为单 exe
6. 是否走本地 HTTP server / 自定义 scheme / 直接注入

简言之：

> **统一接口与推荐实践，不统一业务工程实现。**

## 11. 与 Tauri 的关系

### 11.1 可借鉴之处

可借鉴 Tauri 的这些方向：

- 清晰分层
- 上层 build/codegen/cli 辅助
- 框架体验由上层提供
- 底层 runtime 保持明确边界

### 11.2 不建议直接照搬之处

不建议直接照搬：

- 全套重量级工程化体系
- 过早引入复杂配置系统
- 在项目尚未收敛前一次性做完整 framework

### 11.3 当前建议

MBink 当前更适合采用：

- **架构上参考 Tauri**
- **实现上逐步演进**
- **先有分层，再有工具链**
- **先模板，再 helper crate，再视情况演进到 CLI**

## 12. 当前结论

### 12.1 结论摘要

1. `mbink-sys` 应继续保持纯 native binding 职责
2. `app_bundler / esm_loader` 不应直接放入 `mbink-sys/build.rs`
3. 上层应提供统一脚手架、示例和可选辅助 crate
4. 可以参考 Tauri 的分层思路，但不必一步到位做成完整 framework
5. 最合理的路线是：
   - 先文档 + 示例
   - 再抽 `mbink-build`
   - 再补上层统一接口
   - 最后再评估 CLI

### 12.2 当前推荐执行方案

短期建议：

- 保持现有 `mbink-sys/build.rs` 边界不变
- 新增一个示例项目，展示资源嵌入和加载
- 在文档中明确推荐 app 层自己写 `build.rs`
- 后续将示例中稳定部分抽成 `mbink-build`

## 13. 后续实施建议

建议后续按以下顺序推进：

### P1
- 编写本设计文档
- 明确 crate 职责边界
- 确认目录结构建议

### P2
- 增加 `embedded-assets` 示例
- 增加 `esm-app` 示例
- 验证 `frontend/dist -> Rust embed -> MBink resource serve` 流程

### P3
- 抽象 `AssetProvider` / `ModuleLoader`
- 设计上层 runtime 接口

### P4
- 提炼 `mbink-build`
- 减少应用层重复代码

### P5
- 评估是否需要 `mbink-cli`
- 评估是否进入 framework 化阶段

## 14. 附：一句话设计原则

> **保持 `mbink-sys` 纯粹；在上层统一接口、模板与工具，而不是把应用资源系统硬塞进底层绑定。**
