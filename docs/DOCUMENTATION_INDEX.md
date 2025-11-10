# LightUI 文档索引

> 最后更新: 2025-11-10
> 当前版本: 0.1.0-alpha
> 总进度: 85% (Phase 2.3 完成 ✅)
> 构建状态: ✅ 所有核心模块编译成功
> 测试状态: ✅ 83+ 个测试用例全部通过

## 📖 文档导航

本文档提供LightUI项目所有文档的快速导航和概览。

---

## 🎯 快速开始

如果你是第一次接触LightUI，建议按以下顺序阅读：

1. **[../README.md](../README.md)** - 5分钟了解项目
2. **[../PROJECT_STATUS.md](../PROJECT_STATUS.md)** - 10分钟了解当前状态（推荐）
3. **[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)** - 15分钟深入了解
4. **[GETTING_STARTED.md](GETTING_STARTED.md)** - 30分钟开始开发

### 最新完成
- ✅ **Phase 2.3 渲染引擎** - 100% 完成（2025-11-10）
  - ✅ Skia 渲染器完全集成并测试
  - ✅ 图形/文本/图片渲染
  - ✅ 完整的 CSS 样式支持
  - ✅ DOM 渲染树转换
  - ✅ 完整的渲染优化系统
  - ✅ 构建系统优化（运行时库配置）
- ✅ **Phase 2.2 DOM API** - 完整的 DOM API 实现（2025-11-09）
- ✅ **83+ 测试全部通过** - 核心功能 100% 验证
- 📚 **完整文档** - [BUILD_AND_TEST_REPORT.md](../BUILD_AND_TEST_REPORT.md), [TESTING.md](TESTING.md), [DOM_API.md](DOM_API.md)

---

## 📚 核心文档

### 1. [README.md](README.md)
**项目主页和快速入口**

- 📄 **内容**：项目简介、特性、快速示例、安装方法
- 👥 **目标读者**：所有人
- ⏱️ **阅读时间**：5分钟
- 🎯 **何时阅读**：第一次了解项目时

**关键内容：**
- ✨ 核心特性（轻量、高性能、易用、跨语言）
- 🚀 Python和C++快速示例
- 📊 与Electron/Qt/Tauri对比
- 🎯 当前开发状态

---

### 2. [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
**项目概述和价值主张**

- 📄 **内容**：详细的项目介绍、技术栈、架构图、对比分析
- 👥 **目标读者**：潜在用户、贡献者、决策者
- ⏱️ **阅读时间**：10-15分钟
- 🎯 **何时阅读**：想深入了解项目定位和技术选型时

**关键内容：**
- 🎯 核心价值主张
- 🏗️ 技术栈详解（QuickJS 600KB + Skia 5-8MB + SDL3 1-2MB + Yoga 1-2MB）
- 📊 详细对比表（vs Electron/Qt/Tauri/Dear ImGui）
- 💡 使用场景和示例

---

### 3. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)
**项目完整总结**

- 📄 **内容**：所有设计决策、技术选型理由、完整规划总结
- 👥 **目标读者**：核心开发者、架构师
- ⏱️ **阅读时间**：20-30分钟
- 🎯 **何时阅读**：需要全面了解项目所有方面时

**关键内容：**
- 🎯 技术选型决策过程（为什么选Preact而不是Solid.js/React）
- 🏗️ 架构设计总结
- 📅 开发计划总结（32周，6个阶段）
- 📊 性能目标和成功标准
- 💡 关键洞察和风险分析

---

## 🗺️ 规划文档

### 4. [ROADMAP.md](ROADMAP.md)
**32周详细开发路线图**

- 📄 **内容**：6个阶段、32周详细计划、里程碑、交付物
- 👥 **目标读者**：开发者、项目管理者
- ⏱️ **阅读时间**：15-20分钟
- 🎯 **何时阅读**：想了解开发进度和计划时

**关键内容：**
- **Phase 1** (Week 1-12): 核心框架 - SDL3+Skia+QuickJS+DOM+Layout+Event
- **Phase 2** (Week 13-16): Preact支持 - 完整DOM API
- **Phase 3** (Week 17-20): 组件库支持 - Ant Design/Material-UI
- **Phase 4** (Week 21-24): C API和Python绑定
- **Phase 5** (Week 25-28): 其他语言绑定（Rust/Go）
- **Phase 6** (Week 29-32): 优化和发布

**6个关键里程碑：**
- M1: 简单Preact应用可运行
- M2: Preact完全可用
- M3: 组件库可用
- M4: Python可用
- M5: 多语言支持
- M6: v1.0.0发布

