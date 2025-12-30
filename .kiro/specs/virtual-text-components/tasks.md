# 实现计划

## 第一阶段：共享核心层

- [x] 1. 搭建模块结构


  - 创建 `core/dom/elements/virtual_text/` 目录
  - 创建 `core/dom/elements/terminal/` 目录
  - 创建 `core/dom/elements/logview/` 目录
  - 创建各目录的 CMakeLists.txt 和 README.md
  - _需求: 全部_



- [x] 2. 实现 VirtualBuffer<T>

  - [x] 2.1 创建 VirtualBuffer 模板类

    - 实现 deque 基础的环形缓冲区
    - 实现 Append()、Clear()、operator[]
    - 实现容量管理和自动淘汰
    - _需求: 1.1, 1.2, 1.3, 1.4_
  - [x]* 2.2 编写缓冲区溢出处理的属性测试


    - **属性 1: 缓冲区溢出处理**
    - **验证: 需求 1.1**

  - [x]* 2.3 编写清除重置状态的属性测试
    - **属性 2: 清除重置状态**
    - **验证: 需求 1.4**

- [x] 3. 实现 VirtualScrollRenderer



  - [x] 3.1 创建 VirtualScrollRenderer 基类

    - 实现字体度量计算
    - 实现滚动偏移管理
    - 实现可见行数计算

    - _需求: 2.1, 2.2, 2.3_
  - [x]* 3.2 编写虚拟滚动正确性的属性测试
    - **属性 3: 虚拟滚动正确性**
    - **验证: 需求 2.1**
  - [x] 3.3 实现命中测试



    - HitTestLine() 和 HitTestColumn()
    - _需求: 2.4_
  - [x]* 3.4 编写命中测试准确性的属性测试
    - **属性 4: 命中测试准确性**
    - **验证: 需求 2.4**

- [x] 4. 实现 SelectionManager



  - [x] 4.1 创建 SelectionManager 类

    - 实现选择范围跟踪
    - 实现 StartSelection/UpdateSelection/EndSelection
    - _需求: 14.1, 14.2_

  - [x] 4.2 实现单词和行选择


    - SelectWord() 双击选词
    - SelectLine() 三击选行

    - _需求: 14.3, 14.4_
  - [x]* 4.3 编写单词选择的属性测试
    - **属性 5: 单词选择**
    - **验证: 需求 14.3**
  - [x]* 4.4 编写行选择的属性测试
    - **属性 6: 行选择**
    - **验证: 需求 14.4**

- [x] 5. 实现 TextStyle 和 ColorPalette


  - 创建 TextStyle 结构
  - 创建 ColorPalette 及预设方案
  - _需求: 3.1, 10.4_

- [x] 6. 检查点 - 确保共享核心层测试通过


  - 确保所有测试通过，如有问题询问用户。

---

## 第二阶段：终端元素

- [x] 7. 实现 AnsiParser



  - [x] 7.1 创建 AnsiParser 状态机

    - 实现 ParserState 和状态转换
    - 实现 ProcessByte() 核心逻辑
    - _需求: 3.1, 3.2, 7.1_
  - [x]* 7.2 编写 ANSI 解析正确性的属性测试

    - **属性 7: ANSI 解析正确性**
    - **验证: 需求 3.1, 3.2**


  - [x] 7.3 实现 SGR 颜色和样式处理
    - 基础颜色 (30-37, 40-47)
    - 256 色和 RGB 模式
    - 样式标志
    - _需求: 3.1, 3.2_
  - [x]* 7.4 编写解析器错误恢复的属性测试


    - **属性 10: 解析器错误恢复**
    - **验证: 需求 7.1, 7.2**
  - [x] 7.5 实现 UTF-8 多字节处理
    - UTF-8 缓冲和重组



    - _需求: 7.3_
  - [x]* 7.6 编写 UTF-8 边界处理的属性测试

    - **属性 11: UTF-8 边界处理**
    - **验证: 需求 7.3**

