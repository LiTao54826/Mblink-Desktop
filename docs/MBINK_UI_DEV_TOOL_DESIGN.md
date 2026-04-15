# MBink UI Dev Tool 设计文档

> 版本：v1.0 | 状态：P1 进行中（P0 已完成，info/read/write/build/build-status 已落地）


---

## 1. 定位与目标

### 1.1 核心定位

`mbink-ui-dev` 是一个 **Daemon + CLI + MCP 适配层 + 项目脚手架**。

核心是一个**长驻 Daemon 进程**，持有 MBink 窗口、构建缓存、文件监听器等所有跨调用状态。在此基础上同时暴露两套接入接口：
- **CLI**：`mbink-ui-dev <cmd>` 命令行，首期主力接口，配合 Skills 系统提示让任意 AI Agent 可用，也方便人工调试
- **MCP 适配层**：stdio JSON-RPC，供 Claude Desktop / Cursor 等原生支持 MCP 的工具接入，是 CLI 之上的薄封装

项目脚手架让各语言宿主（Python / Rust / Go / C++）的 MBink 项目 **30 秒内落地**。

### 1.2 与旧设计的根本区别

| 维度 | 旧设计 | 本设计 |
|------|--------|--------|
| 核心用户 | 人类开发者 | **AI Agent（人类通过 AI 使用）** |
| 首期接入方式 | — | **CLI + Skills（兼容任意 AI）** |
| MCP 位置 | 第 15 节附加功能 | **P2 薄适配层，复用 Daemon 逻辑** |
| snapshot | 调试辅助 | **AI 的主要信息摄入通道** |
| 脚手架 | 4 个 CLI 命令 | **含完整模板内容的落地方案** |
| 构建/热更 | 开发体验优化 | AI 迭代循环的必要基础设施 |

### 1.3 不在范围内

- 在线/云端编辑器
- JSX 编译器（使用外部 esbuild 二进制）
- 完整 HMR（只需 Remount 语义）
- 替换现有 `esm_loader` / `app_bundler`（复用它们）
- 生产部署流水线（只管开发阶段）

---

## 2. 设计原则

1. **AI First**：所有接口以"AI 能否高效准确地使用"为首要标准
2. **Daemon 是核心**：所有状态（窗口进程、构建缓存、日志缓冲、文件监听器）集中在 Daemon，CLI 和 MCP 都是薄适配层，不重复实现逻辑
3. **CLI 优先实现**：CLI 比 MCP 更早交付，兼容任意有 shell tool 的 AI Agent，也方便人工调试验证
4. **可观测性优先**：AI 必须能随时获得 UI 的完整可机读状态（snapshot），任何操作都要有确定性反馈
5. **最小依赖**：Daemon 只依赖 MBink 框架 + esbuild 二进制，不引入重型 Node.js 生态
6. **复用优先**：Dev Runtime 直接复用 `esm_loader` 核心逻辑，不重新实现窗口/JS引擎管理
7. **确定性语义**：每个命令/tool 调用都有明确的成功/失败返回，可机器解析

---

## 3. 整体架构

```
┌──────────────────────────────────────────────────────────────┐
│           AI Agent（任意，含无 MCP 支持的 Agent）             │
│                                                              │
│   有 shell tool 的 AI  ──CLI──►  mbink-ui-dev <cmd>          │
│   Claude / Cursor 等   ──MCP──►  mbink-ui-dev serve          │
└───────────┬──────────────────────────┬───────────────────────┘
            │ CLI (stdout JSON)         │ MCP (stdio JSON-RPC)
   ┌────────▼──────────┐     ┌─────────▼──────────┐
   │   CLI 客户端       │     │   MCP 适配层        │
   │   (参数解析 +      │     │   (薄封装，仅做      │
   │    结果格式化)     │     │    协议转换)        │
   └────────┬──────────┘     └─────────┬──────────┘
            │                          │
            └──────────┬───────────────┘
                       │ Unix socket / Named pipe (IPC)
          ┌────────────▼─────────────────────────────────┐
          │              mbink-ui-dev Daemon              │
          │           （长驻进程，持有所有状态）            │
          │                                              │
          │  ┌──────────────────┐  ┌──────────────────┐ │
          │  │   Command Router │  │  Orchestration   │ │
          │  │  open_project    │  │  - Build Queue   │ │
          │  │  snapshot_ui     │  │  - File Watcher  │ │
          │  │  build / reload  │  │  - Event Bus     │ │
          │  │  write/read_file │  │  - Log Buffer    │ │
          │  │  query / inspect │  └──────────────────┘ │
          │  │  click / input   │                       │
          │  │  eval_js         │                       │
          │  └──────────────────┘                       │
          └────────────┬───────────────┬────────────────┘
                       │               │ 子进程调用
             ┌─────────▼──────┐  ┌────▼────────────┐
             │   Dev Runtime  │  │  Build Driver   │
             │  (esm_loader   │  │  (esbuild CLI   │
             │   核心复用)    │  │   外部进程)     │
             │  +Snapshot引擎 │  │  +依赖图缓存    │
             │  +Console捕获  │  └─────────────────┘
             └───────────────┘
```

**进程模型**：
- `mbink-ui-dev daemon`：长驻后台进程，持有窗口、构建缓存、日志缓冲、文件监听器
- CLI 客户端：每次命令调用连接 daemon socket，发送请求，读取 JSON 结果后退出
- MCP 适配层：`mbink-ui-dev serve` 启动，持续监听 stdin，将 MCP tool call 转发给 daemon
- Dev Runtime：daemon 内部子进程，运行 MBink 窗口（QuickJS + Skia + SDL3）
- Build Driver：daemon 按需启动的 esbuild 子进程

---

## 4. MCP Server 规范

### 4.1 传输层

**首期：stdio JSON-RPC 2.0**（与 MCP 标准协议一致）

