# Network Subsystem

网络子系统，提供 HTTP 请求和 Fetch API 支持。

## 模块列表

| 文件 | 描述 |
|------|------|
| `http_client.h/cpp` | HTTP 客户端，处理网络请求 |
| `fetch_bindings.h/cpp` | Fetch API JavaScript 绑定 |

## 依赖关系

### 依赖的模块
- `core/event` - 任务调度（异步请求）
- `core/quickjs` - JavaScript 绑定
- `nlohmann/json` - JSON 解析

### 被依赖的模块
- `core/quickjs` - JavaScript 运行时

## 功能说明

实现 Web 标准的 Fetch API：
- GET/POST/PUT/DELETE 请求
- JSON 响应解析
- Promise 异步处理
