# MBink 布局系统测试计划

## 概述

本文档定义了 MBink 布局引擎的全面测试计划，旨在确保布局系统的正确性、稳定性和浏览器兼容性。

## 1. 测试目标

### 1.1 核心目标
- **正确性**：布局结果与浏览器一致（误差 ≤ 2px）
- **稳定性**：多次布局调用不会产生不一致的结果
- **性能**：布局计算在可接受的时间内完成
- **回归防护**：新修改不会破坏现有功能

### 1.2 覆盖范围
- Block 布局
- Flexbox 布局
- Grid 布局
- IFC（Inline Formatting Context）布局
- 嵌套布局组合

---

## 2. 单元测试

### 2.1 IFC 布局测试

#### 2.1.1 RunMode 行为测试
```cpp
// tests/unit/ifc_layout_test.cpp

TEST(IFCLayout, ComputeSizeShouldNotModifyRenderPosition) {
    // 场景：验证 ComputeSize 模式不修改渲染对象位置
    // 1. 创建带 text-align: center 的容器
    // 2. 调用 PerformLayout（设置正确位置）
    // 3. 调用 ComputeSize（不应覆盖位置）
    // 4. 验证位置仍然正确
}

TEST(IFCLayout, PerformLayoutShouldUpdateRenderPosition) {
    // 场景：验证 PerformLayout 模式正确设置位置
}

TEST(IFCLayout, MultipleLayoutCallsProduceConsistentResults) {
    // 场景：多次调用布局应产生一致的结果
}
```

#### 2.1.2 Text-Align 测试
```cpp
TEST(IFCLayout, TextAlignLeft) { /* ... */ }
TEST(IFCLayout, TextAlignCenter) { /* ... */ }
TEST(IFCLayout, TextAlignRight) { /* ... */ }
TEST(IFCLayout, TextAlignJustify) { /* ... */ }
```

#### 2.1.3 多行文本测试
```cpp
TEST(IFCLayout, MultiLineTextCenter) {
    // 验证多行文本每行都正确居中
}

TEST(IFCLayout, LineBreakingWithPadding) {
    // 验证换行时正确考虑 padding
}
```

### 2.2 Flexbox 布局测试

#### 2.2.1 主轴对齐
```cpp
TEST(FlexLayout, JustifyContentFlexStart) { /* ... */ }
TEST(FlexLayout, JustifyContentFlexEnd) { /* ... */ }
TEST(FlexLayout, JustifyContentCenter) { /* ... */ }
TEST(FlexLayout, JustifyContentSpaceBetween) { /* ... */ }
TEST(FlexLayout, JustifyContentSpaceAround) { /* ... */ }
TEST(FlexLayout, JustifyContentSpaceEvenly) { /* ... */ }
```

#### 2.2.2 交叉轴对齐
```cpp
TEST(FlexLayout, AlignItemsFlexStart) { /* ... */ }
TEST(FlexLayout, AlignItemsFlexEnd) { /* ... */ }
TEST(FlexLayout, AlignItemsCenter) { /* ... */ }
TEST(FlexLayout, AlignItemsStretch) { /* ... */ }
TEST(FlexLayout, AlignItemsBaseline) { /* ... */ }
```

#### 2.2.3 Flex 尺寸计算
```cpp
TEST(FlexLayout, FlexGrowDistribution) { /* ... */ }
TEST(FlexLayout, FlexShrinkDistribution) { /* ... */ }
TEST(FlexLayout, FlexBasisCalculation) { /* ... */ }
```

### 2.3 Grid 布局测试

#### 2.3.1 轨道尺寸
```cpp
TEST(GridLayout, FixedTrackSize) { /* ... */ }
TEST(GridLayout, FractionTrackSize) { /* ... */ }
TEST(GridLayout, AutoTrackSize) { /* ... */ }
TEST(GridLayout, MinMaxTrackSize) { /* ... */ }
TEST(GridLayout, RepeatTrackSize) { /* ... */ }
```

#### 2.3.2 项目定位
```cpp
TEST(GridLayout, GridColumnPlacement) { /* ... */ }
TEST(GridLayout, GridRowPlacement) { /* ... */ }
TEST(GridLayout, GridAreaPlacement) { /* ... */ }
```

---

## 3. 集成测试（布局比较测试）

### 3.1 现有测试用例

| 类别 | 测试 ID | 描述 | 状态 |
|------|---------|------|------|
| Grid+Flex嵌套 | ADV-1-02 | Grid > Flex > Block + text-align | ✅ |
| 深度嵌套 | ADV-1-03 | 4-6层嵌套布局 | ✅ |
| Flex对齐 | ADV-3-xx | 各种 justify/align 组合 | ✅ |
| Grid轨道 | ADV-5-xx | 各种轨道尺寸组合 | ✅ |
| 实际组件 | ADV-7-xx | 卡片、导航栏等 | ✅ |

### 3.2 需要新增的测试用例