```
AI Agent  ──stdin──►  mbink-ui-dev  ──stdout──►  AI Agent
                           │
                        stderr  (日志，不混入协议流)
```

- 每条消息以换行符分隔的 JSON 对象
- 请求格式：`{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{...}}`
- 响应格式：`{"jsonrpc":"2.0","id":1,"result":{...}}`
- 错误格式：`{"jsonrpc":"2.0","id":1,"error":{"code":-32000,"message":"...","data":{...}}}`

**后期扩展（P4）**：HTTP + SSE，支持 IDE 插件场景。

---

### 4.2 MCP Tools 定义

#### 4.2.1 项目管理类

**`open_project`** — 打开一个 MBink 项目目录，启动 Dev Runtime 窗口

```json
{
  "name": "open_project",
  "description": "打开 MBink 项目目录，读取配置并启动 Dev Runtime 窗口。必须在其他工具前调用。",
  "inputSchema": {
    "type": "object",
    "required": ["path"],
    "properties": {
      "path": {
        "type": "string",
        "description": "项目根目录的绝对路径"
      },
      "window": {
        "type": "object",
        "description": "覆盖 mbink.config.json 中的窗口配置（可选）",
        "properties": {
          "width":  { "type": "integer" },
          "height": { "type": "integer" },
          "title":  { "type": "string" }
        }
      }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "project": {
    "name": "my-app",
    "root": "/abs/path/to/project",
    "config": { /* mbink.config.json 内容 */ },
    "template": "preact-jsx",
    "entry": "src/App.jsx",
    "window": { "width": 1280, "height": 800 }
  }
}
```

---

**`init_project`** — 从模板脚手架一个新项目

```json
{
  "name": "init_project",
  "description": "在指定目录用模板初始化一个新 MBink 项目（写入文件，不启动窗口）。",
  "inputSchema": {
    "type": "object",
    "required": ["path", "template"],
    "properties": {
      "path": { "type": "string", "description": "目标目录（必须为空或不存在）" },
      "template": {
        "type": "string",
        "enum": ["preact-jsx", "preact-ts", "vanilla-js", "python-host", "rust-host"],
        "description": "项目模板类型"
      },
      "name": { "type": "string", "description": "项目名（默认取目录名）" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "files_created": ["src/App.jsx", "src/index.html", "mbink.config.json", "mock/host.js", "package.json"],
  "next_step": "调用 open_project 打开此目录开始开发"
}
```

---

**`get_project_info`** — 获取当前项目信息和文件树

```json
{
  "name": "get_project_info",
  "description": "返回当前已打开项目的配置、文件树和运行状态。",
  "inputSchema": { "type": "object", "properties": {} }
}
```

返回：
```json
{
  "name": "my-app",
  "root": "/abs/path",
  "config": { /* mbink.config.json */ },
  "runtime_status": "running",
  "build_status": {
    "ok": true,
    "status": "success",
    "builder": "esbuild",
    "duration_ms": 145,
    "outputs": [".dist/App.js"],
    "build_log": "/abs/path/.dist/build.log",
    "errors": [],
    "warnings": []
  },
  "file_tree": [
    { "path": "src/App.jsx", "type": "file", "size": 1234 },
    { "path": "src/components", "type": "dir", "children": [...] }
  ]
}
```

---

#### 4.2.2 构建类

**`build`** — 触发单次构建（当前为最小可用实现）

> P1 当前实现：CLI 对应 `mbink-ui-dev build`，无参数；daemon 调用外部 `esbuild` 完成单次 build，固定输出 `.dist/App.js`，并将最近一次结果落盘到 `.dist/build.log`。`watch` / 增量构建仍是后续阶段。

```json
{
  "name": "build",
  "description": "执行一次 esbuild 构建，返回最近一次真实构建结果。P1 当前不接收 incremental/watch 参数。",
  "inputSchema": {
    "type": "object",
    "properties": {}
  }
}
```

返回：
```json
{
  "ok": true,
  "status": "success",
  "builder": "esbuild",
  "started_at": "2025-04-14T10:23:45Z",
  "finished_at": "2025-04-14T10:23:46Z",
  "duration_ms": 145,
  "entry_point": "app.js",
  "out_dir": ".dist",
  "outputs": [".dist/App.js"],
  "build_log": "/abs/path/.dist/build.log",
  "errors": [],
  "warnings": [],
  "raw_output": [".dist\\App.js  19.2kb", "Done in 4ms"]
}
```

失败时返回：
```json
{
  "ok": false,
  "status": "failed",
  "builder": "esbuild",
  "entry_point": "app.js",
  "out_dir": ".dist",
  "outputs": [".dist/App.js"],
  "build_log": "/abs/path/.dist/build.log",
  "errors": [],
  "warnings": [],
  "raw_output": ["error text..."]
}
```

---

**`get_build_status`** — 查询最近一次构建状态

```json
{
  "name": "get_build_status",
  "description": "返回 daemon 缓存的最近一次构建结果；若尚未构建则返回 status=not_built。",
  "inputSchema": { "type": "object", "properties": {} }
}
```

返回：
```json
{
  "ok": true,
  "status": "success",
  "builder": "esbuild",
  "started_at": "2025-04-14T10:23:45Z",
  "finished_at": "2025-04-14T10:23:46Z",
  "duration_ms": 145,
  "entry_point": "app.js",
  "out_dir": ".dist",
  "outputs": [".dist/App.js"],
  "build_log": "/abs/path/.dist/build.log",
  "errors": [],
  "warnings": []
}
```

未构建时：
```json
{ "ok": true, "status": "not_built" }
```

---

#### 4.2.3 运行时控制类

**`reload`** — 重载 UI

> P0 当前实现：仅支持无参数 `reload`，语义等价于重启当前 runtime。