---

## 🏗️ 架构文档

### 5. [ARCHITECTURE.md](ARCHITECTURE.md)
**技术架构和模块设计**

- 📄 **内容**：5层架构、核心模块、数据流、性能优化
- 👥 **目标读者**：开发者、架构师
- ⏱️ **阅读时间**：30-40分钟
- 🎯 **何时阅读**：开始开发前必读

**关键内容：**
- 🏗️ 5层架构详解
  - 应用层（Python/C++/Rust/Go）
  - 语言绑定层（ctypes/pybind11/bindgen/cgo）
  - C API层（lightui.h）
  - JavaScript运行时层（QuickJS + DOM + Event）
  - 渲染层（Yoga + Skia + SDL3）

- 📦 核心模块设计（含C++类定义）
  - Window模块
  - QuickJS Runtime模块
  - DOM模块（Node/Element/Text/Document）
  - Event模块
  - Layout模块（Yoga集成）
  - Render模块（Skia集成）
  - Bridge模块

- 🔄 关键流程
  - 启动流程
  - 渲染流程
  - 事件处理流程

---

### 6. [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)
**项目结构和构建系统**

- 📄 **内容**：完整目录结构、模块说明、构建流程、开发工作流
- 👥 **目标读者**：开发者
- ⏱️ **阅读时间**：15-20分钟
- 🎯 **何时阅读**：开始编码前

**关键内容：**
- 📁 完整目录结构
  ```
  lightui/
  ├── core/           # C++核心代码
  ├── third_party/    # 第三方库
  ├── js/             # JavaScript运行时
  ├── bindings/       # 语言绑定
  ├── tools/          # 工具
  ├── examples/       # 示例
  ├── tests/          # 测试
  └── docs/           # 文档
  ```

- 🔧 构建系统（CMake）
- 📦 依赖管理
- 🚀 构建和发布流程

---

## 💻 开发文档

### 7. [GETTING_STARTED.md](GETTING_STARTED.md)
**开发入门指南**

- 📄 **内容**：环境设置、快速开始、贡献方向、当前任务
- 👥 **目标读者**：新贡献者
- ⏱️ **阅读时间**：20-30分钟
- 🎯 **何时阅读**：准备开始贡献时

**关键内容：**
- 🚀 三步快速开始
  1. 了解项目（30分钟）
  2. 设置环境（1-2小时）
  3. 运行示例（15分钟）

- 🎯 6个贡献方向
  1. 核心引擎开发（C++）
  2. JavaScript运行时（QuickJS）
  3. 语言绑定（Python/Rust/Go）
  4. UI框架集成（Preact/React）
  5. 文档和示例
  6. 测试和质量保证

- 📋 当前优先级任务（Phase 1）

---

### 8. [CODING_STANDARDS.md](CODING_STANDARDS.md)
**编码规范和最佳实践**

- 📄 **内容**：C++/JavaScript/Python规范、命名约定、代码格式
- 👥 **目标读者**：所有开发者
- ⏱️ **阅读时间**：15-20分钟
- 🎯 **何时阅读**：开始编码前必读

**关键内容：**
- 📝 C++规范
  - 命名：PascalCase类，camelCase变量，member_变量
  - 格式：4空格缩进，K&R大括号
  - 内存：智能指针，RAII
  - 错误：异常+错误码

- 📝 JavaScript规范
  - 命名：camelCase
  - 格式：2空格缩进
  - 特性：ES6+，现代语法

- 📝 Python规范
  - 遵循PEP 8
  - 类型注解
  - Docstrings

- 📝 Git提交规范
  - 格式：`<type>(<scope>): <subject>`
  - 类型：feat/fix/docs/style/refactor/test/chore

---

### 9. [CONTRIBUTING.md](CONTRIBUTING.md)
**贡献指南**

- 📄 **内容**：如何贡献、开发流程、代码审查、社区规范
- 👥 **目标读者**：所有贡献者
- ⏱️ **阅读时间**：20-25分钟
- 🎯 **何时阅读**：准备提交PR前

**关键内容：**
- 🤝 行为准则
- 🔧 开发环境设置（Linux/macOS/Windows）
- 📝 提交代码流程
  1. Fork和Clone
  2. 创建分支
  3. 编写代码
  4. 提交更改
  5. 创建PR

- 🔍 代码审查流程
- 🐛 Bug报告模板
- 💡 功能请求模板
- 📚 文档贡献

---

## 📖 API文档

### 10. [API_DESIGN.md](API_DESIGN.md)
**C API完整设计**

