# Network Subsystem | 网络子系统

## Overview | 概览

Provides HTTP request handling and Fetch API integration visible in the current repository.
提供当前仓库中可见的 HTTP 请求处理和 Fetch API 集成。

## Main Files | 主要文件

- `http_client.h/cpp` — HTTP client / HTTP 客户端
- `fetch_bindings.h/cpp` — JavaScript Fetch bindings / JavaScript Fetch 绑定

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/event`
- `core/quickjs`
- `nlohmann/json`

Used by | 被依赖：

- `core/quickjs`

## Features | 功能

- GET / POST / PUT / DELETE requests
  GET / POST / PUT / DELETE 请求
- JSON response parsing
  JSON 响应解析
- promise-based async handling
  Promise 异步处理