```json
{
  "name": "reload",
  "description": "重载当前 Dev Runtime。P0 阶段固定执行 restart_runtime 语义，不接收 mode 参数。",
  "inputSchema": {
    "type": "object",
    "properties": {}
  }
}
```

返回：
```json
{ "ok": true, "mode": "restart_runtime" }
```

> `css` / `remount` / `restart` 多模式仍保留为后续阶段扩展方向，当前文档以下方语义说明作为设计预留。

**Reload 语义说明（后续扩展）**：

| mode | 触发场景 | 技术实现 | 状态保留 |
|------|----------|----------|----------|
| `css` | 仅 .css 文件变更 | 重新注入样式 | 全部保留 |
| `remount` | JS/JSX 逻辑变更 | 清理旧 Preact 树，重新 `import()` 入口，重挂载 root | JS 全局变量清空，HostBridge 保留 |
| `restart` | 配置变更 / mock 变更 / 出现崩溃 | 销毁并重建 QuickJS runtime + 重新初始化 bootstrap | 完全重置 |

---

**`eval_js`** — 在运行时执行任意 JS（逃生舱口）

```json
{
  "name": "eval_js",
  "description": "在 Dev Runtime 的 QuickJS 上下文中执行任意 JavaScript 表达式。仅用于其他接口无法满足时的特殊操作。",
  "inputSchema": {
    "type": "object",
    "required": ["code"],
    "properties": {
      "code": { "type": "string", "description": "要执行的 JS 代码（支持 await）" },
      "timeout_ms": { "type": "integer", "default": 5000 }
    }
  }
}
```

返回：
```json
{ "ok": true, "result": "any" }
```

说明：`eval_js` 的 `console.*` 输出与未捕获错误不会内联在响应中，而是分别通过 `get_console_logs` / `get_js_errors` 查询。


---


#### 4.2.4 UI 观测类

**`snapshot_ui`** — 获取完整 UI 状态快照（AI 的主要视觉输入）

```json
{
  "name": "snapshot_ui",
  "description": "获取当前 UI 的完整状态快照，包含 DOM 树、元素位置、交互属性和截图。这是 AI 理解当前 UI 状态的主要方式。",
  "inputSchema": {
    "type": "object",
    "properties": {
      "include_screenshot": { "type": "boolean", "default": true, "description": "是否包含 base64 截图" },
      "max_depth": { "type": "integer", "default": 20, "description": "DOM 树最大展开深度" },
      "root_selector": { "type": "string", "description": "仅快照指定元素的子树（CSS 选择器，默认为 body）" }
    }
  }
}
```

返回（见第 5 节 snapshot 数据格式规范）。

---

**`query_element`** — 查询匹配指定选择器的元素列表

```json
{
  "name": "query_element",
  "description": "返回匹配 CSS 选择器或 mbink-id 的所有元素及其位置、属性。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string", "description": "CSS 选择器 或 #node-id" },
      "include_children": { "type": "boolean", "default": false }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "count": 2,
  "elements": [
    {
      "node_id": "el-42",
      "tag": "button",
      "text": "Submit",
      "rect": { "x": 120, "y": 340, "w": 80, "h": 32 },
      "attrs": { "class": "btn btn-primary", "disabled": false },
      "computed_style": { "color": "#ffffff", "background": "#1a73e8" },
      "interactive": true,
      "visible": true
    }
  ]
}
```

---

**`inspect`** — 深度检查单个元素

```json
{
  "name": "inspect",
  "description": "深度检查单个元素，返回完整样式计算值、布局信息和组件状态（如果是 Preact 组件）。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string" }
    }
  }
}
```

返回：
```json
{
  "node_id": "el-42",
  "tag": "div",
  "rect": { "x": 0, "y": 0, "w": 240, "h": 600 },
  "layout": { "display": "flex", "flex_direction": "column", "padding": [16,16,16,16] },
  "computed_style": { "width": "240px", "height": "600px", "overflow": "hidden" },
  "component": { "name": "Sidebar", "props": { "collapsed": false }, "state": { "activeItem": 0 } }
}
```

---

**`get_console_logs`** — 获取运行时 console 输出

> P0 当前实现：CLI 对应 `mbink-ui-dev logs`，无筛选参数；daemon 优先读取 runtime 写出的结构化 JSON 缓冲，stdout 文件 tail 作为 fallback。

```json
{
  "name": "get_console_logs",
  "description": "返回 Dev Runtime 中 JS console 输出（含 log/warn/error/info）。P0 阶段不接收 level/since/limit 参数。",
  "inputSchema": {
    "type": "object",
    "properties": {}
  }
}
```

返回：
```json
{
  "ok": true,
  "source": "d:/.../mbink-ui-dev.runtime.console.json",
  "count": 2,
  "entries": [
    {
      "line": 1,
      "timestamp": "2025-04-14T10:23:45.123Z",
      "level": "log",
      "stream": "stdout",
      "message": "App mounted",
      "args": ["App mounted"]
    }
  ]
}
```

---

**`get_js_errors`** — 获取未捕获的 JS 运行时错误

> P0 当前实现：CLI 对应 `mbink-ui-dev errors`，无 `clear` 参数；返回自当前 runtime 生命周期内捕获的结构化 JS error，stderr 文件 tail 作为 fallback。

```json
{
  "name": "get_js_errors",
  "description": "返回当前 runtime 生命周期内捕获的未处理 JS 异常与 console.error 上报。P0 阶段不接收 clear 参数。",
  "inputSchema": {
    "type": "object",
    "properties": {}
  }
}
```

返回：
```json
{
  "ok": true,
  "source": "d:/.../mbink-ui-dev.runtime.errors.json",
  "count": 1,
  "errors": [
    {
      "line": 1,
      "timestamp": "2025-04-14T10:23:45.456Z",
      "level": "error",
      "stream": "stderr",
      "where": "console.error",
      "message": "boom from eval"
    }
  ]
}
```

---

#### 4.2.5 交互操作类

