# 需求文档

## 简介

本文档定义 MBink 框架的虚拟文本组件系列功能需求，包含两个核心组件：
1. **终端元素 (Terminal Element)** - 嵌入式终端，支持 ANSI 渲染和交互式 Shell
2. **日志视图元素 (LogView Element)** - 高性能日志查看器，支持过滤、搜索和实时追踪

两个组件共享核心基础设施（虚拟滚动、环形缓冲区、文本渲染），但针对各自场景做专门优化。

## 术语表

- **VirtualBuffer**: 泛型环形缓冲区模板，支持固定容量和自动淘汰旧数据
- **VirtualScrollRenderer**: 虚拟滚动渲染基类，只渲染可见区域
- **Terminal Element**: HTML 自定义元素 `<terminal>`，嵌入式终端界面
- **LogView Element**: HTML 自定义元素 `<logview>`，日志查看器界面
- **ANSI Escape Sequence**: 控制终端显示的转义序列
- **PTY (Pseudo Terminal)**: 伪终端，用于实现交互式 Shell
- **LogEntry**: 日志条目结构，包含时间戳、级别、来源、消息
- **LogLevel**: 日志级别枚举 (DEBUG, INFO, WARN, ERROR, FATAL)

---

## 第一部分：共享核心层需求

### 需求 1

**用户故事:** 作为组件开发者，我需要一个高效的环形缓冲区，以便在固定内存下存储大量文本数据。

#### 验收标准

1. WHEN 缓冲区达到配置的最大容量时 THEN VirtualBuffer SHALL 自动丢弃最旧的条目
2. WHEN 访问缓冲区中的条目时 THEN VirtualBuffer SHALL 支持 O(1) 时间复杂度的随机访问
3. WHEN 追加新条目时 THEN VirtualBuffer SHALL 支持 O(1) 时间复杂度的追加操作
4. WHEN 缓冲区被清空时 THEN VirtualBuffer SHALL 释放所有条目并重置状态

### 需求 2

**用户故事:** 作为组件开发者，我需要虚拟滚动渲染能力，以便高效显示大量数据。

#### 验收标准

1. WHEN 渲染包含 N 行数据的视图时 THEN VirtualScrollRenderer SHALL 只渲染可见区域的 M 行
2. WHEN 用户滚动内容时 THEN VirtualScrollRenderer SHALL 在 16ms 内更新可见区域
3. WHEN 视图大小改变时 THEN VirtualScrollRenderer SHALL 重新计算可见行数并更新渲染
4. WHEN 点击视图内容时 THEN VirtualScrollRenderer SHALL 提供准确的命中测试结果

---

## 第二部分：终端元素需求

### 需求 3

**用户故事:** 作为开发者，我希望在应用中显示带 ANSI 颜色的命令输出，以便构建日志查看器和 CLI 工具包装器。

#### 验收标准

1. WHEN 包含 ANSI 颜色代码的文本写入终端元素时 THEN Terminal Element SHALL 使用正确的前景色和背景色渲染文本
2. WHEN 包含 ANSI 样式代码（粗体、斜体、下划线）的文本写入时 THEN Terminal Element SHALL 使用相应的字体样式渲染文本
3. WHEN 终端缓冲区超过配置的最大行数时 THEN Terminal Element SHALL 丢弃最旧的行
4. WHEN 存在大量数据（10万行以上）时 THEN Terminal Element SHALL 使用虚拟滚动保持流畅渲染

### 需求 4

**用户故事:** 作为开发者，我希望执行系统命令并显示其输出，以便构建系统管理工具。

#### 验收标准

1. WHEN 提交命令执行时 THEN Terminal Element SHALL 启动进程并将 stdout 流式传输到显示区
2. WHEN 执行的进程写入 stderr 时 THEN Terminal Element SHALL 以可区分的方式显示错误输出
3. WHEN 执行的进程终止时 THEN Terminal Element SHALL 发出带有退出码的 'exit' 事件
4. WHEN 在另一个命令运行时请求执行命令 THEN Terminal Element SHALL 根据配置排队或拒绝

### 需求 5

**用户故事:** 作为开发者，我希望在应用中嵌入交互式 shell，以便构建类似 IDE 的集成终端工具。

#### 验收标准

1. WHEN 终端元素配置为交互模式时 THEN Terminal Element SHALL 创建到系统 shell 的 PTY 连接
2. WHEN 用户在交互模式下输入键盘时 THEN Terminal Element SHALL 将输入转发到 PTY
3. WHEN PTY 输出数据时 THEN Terminal Element SHALL 实时解析和渲染输出
4. WHEN 终端调整大小时 THEN Terminal Element SHALL 通知 PTY 新的尺寸
5. WHEN PTY 会话结束时 THEN Terminal Element SHALL 发出 'close' 事件

### 需求 6

**用户故事:** 作为开发者，我希望以编程方式控制终端，以便将其与应用逻辑集成。

#### 验收标准

