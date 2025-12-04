# 布局引擎测试计划

## 1. 测试概述

### 1.1 测试目标

验证 MBink 原生布局引擎与浏览器 CSS 布局行为的一致性，确保：
- 所有支持的 CSS 属性正确计算
- 布局结果与主流浏览器 (Chrome) 像素级一致
- 边界情况正确处理

### 1.2 测试方法

使用 `layout_compare_test` 工具进行可视化对比：
1. 左侧：MBink 渲染结果
2. 右侧：浏览器渲染参考 (browser.html)
3. 人工对比或自动截图对比

### 1.3 验收标准

| 级别 | 标准 |
|------|------|
| P0 | 位置和尺寸完全一致 (±1px 容差) |
| P1 | 位置和尺寸基本一致 (±3px 容差) |
| P2 | 布局行为正确，允许细微差异 |

---

## 2. 测试覆盖率统计

| 模块 | 已测试 | 总计 | 覆盖率 | 状态 |
|------|--------|------|--------|------|
| Box Model | 6 | 15 | 40% | 🟡 进行中 |
| Block Layout | 3 | 10 | 30% | 🟡 进行中 |
| Flexbox | 12 | 25 | 48% | 🟡 进行中 |
| Grid | 0 | 15 | 0% | 🔴 未开始 |
| Positioning | 0 | 10 | 0% | 🔴 未开始 |
| Text/IFC | 2 | 12 | 17% | 🟡 进行中 |
| **总计** | **23** | **87** | **26%** | 🟡 |

---

## 3. 详细测试用例

### 3.1 Box Model 盒模型 (15 项)

#### ✅ 已完成 (6 项)

| ID | 测试项 | 样式 | 状态 |
|----|--------|------|------|
| BM-01 | 固定宽高 | `width: 200px; height: 100px` | ✅ |
| BM-02 | 只设宽度 | `width: 200px` | ✅ |
| BM-03 | 只设高度 | `height: 100px` | ✅ |
| BM-04 | padding 四边 | `padding: 10px` | ✅ |
| BM-05 | 百分比宽度 50% | `width: 50%` | ✅ |
| BM-06 | 百分比宽度 100% | `width: 100%` | ✅ |

#### ⬜ 待完成 (9 项)

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| BM-07 | box-sizing: border-box | `box-sizing: border-box; width: 200px; padding: 20px` | P0 |
| BM-08 | box-sizing: content-box | `box-sizing: content-box; width: 200px; padding: 20px` | P0 |
| BM-09 | margin: auto 水平居中 | `width: 200px; margin: 0 auto` | P0 |
| BM-10 | margin 负值 | `margin-left: -20px` | P1 |
| BM-11 | margin 合并 (垂直) | 两个块级元素 `margin: 20px 0` | P1 |
| BM-12 | padding 百分比 | `padding: 10%` (基于父宽度) | P1 |
| BM-13 | border 各边不同 | `border-left: 5px; border-right: 10px` | P1 |
| BM-14 | 百分比高度 | `height: 50%` (父元素有固定高度) | P1 |
| BM-15 | calc() 计算 | `width: calc(100% - 40px)` | P2 |

---

### 3.2 Block Layout 块级布局 (10 项)

#### ✅ 已完成 (3 项)

| ID | 测试项 | 样式 | 状态 |
|----|--------|------|------|
| BL-01 | width: auto 填满 | 默认块级元素 | ✅ |
| BL-02 | max-width 约束 | `width: 100%; max-width: 200px` | ✅ |
| BL-03 | min-width 约束 | `min-width: 150px` | ✅ |

#### ⬜ 待完成 (7 项)

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| BL-04 | display: inline-block | `display: inline-block; width: 100px` | P0 |
| BL-05 | overflow: hidden | `overflow: hidden; height: 50px` (内容溢出) | P0 |
| BL-06 | overflow: scroll | `overflow: scroll; height: 100px` | P0 |
| BL-07 | overflow: auto | `overflow: auto; height: 100px` | P1 |
| BL-08 | min-height + 内容撑开 | `min-height: 50px` + 大量内容 | P1 |
| BL-09 | max-height 截断 | `max-height: 100px; overflow: hidden` | P1 |
| BL-10 | 嵌套块级元素 | 多层嵌套，各层有 padding/margin | P1 |

---

### 3.3 Flexbox 弹性布局 (25 项)