**`click`** — 点击元素

```json
{
  "name": "click",
  "description": "模拟点击指定元素（用于触发事件验证 UI 交互）。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string" },
      "button": { "type": "string", "enum": ["left", "right", "middle"], "default": "left" },
      "double": { "type": "boolean", "default": false }
    }
  }
}
```

返回：
```json
{ "ok": true, "element": { "node_id": "el-42", "tag": "button", "text": "Submit" } }
```

---

**`input_text`** — 向输入框输入文本

```json
{
  "name": "input_text",
  "description": "向指定输入元素填入文本（会先清空原有内容）。",
  "inputSchema": {
    "type": "object",
    "required": ["selector", "value"],
    "properties": {
      "selector": { "type": "string" },
      "value": { "type": "string" },
      "submit": { "type": "boolean", "default": false, "description": "填入后是否触发回车/提交" }
    }
  }
}
```

返回：
```json
{ "ok": true, "node_id": "el-15", "value_set": "hello world" }
```

---

**`scroll`** — 滚动元素或页面

```json
{
  "name": "scroll",
  "description": "滚动指定容器到目标位置。",
  "inputSchema": {
    "type": "object",
    "properties": {
      "selector": { "type": "string", "description": "滚动容器选择器，默认为根滚动区域" },
      "x": { "type": "integer", "default": 0 },
      "y": { "type": "integer", "default": 0 },
      "behavior": { "type": "string", "enum": ["instant", "smooth"], "default": "instant" }
    }
  }
}
```

---

#### 4.2.6 文件操作类

**`read_file`** — 读取项目文件内容

```json
{
  "name": "read_file",
  "description": "读取项目目录内文件的内容（路径相对于项目根目录）。",
  "inputSchema": {
    "type": "object",
    "required": ["path"],
    "properties": {
      "path": { "type": "string" },
      "encoding": { "type": "string", "enum": ["utf8", "base64"], "default": "utf8" }
    }
  }
}
```

返回：
```json
{ "ok": true, "path": "src/App.jsx", "content": "import { h } from 'preact';\n..." }
```

---

**`write_file`** — 写入/创建项目文件

```json
{
  "name": "write_file",
  "description": "写入或创建项目目录内的文件（路径相对于项目根目录）。会触发文件监听器。",
  "inputSchema": {
    "type": "object",
    "required": ["path", "content"],
    "properties": {
      "path": { "type": "string" },
      "content": { "type": "string" },
      "encoding": { "type": "string", "enum": ["utf8", "base64"], "default": "utf8" }
    }
  }
}
```

返回：
```json
{ "ok": true, "path": "src/App.jsx", "bytes_written": 1234, "triggered_watch": true }
```

---


### 4.3 MCP Resources 定义

Resources 是 AI 可以随时读取的状态数据，不触发副作用。

| URI | MIME Type | 说明 |
|-----|-----------|------|
| `ui://snapshot` | `application/json` | 最新 UI 快照（等同于调用 snapshot_ui） |
| `ui://console_logs` | `application/json` | 最新 console 输出（最近 500 条） |
| `ui://build_status` | `application/json` | 最近构建状态和错误列表 |
| `project://file_tree` | `application/json` | 项目文件树（2 层深度） |
| `project://config` | `application/json` | mbink.config.json 内容 |
| `project://file/{path}` | `text/plain` | 读取指定文件（路径 URL 编码） |

---

## 5. 核心数据格式规范

### 5.1 snapshot_ui 返回格式

```json
{
  "timestamp": "2025-04-14T10:23:45.789Z",
  "viewport": { "width": 1280, "height": 800, "dpr": 1.0 },
  "screenshot_base64": "iVBORw0KGgo...",
  "tree": {
    "node_id": "el-1",
    "tag": "div",
    "id": "app-root",
    "class": "app",
    "text": null,
    "rect": { "x": 0, "y": 0, "w": 1280, "h": 800 },
    "attrs": { "data-component": "App" },
    "interactive": false,
    "visible": true,
    "scroll": { "x": 0, "y": 0, "max_x": 0, "max_y": 0 },
    "children": [
      {
        "node_id": "el-2",
        "tag": "nav",
        "id": "sidebar",
        "class": "sidebar",
        "text": null,
        "rect": { "x": 0, "y": 0, "w": 240, "h": 800 },
        "attrs": {},
        "interactive": false,
        "visible": true,
        "scroll": { "x": 0, "y": 0, "max_x": 0, "max_y": 1200 },
        "children": [
          {
            "node_id": "el-3",
            "tag": "button",
            "id": null,
            "class": "nav-item active",
            "text": "Dashboard",
            "rect": { "x": 8, "y": 16, "w": 224, "h": 40 },
            "attrs": { "aria-selected": "true" },
            "interactive": true,
            "visible": true,
            "scroll": null,
            "children": []
          }
        ]
      }
    ]
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `node_id` | string | 唯一节点标识（本次快照内稳定，可用于 click/query） |
| `rect` | object | 元素在视口中的绝对位置和尺寸 |
| `interactive` | bool | 是否可交互（button/input/select/a 等） |
| `visible` | bool | 是否在视口内可见（非 display:none 且在视口范围内） |
| `scroll` | object \| null | 如果元素是滚动容器则包含当前滚动位置和最大值 |
| `text` | string \| null | 元素文本内容（仅叶子节点或短文本） |

---

### 5.2 错误结构（构建错误 / JS 错误统一格式）

```json
{
  "type": "build_error | js_error | runtime_error",
  "severity": "error | warning | info",
  "message": "人类可读的错误描述",
  "file": "src/App.jsx",
  "line": 23,
  "column": 5,
  "stack": "可选的堆栈跟踪字符串",
  "context": "错误相关的源码片段（可选）"
}
```

---

## 6. AI 开发工作流

### 6.1 标准 UI 开发循环

```
初始化
  │
  ├── 1. get_project_info        ← 了解项目结构和现有文件
  ├── 2. open_project (if needed) ← 启动 Dev Runtime 窗口
  │
