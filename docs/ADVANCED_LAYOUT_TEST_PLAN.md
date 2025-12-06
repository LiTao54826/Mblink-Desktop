# 高级布局测试计划

**版本**: v1.0  
**日期**: 2025-12-06  
**基线**: 基础测试 86/86 通过 (100%)  
**目标**: 测试更复杂的边界情况和组合场景

---

## 📋 测试计划概述

### 基础测试已覆盖
| 类别 | 已测试项 |
|------|----------|
| Box Model | 盒模型基础、box-sizing、margin/padding/border、calc() |
| Block | inline-block、overflow、min/max-height/width |
| Flexbox | direction、justify-content、align-items、wrap、gap、grow/shrink |
| Grid | template-columns、fr、gap、repeat、span、justify/align-items |
| Positioning | relative、absolute、fixed、z-index |
| Text/IFC | text-align、line-height、white-space |

### 高级测试目标
测试以下复杂场景，验证布局引擎在边界情况下的正确性：

1. **深度嵌套组合** - 多层嵌套的 Flex/Grid/Block 混合布局
2. **极限尺寸约束** - min/max 冲突、百分比循环依赖
3. **复杂对齐场景** - margin:auto + flex/grid 对齐组合
4. **动态内容溢出** - 内容超出容器时的各种处理
5. **高级 Grid 功能** - 命名线、区域、自动放置策略
6. **Flexbox 边界情况** - 负 margin、baseline 对齐、嵌套 shrink
7. **表格布局模拟** - 使用 Flex/Grid 模拟表格行为
8. **响应式布局模式** - 常见的响应式布局组件

---

## 🔬 测试用例详细规划

### ADV-1: 深度嵌套组合测试

#### ADV-1-01: Flex 内嵌 Grid 内嵌 Flex
```html
<div style="display: flex; gap: 10px;">
  <div style="flex: 1; display: grid; grid-template-columns: 1fr 1fr; gap: 5px;">
    <div style="display: flex; justify-content: center;">A</div>
    <div style="display: flex; justify-content: center;">B</div>
  </div>
  <div style="flex: 1;">C</div>
</div>
```
**验证点**: 内层 Flex 容器正确继承 Grid 单元格尺寸，justify-content 正确居中

#### ADV-1-02: Grid 内嵌 Flex (stretch vs center)
```html
<div style="display: grid; grid-template-columns: 200px 1fr; height: 200px; align-items: stretch;">
  <div style="display: flex; align-items: center; background: #ddd;">
    <div>内部垂直居中</div>
  </div>
  <div style="display: flex; justify-content: flex-end; align-items: flex-start;">
    <div>右上角</div>
  </div>
</div>
```
**验证点**: Grid stretch 后的 Flex 容器内部对齐是否正确

#### ADV-1-03: 5层嵌套布局
```html
<div style="display: flex;">                     <!-- L1: Flex -->
  <div style="flex: 1; display: grid; grid-template-columns: 1fr;">  <!-- L2: Grid -->
    <div style="display: flex; flex-direction: column;">              <!-- L3: Flex column -->
      <div style="display: grid; grid-template-columns: 1fr 1fr;">    <!-- L4: Grid -->
        <div style="display: flex; justify-content: center;">         <!-- L5: Flex -->
          <span>深度嵌套</span>
        </div>
      </div>
    </div>
  </div>
</div>
```
**验证点**: 尺寸正确传递到最内层，内容正确居中

---

### ADV-2: 极限尺寸约束测试

#### ADV-2-01: min-width > max-width 冲突
```html
<div style="width: 300px;">
  <div style="min-width: 200px; max-width: 100px; background: red;">
    min-width 优先
  </div>
</div>
```
**验证点**: CSS 规范中 min-width 优先于 max-width，结果应为 200px

#### ADV-2-02: 百分比 + min/max 组合
```html
<div style="width: 300px;">
  <div style="width: 50%; min-width: 200px; max-width: 250px;">
    150px → 200px (min 限制)
  </div>
</div>
```
**验证点**: 百分比计算后应用 min/max 约束

#### ADV-2-03: Flex 子项 flex-basis vs min-width
```html
<div style="display: flex; width: 400px;">
  <div style="flex: 1 1 100px; min-width: 150px;">A</div>
  <div style="flex: 1 1 100px;">B</div>
</div>
```
**验证点**: min-width 限制 flex 收缩

#### ADV-2-04: 嵌套百分比高度
```html
<div style="height: 400px;">
  <div style="height: 50%;">    <!-- 200px -->
    <div style="height: 50%;">  <!-- 100px -->
      <div style="height: 50%;"> <!-- 50px -->
        嵌套百分比
      </div>
    </div>
  </div>
</div>
```
**验证点**: 多层百分比正确计算

