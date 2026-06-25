# core/dom/elements/logview/

## 概述

日志视图元素实现，提供高性能日志显示、过滤和搜索能力。

## 模块列表

| 文件 | 描述 |
|------|------|
| `log_entry.h` | 日志条目结构，紧凑 8 字节头部 |
| `log_buffer.h/cpp` | 日志专用缓冲区，紧凑存储 |
| `log_filter.h/cpp` | 日志级别和源名过滤 |
| `log_search.h/cpp` | 日志搜索和高亮 |
| `log_renderer.h/cpp` | 日志渲染器，继承自 VirtualScrollRenderer |
| `html_logview_element.h/cpp` | DOM 元素类，日志视图入口 |

## 设计目标

1. **内存高效** - 100 万条日志约 58MB（vs 终端 640MB）
2. **高性能过滤** - 过滤不复制数据，只维护索引
3. **实时搜索** - 支持普通文本和正则表达式

## 内存布局

```
LogEntryHeader (8 bytes):
  - timestamp: 4 bytes (相对时间戳，毫秒)
  - level: 1 byte
  - source_id: 1 byte (预注册源名索引)
  - message_length: 2 bytes
  - [message bytes follow...]

每条日志: 8 + message_length bytes
100万条 × 平均58字节 = 58 MB
```

## 依赖关系

- 依赖: `virtual_text` (共享核心层), Skia
- 被依赖: DOM 绑定层, JavaScript 绑定

## 使用示例

### HTML

```html
<logview max-entries="100000" auto-scroll="true"></logview>
```

### JavaScript

```javascript
const log = document.querySelector('logview');

// 添加日志
log.append('INFO', 'app', 'Application started');
log.append('ERROR', 'network', 'Connection failed');

// 过滤
log.setLevelFilter(['ERROR', 'WARN']);
log.setSourceFilter(['network']);

// 搜索
const count = log.search('error');
log.nextMatch();
log.prevMatch();

// 导出
const text = log.export('text');
const json = log.export('json');
```

### C++

```cpp
#include "html_logview_element.h"

mblink::HTMLLogViewElement logview;

// 添加日志
logview.Append(mblink::LogLevel::INFO, "app", "Started");
logview.Append("ERROR", "network", "Connection failed");

// 过滤
logview.SetLevelFilter({"ERROR", "WARN"});

// 搜索
int count = logview.Search("error");
logview.NextMatch();

// 渲染
logview.Render(canvas, 0, 0, 800, 600);
```

## 日志级别

| 级别 | 值 | 颜色 |
|------|-----|------|
| DEBUG | 0 | 灰色 |
| INFO | 1 | 浅灰 |
| WARN | 2 | 黄色 |
| ERROR | 3 | 红色 |
| FATAL | 4 | 亮红色 |

## 过滤机制

过滤使用位掩码实现，高效且不复制数据：

```cpp
// 只显示 ERROR 和 FATAL
uint8_t mask = (1 << ERROR) | (1 << FATAL);  // 0x18
filter.SetLevelMask(mask);
```

## 搜索功能

- 普通文本搜索（大小写不敏感）
- 正则表达式搜索
- 匹配高亮显示
- 上一个/下一个导航
