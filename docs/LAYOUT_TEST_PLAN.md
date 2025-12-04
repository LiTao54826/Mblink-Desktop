# 布局引擎测试计划

## 1. 测试概述

### 1.1 测试目标

验证 MBink 原生布局引擎与浏览器 CSS 布局行为的一致性，确保：
- 所有支持的 CSS 属性正确计算
- 布局结果与主流浏览器 (Chrome) 像素级一致
- 边界情况正确处理

### 1.2 测试环境

| 项目 | 说明 |
|------|------|
| 参考浏览器 | Google Chrome (最新稳定版) |
| 测试工具 | `layout_compare_test` |
| 对比方式 | 并排实时对比 |
| 窗口尺寸 | 900×700 (MBink) / 450×700 (内容区) |

### 1.3 验证方式

**采用浏览器与 MBink 一对一真实对比验证：**

```
┌─────────────────────────────────────────────────────────────┐
│                    layout_compare_test                       │
├─────────────────────────────┬───────────────────────────────┤
│                             │                               │
│     MBink 渲染结果          │     浏览器渲染参考            │
│     (左侧 450px)            │     (右侧 450px)              │
│                             │                               │
│  ┌─────────────────────┐    │    打开 browser.html          │
│  │  测试用例 1.1       │    │    在 Chrome 中查看           │
│  │  ┌───────────────┐  │    │                               │
│  │  │   渲染结果    │  │    │    ┌───────────────┐          │
│  │  └───────────────┘  │    │    │   渲染结果    │          │
│  └─────────────────────┘    │    └───────────────┘          │
│                             │                               │
└─────────────────────────────┴───────────────────────────────┘
```

**验证流程：**

1. **启动测试程序**
   ```bash
   cmake --build build --config Release --target layout_compare_test
   build/bin/Release/layout_compare_test.exe
   ```

2. **打开浏览器参考页面**
   ```
   在 Chrome 中打开: examples/demo_html/layout_compare_test/browser.html
   ```

3. **逐项对比验证**
   - 将 MBink 窗口与浏览器窗口并排放置
   - 滚动到相同测试用例位置
   - 目视对比以下内容：
     - 元素位置 (x, y 坐标)
     - 元素尺寸 (宽度、高度)
     - 元素间距 (margin, padding, gap)
     - 文本对齐方式
     - 子元素排列顺序

4. **记录测试结果**
   - ✅ 通过：完全一致或在容差范围内
   - ❌ 失败：超出容差范围，记录差异详情

### 1.4 验收标准

| 检查项 | 通过标准 | 容差 |
|--------|----------|------|
| 元素宽度 | 与浏览器一致 | ±1px |
| 元素高度 | 与浏览器一致 | ±2px (考虑文本行高差异) |
| 元素 X 坐标 | 与浏览器一致 | ±1px |
| 元素 Y 坐标 | 与浏览器一致 | ±2px |
| 子元素间距 | 与浏览器一致 | ±1px |
| 文本水平对齐 | 视觉一致 | 允许亚像素差异 |
| 文本垂直位置 | 与浏览器一致 | ±2px |

**已知可接受差异：**

| 差异项 | 原因 | 处理方式 |
|--------|------|----------|
| 滚动条宽度 | MBink 暂无滚动条 UI | 记录但不阻断 |
| 字体渲染 | 不同渲染引擎差异 | 只验证布局位置 |
| 亚像素渲染 | 浮点精度差异 | ±1px 容差 |

### 1.5 测试用例编写规范

每个测试用例需要在两个文件中同步实现：

| 文件 | 用途 |
|------|------|
| `examples/demo_html/layout_compare_test/app.js` | MBink 测试用例 (Preact) |
| `examples/demo_html/layout_compare_test/browser.html` | 浏览器参考用例 (纯 HTML) |

**用例结构：**

```javascript
// app.js 中的测试用例格式
h('div', { style: testContainerStyle },
    h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '测试编号 测试名称'),
    h('div', { style: 'background: #ddd; padding: 5px;' },
        h('div', { style: '被测试的样式' }, '测试内容')
    )
),
```

