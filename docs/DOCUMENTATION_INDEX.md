# MBink 文档索引

> **最后更新**: 2025-11-11  
> **版本**: 0.3.0-alpha  
> **总进度**: 65%  
> **项目定位**: 轻量级跨平台桌面应用框架 - Electron的轻量级替代品

---

## 🎯 快速开始

如果你是第一次接触MBink，建议按以下顺序阅读：

1. **[README.md](../README.md)** - 5分钟了解项目
2. **[PROJECT_STATUS_2025.md](../PROJECT_STATUS_2025.md)** - 10分钟了解当前状态（推荐）
3. **[GETTING_STARTED.md](GETTING_STARTED.md)** - 30分钟开始开发

---

## 📚 核心文档

### 根目录文档

| 文档 | 描述 | 阅读时间 |
|------|------|---------|
| [README.md](../README.md) | 项目主页、特性、快速示例 | 5分钟 |
| [PROJECT_STATUS_2025.md](../PROJECT_STATUS_2025.md) | 完整的项目状态报告 | 15分钟 |
| [PROJECT_REORGANIZATION_2025.md](../PROJECT_REORGANIZATION_2025.md) | 项目重组计划 | 10分钟 |
| [LICENSE](../LICENSE) | MIT许可证 | 2分钟 |

---

## 🏗️ 架构和设计

### 架构文档

| 文档 | 描述 | 目标读者 |
|------|------|---------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | 5层架构设计、模块职责、数据流 | 开发者、架构师 |
| [API_DESIGN.md](API_DESIGN.md) | C API设计、接口规范 | API用户、绑定开发者 |
| [PROJECT_STANDARDS.md](PROJECT_STANDARDS.md) | **强制执行的开发规范** | **所有开发者（必读）** |
| [ROADMAP.md](ROADMAP.md) | 开发路线图、Phase计划 | 项目管理者、贡献者 |

**重点推荐**:
- 🔥 **PROJECT_STANDARDS.md** - 所有开发者必读，包含强制规范
- 🔥 **ARCHITECTURE.md** - 了解项目架构和技术选型

---

## 💻 开发指南

### 入门和规范

| 文档 | 描述 | 何时阅读 |
|------|------|---------|
| [GETTING_STARTED.md](GETTING_STARTED.md) | 环境搭建、编译、运行 | 开始开发前 |
| [CODING_STANDARDS.md](CODING_STANDARDS.md) | C++/JavaScript代码规范 | 编写代码前 |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 贡献指南、PR流程 | 提交代码前 |
| [TESTING.md](TESTING.md) | 测试规范、如何编写测试 | 编写测试前 |

**开发流程**:
1. 阅读 `GETTING_STARTED.md` 搭建环境
2. 阅读 `PROJECT_STANDARDS.md` 了解强制规范
3. 阅读 `CODING_STANDARDS.md` 了解代码风格
4. 开始开发
5. 阅读 `TESTING.md` 编写测试
6. 阅读 `CONTRIBUTING.md` 提交PR

---

## 📖 API文档

### API参考

| 文档 | 描述 | 目标读者 |
|------|------|---------|
| [DOM_API.md](DOM_API.md) | DOM API完整参考 | JavaScript开发者 |
| [PYTHON_API.md](PYTHON_API.md) | Python绑定API | Python开发者 |

**示例代码**:
- [examples/](../examples/) - C++示例
- [examples/python/](../examples/python/) - Python示例

---

## 🚀 性能和示例

### 性能和示例文档

| 文档 | 描述 | 目标读者 |
|------|------|---------|
| [PERFORMANCE.md](PERFORMANCE.md) | 性能优化指南、基准测试 | 性能优化者 |
| [EXAMPLES.md](EXAMPLES.md) | 示例代码说明 | 学习者 |

---

## 🔧 环境配置

### 特定环境配置

| 文档 | 描述 | 何时需要 |
|------|------|---------|
| [setup/PROXY_SETUP.md](setup/PROXY_SETUP.md) | 代理配置 | 在代理环境下开发 |
| [setup/WINDOWS_UTF8.md](setup/WINDOWS_UTF8.md) | Windows UTF8配置 | Windows开发环境 |

---

## 📜 历史文档

所有历史文档（Phase报告、历史决策、Bug修复记录等）都在：

- **[history_task_docs/](../history_task_docs/)** - 历史文档归档（只增不改）