迭代循环
  │
  ├── 3. snapshot_ui             ← 看当前 UI 状态（AI 的眼睛）
  │         │
  │    [理解当前状态]
  │         │
  ├── 4. write_file              ← 修改源代码
  ├── 5. build                   ← 触发构建（JSX → JS）
  │         │
  │    [检查 errors]
  │    [如有错误 → 4]
  │         │
  ├── 6. reload                  ← 重载 UI（mode: remount）
  ├── 7. snapshot_ui             ← 观测效果
  ├── 8. get_console_logs        ← 检查运行时错误
  │         │
  │    [达到目标？]
  │    [是 → 完成]
  │    [否 → 4]
  │
完成
```

### 6.2 工具调用优先级原则

1. **先 snapshot，再操作**：任何修改前先调用 snapshot_ui 了解当前状态
2. **改代码优先于 eval_js**：通过写文件+构建+重载，而非注入补丁
3. **用 query_element 定位，用 inspect 诊断**：不要靠猜 selector
4. **eval_js 是最后手段**：仅用于验证假设或执行一次性操作
5. **build 失败立即停**：不要 reload 一个构建失败的版本

### 6.3 典型场景示例：修复侧边栏宽度异常

```
# AI 发现问题
snapshot_ui → 看到 sidebar rect.w = 0（异常）

# 定位问题
query_element { selector: "#sidebar" }
  → rect: { w: 0 }, computed_style: { flex: "0 0 0px" }

# 读取源码
read_file { path: "src/components/Sidebar.jsx" }
  → 发现 style={{ width: collapsed ? 0 : 240 }} 中 collapsed 状态异常

read_file { path: "src/store/layout.js" }
  → 发现 collapsed 初始值为 true（应为 false）

# 修复
write_file { path: "src/store/layout.js", content: "...collapsed: false..." }

# 重建+重载
build { incremental: true }
  → ok: true, errors: []
reload { mode: "remount" }
  → ok: true, duration_ms: 45

# 验证
snapshot_ui → sidebar rect.w = 240 ✓
```

---

## 7. Dev Runtime 规范

### 7.1 与 esm_loader 的关系

Dev Runtime **直接复用** `tools/esm_loader/main.cpp` 的核心逻辑：

| 功能 | esm_loader 现状 | Dev Runtime 复用方式 |
|------|-----------------|---------------------|
| 窗口创建/管理 | ✅ 完整实现 | 直接复用 |
| QuickJS 初始化 | ✅ 完整实现 | 直接复用 |
| JS/HTML/字节码加载 | ✅ 三种模式 | 复用 JS 模式 |
| Preact 嵌入 | ✅ 全局对象注入 | 直接复用 |
| bootstrap.js | ✅ 完整实现 | 直接复用 |
| DevTools | ✅ 条件编译 | 复用并增强 |
| console 捕获 | ⚠️ 基础实现 | **增强：结构化 + 时间戳 + 缓存** |
| Snapshot 引擎 | ❌ 不存在 | **新增：DOM 遍历 + 截图** |
| IPC 通道 | ❌ 不存在 | **新增：与 MCP Server 通信** |

### 7.2 Snapshot 引擎实现

Snapshot 引擎通过 C++ 直接遍历 MBink DOM 树生成快照，不经过 JS：

```
MBink DOM (C++ 对象树)
    ↓  mbink_node_iterate()
Snapshot 遍历器
    ↓  mbink_node_get_rect() / get_attrs() / get_computed_style()
JSON 序列化
    ↓  + SDL3 截图 (PNG → base64)
snapshot_result_t
    ↓  IPC
MCP Server → AI Agent
```

**关键 C API 依赖**（来自 `core/api/mbink.h`）：
- `mbink_get_root_node()` — 获取根节点
- `mbink_node_get_children()` — 遍历子节点
- `mbink_node_get_rect()` — 获取布局位置
- `mbink_node_get_attrs()` — 获取 HTML 属性
- `mbink_node_get_computed_style()` — 获取计算样式
- `mbink_screenshot()` — 截取当前帧

---


## 8. Build Driver 规范

### 8.1 esbuild 调用配置

Build Driver 通过外部进程调用 `esbuild` CLI，不嵌入 Node.js：

```json
{
  "entryPoints": ["src/App.jsx"],
  "bundle": true,
  "format": "esm",
  "outdir": ".dist",
  "jsx": "transform",
  "jsxFactory": "Preact.h",
  "jsxFragment": "Preact.Fragment",
  "external": ["preact", "preact/hooks"],
  "sourcemap": "inline",
  "define": {
    "process.env.NODE_ENV": "\"development\""
  }
}
```

**注意**：Preact 和 PreactHooks 通过 MBink 全局对象注入，必须设为 external 并在 `src/index.html` 中通过全局 `Preact`/`PreactHooks` 引用。

### 8.2 构建产物结构

```
.dist/
  ├── App.js           ← 入口构建产物（包含所有本地依赖）
  ├── components/      ← 非打包模式下的组件产物
  └── build.log        ← 构建日志（机器可读 JSON lines）
```

### 8.3 文件监听与自动重建

在 `build { watch: true }` 模式下：

```
文件变更事件
    ↓
