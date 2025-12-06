# 高级布局对比测试报告

**测试日期**: 2025-12-06
**测试环境**: MBink Layout Engine vs Chrome Browser
**视口宽度**: 453px (Browser) / 438px (MBink, DPI=2x)

---

## 📊 测试结果总览

| 指标 | 初始值 | 当前值 |
|------|--------|--------|
| 总测试元素 | 159 | 159 |
| ✅ 通过 | 84 | 117 |
| ❌ 失败 | 75 | 42 |
| ⚠️ 缺失 | 0 | 0 |
| 📈 通过率 | **52.8%** | **73.6%** |
| 🏆 评级 | **D级 (不合格)** | **C级 (及格)** |

**容差标准**: 宽度/X ±1px, 高度/Y ±3px

---

## 🔧 已修复问题

### ✅ P0-1: min-width > max-width 优先级 (ADV-2-01)
- **问题**: 当 `min-width > max-width` 时，应该 `min-width` 优先
- **修复**: 修改 `MaybeMath::MaybeClamp()` 先应用 max，再应用 min

### ✅ P0-2: place-items/place-self 未实现 (ADV-3-03, ADV-3-04)
- **问题**: Grid 子项默认拉伸，`place-items: center` 未正确收缩子项
- **修复**: 添加 `place-items`、`place-self`、`place-content` 简写属性解析

### ✅ P0-3: grid-auto-rows 未实现 (ADV-5-02)
- **问题**: `grid-auto-rows: 60px` 未生效
- **修复**: 添加 `grid_auto_rows` 和 `grid_auto_columns` 字段并在创建隐式轨道时使用

### ✅ P1-1: flex-basis:0 空间分配 (ADV-6-02)
- **问题**: `flex-basis: 0` 时空间分配不正确
- **修复**: `flex_basis` 必须 floor 到 `padding + border` 之和

### ✅ P1-2: Grid 子项对齐 (ADV-1-02)
- **问题**: Grid 单元格内的 Flex 容器无法正确对齐子项
- **修复**: 在 Grid 布局中添加最终布局传递，使用已知尺寸

### ✅ P1-3: Grid 显式尺寸拉伸 (ADV-4-02-A)
- **问题**: 有显式 width/height 的 Grid 子项仍被拉伸
- **修复**: 检查子项是否有显式尺寸，有则不拉伸

### ✅ P1-4: Grid 负数索引 (ADV-5-03-C)
- **问题**: `grid-column: 1 / -1` 计算错误
- **修复**: 使用公式 `num_cols + 1 + end_line` 计算负数索引

### ✅ P1-5: flex-basis 不应被 min-width 限制 (ADV-2-03)
- **问题**: `flex-basis` 被 `min-width` 错误限制
- **修复**: 只对 `flex-basis` 应用 `max-width`，`min-width` 在 flex 算法的 freeze/clamp 步骤应用

### ✅ P1-6: Baseline 对齐 (ADV-3-05-A)
- **问题**: `align-items: baseline` 计算不正确
- **修复**: 实现两遍算法：第一遍找最大 baseline，第二遍对齐

### ✅ P1-7: Grid fr 轨道空间分配 (ADV-7-02)
- **问题**: `grid-template-rows: auto 1fr auto` 时，额外空间平均分配给所有行
- **修复**: 优先将额外空间分配给 fr 轨道

### ✅ P1-8: Grid 自动放置 (ADV-7-02)
- **问题**: 有显式列但无显式行的项目没有自动放置到下一行
- **修复**: 添加 `has_explicit_col && !has_explicit_row` 情况的处理

---

## 📋 问题分类分析

### 🔴 严重问题 (需要优先修复)

#### 1. Grid place-items/place-self 未实现 (ADV-3-03, ADV-3-04)
**影响测试**: 4个元素  
**问题描述**: `place-items: center` 和 `justify-self`/`align-self` 覆盖未正确工作

| 元素 | 期望宽度 | 实际宽度 | 期望位置 | 实际位置 |
|------|---------|---------|----------|----------|
| ADV-3-03-A | 50px | 183px | (66.5, 40) | (0, 0) |
| ADV-3-03-B | 50px | 183px | (259.5, 40) | (193, 0) |
| ADV-3-04-B | 50px | 183px | - | 拉伸到整个单元格 |

**根因**: Grid 子项默认拉伸，`place-items: center` 未正确收缩子项到内容尺寸

#### 2. Grid grid-template-rows 未实现 (ADV-5-01, ADV-5-02)
**影响测试**: 12个元素  
**问题描述**: `grid-template-rows: 50px 80px 50px` 和 `grid-auto-rows: 60px` 未生效

| 元素 | 期望高度 | 实际高度 |
|------|---------|---------|
| ADV-5-02-A~F | 60px | 41px (仅内容高度) |

**根因**: `grid-template-rows` 和 `grid-auto-rows` CSS 属性未解析或未应用

#### 3. Grid 命名网格区域问题 (ADV-7-02 圣杯布局)
**影响测试**: 5个元素  
**问题描述**: `grid-template-areas` 布局完全错误

| 元素 | 期望位置 | 实际位置 |
|------|----------|----------|
| header | (0, 0) 全宽 | 错误尺寸 |
| left | (0, 43.5) | (276, 0) 完全错位 |
| main | (105, 43.5) | (0, 82) 错位 |
| right | (276, 43.5) | (105, 82) 错位 |
| footer | (0, 161.5) 全宽 | 错误尺寸和位置 |