**重要历史文档**:
- `PHASE_2_1_COMPLETION_REPORT.md` - JavaScript运行时完成报告
- `PHASE_2_2_FINAL_REPORT.md` - DOM API完成报告
- `PHASE_2_3_COMPLETION_SUMMARY.md` - 布局引擎完成报告
- `PHASE_2_4_COMPLETION_REPORT.md` - 窗口和事件系统完成报告
- `PHASE_2_5_PLAN.md` - JavaScript基础设施计划（已完成）

---

## 🔍 参考项目

### RmlUi参考

- **[ReferenceProject/RmlUi/](../ReferenceProject/RmlUi/)** - RmlUi参考项目

**借鉴内容**:
- 拖拽系统 (drag-and-drop)
- 焦点管理 (focus management)
- CSS动画和过渡 (animations & transitions)
- 键盘事件 (keyboard events)

**不借鉴内容**:
- 数据绑定系统（React已提供）
- 装饰器系统（不符合定位）
- 自研布局引擎（Yoga已足够）

---

## 📊 文档统计

### 文档分布

| 位置 | 数量 | 说明 |
|------|------|------|
| 根目录 | 5 | 核心文档 |
| docs/ | 13 | 长期文档 |
| docs/setup/ | 2 | 环境配置 |
| history_task_docs/ | 50+ | 历史文档 |
| **总计** | **70+** | |

### 文档类型

| 类型 | 数量 | 示例 |
|------|------|------|
| 架构设计 | 4 | ARCHITECTURE.md, API_DESIGN.md |
| 开发指南 | 4 | GETTING_STARTED.md, CODING_STANDARDS.md |
| API文档 | 2 | DOM_API.md, PYTHON_API.md |
| 性能和示例 | 2 | PERFORMANCE.md, EXAMPLES.md |
| 环境配置 | 2 | PROXY_SETUP.md, WINDOWS_UTF8.md |
| 历史文档 | 50+ | PHASE_*_REPORT.md |

---

## 🎯 按角色推荐阅读

### 新手开发者
1. README.md
2. PROJECT_STATUS_2025.md
3. GETTING_STARTED.md
4. CODING_STANDARDS.md
5. EXAMPLES.md

### 贡献者
1. PROJECT_STANDARDS.md ⭐ 必读
2. ARCHITECTURE.md
3. CODING_STANDARDS.md
4. CONTRIBUTING.md
5. TESTING.md

### 架构师
1. ARCHITECTURE.md
2. PROJECT_STANDARDS.md
3. API_DESIGN.md
4. ROADMAP.md
5. PERFORMANCE.md

### API用户
1. README.md
2. DOM_API.md
3. PYTHON_API.md
4. EXAMPLES.md

### 项目管理者
1. PROJECT_STATUS_2025.md
2. ROADMAP.md
3. ARCHITECTURE.md
4. history_task_docs/ (历史进度)

---

## 🔗 外部资源

### 技术文档
- [QuickJS Documentation](https://bellard.org/quickjs/)
- [Skia Documentation](https://skia.org/docs/)
- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/)
- [Yoga Documentation](https://yogalayout.com/docs)
- [Lexbor Documentation](https://lexbor.com/docs/)

### 参考项目
- [RmlUi Documentation](https://mikke89.github.io/RmlUiDoc/)
- [Electron Documentation](https://www.electronjs.org/docs)
- [Tauri Documentation](https://tauri.app/v1/guides/)
- [React Documentation](https://react.dev/)

---

## 📝 文档维护

### 文档更新规则

1. **长期文档** (docs/) - 持续更新，反映最新状态
2. **历史文档** (history_task_docs/) - 只增不改，保留历史记录
3. **根目录文档** - 核心文档，定期更新

### 文档审查周期

- **每周**: 更新 PROJECT_STATUS_2025.md
- **每月**: 审查 ROADMAP.md
- **每季度**: 审查所有长期文档

### 贡献文档

如果你想贡献文档，请：
1. 阅读 [CONTRIBUTING.md](CONTRIBUTING.md)
2. 遵循 [PROJECT_STANDARDS.md](PROJECT_STANDARDS.md) 中的文档规范
3. 提交PR

---

**最后更新**: 2025-11-11  
**维护者**: MBink Team  
**反馈**: 如有文档问题，请提Issue