- [x] 8. 实现 TerminalBuffer
  - [x] 8.1 创建 TerminalBuffer 类
    - 基于 VirtualBuffer<vector<Cell>> 实现
    - 实现 PutCell()、NewLine()、CarriageReturn()
    - _需求: 3.3, 6.1_
  - [x] 8.2 实现光标管理

    - SetCursor()、MoveCursor()
    - 边界检查
    - _需求: 3.1_
  - [x] 8.3 实现序列化


    - Serialize() 导出纯文本
    - GetText() 获取选中文本
    - _需求: 6.4_

  - [x]* 8.4 编写 Write/Serialize 往返的属性测试
    - **属性 8: Write/Serialize 往返**

    - **验证: 需求 6.1, 6.4**
  - [x]* 8.5 编写行换行的属性测试
    - **属性 12: 行换行**
    - **验证: 需求 7.4**

- [x] 9. 实现 TerminalRenderer




  - [x] 9.1 创建 TerminalRenderer 类
    - 继承 VirtualScrollRenderer


    - 实现 Render() 渲染单元格
    - _需求: 3.4_
  - [x] 9.2 实现颜色调色板渲染
    - 应用 ColorPalette
    - 渲染样式（粗体、斜体等）





    - _需求: 3.1, 3.2_
  - [x]* 9.3 编写 ScrollTo 定位的属性测试

    - **属性 9: ScrollTo 定位**
    - **验证: 需求 6.3**


- [x] 10. 实现 HTMLTerminalElement

  - [x] 10.1 创建 HTMLTerminalElement 类
    - 继承 Element
    - 组装 AnsiParser、TerminalBuffer、TerminalRenderer

    - _需求: 全部终端需求_
  - [x] 10.2 实现 DOM 属性
    - rows, cols, scrollback
    - _需求: 3.3_
  - [x] 10.3 实现公共方法
    - write(), clear(), scrollTo(), serialize(), focus()
    - _需求: 6.1, 6.2, 6.3, 6.4_
  - [x] 10.4 实现事件处理
    - 键盘输入
    - 鼠标选择
    - _需求: 5.2, 14.1_
  - [x] 10.5 注册元素到文档工厂
    - 添加 "terminal" 标签
    - _需求: 全部_

- [x] 11. 检查点 - 确保终端元素测试通过
  - 确保所有测试通过，如有问题询问用户。

- [x] 12. 实现 CommandExecutor
  - [x] 12.1 创建 CommandExecutor 类
    - 进程启动抽象
    - stdout/stderr 捕获
    - _需求: 4.1, 4.2_
  - [x] 12.2 实现 Windows 进程执行
    - CreateProcess() + 管道
    - _需求: 8.1_
  - [x] 12.3 实现 Unix 进程执行
    - fork()/exec() + 管道
    - _需求: 8.2_
  - [x] 12.4 连接到 HTMLTerminalElement
    - execute() 方法
    - exit 事件
    - _需求: 4.1, 4.3_

- [x] 13. 实现 PtyBackend
  - [x] 13.1 创建 PtyBackend 抽象接口
    - Start(), Stop(), Write(), Resize()
    - _需求: 5.1_
  - [x] 13.2 实现 ConPtyBackend (Windows)
    - CreatePseudoConsole()
    - 管道 I/O
    - _需求: 8.1_
  - [x] 13.3 实现 UnixPtyBackend
    - forkpty()
    - select()/poll() 异步读取
    - _需求: 8.2_
  - [x] 13.4 连接到 HTMLTerminalElement
    - startShell(), sendInput(), resize()
    - _需求: 5.1, 5.2, 5.4, 5.5_

- [x] 14. 检查点 - 确保终端完整功能测试通过
  - 确保所有测试通过，如有问题询问用户。

---

## 第三阶段：日志视图元素