**根因**: `grid-template-areas` 和 `grid-area` 未正确实现

#### 4. min-width > max-width 冲突处理 (ADV-2-01)
**影响测试**: 1个元素  
**问题描述**: 当 `min-width: 200px` 且 `max-width: 100px` 时，应该 min-width 优先

| 元素 | 期望宽度 | 实际宽度 |
|------|---------|---------|
| ADV-2-01-box | 200px | 100px |

**根因**: min-width 应该覆盖 max-width，但当前 max-width 优先

---

### 🟠 中等问题

#### 5. Flex baseline 对齐 (ADV-3-05)
**影响测试**: 2个元素  
**问题描述**: `align-items: baseline` 未正确计算不同字体大小的基线位置

| 元素 | 期望 relY | 实际 relY |
|------|----------|----------|
| ADV-3-05-A | 23px | 10px |
| ADV-3-05-C | 13.5px | 10px |

**根因**: 不同 font-size 元素的 baseline 偏移计算不正确

#### 6. flex-basis vs min-width 交互 (ADV-2-03)
**影响测试**: 2个元素  
**问题描述**: `flex-basis: 100px` + `min-width: 150px` 应该使用 min-width

| 元素 | 期望宽度 | 实际宽度 |
|------|---------|---------|
| ADV-2-03-A | 195px | 220px |
| ADV-2-03-B | 195px | 170px |

**根因**: flex-basis 和 min-width 的优先级处理不正确

#### 7. flex-basis: 0 vs auto (ADV-6-02)
**影响测试**: 3个元素  
**问题描述**: `flex-basis: 0` 应该让 flex-grow 完全基于增长比例分配空间

| 元素 | 期望宽度 | 实际宽度 |
|------|---------|---------|
| ADV-6-02-A | 71.5px | 61.5px |
| ADV-6-02-B | 208.5px | 218.5px |

**根因**: `flex-basis: 0` 时的空间分配算法不正确

#### 8. 嵌套 Flex 收缩 (ADV-6-03)
**影响测试**: 4个元素  
**问题描述**: 嵌套 Flex 容器的收缩行为不正确

| 元素 | 期望高度 | 实际高度 |
|------|---------|---------|
| ADV-6-03-A | 62px | 41px |
| ADV-6-03-B | 62px | 41px |

**根因**: 嵌套 Flex 的最小内容尺寸计算不正确

---

### 🟡 轻微问题

#### 9. Grid 子项溢出 (ADV-4-02)
**影响测试**: 1个元素  
**问题描述**: 子项宽度超出 grid-template-columns 定义的单元格宽度

| 元素 | 期望宽度 | 实际宽度 |
|------|---------|---------|
| ADV-4-02-A | 150px | 100px |

#### 10. Grid column span (ADV-5-03)
**影响测试**: 1个元素  
**问题描述**: `grid-column: 1 / -1` 未正确跨越全部列

| 元素 | 期望宽度 | 实际宽度 |
|------|---------|---------|
| ADV-5-03-C | 356px | 265.8px |

---

## ✅ 通过的测试类别

以下功能工作正常：

| 类别 | 通过测试数 | 说明 |
|------|-----------|------|
| 基础 Flex 布局 | 多数 | 基本的 flex 方向、对齐工作正常 |
| 基础 Grid 布局 | 多数 | grid-template-columns 基本工作 |
| margin:auto 居中 | ✅ | ADV-3-01 通过 |
| margin-left:auto 推动 | ✅ | ADV-3-02 通过 |
| 粘性页脚布局 | ✅ | ADV-7-01 完全通过 |
| 等高列布局 | ✅ | ADV-7-03 完全通过 |
| 媒体对象布局 | ✅ | ADV-7-05 完全通过 |
| 双飞翼布局 | ✅ | ADV-7-06 完全通过 |

---

## 🔧 修复优先级建议

### P0 - 立即修复 (影响多个测试)

1. **实现 `grid-template-rows` 和 `grid-auto-rows`**
   - 影响: ADV-5-01, ADV-5-02 (12个元素)
   - 预计提升: +7.5% 通过率

2. **实现 `place-items` 和 `place-self`**
   - 影响: ADV-3-03, ADV-3-04 (4个元素)
   - 预计提升: +2.5% 通过率

3. **实现 `grid-template-areas`**
   - 影响: ADV-7-02 (5个元素)
   - 预计提升: +3% 通过率

### P1 - 高优先级

4. **修复 min-width > max-width 优先级**
   - 影响: ADV-2-01 (1个元素)

5. **修复 flex-basis: 0 空间分配**
   - 影响: ADV-6-02 (3个元素)

6. **修复 baseline 对齐计算**
   - 影响: ADV-3-05 (2个元素)

### P2 - 中优先级

7. **修复嵌套 Flex 高度计算**
8. **修复 Grid column span 到末尾**

---

## 📈 预期改进

如果完成 P0 和 P1 修复：

| 指标 | 当前 | 预期 |
|------|------|------|
| 通过率 | 52.8% | ~80% |
| 评级 | D级 | B级 |

---

## 📁 相关文件

- 浏览器数据: `tests/layout_comparison/browser_advanced_data.json`
- MBink 输出: `tests/layout_comparison/mbink_advanced_output.txt`
- 对比脚本: `tests/layout_comparison/compare_advanced.py`
- 详细报告: `tests/layout_comparison/advanced_comparison_report.json`