---

### ADV-3: 复杂对齐场景测试

#### ADV-3-01: margin:auto 在 Flex 中
```html
<div style="display: flex; width: 400px; height: 200px;">
  <div style="margin: auto; width: 100px; height: 50px;">
    完全居中
  </div>
</div>
```
**验证点**: margin:auto 在 Flex 中实现水平垂直居中

#### ADV-3-02: 部分 margin:auto
```html
<div style="display: flex; width: 400px;">
  <div style="width: 50px;">A</div>
  <div style="margin-left: auto; width: 50px;">B</div>
  <div style="width: 50px;">C</div>
</div>
```
**验证点**: margin-left:auto 推动 B、C 到右边

#### ADV-3-03: Grid + place-items
```html
<div style="display: grid; grid-template-columns: 1fr 1fr; height: 200px; place-items: center;">
  <div style="width: 50px; height: 50px;">A</div>
  <div style="width: 50px; height: 50px;">B</div>
</div>
```
**验证点**: place-items: center 同时水平垂直居中

#### ADV-3-04: justify-self / align-self 覆盖
```html
<div style="display: grid; grid-template-columns: 1fr 1fr; height: 200px; place-items: start;">
  <div style="justify-self: end; align-self: end;">右下</div>
  <div>左上</div>
</div>
```
**验证点**: 子项覆盖容器对齐设置

---

### ADV-4: 动态内容溢出测试

#### ADV-4-01: Flex nowrap 溢出
```html
<div style="display: flex; width: 200px; flex-wrap: nowrap;">
  <div style="flex: 0 0 100px;">A</div>
  <div style="flex: 0 0 100px;">B</div>
  <div style="flex: 0 0 100px;">C</div>
</div>
```
**验证点**: 300px 内容在 200px 容器中溢出

#### ADV-4-02: Grid 内容溢出单元格
```html
<div style="display: grid; grid-template-columns: 100px 100px; overflow: visible;">
  <div style="width: 150px; background: red;">溢出</div>
  <div>正常</div>
</div>
```
**验证点**: Grid 单元格内容超出时的处理

#### ADV-4-03: 文本溢出 + flex-shrink
```html
<div style="display: flex; width: 200px;">
  <div style="flex: 0 1 auto; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;">
    Very long text that should be truncated with ellipsis
  </div>
  <div style="flex: 0 0 50px;">Fixed</div>
</div>
```
**验证点**: 文本正确收缩并显示省略号

---

### ADV-5: 高级 Grid 功能测试

#### ADV-5-01: grid-template-rows 明确行高
```html
<div style="display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: 50px 100px 50px; gap: 10px;">
  <div>R1-C1</div><div>R1-C2</div>
  <div>R2-C1</div><div>R2-C2</div>
  <div>R3-C1</div><div>R3-C2</div>
</div>
```
**验证点**: 行高按 50/100/50 分配

#### ADV-5-02: auto-fill / auto-fit
```html
<div style="display: grid; grid-template-columns: repeat(auto-fill, minmax(100px, 1fr)); gap: 10px;">
  <div>A</div><div>B</div><div>C</div><div>D</div>
</div>
```
**验证点**: 根据容器宽度自动计算列数

#### ADV-5-03: grid-auto-rows
```html
<div style="display: grid; grid-template-columns: 1fr 1fr; grid-auto-rows: 60px; gap: 10px;">
  <!-- 6个项目，只定义了2列，自动生成3行 -->
  <div>1</div><div>2</div><div>3</div><div>4</div><div>5</div><div>6</div>
</div>
```
**验证点**: 隐式行使用 grid-auto-rows 高度

#### ADV-5-04: grid-column 指定位置
```html
<div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px;">
  <div style="grid-column: 2 / 4;">跨越2-3列</div>
  <div>A</div>
  <div style="grid-column: 1 / -1;">全宽</div>
</div>
```
**验证点**: 明确指定起止位置

---

### ADV-6: Flexbox 边界情况测试

#### ADV-6-01: 负 margin 在 Flex 中
```html
<div style="display: flex; gap: 20px; padding: 20px;">
  <div style="margin-right: -10px;">A</div>
  <div>B</div>
</div>
```
**验证点**: 负 margin 减少间距

#### ADV-6-02: flex-basis: 0 vs auto
```html
<div style="display: flex; width: 300px;">
  <div style="flex: 1 1 0;">flex-basis: 0</div>
  <div style="flex: 1 1 auto;">flex-basis: auto (有内容)</div>
</div>
```
**验证点**: basis:0 忽略内容宽度，basis:auto 基于内容