变更类型判断
  ├── .css → reload { mode: "css" }
  ├── .jsx/.js/.tsx/.ts → build (incremental) → reload { mode: "remount" }
  ├── mbink.config.json → reload { mode: "restart" }
  └── mock/*.js → reload { mode: "restart" }
```

**防抖策略**：同一文件 100ms 内的多次变更合并为一次构建请求。构建进行中的新变更进入队列，构建完成后立即处理。

---

## 9. 项目脚手架规范

### 9.1 可用模板

| 模板 | 说明 | 宿主语言 |
|------|------|----------|
| `preact-jsx` | JSX + Preact，使用全局 Preact 对象，无需 npm | 无宿主（纯 JS） |
| `preact-ts` | TypeScript + JSX + Preact，需要 esbuild 转译 | 无宿主（纯 JS） |
| `vanilla-js` | 纯 JS，无框架依赖，适合简单工具窗口 | 无宿主（纯 JS） |
| `python-host` | preact-jsx + Python 宿主集成模板 | Python |
| `rust-host` | preact-jsx + Rust 宿主集成模板 | Rust |

### 9.2 `preact-jsx` 模板目录结构

```
my-app/
  ├── mbink.config.json      ← 项目配置（见 9.3）
  ├── src/
  │   ├── App.jsx            ← 应用入口组件
  │   ├── index.html         ← 入口 HTML（注入全局 Preact）
  │   └── components/
  │       └── .gitkeep
  ├── mock/
  │   ├── host.js            ← Mock HostBridge（开发时模拟宿主）
  │   └── state.js           ← Mock 共享状态初始数据
  └── .dist/                 ← 构建产物（gitignore）
```

**`src/App.jsx` 起始内容：**

```jsx
// MBink App - 使用全局 Preact 对象（由框架注入，无需 import）
const { h, render } = Preact;
const { useState, useEffect } = PreactHooks;

function App() {
  const [count, setCount] = useState(0);

  return (
    <div class="app">
      <h1>MBink App</h1>
      <button onClick={() => setCount(c => c + 1)}>
        Count: {count}
      </button>
    </div>
  );
}

// MBink 通过 bootstrap.js 调用 __mbink_register_root__
__mbink_register_root__(App, document.getElementById('app'));
```

**`src/index.html` 起始内容：**

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>MBink App</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: system-ui, sans-serif; background: #1a1a2e; color: #eee; }
    .app { display: flex; flex-direction: column; align-items: center;
           justify-content: center; height: 100vh; gap: 16px; }
    button { padding: 8px 24px; font-size: 16px; border-radius: 6px;
             border: none; background: #4a9eff; color: #fff; cursor: pointer; }
    button:hover { background: #2d7dd2; }
  </style>
</head>
<body>
  <div id="app"></div>
  <script src="../.dist/App.js"></script>
</body>
</html>
```

**`mock/host.js` 起始内容：**

```javascript
// Mock HostBridge - 开发期间模拟宿主语言调用
// 生产环境由真实宿主（Python/Rust/Go）替换

const MockHost = {
  // 示例：模拟从宿主获取数据
  async getData(key) {
    console.log(`[MockHost] getData(${key})`);
    return { status: 'ok', value: `mock_${key}` };
  },

  // 示例：模拟宿主方法调用
  async doAction(action, payload) {
    console.log(`[MockHost] doAction(${action})`, payload);
    return { success: true };
  }
};

// 注册到 HostBridge（MBink 开发模式下自动加载）
if (typeof __mbink_mock_host__ !== 'undefined') {
  __mbink_mock_host__(MockHost);
}
```

### 9.3 `mbink.config.json` 完整规范

```json
{
  "$schema": "https://mbink.dev/schema/config/v1.json",
  "name": "my-app",
  "template": "preact-jsx",
  "entry": "src/index.html",
  "src_dir": "src",
  "out_dir": ".dist",
  "mock_dir": "mock",
  "window": {
    "title": "My MBink App",
    "width": 1280,
    "height": 800,
    "resizable": true,
    "gpu": true
  },
  "build": {
    "builder": "esbuild",
    "jsx_factory": "Preact.h",
    "jsx_fragment": "Preact.Fragment",
    "external": ["preact", "preact/hooks"],
    "sourcemap": true,
    "minify": false
  },
  "dev": {
    "watch": true,
    "debounce_ms": 100,
    "auto_reload": true,
    "devtools": true
  },
  "mcp": {
    "transport": "stdio",
    "snapshot_include_screenshot": true,
    "snapshot_max_depth": 20
  }
}
```

---


## 10. CLI 接口

`mbink-ui-dev` 是首期主力接口。AI Agent 通过 shell tool 调用 CLI 命令与 Daemon 通信；所有命令均输出机器可读的 JSON，方便 AI 直接解析。

```
# 初始化新项目
mbink-ui-dev init <path> --template <preact-jsx|preact-ts|vanilla-js|python-host|rust-host>

# 启动 Daemon（AI 使用前必须先调用，或由 open 命令自动触发）
mbink-ui-dev daemon start [--project <path>]  → 启动长驻 Daemon，输出 socket 路径
mbink-ui-dev daemon stop                      → 停止 Daemon
mbink-ui-dev daemon status                    → 查询 Daemon 运行状态（JSON）

# 启动 MCP 适配层（供 Claude Desktop / Cursor 等 MCP 原生工具接入）
mbink-ui-dev serve [--project <path>]         → 启动 stdio MCP Server（自动确保 Daemon 运行）

# 开发（人工调试用，自动启动 Daemon + 打开窗口 + 监听）
mbink-ui-dev dev <project-path>

# 单次构建
mbink-ui-dev build <project-path> [--watch]

# 检查项目配置
mbink-ui-dev inspect <project-path>
```

### 10.1 CLI 命令完整列表（P1 当前已落地部分）

当前命令集合以真实已落地实现为准。所有命令 stdout 输出 JSON，stderr 输出人类可读日志，`exit code 0` 表示成功。

```
# Daemon 管理
mbink-ui-dev daemon start [--project <path>]   → 启动 daemon，并返回状态 JSON
mbink-ui-dev daemon run                        → 前台运行 daemon
mbink-ui-dev daemon stop                       → 停止 daemon + runtime，并清理临时文件
mbink-ui-dev daemon status                     → 查询 daemon 状态（JSON）

# 项目/运行时
mbink-ui-dev open <project-path>               → 打开项目，必要时自动启动 daemon 与 runtime
mbink-ui-dev info                              → 查询当前项目配置、文件树与最近构建状态
mbink-ui-dev reload                            → 重启当前 runtime（当前固定语义）
mbink-ui-dev eval "<js code>"                  → 在 QuickJS 中执行 JS，返回结果（JSON）

# 文件操作
mbink-ui-dev read <path> [--encoding utf8|base64]
mbink-ui-dev write <path> [--content <text> | --from <file>]
# 或：stdin 输入
mbink-ui-dev write <path>

# 构建/观测
mbink-ui-dev build                             → 触发单次 build，返回真实构建结果
mbink-ui-dev build-status                      → 查询最近一次 build 结果
mbink-ui-dev snapshot                          → 获取当前真实 UI snapshot（JSON）
mbink-ui-dev logs                              → 获取结构化 console 输出（JSON）
mbink-ui-dev errors                            → 获取结构化 JS error 输出（JSON）
```

以下命令仍属于后续阶段规划，不应视为当前已实现能力：`build --watch`、`query`、`inspect`、`click`、`input`、`scroll`、`serve`、`dev`、`init`。

### 10.2 write 命令说明（P1 当前已实现）

`write` 已支持 stdin、`--from`、`--content` 三种输入方式。考虑到 shell 转义风险，以下仍是推荐调用约定：

```bash
# ✅ 推荐：AI 通过 heredoc 或 stdin pipe 传内容，完全避免转义
mbink-ui-dev write src/App.jsx - << 'EOF'
function App() {
  return <div class="app">Hello</div>;
}
EOF

# ✅ 也可以：AI 先用文件写工具写到临时文件，再用 write 导入
mbink-ui-dev write src/App.jsx --from /tmp/ai_generated.jsx

# ⚠️ 可用但不推荐：直接拼字符串（有转义风险）
mbink-ui-dev write src/App.jsx --content "..."
```

### 10.3 MCP 适配层配置（P2 后可用）

MCP 适配层是 CLI 之上的薄封装，§4 中定义的所有 tool schema 不变，底层转发给 Daemon：

在 Claude Desktop / Cursor 中配置：
```json
{
  "mcpServers": {
    "mbink-ui-dev": {
      "command": "mbink-ui-dev",
      "args": ["serve", "--project", "/path/to/project"],
      "env": {}
    }
  }
}
```

### 10.4 Skills 系统提示模板（P1 当前已落地部分）

Skills 是一段系统提示词，教会任意有 shell tool 的 AI Agent 如何操作 `mbink-ui-dev`。下面模板已经对齐当前真实实现：

```markdown
## MBink UI Dev Tool — Skills

你可以通过 shell 命令控制 MBink UI 开发环境。所有命令输出 JSON，exit code 0 表示成功。

### 会话启动
mbink-ui-dev open /path/to/project

### 当前可用命令
- `mbink-ui-dev info`
- `mbink-ui-dev read <path>`
- `mbink-ui-dev write <path>`
- `mbink-ui-dev build`
- `mbink-ui-dev build-status`
- `mbink-ui-dev snapshot`
- `mbink-ui-dev logs`
- `mbink-ui-dev errors`
- `mbink-ui-dev eval "1+1"`
- `mbink-ui-dev reload`
- `mbink-ui-dev daemon stop`

### 当前推荐工作流
1. `mbink-ui-dev open /path/to/project`
2. `mbink-ui-dev info`
3. `mbink-ui-dev snapshot`
4. `mbink-ui-dev read <path>` / `mbink-ui-dev write <path>` 修改代码
5. `mbink-ui-dev build`
6. `mbink-ui-dev reload`
7. 再次执行 `mbink-ui-dev snapshot` 验证效果
8. 如需补充定位，执行 `mbink-ui-dev logs` / `mbink-ui-dev errors`

### 关键原则
- **先 open，再 snapshot/info**：未打开项目时不要直接假设 runtime 已可用
- **snapshot 是主要观测入口**：logs/errors 用于补充运行时信息
- **优先走 read/write/build/reload 主链路**：`eval` 只用于少量临时验证
- **reload 当前是 restart_runtime 语义**：暂不支持 `--mode`
- **build 当前是单次构建**：`watch` 仍未实现，不要假设自动重建
```

---

## 11. 与现有 MBink 工具的关系

| 现有工具 | 关系 | 说明 |
|----------|------|------|
| `tools/esm_loader` | **Dev Runtime 的实现基础** | 直接复用其 C++ 核心，在其基础上增加 Snapshot 引擎和 IPC 通道 |
| `tools/app_bundler` | **生产打包独立工具** | 不被 mbink-ui-dev 替代；`build` tool 开发用，app_bundler 生产用 |
| `core/devtools/` | **Snapshot 引擎的能力基础** | DevToolsManager 的 DOM 访问能力被 Snapshot 引擎复用 |
| `js/runtime/bootstrap.js` | **直接复用** | 不修改；bootstrap.js 的 `__mbink_register_root__` 是脚手架模板的依赖 |
| `bindings/python` 等 | **集成目标（非依赖）** | `python-host` / `rust-host` 模板提供与各语言绑定集成的起点 |

---

## 12. 分阶段实施路线

### P0 — Daemon + CLI Core（AI 能"看见"和控制 UI）

**目标**：Daemon 启动，CLI 可用，AI Agent 通过 shell tool 连接运行中的 MBink 窗口，读取 UI 状态，触发简单操作。

**交付物**：
- [x] Daemon 进程框架（Windows Named Pipe IPC + 请求路由）
- [x] `daemon start/stop/status` 命令
- [x] `open` 命令（读配置 + 启动 Dev Runtime，JSON 输出）
- [x] `snapshot` 命令（真实 DOM snapshot + JSON 输出）
- [x] `logs` / `errors` 命令（优先读取 runtime 结构化 JSON，stdout/stderr tail 为 fallback）
- [x] `eval` 命令
- [x] `reload` 命令（P0 当前固定为 `restart_runtime` 语义）
- [x] Skills 系统提示文档（§10.4，已对齐当前 P0 CLI）

**验收标准**：至少完成以下回归并得到可机读 JSON 结果：
1. `mbink-ui-dev open <project>`
2. `mbink-ui-dev snapshot`
3. `mbink-ui-dev eval "console.log('hi from eval'); console.error('boom from eval'); 1+1"`
4. `mbink-ui-dev logs`
5. `mbink-ui-dev errors`
6. `mbink-ui-dev reload`
7. `mbink-ui-dev daemon stop`

其中：`snapshot` 需返回真实 UI 树；`eval` 需返回结果；`logs/errors` 需可读到结构化运行时输出；`reload` 需返回成功 JSON。

---

### P1 — Build Integration + Write（AI 能"写代码-构建-看效果"循环）

**目标**：完整的 AI 开发循环可以运行。

**交付物**：
- [x] `build` 命令（已落地单次 esbuild build，输出 `.dist/App.js` 与 `.dist/build.log`）
- [x] `build-status` 命令
- [x] `write` 命令（stdin pipe 模式 + `--from` 模式 + `--content` 模式）
- [x] `read` 命令
- [x] `info` 命令（项目信息 + 文件树 + 最近构建状态）
- [ ] `build --watch` 文件监听模式

**当前说明**：P1 主链路里 `info/read/write/build/build-status` 已完成并通过构建与 smoke；下一步是补齐 `watch`，再做 write → build → reload → snapshot 闭环验收。

**验收标准**：AI 能完整执行「write → build → reload → snapshot → 判断」循环，整个循环 < 10 秒。

---

### P2 — Scaffolding + MCP 适配层

**目标**：AI 能从零初始化新项目；Claude Desktop / Cursor 用户可通过 MCP 原生接入。

**交付物**：
- [ ] `init` 命令（模板文件写入）
- [ ] `preact-jsx` / `vanilla-js` / `python-host` / `rust-host` 模板完整内容
- [ ] `mbink.config.json` 解析和验证
- [ ] MCP 适配层（`serve` 命令，将 §4 定义的 tool schema 转发给 Daemon）
- [ ] Claude Desktop / Cursor 集成验证

**验收标准**：① CLI 路径：AI 执行 `init` + `open` + `build` + `reload`，从空目录到运行 UI < 30 秒；② MCP 路径：Claude Desktop 通过 MCP 完成同等操作。

---

### P3 — UI Precision Control（精确操作与自动化测试）

**目标**：AI 能精确定位、检查、操作任意 UI 元素。

**交付物**：
- [ ] `query_element` tool 实现（CSS 选择器匹配 + 元素属性）
- [ ] `inspect` tool 实现（计算样式 + 布局 + 组件状态）
- [ ] `click` / `input_text` / `scroll` tool 实现
- [ ] `highlight` tool（在 UI 上高亮指定元素，辅助调试）

**验收标准**：AI 能通过 CSS 选择器定位任意元素，读取其位置/样式，模拟点击并通过 snapshot 验证 UI 响应。

---

### P4 — Ecosystem（生态完善）

**目标**：覆盖更多使用场景和集成方式。

**交付物**：
- [ ] HTTP + SSE 传输层（支持 IDE 插件集成）
- [ ] `preact-ts` 模板（TypeScript 支持）
- [ ] MCP Notification：构建状态、文件变更事件推送给 AI
- [ ] snapshot 性能优化（懒加载子树 + max_depth 控制）
- [ ] VS Code 插件：在侧边栏显示 MBink UI 预览

---

## 13. 风险与缓解措施

| 风险 | 等级 | 缓解方案 |
|------|------|----------|
| snapshot 数据格式 AI 不能有效利用（信息不够或太多） | 高 | P0 阶段与真实 AI 交互验证格式，预留 `include_screenshot`/`max_depth` 调参 |
| remount 后 QuickJS 状态残留导致 UI 异常 | 中 | 参考 esm_loader 清理序列；提供 `restart` 作为保底 |
| esbuild 跨平台路径问题（Windows 路径分隔符） | 中 | Build Driver 统一使用 UTF-8 + 正斜杠规范化 |
| 大型 UI DOM 树 snapshot 性能（>1000 节点） | 低 | 默认 `max_depth: 20`；支持 `root_selector` 局部快照 |
| MCP 协议版本迭代（MCP spec 尚在演进） | 低 | Transport 层做好抽象，协议层独立，便于升级 |
| Daemon IPC 通道跨平台（Windows Named pipe vs Unix socket） | 中 | 抽象 IPC 层，Windows 用 Named pipe，Unix 用 Unix socket，接口一致 |
| Daemon 异常崩溃后 CLI 客户端挂起 | 中 | CLI 连接 socket 设 3s 超时，超时后提示用户重启 daemon |
| write 命令 heredoc 在某些 AI shell tool 中不可用 | 低 | 同时支持 `--from <tmpfile>` 模式；AI 可先 write_tmp_file 再 import |

---

## 14. 设计最高原则

> **让 AI Agent 能够独立完成从空目录到可运行产物的完整 MBink UI 功能开发，不需要任何人工干预。**

每一个 tool 的设计决策都应回答这个问题：
- AI 调用这个 tool 后，是否能得到足够的信息决定下一步？
- 这个 tool 的返回是否机器可解析、无歧义？
- AI 是否能通过这些 tool 的组合完成任意 UI 开发任务？

开发完成后的 UI 代码（src/ 目录）可以直接被 Python、Rust、Go 等各语言宿主通过 `app_bundler` 打包复用，实现"AI 写 UI，开发者写业务逻辑"的协作模式。
