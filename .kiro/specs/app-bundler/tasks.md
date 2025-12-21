# 实现计划

## 阶段 1: 核心基础设施

- [x] 1. 搭建 app_bundler 项目结构

  - [x] 1.1 创建 tools/app_bundler 目录和 CMakeLists.txt


    - 添加可执行目标，链接 QuickJS
    - _需求: 3.1_
  - [x] 1.2 创建 main.cpp 实现命令行参数解析


    - 解析输入文件、输出文件、--width、--height、--title、--include、--verbose、--no-overwrite
    - _需求: 1.1, 4.1-4.4_

## 阶段 2: Payload 系统



- [x] 2. 实现 payload 格式和工具类


  - [x] 2.1 创建 payload.h/cpp 实现 PayloadBuilder 类

    - 定义 PAYLOAD_MAGIC、PAYLOAD_VERSION 常量
    - 实现 Build() 序列化配置 + 字节码 + 尾部
    - 实现静态 Parse() 反序列化 payload
    - _需求: 7.1, 7.2_

  - [x] 2.2 实现 CRC32 校验和计算
    - 使用标准 CRC32 算法


    - _需求: 8.1_

  - [x] 2.3 编写 payload 往返属性测试
    - **属性 7: Payload 检测往返**
    - **验证: 需求 7.1, 7.2**
  - [x] 2.4 编写校验和完整性属性测试
    - **属性 6: 校验和完整性**
    - **验证: 需求 8.1, 8.2, 8.3**

## 阶段 3: 模块解析

- [x] 3. 实现 ES6 模块解析器
  - [x] 3.1 创建 module_resolver.h/cpp
    - 实现 ParseImports() 使用正则提取 import 语句
    - 实现 ResolvePath() 处理相对路径和裸模块路径
    - _需求: 2.1, 2.2, 2.3_
  - [x] 3.2 实现依赖图遍历
    - 从入口文件构建完整依赖图
    - 检测并处理循环依赖
    - _需求: 2.4_
  - [x] 3.3 实现拓扑排序确定模块顺序
    - 确保依赖项在被依赖项之前加载
    - _需求: 2.7_
  - [x] 3.4 注册内置模块 (preact, hooks)
    - 将 'preact' 和 'preact/hooks' 映射到嵌入的源码
    - _需求: 2.3_
  - [x] 3.5 编写模块解析完整性属性测试
    - **属性 3: 模块解析完整性**
    - **验证: 需求 2.2, 2.5**
  - [x] 3.6 编写依赖顺序属性测试
    - **属性 4: 依赖顺序保持**
    - **验证: 需求 2.7**

## 阶段 4: 字节码编译

- [x] 4. 实现字节码编译器
  - [x] 4.1 创建 bytecode_compiler.h/cpp
    - 初始化 QuickJS 运行时和上下文
    - 使用 JS_Eval 配合 JS_EVAL_FLAG_COMPILE_ONLY 实现 CompileModule()
    - 使用 JS_WriteObject 获取字节码
    - _需求: 1.3, 2.6_
  - [x] 4.2 实现 CompileModules() 编译多个模块
    - 按依赖顺序编译所有解析到的模块
    - 将字节码与模块元数据连接
    - _需求: 2.6, 2.7_
  - [x] 4.3 编写字节码编译属性测试
    - **属性 2: 所有模块编译为字节码**
    - **验证: 需求 2.1, 2.6**

- [x] 5. 检查点 - 确保所有测试通过
  - 30 个 bundler 属性测试全部通过

## 阶段 5: Exe 写入

- [x] 6. 实现 exe 写入器
  - [x] 6.1 创建 exe_writer.h/cpp
    - 实现 LoadTemplate() 读取 app_loader.exe
    - 实现 WriteOutput() 追加 payload
    - _需求: 1.1, 3.2_
  - [x] 6.2 实现模板 exe 发现
    - 在 bundler 同目录查找 app_loader.exe
    - 支持 --template 选项指定自定义路径
    - _需求: 3.2, 3.3_
  - [x] 6.3 集成到 main.cpp
    - 完整的打包流程：解析 -> 编译 -> 构建 payload -> 写入
    - _需求: 1.1, 1.3_

## 阶段 6: App Loader 集成

- [x] 7. 修改 app_loader 检测嵌入的 payload
  - [x] 7.1 在 app_loader main() 中添加 payload 检测
    - 读取自身可执行文件
    - 检查文件末尾预期偏移处的 PAYLOAD_MAGIC
    - _需求: 7.1_
  - [x] 7.2 实现字节码加载和执行
    - 使用 JS_ReadObject 加载字节码
    - 使用 JS_EvalFunction 执行
    - _需求: 7.2, 7.3_
  - [x] 7.3 实现校验和验证
    - 计算 payload 数据的 CRC32
    - 与存储的校验和比较
    - _需求: 8.2, 8.3_
  - [x] 7.4 应用嵌入的窗口配置
    - 从 payload 配置读取 width、height、title
    - 覆盖默认值
    - _需求: 4.1-4.4_

- [x] 8. 检查点 - 确保所有测试通过
  - app_loader 和 app_bundler 编译成功

## 阶段 7: CLI 和错误处理

- [x] 9. 完善 CLI 实现
  - [x] 9.1 实现 --include 选项处理
    - 在主应用之前加载额外的 JS 文件
    - _需求: 5.1, 5.2, 5.3_
  - [x] 9.2 实现 --no-overwrite 选项
    - 检查输出文件是否存在，拒绝覆盖
    - _需求: 1.4_
  - [x] 9.3 实现进度输出
    - 显示解析、编译、嵌入步骤
    - _需求: 6.1, 6.3_
  - [x] 9.4 实现 --verbose 输出
    - 显示所有解析到的模块和大小
    - _需求: 6.4_

- [x] 10. 实现错误处理
  - [x] 10.1 处理文件未找到错误
    - 清晰的错误消息包含路径
    - _需求: 2.5, 5.3_
  - [x] 10.2 处理 JS 语法错误
    - 报告来自 QuickJS 的文件、行、列
    - _需求: 6.2_
  - [x] 10.3 处理模板未找到
    - 显示定位 app_loader.exe 的说明
    - _需求: 3.3_

## 阶段 8: 集成测试

- [x] 11. 编写集成测试
  - [x] 11.1 测试打包简单应用
    - 打包单个 JS 文件，验证 exe 生成
  - [ ] 11.2 测试打包 ES6 模块应用
    - 打包带 import 的应用，验证所有模块工作
  - [ ] 11.3 测试打包 Preact 应用
    - 打包使用 preact 的应用，验证渲染正常
  - [ ] 11.4 测试窗口配置
    - 使用 --width --height --title 打包，验证应用
    - **属性 5: 配置嵌入**
    - **验证: 需求 4.1, 4.2, 4.3, 4.4**

- [x] 12. 最终检查点
  - app_bundler 可以成功打包简单 JS 应用
  - 生成的 exe 包含有效的 payload