#### ADV-6-03: baseline 对齐
```html
<div style="display: flex; align-items: baseline;">
  <div style="font-size: 12px;">小字</div>
  <div style="font-size: 24px;">大字</div>
  <div style="font-size: 16px; padding-top: 20px;">有 padding</div>
</div>
```
**验证点**: 按文本基线对齐

#### ADV-6-04: flex-shrink 嵌套
```html
<div style="display: flex; width: 200px;">
  <div style="flex: 0 0 100px;">不收缩</div>
  <div style="flex: 1 1 auto; display: flex;">
    <div style="flex: 1 1 auto;">内部收缩</div>
  </div>
</div>
```
**验证点**: 嵌套 Flex 的收缩行为

---

### ADV-7: 实用布局模式测试

#### ADV-7-01: 粘性页脚 (Sticky Footer)
```html
<div style="display: flex; flex-direction: column; min-height: 300px;">
  <header style="height: 50px;">Header</header>
  <main style="flex: 1;">Content</main>
  <footer style="height: 30px;">Footer</footer>
</div>
```
**验证点**: main 自动填充剩余空间，footer 固定在底部

#### ADV-7-02: 圣杯布局
```html
<div style="display: grid; grid-template-columns: 150px 1fr 150px; grid-template-rows: auto 1fr auto; min-height: 300px;">
  <header style="grid-column: 1 / -1;">Header</header>
  <aside style="grid-column: 1;">Left</aside>
  <main style="grid-column: 2;">Main</main>
  <aside style="grid-column: 3;">Right</aside>
  <footer style="grid-column: 1 / -1;">Footer</footer>
</div>
```
**验证点**: 三栏布局正确，header/footer 全宽

#### ADV-7-03: 等高列 (Flex)
```html
<div style="display: flex; gap: 10px;">
  <div style="flex: 1;">短内容</div>
  <div style="flex: 1;">长内容长内容长内容长内容长内容长内容</div>
  <div style="flex: 1;">中等</div>
</div>
```
**验证点**: 三列高度相等

#### ADV-7-04: 卡片网格 (Grid)
```html
<div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px;">
  <div style="display: flex; flex-direction: column;">
    <div style="flex: 1;">内容</div>
    <div>按钮</div>
  </div>
  <!-- 重复 2 个卡片 -->
</div>
```
**验证点**: 卡片等高，按钮对齐

---

## 📊 测试验收标准

### 容差标准 (与基础测试一致)
| 属性 | 容差 |
|------|------|
| width / x | ±1px |
| height / y | ±2px |
| display | 完全一致 |

### 验收等级
| 等级 | 通过率 | 说明 |
|------|--------|------|
| 🏆 A级 | ≥ 95% | 高级场景完全可用 |
| ✅ B级 | ≥ 85% | 大部分高级场景可用 |
| ⚠️ C级 | ≥ 70% | 核心高级场景可用 |
| ❌ D级 | < 70% | 需要进一步开发 |

### 必须通过项 (P0)
- ADV-2-01: min-width 优先级
- ADV-3-01: margin:auto 居中
- ADV-7-01: 粘性页脚
- ADV-7-03: 等高列

---

## 📁 文件结构

```
examples/demo_html/layout_compare_test/
├── app.js              # 现有基础测试
├── app_advanced.js     # 新增：高级测试用例
├── browser.html        # 浏览器测试入口

tests/layout_comparison/
├── browser_reference_data.json          # 基础测试参考数据
├── browser_reference_data_advanced.json # 新增：高级测试参考数据
```

---

## 🚀 执行步骤

### 1. 实现测试用例
在 `app_advanced.js` 中添加上述测试用例

### 2. 收集浏览器参考数据
```bash
# 在 Chrome 中打开测试页面，DevTools Console 运行：
collectAdvancedLayoutData()
# 保存输出到 browser_reference_data_advanced.json
```

### 3. 运行 MBink 测试
```bash
cmake --build build --config Release --target layout_compare_test
.\build\bin\Release\layout_compare_test.exe -q 2>&1 | Out-File -FilePath mb_advanced_output.txt -Encoding utf8
```

### 4. 对比分析
逐项对比 MBink 输出与浏览器参考数据

---

## 📝 已知待修复问题 (来自基础测试)

在进行高级测试前，建议先解决以下待修复问题：

| 问题ID | 描述 | 影响范围 |
|--------|------|----------|
| FL-23 | order 排序错误 | 可能影响 ADV-6 测试 |
| FL-24/25 | row-reverse/column-reverse 对齐 | 可能影响 ADV-6 测试 |
| GR-11 | grid-row: span 2 | 直接影响 ADV-5 测试 |
| TX-10/12 | 文字高度/垂直居中 | 可能影响 ADV-6-03 baseline |

---

*文档作者: Augment Agent*  
*创建日期: 2025-12-06*

