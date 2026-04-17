# MBink UI Dev Tool 设计文档

> 版本：v1.1 | 状态：P2 已完成（P0/P1 已完成；init/serve 已落地；MCP tools/resources 最小完整集已落地；多项目并行管理、`.devui` 项目标识、自动项目定位、per-project daemon/runtime 隔离已落地；Claude Desktop / Cursor 配置接入方式已补齐，真实桌面端联调需在外部客户端环境继续验证）


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
- `mbink-ui-dev daemon`：按项目启动的长驻后台进程；每个项目一个 daemon，持有该项目独立的窗口、构建缓存、日志缓冲、文件监听器
- CLI 客户端：每次命令调用按项目解析目标，连接对应 daemon pipe，发送请求，读取 JSON 结果后退出
- MCP 适配层：`mbink-ui-dev serve` 启动，持续监听 stdin；会话内维护 `active_project`，并将 tool/resource 调用转发给对应项目 daemon
- Dev Runtime：daemon 内部子进程，运行 MBink 窗口（QuickJS + Skia + SDL3）
- Build Driver：daemon 按需启动的 esbuild 子进程

**多项目隔离（当前已实现）**：
- 每个项目根目录写入独立 `.devui`，记录 `project_id` / `runtime_id`
- daemon state、Named pipe、runtime snapshot/command/response/console/errors/stdout/stderr/lifecycle 全部按 `project_id` 隔离
- Windows pipe 命名规则为 `\\.\\pipe\\mbink-ui-dev-<sanitized_project_id>`
- CLI / MCP 项目解析顺序为：`--project` → 位置参数中的项目路径 → 当前目录/父目录 `.devui` → 当前目录/父目录 `mbink.config.json`（必要时自动创建 `.devui`）→ 仅存在一个 managed project 时自动回退
- 若同时存在多个 managed project 且无法唯一定位，则返回错误，要求显式传 `--project <path>`

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
  "description": "打开 MBink 项目目录，读取配置并启动 Dev Runtime 窗口。path / project_root 可省略；省略时按当前目录自动解析项目。成功后会把该项目设为当前 MCP 会话的 active_project。",
  "inputSchema": {
    "type": "object",
    "properties": {
      "path": {
        "type": "string",
        "description": "项目根目录的绝对路径（可选）"
      },
      "project_root": {
        "type": "string",
        "description": "项目根目录的绝对路径（`path` 的兼容别名，可选）"
      },
      "window": {
        "type": "object",
        "description": "覆盖 mbink.config.json 中的窗口配置（当前实现暂未消费，仅保留兼容位）",
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
    "window": { "width": 800, "height": 600 }
  },
  "daemon": {
    "project_id": "project-abc123",
    "runtime_id": "project-abc123",
    "running": true
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
  "next_step": "调用 open_project 或 CLI `open` 打开此目录开始开发"
}
```

---

**`get_project_info`** — 获取当前项目信息和文件树

```json
{
  "name": "get_project_info",
  "description": "返回当前 MCP active_project 或按 CLI 规则解析到的当前项目配置、文件树和运行状态。",
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

**`build`** — 触发构建（支持单次 / watch）

> P1 当前实现：CLI 对应 `mbink-ui-dev build` 与 `mbink-ui-dev build --watch`；daemon 调用外部 `esbuild` 完成构建，固定输出 `.dist/App.js`，并将最近一次结果落盘到 `.dist/build.log`。当 `watch=true` 时，watcher 由 daemon 持有，后台线程轮询项目文件快照并以 100ms debounce 合并变更；文件变更后自动 rebuild，成功后自动执行 `restart_runtime`，同时将 watch 状态写入 `DaemonState.watch` / state json / `build-status`。

```json
{
  "name": "build",
  "description": "执行一次 esbuild 构建；当 watch=true 时进入 daemon 持有的持续监听模式，并返回最近一次真实构建结果。",
  "inputSchema": {
    "type": "object",
    "properties": {
      "watch": {
        "type": "boolean",
        "description": "是否启用 daemon 持有的 watch 模式"
      }
    }
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

> 当前限制：在 Windows `cmd.exe` 下直接传入复杂 JS 时，仍可能受到 shell quoting 影响；复杂表达式更建议通过 MCP tool 调用或 PowerShell 执行。


---


#### 4.2.4 UI 观测类

**`snapshot_ui`** — 获取完整 UI 状态快照（AI 的主要视觉输入）

```json
{
  "name": "snapshot_ui",
  "description": "获取当前 UI 的完整状态快照，包含 DOM 树、元素位置、交互属性。这是 AI 理解当前 UI 状态的主要方式。",
  "inputSchema": {
    "type": "object",
    "properties": {
      "include_screenshot": { "type": "boolean", "default": false, "description": "预留字段，当前实现未返回截图" },
      "max_depth": { "type": "integer", "default": 20, "description": "预留字段，当前实现未消费" },
      "root_selector": { "type": "string", "description": "预留字段，当前实现固定导出 body/documentElement" }
    }
  }
}
```

返回（见第 5 节 snapshot 数据格式规范）。

> 当前实现补充：runtime 启动后会主动生成首个 snapshot；daemon 在首次读取 snapshot 文件未就绪时会短轮询等待，因此 `open` 后第一次 `snapshot_ui` 不应再回落到 `stub-root`。

---

**`query_element`** — 查询匹配指定选择器的元素列表

```json
{
  "name": "query_element",
  "description": "返回匹配 CSS selector 的所有元素摘要，包括位置、属性、可见性和滚动信息。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string", "description": "CSS selector；`body` / `html` 支持直接查询" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "result": {
    "selector": "span",
    "count": 2,
    "matches": [
      {
        "tag": "span",
        "id": "",
        "class_name": "",
        "text": "1 active / 0 done",
        "attrs": { "data-preact-id": "10" },
        "rect": { "x": 20, "y": 140, "w": 110, "h": 24, "top": 140, "right": 130, "bottom": 164, "left": 20 },
        "interactive": false,
        "visible": true,
        "value": null,
        "scroll": { "x": 0, "y": 0, "max_x": 0, "max_y": 0 }
      }
    ]
  }
}
```

---

**`inspect`** — 深度检查单个元素

```json
{
  "name": "inspect",
  "description": "检查单个元素，返回元素摘要、部分 computed style 和 outerHTML。",
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
  "ok": true,
  "result": {
    "selector": "#todo-input",
    "found": true,
    "element": {
      "tag": "input",
      "id": "todo-input",
      "class_name": "",
      "text": "",
      "attrs": { "id": "todo-input", "placeholder": "add task" },
      "rect": { "x": 20, "y": 71, "w": 1156.4, "h": 37.5, "top": 71, "right": 1176.4, "bottom": 108.5, "left": 20 },
      "interactive": true,
      "visible": true,
      "value": "",
      "scroll": { "x": 0, "y": 0, "max_x": 0, "max_y": 0 }
    },
    "computed_style": {
      "display": "block",
      "position": "static",
      "width": "1156.4px",
      "height": "37.5px",
      "color": "rgb(34, 34, 34)",
      "background_color": "rgb(255, 255, 255)",
      "opacity": "1",
      "overflow_x": "visible",
      "overflow_y": "visible",
      "z_index": "auto",
      "flex": "1 1 0%"
    },
    "outer_html": "<input id=\"todo-input\" placeholder=\"add task\">"
  }
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
  "description": "模拟点击指定元素，并在返回前推进 event loop / microtasks / render，保证结果与最新 UI 对齐。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string", "description": "CSS selector；`body` / `html` 支持直接查询" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "result": {
    "selector": "button[type=\"submit\"]",
    "clicked": true,
    "element": {
      "tag": "button",
      "id": "",
      "class_name": "",
      "text": "+ add",
      "interactive": true,
      "visible": true
    }
  }
}
```

---

**`input_text`** — 向输入框输入文本

```json
{
  "name": "input_text",
  "description": "向指定输入元素写入文本；当前参数名为 `text`，写入后会派发 input/change 事件。",
  "inputSchema": {
    "type": "object",
    "required": ["selector", "text"],
    "properties": {
      "selector": { "type": "string" },
      "text": { "type": "string" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "result": {
    "selector": "#todo-input",
    "value": "hello world",
    "element": {
      "tag": "input",
      "id": "todo-input",
      "interactive": true,
      "visible": true
    }
  }
}
```

---

**`scroll`** — 滚动元素或页面

```json
{
  "name": "scroll",
  "description": "滚动指定容器到目标位置；至少需要 `x` 或 `y` 之一。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string", "description": "滚动容器 selector；`body` / `html` 支持直接滚动" },
      "x": { "type": "number" },
      "y": { "type": "number" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "result": {
    "selector": "body",
    "scroll": { "x": 0, "y": 400, "max_x": 0, "max_y": 1280 },
    "element": {
      "tag": "body",
      "interactive": false,
      "visible": true
    }
  }
}
```

---

**`highlight`** — 高亮元素

```json
{
  "name": "highlight",
  "description": "给目标元素设置 outline 高亮，便于调试定位。",
  "inputSchema": {
    "type": "object",
    "required": ["selector"],
    "properties": {
      "selector": { "type": "string" },
      "color": { "type": "string", "default": "#ff4d4f" }
    }
  }
}
```

返回：
```json
{
  "ok": true,
  "result": {
    "selector": "#todo-input",
    "highlighted": true,
    "color": "#ff4d4f",
    "previous_outline": "",
    "element": {
      "tag": "input",
      "id": "todo-input"
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

当前资源读取会优先使用 MCP 会话内的 `active_project`；若当前会话尚未显式打开项目，则按 CLI 相同规则自动解析项目。多项目场景下若无法唯一定位，会返回错误，要求显式指定项目或先调用 `open_project`。

| URI | MIME Type | 说明 |
|-----|-----------|------|
| `ui://snapshot` | `application/json` | 最新 UI 快照（按当前项目读取） |
| `ui://console_logs` | `application/json` | 最新 console 输出（按当前项目读取） |
| `ui://build_status` | `application/json` | 最近构建状态和错误列表（按当前项目读取） |
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

1. **先 snapshot，再操作**：任何修改前先调用 `snapshot_ui` 了解当前状态
2. **优先专用 UI 控制命令**：定位/诊断/交互优先使用 `query_element` / `inspect` / `click` / `input_text` / `scroll` / `highlight`
3. **改代码优先于 eval_js**：通过写文件+构建+重载，而非注入补丁
4. **eval_js 是最后手段**：仅用于验证假设或执行一次性操作
5. **交互后优先重新 snapshot 或 query**：当前实现会在交互命令后推进渲染并二次采样，适合立刻做结果校验
6. **build 失败立即停**：不要 reload 一个构建失败的版本

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

在 `mbink-ui-dev build --watch` 模式下：

```
CLI 透传 --watch
    ↓
daemon 持有 watcher
    ↓
后台线程轮询项目文件快照
    ↓
100ms debounce 合并变更
    ↓
触发一次普通 build
    ↓
构建成功后自动 restart_runtime
```

**当前实现特征**：watcher 不挂在 CLI 进程上，而是挂在 daemon 内部；最近一次变更路径会写入 `DaemonState.watch.changed_paths`，watch 状态会同步到 state json 与 `build-status` 返回值，便于外部轮询观察。

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
    "width": 800,
    "height": 600,
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

# 启动 Daemon（按项目启动；project 可自动解析）
mbink-ui-dev daemon start [--project <path>]
mbink-ui-dev daemon stop [--project <path>]
mbink-ui-dev daemon status [--project <path>]

# 启动 MCP 适配层（供 Claude Desktop / Cursor 等 MCP 原生工具接入）
mbink-ui-dev serve [--project <path>]         → 启动 stdio MCP Server；启动时会尝试自动解析当前项目，但未唯一定位时仍可先启动服务

# 打开项目（自动确保该项目 daemon/runtime 就绪）
mbink-ui-dev open [<project-path>]

# 单次构建 / watch
mbink-ui-dev build [--watch] [--project <path>]
```

### 10.1 CLI 命令完整列表（P2 当前已落地部分）

当前命令集合以真实已落地实现为准。所有命令 stdout 输出 JSON，stderr 输出人类可读日志，`exit code 0` 表示成功。

```
# Daemon 管理（按项目路由；project 可自动解析）
mbink-ui-dev daemon start [--project <path>]   → 启动对应项目 daemon，并返回状态 JSON
mbink-ui-dev daemon run [--project <path>]     → 前台运行对应项目 daemon
mbink-ui-dev daemon stop [--project <path>]    → 停止对应项目 daemon + runtime，并清理该项目临时状态
mbink-ui-dev daemon status [--project <path>]  → 查询对应项目 daemon 状态（JSON）
mbink-ui-dev stop [--project <path>]           → 顶层停止命令；幂等停止当前/指定项目 daemon + runtime

# 项目初始化 / MCP
mbink-ui-dev init <path> [--template <preact-jsx|preact-ts|vanilla-js|python-host|rust-host>]
mbink-ui-dev serve [--project <path>]          → 启动 stdio MCP Server（支持 initialize/tools/list/tools/call/resources/list/resources/read；会话内维护 active_project）

# 项目/运行时
mbink-ui-dev open [<project-path>]             → 打开项目，必要时自动启动该项目 daemon 与 runtime
mbink-ui-dev info [--project <path>]           → 查询当前/指定项目配置、文件树与最近构建状态
mbink-ui-dev reload [--project <path>]         → 重启当前/指定项目 runtime（当前固定语义）
mbink-ui-dev eval "<js code>" [--project <path>] → 在 QuickJS 中执行 JS，返回结果（JSON）

# 文件操作
mbink-ui-dev read <path> [--encoding utf8|base64] [--project <path>]
mbink-ui-dev write <path> [--content <text> | --from <file>] [--project <path>]
# 或：stdin 输入
mbink-ui-dev write <path> [--project <path>]

# 构建/观测
mbink-ui-dev build [--watch] [--project <path>] → 单次 build 或进入 daemon 持有的 watch 模式
mbink-ui-dev build-status [--project <path>]    → 查询最近一次 build 结果与 watch 状态
mbink-ui-dev snapshot [--project <path>]        → 获取当前/指定项目真实 UI snapshot（JSON）
mbink-ui-dev logs [--project <path>]            → 获取结构化 console 输出（JSON）
mbink-ui-dev errors [--project <path>]          → 获取结构化 JS error 输出（JSON）
```

项目解析顺序为：`--project` → 位置参数项目路径 → 当前目录/父目录 `.devui` → 当前目录/父目录 `mbink.config.json`（必要时自动创建 `.devui`）→ 仅存在一个 managed project 时自动回退。若同时存在多个 managed project 且无法唯一定位，则返回错误，要求显式传 `--project <path>`。

当前 CLI / MCP 已实现的 UI 精确控制能力包括：`query` / `query-element`、`inspect`、`click`、`input-text`、`scroll`、`highlight`。仍属于后续阶段的主要是生态与集成项，而不是这些基础 UI 控制命令。

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

### 10.3 MCP 适配层配置（P2 当前可用）

MCP 适配层是 CLI 之上的薄封装，§4 中定义的所有 tool schema 不变，底层按项目转发给对应 Daemon。

推荐配置方式：
- **单项目固定接入**：直接把 `serve --project <path>` 写进 MCP 配置
- **在项目目录内启动**：只配 `serve`，由当前工作目录 / `.devui` / `mbink.config.json` 自动解析项目
- **多项目会话**：先启动 `serve`，再通过 `open_project` 显式切换 `active_project`

在 Claude Desktop / Cursor 中配置：
```json
{
  "mcpServers": {
    "mbink-ui-dev": {
      "command": "mbink-ui-dev",
      "args": ["serve"],
      "env": {}
    }
  }
}
```

若希望固定绑定到单个项目，也可以：
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

### 10.4 Skills 系统提示模板（P1/P2 当前已落地部分）

Skills 是一段系统提示词，教会任意有 shell tool 的 AI Agent 如何操作 `mbink-ui-dev`。下面模板已经对齐当前真实实现：

```markdown
## MBink UI Dev Tool — Skills

你可以通过 shell 命令控制 MBink UI 开发环境。所有命令输出 JSON，exit code 0 表示成功。

### 会话启动
- 优先在项目目录内执行：`mbink-ui-dev open`
- 若当前目录无法唯一定位项目：`mbink-ui-dev open --project /path/to/project`
- 多项目场景下，后续命令也可显式补 `--project /path/to/project`

### 当前可用命令
- `mbink-ui-dev info [--project <path>]`
- `mbink-ui-dev read <path> [--project <path>]`
- `mbink-ui-dev write <path> [--project <path>]`
- `mbink-ui-dev build [--watch] [--project <path>]`
- `mbink-ui-dev build-status [--project <path>]`
- `mbink-ui-dev snapshot [--project <path>]`
- `mbink-ui-dev logs [--project <path>]`
- `mbink-ui-dev errors [--project <path>]`
- `mbink-ui-dev eval "1+1" [--project <path>]`
- `mbink-ui-dev reload [--project <path>]`
- `mbink-ui-dev stop [--project <path>]`

### 当前推荐工作流
1. 在项目目录内执行 `mbink-ui-dev open`；若失败再改用 `--project`
2. `mbink-ui-dev info`
3. `mbink-ui-dev snapshot`
4. `mbink-ui-dev read <path>` / `mbink-ui-dev write <path>` 修改代码
5. 开启持续迭代时执行 `mbink-ui-dev build --watch`；仅需单次构建时执行 `mbink-ui-dev build`
6. 通过 `mbink-ui-dev build-status` 观察最近一次 build / watch 状态
7. 再次执行 `mbink-ui-dev snapshot` 验证效果
8. 如需补充定位，执行 `mbink-ui-dev logs` / `mbink-ui-dev errors`
9. 结束会话时执行 `mbink-ui-dev stop`

### MCP 使用时
- 启动服务：`mbink-ui-dev serve`（可选 `--project /path/to/project`）
- `open_project` 的 `path / project_root` 可以省略；省略时自动解析当前项目
- MCP 会话会维护 `active_project`；`resources/read` / `tools/call` 会优先作用于当前 active_project
- 多项目无法唯一定位时，应显式调用 `open_project({"path":"/path/to/project"})`

### 关键原则
- **先 open，再 snapshot/info**：未打开项目时不要直接假设 runtime 已可用
- **snapshot 是主要观测入口**：logs/errors 用于补充运行时信息
- **优先走 read/write/build/reload 主链路**：`eval` 只用于少量临时验证
- **reload 当前是 restart_runtime 语义**：暂不支持 `--mode`
- **build 已支持 daemon 持有的 watch**：需要持续迭代时优先用 `build --watch`
- **用户手动关闭窗口视为 stop**：出现 `user_closed` 后，watch/reload/fallback 不会自动重新拉起 runtime
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
- [x] 顶层 `stop` 命令（幂等停止 daemon + runtime）
- [x] Skills 系统提示文档（§10.4，已对齐当前 P0 CLI）

**验收标准**：至少完成以下回归并得到可机读 JSON 结果：
1. `mbink-ui-dev open <project>`
2. `mbink-ui-dev snapshot`
3. `mbink-ui-dev eval "console.log('hi from eval'); console.error('boom from eval'); 1+1"`
4. `mbink-ui-dev logs`
5. `mbink-ui-dev errors`
6. `mbink-ui-dev reload`
7. `mbink-ui-dev stop`

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
- [x] `build --watch` 文件监听模式（daemon 持有 watcher，构建成功后自动 `restart_runtime`）
- [x] `open` / `snapshot` / `logs` / `errors` / `eval` / `reload` 配套能力闭环可用

**当前说明**：P1 已完成；真实 smoke 已覆盖 `open → build --watch → write → 自动 rebuild → restart_runtime → snapshot`，`build-status` 可返回 `trigger/watch/changed_paths/reload` 等状态，顶层 `stop` 也已完成回收闭环验证。

**验收标准**：AI 能完整执行「write → build 或 build --watch → reload / 自动 restart_runtime → snapshot → 判断」循环，整个循环 < 10 秒。

---

### P2 — Scaffolding + MCP 适配层

**目标**：AI 能从零初始化新项目；Claude Desktop / Cursor 用户可通过 MCP 原生接入；同一台机器上可并行管理多个 MBink 项目。

**交付物**：
- [x] `init` 命令（模板文件写入）
- [x] `preact-jsx` / `preact-ts` / `vanilla-js` / `python-host` / `rust-host` 模板完整内容
- [x] `mbink.config.json` 解析和验证
- [x] MCP 适配层（`serve` 命令；当前支持 `initialize` / `tools/list` / `tools/call` / `resources/list` / `resources/read`，并复用现有 CLI / daemon 能力）
- [x] 多项目并行管理：`.devui`、`project_id/runtime_id`、per-project daemon pipe/state/runtime 产物隔离
- [x] CLI / MCP 自动项目定位与 MCP 会话 `active_project`
- [x] 手动关闭 runtime 窗口视为该项目 stopped，`watch/reload/fallback` 不自动拉起 `user_closed` runtime
- [ ] Claude Desktop / Cursor 集成验证

**当前说明**：P2 代码与文档定义的最小完整闭环已落地。CLI 路径已覆盖 `init → open → build → snapshot → stop`，并支持多项目自动定位/显式切换；MCP 路径已支持 tools 与 resources 的最小完整读写/转发闭环，`open_project` 可省略路径并绑定会话 `active_project`。桌面客户端真实联调仍需在外部 Claude Desktop / Cursor 环境继续验证。

**验收标准**：① CLI 路径：AI 执行 `init` + `open` + `build` + `reload`，从空目录到运行 UI < 30 秒，且在多项目同时存在时能正确隔离并在歧义时返回显式错误；② MCP 路径：`serve` + `open_project` + `resources/read` / `tools/call` 可按当前项目正确路由；③ 用户手动关闭窗口后，该项目状态变为 stopped，不会被 watch/reload/fallback 自动拉起。

---

### P3 — UI Precision Control（精确操作与自动化测试）

**目标**：AI 能精确定位、检查、操作任意 UI 元素。

**交付物**：
- [x] `query_element` tool 实现（CSS 选择器匹配 + 元素属性）
- [x] `inspect` tool 实现（计算样式 + 布局 + 组件状态）
- [x] `click` / `input_text` / `scroll` tool 实现
- [x] `highlight` tool（在 UI 上高亮指定元素，辅助调试）
- [x] 交互后结果二次采样与 snapshot 时序修复（避免返回旧 DOM / 旧 snapshot）
- [x] 冷启动首帧 snapshot 导出与 daemon 短轮询修复（避免首次返回 `stub-root`）

**当前说明**：P3 最小闭环已落地，CLI / MCP → daemon → runtime 的元素级查询、检查、点击、输入、滚动、高亮链路已打通；真实联调已验证交互后返回结果与 snapshot 同步，`open` 后首次 `snapshot_ui` 也可直接返回真实 DOM。

**验收标准**：AI 能通过 CSS 选择器定位任意元素，读取其位置/样式，模拟点击、输入、滚动并通过 query / snapshot 验证 UI 响应。

---

### P4 — Ecosystem（生态完善）

**目标**：覆盖更多使用场景和集成方式。

**交付物**：
- [ ] HTTP + SSE 传输层（支持 IDE 插件集成）
- [ ] 更多模板生态完善（如官方示例扩展、宿主集成增强）
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
