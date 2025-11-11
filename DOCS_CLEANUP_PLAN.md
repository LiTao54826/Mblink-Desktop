# 文档清理计划 - 第二阶段

> **创建日期**: 2025-11-11  
> **目标**: 清理docs目录中的重复、过时文档，保持文档体系简洁

---

## 📋 当前docs目录文档分析

### 当前文档列表 (24个)

1. API_DESIGN.md
2. ARCHITECTURE.md ✅ 已更新
3. BUGFIX_TEXT_RENDERING.md
4. CODING_STANDARDS.md
5. CONTRIBUTING.md
6. DOCUMENTATION_INDEX.md
7. DOM_API.md
8. EXAMPLES.md
9. GETTING_STARTED.md
10. PERFORMANCE.md
11. PROJECT_OVERVIEW.md
12. PROJECT_STANDARDS.md ✅ 新建
13. PROJECT_STRUCTURE.md
14. PROJECT_SUMMARY.md
15. PROXY_SETUP.md
16. PYTHON_API.md
17. QUICKSTART.md
18. README.md
19. ROADMAP.md ✅ 已更新
20. TECH_STACK_ANALYSIS.md
21. TECH_STACK_DECISION_SUMMARY.md
22. TESTING.md
23. THIRD_PARTY_INTEGRATION_PLAN.md
24. WINDOWS_UTF8.md

---

## 🗑️ 建议清理的文档

### 类别1: 重复文档（应合并或删除）

#### 1. PROJECT_OVERVIEW.md vs PROJECT_SUMMARY.md
**问题**: 内容重复，都是项目概述  
**建议**: 
- ❌ 删除 `PROJECT_OVERVIEW.md`
- ❌ 删除 `PROJECT_SUMMARY.md`
- ✅ 内容已包含在 `README.md` 和 `PROJECT_STATUS_2025.md` 中

#### 2. QUICKSTART.md vs GETTING_STARTED.md
**问题**: 都是入门指南，内容可能重复  
**建议**: 
- ✅ 保留 `GETTING_STARTED.md`（更详细）
- ❌ 删除 `QUICKSTART.md`（或合并到GETTING_STARTED.md）

#### 3. README.md (docs目录)
**问题**: docs目录不需要单独的README  
**建议**: 
- ❌ 删除 `docs/README.md`
- ✅ 使用 `DOCUMENTATION_INDEX.md` 作为文档索引

#### 4. PROJECT_STRUCTURE.md
**问题**: 内容已包含在 `ARCHITECTURE.md` 和 `PROJECT_STANDARDS.md` 中  
**建议**: 
- ❌ 删除 `PROJECT_STRUCTURE.md`
- ✅ 内容已在其他文档中

### 类别2: 历史文档（应移动到history_task_docs/）

#### 5. TECH_STACK_ANALYSIS.md
**问题**: 技术栈分析是历史决策文档  
**建议**: 
- 📦 移动到 `history_task_docs/TECH_STACK_ANALYSIS.md`

#### 6. TECH_STACK_DECISION_SUMMARY.md
**问题**: 技术栈决策总结是历史文档  
**建议**: 
- 📦 移动到 `history_task_docs/TECH_STACK_DECISION_SUMMARY.md`

#### 7. THIRD_PARTY_INTEGRATION_PLAN.md
**问题**: 第三方库集成计划已完成，是历史文档  
**建议**: 
- 📦 移动到 `history_task_docs/THIRD_PARTY_INTEGRATION_PLAN.md`

#### 8. BUGFIX_TEXT_RENDERING.md
**问题**: 特定Bug修复文档，应该是历史记录  
**建议**: 
- 📦 移动到 `history_task_docs/BUGFIX_TEXT_RENDERING.md`

### 类别3: 特定环境文档（可移动到单独目录）

#### 9. PROXY_SETUP.md
**问题**: 特定环境配置，不是核心文档  
**建议**: 
- 📦 移动到 `docs/setup/PROXY_SETUP.md`（或删除，放到Wiki）

#### 10. WINDOWS_UTF8.md
**问题**: 特定平台配置，不是核心文档  
**建议**: 
- 📦 移动到 `docs/setup/WINDOWS_UTF8.md`（或删除，放到Wiki）

---

## ✅ 应保留的核心文档 (12个)

### 架构和设计 (4个)
1. ✅ **ARCHITECTURE.md** - 架构设计（已更新）
2. ✅ **API_DESIGN.md** - API设计
3. ✅ **PROJECT_STANDARDS.md** - 项目规范（新建）
4. ✅ **ROADMAP.md** - 开发路线图（已更新）

### 开发指南 (4个)
5. ✅ **GETTING_STARTED.md** - 入门指南
6. ✅ **CODING_STANDARDS.md** - 代码规范
7. ✅ **CONTRIBUTING.md** - 贡献指南
8. ✅ **TESTING.md** - 测试指南

### API文档 (2个)
9. ✅ **DOM_API.md** - DOM API文档
10. ✅ **PYTHON_API.md** - Python API文档

### 其他 (2个)
11. ✅ **PERFORMANCE.md** - 性能文档
12. ✅ **EXAMPLES.md** - 示例文档
13. ✅ **DOCUMENTATION_INDEX.md** - 文档索引（需更新）

---

## 📝 执行计划