#### 3.2.1 Text-Align 边界情况
```javascript
// app_advanced.js 新增
{
    id: 'TEXT-ALIGN-01',
    name: '多行文本居中',
    html: `
        <div style="width: 200px; text-align: center;">
            这是一段很长的文本，会自动换行到多行，
            每一行都应该居中对齐。
        </div>
    `
}

{
    id: 'TEXT-ALIGN-02',
    name: '嵌套 text-align',
    html: `
        <div style="text-align: center;">
            <div style="text-align: left;">左对齐</div>
            <div>继承居中</div>
        </div>
    `
}
```

#### 3.2.2 多次布局重排
```javascript
{
    id: 'REFLOW-01',
    name: 'resize 后布局',
    html: `<div id="resize-test" style="width: 100%;">内容</div>`,
    script: `
        // 改变容器宽度后验证布局
        document.getElementById('resize-test').style.width = '200px';
    `
}
```

#### 3.2.3 RTL 布局
```javascript
{
    id: 'RTL-01',
    name: 'RTL text-align',
    html: `
        <div style="direction: rtl; text-align: start;">
            RTL 文本
        </div>
    `
}
```

---

## 4. 回归测试

### 4.1 关键修复的回归测试

每次修复后，确保以下场景不受影响：

| 修复 | 回归测试点 |
|------|-----------|
| IFC apply_results | 所有 text-align 组合 |
| 行高计算 | CJK/ASCII 混合文本 |
| CSS 属性顺序 | JS 动态修改样式 |

### 4.2 自动化回归测试脚本

```bash
#!/bin/bash
# scripts/run_layout_regression_tests.sh

# 1. 构建测试
cmake --build build --target layout_compare_test

# 2. 运行 advanced 测试
./build/bin/Debug/layout_compare_test.exe --advanced -q > output.txt

# 3. 比较结果
python tests/layout_comparison/compare_layouts.py output.txt browser_data.json

# 4. 检查通过率
# 期望 100% 通过
```

---

## 5. 性能测试

### 5.1 布局性能基准

```cpp
TEST(LayoutPerformance, LargeDocumentLayout) {
    // 1000 个元素的布局时间 < 100ms
}

TEST(LayoutPerformance, DeepNestedLayout) {
    // 50 层嵌套的布局时间 < 50ms
}

TEST(LayoutPerformance, RepeatedLayoutCalls) {
    // 1000 次重复布局的总时间
}
```

### 5.2 缓存效率测试

```cpp
TEST(LayoutCache, CacheHitRate) {
    // 验证缓存命中率 > 80%
}

TEST(LayoutCache, CacheInvalidation) {
    // 验证样式变化后缓存正确失效
}
```

---

## 6. 测试执行计划

### 6.1 阶段一：单元测试（优先级：高）

| 任务 | 状态 | 预计时间 |
|------|------|---------|
| 创建 IFC 单元测试框架 | ⬜ | 2h |
| 实现 RunMode 行为测试 | ⬜ | 2h |
| 实现 text-align 测试 | ⬜ | 2h |
| 实现多行文本测试 | ⬜ | 1h |

### 6.2 阶段二：集成测试（优先级：高）

| 任务 | 状态 | 预计时间 |
|------|------|---------|
| 新增 text-align 边界用例 | ⬜ | 1h |
| 新增 RTL 测试用例 | ⬜ | 1h |
| 新增多次重排测试 | ⬜ | 1h |
| 更新浏览器参考数据 | ⬜ | 1h |

### 6.3 阶段三：回归测试自动化（优先级：中）

| 任务 | 状态 | 预计时间 |
|------|------|---------|
| 编写回归测试脚本 | ⬜ | 1h |
| 集成到 CI 流程 | ⬜ | 2h |
| 设置通过率阈值 | ⬜ | 0.5h |

### 6.4 阶段四：性能测试（优先级：低）

| 任务 | 状态 | 预计时间 |
|------|------|---------|
| 实现性能基准测试 | ⬜ | 2h |
| 实现缓存效率测试 | ⬜ | 1h |
| 建立性能基线 | ⬜ | 1h |

---

## 7. 测试工具

### 7.1 现有工具
- `layout_compare_test.exe`：布局比较测试可执行文件
- `compare_layouts.py`：布局数据比较脚本
- `browser_advanced_data.json`：浏览器参考数据

### 7.2 需要开发的工具
- IFC 单元测试框架
- 自动化回归测试脚本
- 性能测试框架

---

## 8. 成功标准

| 指标 | 目标 |
|------|------|
| 布局比较测试通过率 | ≥ 98% |
| 单元测试覆盖率 | ≥ 80% |
| 回归测试通过率 | 100% |
| 大文档布局时间 | < 100ms |

---

## 更新日志

| 日期 | 版本 | 更新内容 |
|------|------|---------|
| 2025-12-06 | 1.0 | 初始版本，定义测试计划框架 |