- 📄 **内容**：C API规范、函数签名、错误处理、使用示例
- 👥 **目标读者**：C/C++开发者、语言绑定开发者
- ⏱️ **阅读时间**：30-40分钟
- 🎯 **何时阅读**：开发C API或语言绑定时

**关键内容：**
- 🎯 设计原则（简单、一致、安全、跨语言）
- 📝 核心API
  ```c
  // 初始化
  int lightui_init(void);
  void lightui_cleanup(void);
  
  // 窗口
  LightUIWindowHandle lightui_create_window(const char* title, int w, int h);
  void lightui_destroy_window(LightUIWindowHandle window);
  
  // UI加载
  int lightui_load_ui(LightUIWindowHandle window, const char* js_code);
  
  // 函数绑定
  int lightui_bind_function(LightUIWindowHandle window, const char* name,
                            LightUICallback callback, void* user_data);
  
  // 事件循环
  void lightui_run(LightUIWindowHandle window);
  ```

- 🔧 错误处理
- 🧵 线程安全
- 💾 内存管理
- 📊 性能考虑

---

### 11. [PYTHON_API.md](PYTHON_API.md)
**Python API文档和示例**

- 📄 **内容**：Python API、完整示例、最佳实践
- 👥 **目标读者**：Python开发者
- ⏱️ **阅读时间**：30-40分钟
- 🎯 **何时阅读**：使用Python开发应用时

**关键内容：**
- 🚀 快速开始
  ```python
  import lightui
  
  window = lightui.Window("App", 800, 600)
  
  @window.bind("getData")
  def get_data():
      return {"data": [...]}
  
  window.load_ui("...")
  window.run()
  ```

- 📚 完整API参考
  - Window类
  - 函数绑定
  - JavaScript调用
  - 事件循环

- 💡 完整示例
  - Todo应用
  - 数据可视化
  - 系统监控

- 🎯 最佳实践
- ❓ 常见问题

---

### 12. [TESTING.md](TESTING.md)
**测试文档和测试指南**

- 📄 **内容**：测试套件详情、运行方式、测试覆盖率、编写新测试
- 👥 **目标读者**：开发者、QA工程师
- ⏱️ **阅读时间**：20-30分钟
- 🎯 **何时阅读**：运行测试或编写新测试时

**关键内容：**
- 📊 测试统计
  - 83+ 个测试用例全部通过
  - 10 个测试套件
  - 核心功能 100% 覆盖

- 🧪 测试套件详情
  - DOM 测试 (78 个测试)
    - test_dom_node (25 tests)
    - test_dom_document (17 tests)
    - test_dom_query (27 tests)
    - test_dom_integration (9 tests)
  - 渲染测试 (3+ 个测试)
    - test_css_rendering
    - test_render_tree
  - JavaScript 测试 (2+ 个测试)
    - test_quickjs_runtime
    - test_simple

- 🚀 运行测试
  ```bash
  # Windows
  cd build/bin/Debug
  ./test_dom_node.exe
  ./test_css_rendering.exe

  # 运行所有测试
  cd build
  ctest -C Debug --output-on-failure
  ```

- 📝 编写新测试
  - 测试模板
  - 添加到构建系统
  - 测试最佳实践

---

### 13. [BUILD_AND_TEST_REPORT.md](../BUILD_AND_TEST_REPORT.md)
**构建和测试完成报告**

- 📄 **内容**：最新构建状态、测试结果、已知问题、下一步计划
- 👥 **目标读者**：项目管理者、开发者
- ⏱️ **阅读时间**：15-20分钟
- 🎯 **何时阅读**：了解项目最新状态时

**关键内容：**
- ✅ 执行摘要
  - Skia 渲染引擎已启用
  - 所有核心模块编译成功
  - 83+ 个测试全部通过

- 🔨 构建配置
  - 环境信息
  - 关键配置更改
  - 编译成功的模块

- 🧪 测试结果
  - 详细测试统计
  - 测试覆盖率
  - 性能指标

- 🐛 已知问题
  - 3 个次要问题
  - 不影响核心功能

- 🎯 下一步计划
  - 短期目标
  - 中期目标

---

## 📋 文档使用指南

### 按角色阅读

#### 🎯 项目评估者
1. [README.md](README.md) - 快速了解
2. [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) - 深入了解
3. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - 全面评估