```html
<!-- browser.html 中的对应用例 -->
<div style="...testContainerStyle...">
    <h3 style="margin: 0 0 5px 0; font-size: 14px;">测试编号 测试名称</h3>
    <div style="background: #ddd; padding: 5px;">
        <div style="被测试的样式">测试内容</div>
    </div>
</div>
```

**重要：两边的样式必须完全一致！**

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

### A. 完整测试执行流程

#### Step 1: 编译测试程序

```bash
# 在项目根目录执行
cmake --build build --config Release --target layout_compare_test
```

#### Step 2: 启动 MBink 测试窗口

```bash
# Windows
build\bin\Release\layout_compare_test.exe

# 或使用 start 命令后台启动
start build\bin\Release\layout_compare_test.exe
```

#### Step 3: 打开浏览器参考页面

```
1. 打开 Chrome 浏览器
2. 按 Ctrl+O 打开文件
3. 选择: examples/demo_html/layout_compare_test/browser.html
4. 或直接在地址栏输入文件路径
```

#### Step 4: 并排对比

```
1. 将 MBink 窗口放在屏幕左侧
2. 将 Chrome 窗口放在屏幕右侧
3. 调整两个窗口大小，使内容区域宽度一致 (约 450px)
4. 同步滚动到相同测试用例位置
5. 逐项目视对比
```

#### Step 5: 记录结果

对每个测试用例，检查并记录：

| 检查项 | 对比方法 |
|--------|----------|
| 元素位置 | 观察元素在容器中的相对位置 |
| 元素尺寸 | 观察元素的宽高比例 |
| 间距 | 观察元素之间的间隙 |
| 对齐 | 观察文本和子元素的对齐方式 |
| 换行 | 观察多行内容的换行位置 |

### B. 添加新测试用例

#### 1. 在 app.js 中添加 MBink 测试用例

文件: `examples/demo_html/layout_compare_test/app.js`

```javascript
// 在对应分类下添加新用例
h('div', { style: testContainerStyle },
    h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'XX-NN 测试名称'),
    h('div', { style: 'background: #ddd; padding: 5px;' },
        // 测试内容
        h('div', { style: '被测试的CSS样式' }, '显示内容')
    )
),
```

#### 2. 在 browser.html 中添加对应的浏览器参考

文件: `examples/demo_html/layout_compare_test/browser.html`

```html
<!-- 在对应分类下添加完全相同的 HTML -->
<div style="background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;">
    <h3 style="margin: 0 0 5px 0; font-size: 14px;">XX-NN 测试名称</h3>
    <div style="background: #ddd; padding: 5px;">
        <!-- 测试内容 -->
        <div style="被测试的CSS样式">显示内容</div>
    </div>
</div>
```

#### 3. 更新测试计划文档

在本文档对应章节中添加测试用例记录。

### C. 测试用例命名规范

| 前缀 | 模块 | 示例 |
|------|------|------|
| BM | Box Model | BM-01, BM-02 |
| BL | Block Layout | BL-01, BL-02 |
| FL | Flexbox | FL-01, FL-02 |
| GR | Grid | GR-01, GR-02 |
| PS | Positioning | PS-01, PS-02 |
| TX | Text/IFC | TX-01, TX-02 |

### D. 常见问题排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| MBink 和浏览器宽度不同 | 滚动条宽度差异 | 记录差异，不影响测试 |
| 文字高度有 1-2px 差异 | 字体渲染差异 | 在容差范围内，通过 |
| 元素位置偏差 > 3px | 布局算法问题 | 标记失败，创建 Issue |
| 样式完全不生效 | CSS 属性不支持 | 标记失败，记录不支持属性 |

### E. 参考资源

- [CSS Box Model - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Model)
- [Flexbox - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Flexible_Box_Layout)
- [Grid - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Grid_Layout)
- [CSS Positioning - MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/position)
- [Taffy Layout Engine](https://github.com/DioxusLabs/taffy)

---

*文档版本: 1.1*
*创建日期: 2024-12-05*
*更新日期: 2024-12-05*
*维护者: MBink Team*