#### ✅ 已完成 (12 项)

| ID | 测试项 | 样式 | 状态 |
|----|--------|------|------|
| FL-01 | flex-direction: row | 默认水平排列 | ✅ |
| FL-02 | flex-direction: column | `flex-direction: column` | ✅ |
| FL-03 | justify-content: flex-start | 默认 | ✅ |
| FL-04 | justify-content: flex-end | `justify-content: flex-end` | ✅ |
| FL-05 | justify-content: center | `justify-content: center` | ✅ |
| FL-06 | justify-content: space-between | `justify-content: space-between` | ✅ |
| FL-07 | justify-content: space-around | `justify-content: space-around` | ✅ |
| FL-08 | justify-content: space-evenly | `justify-content: space-evenly` | ✅ |
| FL-09 | align-items: flex-start | `align-items: flex-start` | ✅ |
| FL-10 | align-items: flex-end | `align-items: flex-end` | ✅ |
| FL-11 | align-items: center | `align-items: center` | ✅ |
| FL-12 | align-items: stretch | `align-items: stretch` | ✅ |

#### ⬜ 待完成 (13 项)

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| FL-13 | flex-wrap: wrap | `flex-wrap: wrap` (子元素超宽) | P0 |
| FL-14 | flex-wrap: wrap-reverse | `flex-wrap: wrap-reverse` | P1 |
| FL-15 | gap (行列间距) | `gap: 10px` | P0 |
| FL-16 | row-gap + column-gap | `row-gap: 10px; column-gap: 20px` | P1 |
| FL-17 | flex-grow 分配 | `flex-grow: 1` vs `flex-grow: 2` | ✅ |
| FL-18 | flex-shrink 收缩 | `flex-shrink: 0` (禁止收缩) | P0 |
| FL-19 | flex-basis | `flex-basis: 100px` | ✅ |
| FL-20 | flex 简写 | `flex: 1` / `flex: 1 1 auto` | P0 |
| FL-21 | align-self | 单项覆盖 `align-self: flex-end` | P1 |
| FL-22 | align-content | 多行对齐 `align-content: center` | P1 |
| FL-23 | order | `order: -1` / `order: 1` | P2 |
| FL-24 | flex-direction: row-reverse | `flex-direction: row-reverse` | P1 |
| FL-25 | flex-direction: column-reverse | `flex-direction: column-reverse` | P1 |

---

### 3.4 Grid 网格布局 (15 项)

#### ⬜ 全部待完成

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| GR-01 | 基础 Grid | `display: grid; grid-template-columns: 1fr 1fr` | P0 |
| GR-02 | 固定列宽 | `grid-template-columns: 100px 200px 100px` | P0 |
| GR-03 | fr 单位混合 | `grid-template-columns: 100px 1fr 2fr` | P0 |
| GR-04 | gap | `gap: 10px` | P0 |
| GR-05 | row-gap + column-gap | `row-gap: 10px; column-gap: 20px` | P1 |
| GR-06 | repeat() | `grid-template-columns: repeat(3, 1fr)` | P0 |
| GR-07 | minmax() | `grid-template-columns: minmax(100px, 1fr)` | P1 |
| GR-08 | auto-fill | `repeat(auto-fill, minmax(100px, 1fr))` | P1 |
| GR-09 | auto-fit | `repeat(auto-fit, minmax(100px, 1fr))` | P1 |
| GR-10 | grid-column span | `grid-column: span 2` | P1 |
| GR-11 | grid-row span | `grid-row: span 2` | P1 |
| GR-12 | grid-area 定位 | `grid-column: 1/3; grid-row: 1/2` | P1 |
| GR-13 | grid-template-areas | 命名区域布局 | P2 |
| GR-14 | justify-items | `justify-items: center` | P1 |
| GR-15 | align-items | `align-items: center` | P1 |

---

### 3.5 Positioning 定位 (10 项)