- [x] 15. 实现 LogEntry 和 LogBuffer
  - [x] 15.1 创建 LogEntry 结构
    - 紧凑的 8 字节头
    - LogLevel 枚举
    - _需求: 9.1_
  - [x] 15.2 创建 LogBuffer 类
    - 紧凑存储实现
    - Append()、Clear()

    - 源名注册
    - _需求: 9.1, 9.3_
  - [x]* 15.3 编写日志追加正确性的属性测试
    - **属性 13: 日志追加正确性**
    - **验证: 需求 9.1**
  - [x] 15.4 实现导出功能
    - ExportAsText()
    - ExportAsJson()
    - _需求: 12.5_

- [x] 16. 实现 LogFilter
  - [x] 16.1 创建日志级别过滤
    - 位掩码过滤
    - 过滤索引维护

    - _需求: 10.1, 10.2, 10.3_
  - [x]* 16.2 编写日志级别过滤的属性测试
    - **属性 15: 日志级别过滤**
    - **验证: 需求 10.1, 10.3**
  - [x] 16.3 实现源过滤
    - 按源名过滤
    - _需求: 10.1_

- [x] 17. 实现 LogSearch
  - [x] 17.1 创建搜索功能
    - 普通文本搜索
    - 匹配高亮
    - _需求: 11.1, 11.2_

  - [x]* 17.2 编写搜索匹配正确性的属性测试
    - **属性 16: 搜索匹配正确性**
    - **验证: 需求 11.1, 11.2**
  - [x] 17.3 实现正则搜索
    - std::regex 支持
    - 错误处理
    - _需求: 11.4_
  - [x]* 17.4 编写正则搜索的属性测试

    - **属性 17: 正则搜索**
    - **验证: 需求 11.4**
  - [x] 17.5 实现搜索导航
    - NextMatch(), PrevMatch()
    - _需求: 11.2_

- [x] 18. 实现 LogRenderer
  - [x] 18.1 创建 LogRenderer 类
    - 继承 VirtualScrollRenderer
    - 实现 Render()
    - _需求: 9.1, 9.2_
  - [x] 18.2 实现级别颜色渲染
    - 不同级别不同颜色
    - _需求: 10.4_
  - [x] 18.3 实现搜索高亮渲染
    - 匹配文本高亮
    - 当前匹配特殊标记
    - _需求: 11.1, 11.3_
  - [x] 18.4 实现时间戳格式化
    - 可配置格式
    - _需求: 13.1_

- [x] 19. 实现 HTMLLogViewElement
  - [x] 19.1 创建 HTMLLogViewElement 类
    - 继承 Element
    - 组装 LogBuffer、LogRenderer
    - _需求: 全部日志需求_
  - [x] 19.2 实现 DOM 属性
    - max_entries, auto_scroll
    - _需求: 9.3, 9.4_

  - [x]* 19.3 编写自动滚动行为的属性测试

    - **属性 14: 自动滚动行为**
    - **验证: 需求 9.4**
  - [x] 19.4 实现公共方法
    - append(), clear(), setFilter(), search(), export()
    - _需求: 12.1, 12.2, 12.3, 12.4, 12.5_
  - [x] 19.5 实现事件处理
    - 鼠标选择
    - 键盘快捷键
    - _需求: 14.1, 14.2_
  - [x] 19.6 注册元素到文档工厂
    - 添加 "logview" 标签
    - _需求: 全部_

- [x] 20. 检查点 - 确保日志视图测试通过
  - 确保所有测试通过，如有问题询问用户。

---

## 第四阶段：JavaScript 绑定和集成

- [x] 21. 实现 JavaScript 绑定
  - [x] 21.1 创建 Terminal 元素 JS 绑定
    - 暴露属性和方法
    - 事件分发
    - _需求: 6.1-6.4, 4.3, 5.5_
  - [x] 21.2 创建 LogView 元素 JS 绑定
    - 暴露属性和方法
    - 事件分发
    - _需求: 12.1-12.5_
  - [x]* 21.3 编写 JS API 集成测试
    - 从 JavaScript 端测试两个组件
    - _需求: 全部_

- [x] 22. 最终检查点 - 确保所有测试通过
  - 确保所有测试通过，如有问题询问用户。
