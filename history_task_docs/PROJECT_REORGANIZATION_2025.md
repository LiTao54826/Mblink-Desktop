# MBink 项目重组计划 2025

> **执行日期**: 2025-11-11  
> **目标**: 清理过时文档，建立严格规范，重新定义项目结构

---

## 📋 重组目标

### 1. 清理过时文档
- 删除重复、过时、临时的文档
- 整理历史文档到 `history_task_docs/`
- 保持根目录简洁

### 2. 建立严格规范
- 制定强制执行的开发规范
- 定义清晰的模块边界
- 规范文件组织和命名

### 3. 重新定义项目结构
- 明确项目定位（Electron替代品，非RmlUi竞品）
- 整理技术栈和架构
- 更新完成度报告

---

## 🗑️ 文档清理计划

### 需要删除的文档

#### 根目录 (删除 8 个)
- ❌ `TODO.md` → 合并到 `PHASE_2_5_PLAN.md`
- ❌ `PHASE_2_5_PLAN.md` → 移动到 `history_task_docs/`
- ❌ `README.md` (旧版) → 用新版替换

#### docs/ 目录 (删除 5 个)
- ❌ `docs/REFACTOR_PROGRESS_REPORT.md` → 移动到 `history_task_docs/`
- ❌ `docs/LEXBOR_REFACTOR_PLAN.md` → 移动到 `history_task_docs/`
- ❌ `docs/LEXBOR_INTEGRATION_SUMMARY.md` → 移动到 `history_task_docs/`
- ❌ `docs/REFACTOR_MASTER_PLAN.md` → 移动到 `history_task_docs/`
- ❌ `docs/PHASE_2_4_COMPLETION_REPORT.md` → 移动到 `history_task_docs/`

### 需要移动的文档

#### 移动到 history_task_docs/
- `PHASE_2_5_PLAN.md`
- `docs/REFACTOR_PROGRESS_REPORT.md`
- `docs/LEXBOR_REFACTOR_PLAN.md`
- `docs/LEXBOR_INTEGRATION_SUMMARY.md`
- `docs/REFACTOR_MASTER_PLAN.md`
- `docs/PHASE_2_4_COMPLETION_REPORT.md`

### 需要保留的核心文档

#### 根目录 (5个)
1. ✅ `README.md` - 项目主页 (更新)
2. ✅ `PROJECT_STATUS_2025.md` - 项目状态 (新建)
3. ✅ `ROADMAP.md` - 开发路线图 (更新)
4. ✅ `CMakeLists.txt` - 构建配置
5. ✅ `LICENSE` - 许可证

#### docs/ 目录 (保留并更新)
1. ✅ `docs/PROJECT_STANDARDS.md` - 项目规范 (新建)
2. ✅ `docs/ARCHITECTURE.md` - 架构设计 (更新)
3. ✅ `docs/API_DESIGN.md` - API设计
4. ✅ `docs/CODING_STANDARDS.md` - 代码规范
5. ✅ `docs/DOM_API.md` - DOM API文档
6. ✅ `docs/GETTING_STARTED.md` - 入门指南
7. ✅ `docs/CONTRIBUTING.md` - 贡献指南
8. ✅ `docs/TESTING.md` - 测试指南
9. ✅ `docs/PERFORMANCE.md` - 性能文档
10. ✅ `docs/EXAMPLES.md` - 示例文档

---

## 📁 新的文件组织结构

### 根目录结构