#### ⬜ 全部待完成

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| PS-01 | position: relative | `position: relative; top: 10px; left: 20px` | P0 |
| PS-02 | position: absolute (相对父) | 父 relative，子 absolute | P0 |
| PS-03 | position: absolute (四角) | `top: 0; right: 0` 等四角定位 | P0 |
| PS-04 | position: absolute 居中 | `top: 50%; left: 50%; transform: translate(-50%, -50%)` | P1 |
| PS-05 | position: fixed | `position: fixed; top: 0` | P1 |
| PS-06 | position: sticky | `position: sticky; top: 0` | P2 |
| PS-07 | z-index 层叠 | 多元素不同 z-index | P1 |
| PS-08 | absolute + 百分比 | `width: 50%; height: 50%` (相对定位祖先) | P1 |
| PS-09 | inset 简写 | `inset: 10px 20px` | P2 |
| PS-10 | 嵌套定位 | 多层嵌套定位上下文 | P2 |

---

### 3.6 Text/IFC 文本布局 (12 项)

#### ✅ 已完成 (2 项)

| ID | 测试项 | 样式 | 状态 |
|----|--------|------|------|
| TX-01 | text-align: center | `text-align: center` | ✅ |
| TX-02 | 单行文本 | 默认文本渲染 | ✅ |

#### ⬜ 待完成 (10 项)

| ID | 测试项 | 样式 | 优先级 |
|----|--------|------|--------|
| TX-03 | text-align: left | `text-align: left` | P0 |
| TX-04 | text-align: right | `text-align: right` | P0 |
| TX-05 | text-align: justify | `text-align: justify` (多行) | P2 |
| TX-06 | line-height 数值 | `line-height: 2` | P0 |
| TX-07 | line-height 像素 | `line-height: 24px` | P0 |
| TX-08 | 多行文本换行 | 自动换行 | P0 |
| TX-09 | white-space: nowrap | `white-space: nowrap` | P1 |
| TX-10 | white-space: pre | `white-space: pre` | P1 |
| TX-11 | text-overflow: ellipsis | `text-overflow: ellipsis; overflow: hidden` | P1 |
| TX-12 | vertical-align | `vertical-align: middle` (inline-block) | P1 |

---

## 4. 测试优先级规划

### Phase 1: 核心功能 (P0) - 预计 2 天

| 模块 | 测试项数 | 说明 |
|------|----------|------|
| Box Model | 3 | box-sizing, margin auto |
| Block | 3 | inline-block, overflow |
| Flexbox | 4 | wrap, gap, shrink, flex 简写 |
| Grid | 4 | 基础 grid, fr, gap, repeat |
| Positioning | 3 | relative, absolute 基础 |
| Text | 4 | text-align, line-height, 多行 |
| **合计** | **21** | |

### Phase 2: 扩展功能 (P1) - 预计 3 天

| 模块 | 测试项数 |
|------|----------|
| Box Model | 5 |
| Block | 4 |
| Flexbox | 6 |
| Grid | 8 |
| Positioning | 4 |
| Text | 4 |
| **合计** | **31** |

### Phase 3: 高级功能 (P2) - 预计 2 天

| 模块 | 测试项数 |
|------|----------|
| Box Model | 1 |
| Flexbox | 1 |
| Grid | 1 |
| Positioning | 3 |
| Text | 1 |
| **合计** | **7** |

---

## 5. 测试执行记录

### 执行日志

| 日期 | 版本 | 执行人 | 通过/失败 | 备注 |
|------|------|--------|-----------|------|
| 2024-12-05 | v1.0 | - | 23/23 | 初始测试全部通过 |

### 已知问题

| ID | 描述 | 状态 | 优先级 |
|----|------|------|--------|
| - | 暂无 | - | - |

---

## 6. 附录

### A. 测试工具使用

```bash
# 编译测试程序
cmake --build build --config Release --target layout_compare_test

# 运行测试
build/bin/Release/layout_compare_test.exe

# 浏览器参考
# 在浏览器中打开 examples/demo_html/layout_compare_test/browser.html
```

### B. 添加新测试用例

编辑 `examples/demo_html/layout_compare_test/app.js`:

```javascript
// 1. 添加测试容器
h('div', { style: testContainerStyle },
    h('h3', { style: '...' }, '测试标题'),
    h('div', { style: 'background: #ddd; padding: 5px;' },
        h('div', { style: '你的测试样式' }, '测试内容')
    )
),

// 2. 同步更新 browser.html
```

### C. 参考资源

- [CSS Box Model - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Model)
- [Flexbox - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Flexible_Box_Layout)
- [Grid - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Grid_Layout)
- [Taffy Layout Engine](https://github.com/DioxusLabs/taffy)

---

*文档版本: 1.0*
*创建日期: 2024-12-05*
*维护者: MBink Team*