1. WHEN 调用 write() 方法时 THEN Terminal Element SHALL 将文本追加到缓冲区并渲染
2. WHEN 调用 clear() 方法时 THEN Terminal Element SHALL 清空缓冲区并重置显示
3. WHEN 调用 scrollTo() 方法时 THEN Terminal Element SHALL 滚动到指定的行位置
4. WHEN 调用 serialize() 方法时 THEN Terminal Element SHALL 以纯文本形式返回缓冲区内容

### 需求 7

**用户故事:** 作为开发者，我希望终端能优雅地处理边界情况，以便应用保持稳定。

#### 验收标准

1. WHEN 收到格式错误的 ANSI 序列时 THEN Terminal Element SHALL 跳过无效序列并继续解析
2. WHEN 二进制数据写入终端时 THEN Terminal Element SHALL 为不可打印字节显示替换字符
3. WHEN UTF-8 多字节字符跨越缓冲区边界时 THEN Terminal Element SHALL 正确重组并显示
4. WHEN 超长行超过终端宽度时 THEN Terminal Element SHALL 根据配置进行换行或截断

### 需求 8

**用户故事:** 作为开发者，我希望跨平台终端支持，以便应用在各平台一致运行。

#### 验收标准

1. WHEN 在 Windows 上运行时 THEN Terminal Element SHALL 使用 ConPTY 支持交互式 shell
2. WHEN 在 macOS 或 Linux 上运行时 THEN Terminal Element SHALL 使用 forkpty() 支持交互式 shell
3. WHEN 处理行尾时 THEN Terminal Element SHALL 在各平台上一致地规范化 CR、LF 和 CRLF

---

## 第三部分：日志视图元素需求

### 需求 9

**用户故事:** 作为开发者，我希望高效显示大量日志，以便构建日志查看器和监控工具。

#### 验收标准

1. WHEN 日志条目写入日志视图时 THEN LogView Element SHALL 以紧凑格式存储并渲染
2. WHEN 日志视图包含 100 万条日志时 THEN LogView Element SHALL 保持流畅的滚动性能
3. WHEN 日志缓冲区超过配置的最大条目数时 THEN LogView Element SHALL 丢弃最旧的条目
4. WHEN 新日志到达且视图在底部时 THEN LogView Element SHALL 自动滚动显示新日志

### 需求 10

**用户故事:** 作为开发者，我希望按日志级别过滤日志，以便快速定位问题。

#### 验收标准

1. WHEN 设置日志级别过滤器时 THEN LogView Element SHALL 只显示符合级别的日志条目
2. WHEN 过滤器改变时 THEN LogView Element SHALL 立即更新显示而不重新加载数据
3. WHEN 多个级别被选中时 THEN LogView Element SHALL 显示所有选中级别的日志
4. WHEN 日志级别为 ERROR 或 FATAL 时 THEN LogView Element SHALL 使用醒目的颜色高亮显示

### 需求 11

**用户故事:** 作为开发者，我希望搜索和高亮日志内容，以便快速找到相关信息。

#### 验收标准

1. WHEN 输入搜索关键词时 THEN LogView Element SHALL 高亮所有匹配的文本
2. WHEN 搜索有结果时 THEN LogView Element SHALL 显示匹配数量并支持跳转到下一个/上一个
3. WHEN 搜索关键词改变时 THEN LogView Element SHALL 实时更新高亮显示
4. WHEN 使用正则表达式搜索时 THEN LogView Element SHALL 支持正则匹配模式

### 需求 12

**用户故事:** 作为开发者，我希望以编程方式控制日志视图，以便与应用逻辑集成。

#### 验收标准

1. WHEN 调用 append() 方法时 THEN LogView Element SHALL 添加新日志条目
2. WHEN 调用 clear() 方法时 THEN LogView Element SHALL 清空所有日志
3. WHEN 调用 setFilter() 方法时 THEN LogView Element SHALL 应用指定的过滤条件
4. WHEN 调用 search() 方法时 THEN LogView Element SHALL 执行搜索并返回结果
5. WHEN 调用 export() 方法时 THEN LogView Element SHALL 导出日志为文本或 JSON 格式

### 需求 13

**用户故事:** 作为开发者，我希望自定义日志显示格式，以便匹配应用需求。

#### 验收标准

1. WHEN 配置时间戳格式时 THEN LogView Element SHALL 按指定格式显示时间
2. WHEN 配置列显示时 THEN LogView Element SHALL 显示或隐藏指定的列（时间、级别、来源）
3. WHEN 配置颜色方案时 THEN LogView Element SHALL 使用指定的颜色渲染各级别日志
4. WHEN CSS 样式改变时 THEN LogView Element SHALL 更新字体和颜色

---

## 第四部分：共享交互需求

### 需求 14

**用户故事:** 作为用户，我希望选择和复制文本内容，以便提取信息。

#### 验收标准

1. WHEN 用户点击并拖动时 THEN 组件 SHALL 高亮显示选中的文本区域
2. WHEN 用户按 Ctrl+C 且有选择时 THEN 组件 SHALL 将选中文本复制到剪贴板
3. WHEN 用户双击单词时 THEN 组件 SHALL 选中整个单词
4. WHEN 用户三击行时 THEN 组件 SHALL 选中整行