```
MBink/
├── README.md                    # 项目主页
├── PROJECT_STATUS_2025.md       # 项目状态报告
├── ROADMAP.md                   # 开发路线图
├── LICENSE                      # MIT许可证
├── CMakeLists.txt               # 构建配置
│
├── core/                        # 核心C++代码
│   ├── api/                     # C API接口
│   ├── dom/                     # DOM实现
│   ├── event/                   # 事件系统
│   ├── layout/                  # 布局引擎
│   ├── lexbor/                  # Lexbor包装
│   ├── quickjs/                 # QuickJS运行时
│   ├── render/                  # 渲染引擎
│   ├── utils/                   # 工具类
│   └── window/                  # 窗口管理
│
├── bindings/                    # 语言绑定
│   ├── python/
│   ├── rust/
│   ├── go/
│   └── nodejs/
│
├── js/                          # JavaScript运行时
│   ├── polyfills/               # Polyfills
│   ├── preact/                  # Preact库
│   └── runtime/                 # 运行时脚本
│
├── examples/                    # 示例代码
│   ├── hello_world.cpp
│   ├── counter_app.cpp
│   ├── animation_demo.cpp
│   └── ...
│
├── tests/                       # 测试代码
│   ├── unit/                    # 单元测试
│   ├── integration/             # 集成测试
│   └── benchmarks/              # 性能测试
│
├── docs/                        # 长期文档
│   ├── PROJECT_STANDARDS.md     # 项目规范 (新)
│   ├── ARCHITECTURE.md          # 架构设计
│   ├── API_DESIGN.md            # API设计
│   ├── CODING_STANDARDS.md      # 代码规范
│   ├── DOM_API.md               # DOM API
│   ├── GETTING_STARTED.md       # 入门指南
│   ├── CONTRIBUTING.md          # 贡献指南
│   ├── TESTING.md               # 测试指南
│   ├── PERFORMANCE.md           # 性能文档
│   └── EXAMPLES.md              # 示例文档
│
├── history_task_docs/           # 历史文档（只增不改）
│   ├── PHASE_2_1_REPORT.md
│   ├── PHASE_2_2_REPORT.md
│   ├── PHASE_2_3_REPORT.md
│   ├── PHASE_2_4_REPORT.md
│   ├── PHASE_2_5_PLAN.md        # 移动
│   ├── REFACTOR_PROGRESS_REPORT.md  # 移动
│   └── ...
│
├── ReferenceProject/            # 参考项目（只读）
│   └── RmlUi/                   # RmlUi参考
│
└── third_party/                 # 第三方库
    ├── SDL3/
    ├── skia/
    ├── quickjs/
    ├── yoga/
    ├── lexbor/
    └── ...
```

---

## 📝 文档更新计划

### 1. README.md (更新)

**更新内容**:
- ✅ 明确项目定位（Electron替代品）
- ✅ 更新技术栈说明
- ✅ 更新项目状态（Phase 2.5）
- ✅ 更新测试统计（155个测试）
- ✅ 添加与竞品对比

### 2. PROJECT_STATUS_2025.md (新建)

**内容**:
- ✅ 项目概览和定位
- ✅ 架构总览（5层架构）
- ✅ 已完成功能（Phase 1-2.4）
- ✅ 进行中功能（Phase 2.5）
- ✅ 待完成功能（Phase 2.6+）
- ✅ 测试状态和性能基准
- ✅ 已知问题
- ✅ 下一步计划

### 3. docs/PROJECT_STANDARDS.md (新建)

**内容**:
- ✅ 项目定位
- ✅ 强制规范（技术栈锁定、模块边界、文件组织）
- ✅ 架构规范
- ✅ 代码规范
- ✅ 文档规范
- ✅ 测试规范
- ✅ Git规范

### 4. docs/ARCHITECTURE.md (更新)

**更新内容**:
- 更新5层架构图
- 明确模块职责
- 添加与RmlUi的对比
- 更新技术选型说明

### 5. ROADMAP.md (更新)

**更新内容**:
- 更新Phase 2.5计划
- 添加Phase 2.6 (Lexbor完整集成)
- 添加Phase 3 (React生态支持)
- 添加Phase 4 (高级功能，参考RmlUi)
- 添加Phase 5 (多语言绑定)

---

## 🎯 RmlUi 借鉴计划

### 高优先级借鉴 (Phase 4)

#### 1. 拖拽系统 ⭐⭐⭐⭐⭐
**RmlUi特性**:
- dragstart, drag, dragmove, dragover, dragdrop, dragout, dragend
- 完整的拖拽生命周期

**MBink实施**:
- 创建 `core/event/drag_manager.h`
- 实现拖拽事件类型
- 集成到EventLoop
- 支持React DnD库

**预计时间**: 2-3周

#### 2. 焦点管理 ⭐⭐⭐⭐⭐
**RmlUi特性**:
- focus, blur事件
- Tab键导航
- 焦点陷阱（模态框）

**MBink实施**:
- 创建 `core/event/focus_manager.h`
- 实现焦点事件
- 实现Tab导航
- 支持React焦点管理

**预计时间**: 1-2周

#### 3. CSS动画和过渡 ⭐⭐⭐⭐⭐
**RmlUi特性**:
- CSS Transitions
- CSS Animations (@keyframes)
- CSS Transforms

**MBink实施**:
- 创建 `core/render/animation.h`
- 实现transition属性
- 实现@keyframes解析
- 集成到渲染循环

**预计时间**: 3-4周