#### 👨‍💻 核心开发者
1. [GETTING_STARTED.md](GETTING_STARTED.md) - 入门
2. [ARCHITECTURE.md](ARCHITECTURE.md) - 架构理解
3. [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - 项目结构
4. [CODING_STANDARDS.md](CODING_STANDARDS.md) - 编码规范
5. [ROADMAP.md](ROADMAP.md) - 开发计划

#### 🔧 语言绑定开发者
1. [API_DESIGN.md](API_DESIGN.md) - C API规范
2. [ARCHITECTURE.md](ARCHITECTURE.md) - 架构理解
3. [PYTHON_API.md](PYTHON_API.md) - Python示例参考

#### 🐍 Python应用开发者
1. [README.md](README.md) - 快速开始
2. [PYTHON_API.md](PYTHON_API.md) - API文档
3. [GETTING_STARTED.md](GETTING_STARTED.md) - 深入学习

#### 📝 文档贡献者
1. [CONTRIBUTING.md](CONTRIBUTING.md) - 贡献指南
2. 所有现有文档 - 了解内容
3. [CODING_STANDARDS.md](CODING_STANDARDS.md) - 文档规范

---

### 按任务阅读

#### 🎯 了解项目
1. [README.md](README.md)
2. [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
3. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)

#### 🚀 开始开发
1. [GETTING_STARTED.md](GETTING_STARTED.md)
2. [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)
3. [CODING_STANDARDS.md](CODING_STANDARDS.md)

#### 🏗️ 理解架构
1. [ARCHITECTURE.md](ARCHITECTURE.md)
2. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)
3. [API_DESIGN.md](API_DESIGN.md)

#### 📅 了解进度
1. [ROADMAP.md](ROADMAP.md)
2. [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)

#### 🤝 贡献代码
1. [CONTRIBUTING.md](CONTRIBUTING.md)
2. [CODING_STANDARDS.md](CODING_STANDARDS.md)
3. [GETTING_STARTED.md](GETTING_STARTED.md)

---

## 📊 文档统计

| 文档 | 字数 | 阅读时间 | 状态 |
|------|------|---------|------|
| README.md | ~2000 | 5分钟 | ✅ 完成 |
| PROJECT_OVERVIEW.md | ~3000 | 10分钟 | ✅ 完成 |
| PROJECT_SUMMARY.md | ~4000 | 20分钟 | ✅ 完成 |
| ROADMAP.md | ~3500 | 15分钟 | ✅ 完成 |
| ARCHITECTURE.md | ~5000 | 30分钟 | ✅ 完成 |
| PROJECT_STRUCTURE.md | ~3000 | 15分钟 | ✅ 完成 |
| GETTING_STARTED.md | ~3000 | 20分钟 | ✅ 完成 |
| CODING_STANDARDS.md | ~3500 | 15分钟 | ✅ 完成 |
| CONTRIBUTING.md | ~3000 | 20分钟 | ✅ 完成 |
| API_DESIGN.md | ~4000 | 30分钟 | ✅ 完成 |
| PYTHON_API.md | ~4000 | 30分钟 | ✅ 完成 |
| **总计** | **~38000** | **~3.5小时** | **11/11** |

---

## 🔄 文档更新

### 更新频率

- **README.md**: 每个版本更新
- **ROADMAP.md**: 每周更新进度
- **ARCHITECTURE.md**: 重大架构变更时更新
- **API_DESIGN.md**: API变更时更新
- **其他文档**: 根据需要更新

### 文档维护

- 📝 所有文档使用Markdown格式
- 🔍 定期检查链接有效性
- 📊 保持示例代码可运行
- 🌍 考虑多语言版本（中英文）

---

## 📞 反馈

如果你发现文档有任何问题：

- 🐛 [报告文档Bug](https://github.com/lightui/lightui/issues/new?labels=documentation)
- 💡 [建议改进](https://github.com/lightui/lightui/issues/new?labels=documentation,enhancement)
- 📝 [贡献文档](CONTRIBUTING.md#文档贡献)

---

## 📚 外部资源

### 学习资源

- **QuickJS**: https://bellard.org/quickjs/
- **Skia**: https://skia.org/docs/
- **SDL3**: https://wiki.libsdl.org/SDL3/
- **Yoga**: https://yogalayout.com/
- **Preact**: https://preactjs.com/

### 相关项目

- **Electron**: https://www.electronjs.org/
- **Tauri**: https://tauri.app/
- **Qt**: https://www.qt.io/
- **Flutter**: https://flutter.dev/

---

<div align="center">

**文档索引最后更新：2025-11-09**

[返回主页](README.md) | [开始开发](GETTING_STARTED.md) | [贡献代码](CONTRIBUTING.md)

</div>