### 第一步: 删除重复文档 (5个)

```bash
# 删除重复的项目概述文档
rm docs/PROJECT_OVERVIEW.md
rm docs/PROJECT_SUMMARY.md

# 删除重复的入门指南（或合并后删除）
rm docs/QUICKSTART.md

# 删除docs目录的README
rm docs/README.md

# 删除重复的项目结构文档
rm docs/PROJECT_STRUCTURE.md
```

### 第二步: 移动历史文档 (4个)

```bash
# 移动技术栈分析文档
mv docs/TECH_STACK_ANALYSIS.md history_task_docs/
mv docs/TECH_STACK_DECISION_SUMMARY.md history_task_docs/

# 移动集成计划文档
mv docs/THIRD_PARTY_INTEGRATION_PLAN.md history_task_docs/

# 移动Bug修复文档
mv docs/BUGFIX_TEXT_RENDERING.md history_task_docs/
```

### 第三步: 移动环境配置文档 (2个)

```bash
# 创建setup目录
mkdir -p docs/setup

# 移动环境配置文档
mv docs/PROXY_SETUP.md docs/setup/
mv docs/WINDOWS_UTF8.md docs/setup/
```

### 第四步: 更新文档索引

更新 `docs/DOCUMENTATION_INDEX.md`，反映新的文档结构。

---

## 📊 清理前后对比

| 类型 | 清理前 | 清理后 | 变化 |
|------|--------|--------|------|
| docs/ 核心文档 | 24 | 13 | -11 |
| docs/setup/ | 0 | 2 | +2 |
| history_task_docs/ | 46 | 50 | +4 |
| **总计** | 70 | 65 | **-5** |

---

## 🎯 清理后的docs目录结构

```
docs/
├── ARCHITECTURE.md              # 架构设计
├── API_DESIGN.md                # API设计
├── PROJECT_STANDARDS.md         # 项目规范（强制）
├── ROADMAP.md                   # 开发路线图
│
├── GETTING_STARTED.md           # 入门指南
├── CODING_STANDARDS.md          # 代码规范
├── CONTRIBUTING.md              # 贡献指南
├── TESTING.md                   # 测试指南
│
├── DOM_API.md                   # DOM API文档
├── PYTHON_API.md                # Python API文档
│
├── PERFORMANCE.md               # 性能文档
├── EXAMPLES.md                  # 示例文档
├── DOCUMENTATION_INDEX.md       # 文档索引
│
└── setup/                       # 环境配置
    ├── PROXY_SETUP.md           # 代理配置
    └── WINDOWS_UTF8.md          # Windows UTF8配置
```

---

## ✅ 执行清单

### 删除文档 (5个)
- [x] 删除 `docs/PROJECT_OVERVIEW.md` ✅
- [x] 删除 `docs/PROJECT_SUMMARY.md` ✅
- [x] 删除 `docs/QUICKSTART.md` ✅
- [x] 删除 `docs/README.md` ✅
- [x] 删除 `docs/PROJECT_STRUCTURE.md` ✅

### 移动到历史 (4个)
- [x] 移动 `docs/TECH_STACK_ANALYSIS.md` ✅
- [x] 移动 `docs/TECH_STACK_DECISION_SUMMARY.md` ✅
- [x] 移动 `docs/THIRD_PARTY_INTEGRATION_PLAN.md` ✅
- [x] 移动 `docs/BUGFIX_TEXT_RENDERING.md` ✅

### 移动到setup (2个)
- [x] 创建 `docs/setup/` 目录 ✅
- [x] 移动 `docs/PROXY_SETUP.md` ✅
- [x] 移动 `docs/WINDOWS_UTF8.md` ✅

### 更新文档 (2个)
- [x] 更新 `docs/DOCUMENTATION_INDEX.md` ✅
- [x] 移动旧版到 `history_task_docs/DOCUMENTATION_INDEX_OLD.md` ✅

---

## 🤔 需要确认的问题

### 问题1: QUICKSTART.md
**选项A**: 直接删除（如果内容已包含在GETTING_STARTED.md中）  
**选项B**: 合并到GETTING_STARTED.md后删除  
**建议**: 先检查内容，再决定

### 问题2: 环境配置文档
**选项A**: 移动到 `docs/setup/`  
**选项B**: 直接删除（放到项目Wiki）  
**建议**: 移动到 `docs/setup/`，保留但不作为核心文档

---

## 📚 清理后的文档体系

### 根目录 (5个核心文档)
1. `README.md` - 项目主页
2. `PROJECT_STATUS_2025.md` - 项目状态
3. `PROJECT_REORGANIZATION_2025.md` - 重组计划
4. `PROJECT_REORGANIZATION_SUMMARY.md` - 重组总结
5. `LICENSE` - 许可证

### docs/ (13个长期文档)
- 架构和设计 (4个)
- 开发指南 (4个)
- API文档 (2个)
- 其他 (3个)

### docs/setup/ (2个环境配置)
- 代理配置
- Windows UTF8配置

### history_task_docs/ (50个历史文档)
- 所有Phase报告
- 所有历史决策文档
- 所有Bug修复记录

---

**创建日期**: 2025-11-11  
**执行建议**: 在执行前先备份，确认无误后再删除  
**下次审查**: 2025-11-18