#### 4. 键盘事件完善 ⭐⭐⭐⭐
**RmlUi特性**:
- keydown, keyup, textinput
- 修饰键状态（Ctrl/Shift/Alt/Meta）
- 按键标识符

**MBink实施**:
- 完善 `core/event/keyboard_event.h`
- 添加修饰键支持
- 实现textinput事件
- 支持IME输入

**预计时间**: 1周

### 中优先级借鉴 (Phase 5)

#### 5. 事件规范化系统 ⭐⭐⭐
**RmlUi特性**:
- EventSpecification (type, interruptible, bubbles)
- 事件注册表

**MBink实施**:
- 创建事件规范系统
- 支持自定义事件
- 类型安全

**预计时间**: 3-5天

### 不借鉴的部分

❌ **数据绑定系统** - React已提供  
❌ **装饰器系统** - 不符合MBink定位  
❌ **EventInstancer** - QuickJS已足够  
❌ **自研布局引擎** - Yoga已生产级

---

## 📊 重组后的项目指标

### 文档统计

| 类型 | 重组前 | 重组后 | 变化 |
|------|--------|--------|------|
| 根目录文档 | 5 | 5 | 0 |
| docs/文档 | 27 | 10 | -17 |
| history_task_docs/ | 40 | 46 | +6 |
| **总计** | 72 | 61 | **-11** |

### 代码统计

| 指标 | 数值 |
|------|------|
| C++代码 | ~12,000 行 |
| JavaScript代码 | ~2,000 行 |
| 测试代码 | ~5,000 行 |
| 文档 | ~8,000 行 |

### 模块完成度

| 模块 | 完成度 | 测试覆盖率 |
|------|--------|-----------|
| core/window | 100% | 95% |
| core/dom | 95% | 95% |
| core/layout | 90% | 60% |
| core/render | 85% | 70% |
| core/event | 60% | 90% |
| core/quickjs | 70% | 85% |
| core/lexbor | 25% | 95% |
| **总体** | **75%** | **85%** |

---

## ✅ 执行清单

### 第一步: 文档清理 (30分钟)

- [ ] 移动 `PHASE_2_5_PLAN.md` 到 `history_task_docs/`
- [ ] 移动 `docs/REFACTOR_PROGRESS_REPORT.md` 到 `history_task_docs/`
- [ ] 移动 `docs/LEXBOR_REFACTOR_PLAN.md` 到 `history_task_docs/`
- [ ] 移动 `docs/LEXBOR_INTEGRATION_SUMMARY.md` 到 `history_task_docs/`
- [ ] 移动 `docs/REFACTOR_MASTER_PLAN.md` 到 `history_task_docs/`
- [ ] 移动 `docs/PHASE_2_4_COMPLETION_REPORT.md` 到 `history_task_docs/`
- [ ] 删除 `TODO.md`（内容已合并）

### 第二步: 创建新文档 (1小时)

- [x] 创建 `docs/PROJECT_STANDARDS.md`
- [x] 创建 `PROJECT_STATUS_2025.md`
- [x] 创建 `PROJECT_REORGANIZATION_2025.md`（本文档）

### 第三步: 更新现有文档 (1小时)

- [ ] 更新 `README.md`
- [ ] 更新 `docs/ARCHITECTURE.md`
- [ ] 更新 `ROADMAP.md`

### 第四步: Git提交 (10分钟)

```bash
git add .
git commit -m "docs: 项目重组2025 - 清理过时文档，建立严格规范"
git push
```

---

## 🎯 重组后的优势

### 1. 清晰的项目定位
- ✅ 明确是Electron替代品，不是RmlUi竞品
- ✅ 强调React生态支持
- ✅ 突出轻量级优势

### 2. 严格的开发规范
- ✅ 技术栈锁定，避免随意更改
- ✅ 模块边界清晰，避免循环依赖
- ✅ 文件组织规范，易于维护

### 3. 完整的文档体系
- ✅ 长期文档（docs/）持续更新
- ✅ 历史文档（history_task_docs/）只增不改
- ✅ 根目录简洁，只保留核心文档

### 4. 明确的发展路线
- ✅ Phase 2.5: JavaScript基础设施
- ✅ Phase 2.6: Lexbor完整集成
- ✅ Phase 3: React生态支持
- ✅ Phase 4: 高级功能（借鉴RmlUi）
- ✅ Phase 5: 多语言绑定

---

**执行日期**: 2025-11-11  
**执行人**: MBink Team  
**状态**: 进行中

