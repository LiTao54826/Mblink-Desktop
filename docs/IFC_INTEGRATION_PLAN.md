# IFC 集成到 Taffy 布局引擎计划

## 概述

本文档描述将 IFC（Inline Formatting Context）布局逻辑集成到 Taffy 布局引擎的详细计划。

### 目标
- 统一布局流程，减少重复遍历
- 消除 Taffy 和 IFC 之间的数据转换开销
- 解决 vertical-align、line-height 等特性在两系统间协调困难的问题
- 提升整体布局性能约 30-50%

### 当前状态
- **已修复**：FL-13/14 (flex-wrap row-gap)、GR-11 (grid span)、TX-10 (white-space: pre)
- **待修复**：TX-12 (vertical-align 文本位置)
- **根本原因**：IFC 和 Taffy 分离导致位置计算不一致

---

## 当前架构分析

### 布局流程

```
┌─────────────────────────────────────────────────────────────────┐
│                        当前布局流程                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  DOM Tree                                                       │
│      ↓                                                          │
│  RenderTree (RenderObject 树)                                   │
│      ↓                                                          │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ NativeLayoutEngine::ComputeLayout()                      │   │
│  │   ├── BuildLayoutTree() - 构建 LayoutNode 树             │   │
│  │   ├── ComputeLayout() - Taffy 布局 (Block/Flex/Grid)    │   │
│  │   ├── PerformIFCLayout() - 单独的 IFC 布局 ←── 问题所在  │   │
│  │   └── ReadLayoutResults() - 结果写回 RenderObject        │   │
│  └─────────────────────────────────────────────────────────┘   │
│      ↓                                                          │
│  渲染 (Paint)                                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 问题详细分析

#### 1. 重复遍历
```cpp
// native_layout_engine.cpp
void NativeLayoutEngine::ComputeLayout() {
    BuildLayoutTree(root);           // 第1次遍历
    ComputeTaffyLayout(root_node);   // 第2次遍历 (Taffy内部)
    PerformIFCLayout(root);          // 第3次遍历 (IFC)
    ReadLayoutResults(root);         // 第4次遍历
}
```

#### 2. 数据转换开销
```cpp
// 当前：ComputedStyle → taffy::Style 转换
taffy::Style ConvertToTaffyStyle(const ComputedStyle& style);

// 当前：IFC 单独处理 RenderObject
void IFCLayout::ApplyLayoutResults(RenderObject* container);
```

#### 3. 位置计算不一致 (TX-12 问题根源)
```cpp
// Taffy 设置容器位置
layout.x = node->x;
layout.y = node->y;

// IFC 单独设置子元素位置 (相对于容器)
box->y = line_y + y_offset;

// RenderText::Paint 又计算一次 half-leading
float baseline_y = half_leading + ascent;  // 导致重复偏移
```

---

## 目标架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        目标布局流程                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  DOM Tree                                                       │
│      ↓                                                          │
│  RenderTree                                                     │
│      ↓                                                          │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ 统一布局引擎                                              │   │
│  │   ├── Block Layout                                       │   │
│  │   │     └── IFC Layout (内联内容) ←── 集成到 Block 中    │   │
│  │   ├── Flex Layout                                        │   │
│  │   └── Grid Layout                                        │   │
│  └─────────────────────────────────────────────────────────┘   │
│      ↓                                                          │
│  渲染 (Paint)                                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 实施计划

### Phase 1: 接口统一 (预计 1-2 天)

#### 目标
定义清晰的 IFC 调用接口，使 Taffy 可以直接调用 IFC 测量和布局。

#### 任务清单

- [ ] **1.1** 定义 IFC 测量接口
  ```cpp
  // core/layout/taffy/tree.h
  struct IFCMeasureResult {
      float content_width;
      float content_height;
      std::vector<LineBoxInfo> line_boxes;
  };
  
  using IFCMeasureFunc = std::function<IFCMeasureResult(
      RenderObject* container,
      float available_width
  )>;
  ```

- [ ] **1.2** 重构 `MeasureTextForIFC` 为可复用接口
  ```cpp
  // core/layout/ifc_layout.h
  class IFCLayout {
  public:
      // 新增：供 Taffy 调用的静态测量方法
      static TextMeasurement MeasureText(
          const std::string& text,
          const ComputedStyle& style
      );
  };
  ```

- [ ] **1.3** 在 LayoutNode 中添加 IFC 相关字段
  ```cpp
  // core/layout/native_layout_engine.h
  struct LayoutNode {
      // ... 现有字段 ...
      
      // 新增：IFC 布局结果
      bool is_ifc_container = false;
      std::vector<InlineBox> inline_boxes;
      std::vector<LineBox> line_boxes;
  };
  ```

#### 验收标准
- [ ] IFC 测量可通过新接口调用
- [ ] 现有 layout_compare_test 测试全部通过

---

### Phase 2: Block 布局集成 IFC (预计 2-3 天)

#### 目标
在 `ComputeBlockLayout` 中直接处理 IFC 内容，无需单独调用。

#### 任务清单

- [ ] **2.1** 修改 `ComputeBlockLayoutInner` 处理 IFC
  ```cpp
  // core/layout/taffy/compute/block.cpp
  LayoutOutput ComputeBlockLayoutInner(...) {
      // 检测是否为 IFC 容器
      if (HasInlineContent(container)) {
          return ComputeBlockWithIFC(tree, node_id, inputs);
      }
      // 原有逻辑...
  }
  ```

- [ ] **2.2** 实现 `ComputeBlockWithIFC`
  ```cpp
  LayoutOutput ComputeBlockWithIFC(
      LayoutBlockContainer& tree,
      NodeId node_id,
      const LayoutInput& inputs
  ) {
      // 1. 收集内联内容
      // 2. 断行
      // 3. 垂直对齐
      // 4. 设置子元素位置
      // 5. 返回容器尺寸
  }
  ```

- [ ] **2.3** 移除 `PerformIFCLayout` 单独调用
  ```cpp
  // native_layout_engine.cpp
  void NativeLayoutEngine::ComputeLayout() {
      BuildLayoutTree(root);
      ComputeTaffyLayout(root_node);
      // 移除: PerformIFCLayout(root);  ← 已集成到 Block 布局
      ReadLayoutResults(root);
  }
  ```

#### 验收标准
- [ ] IFC 布局在 Block 布局中执行
- [ ] TX-01 到 TX-12 测试全部通过
- [ ] 无性能回归

---

### Phase 3: 位置计算统一 (预计 1-2 天)

#### 目标
解决 TX-12 vertical-align 问题，统一位置计算逻辑。

#### 任务清单

- [ ] **3.1** IFC 结果直接写入 LayoutNode
  ```cpp
  // 在 ComputeBlockWithIFC 中
  for (auto& line_box : line_boxes) {
      for (auto* box : line_box.boxes) {
          LayoutNode* child_node = GetLayoutNode(box->render_object);
          child_node->x = box->x;
          child_node->y = box->y;  // 已包含 vertical-align 偏移
      }
  }
  ```

- [ ] **3.2** 修改 `ReadLayoutResults` 处理 IFC 子元素
  ```cpp
  void ReadLayoutResults(LayoutNode* node) {
      // 对于 IFC 容器的子元素，位置已在布局阶段设置
      if (node->parent && node->parent->is_ifc_container) {
          // 直接使用已计算的位置，不再累加父元素偏移
      }
  }
  ```

- [ ] **3.3** 简化 `RenderText::Paint`
  ```cpp
  void RenderText::Paint(SkCanvas* canvas) {
      // layout.y 已是正确的盒子顶部位置（由 IFC 计算）
      // 只需添加 ascent 得到基线位置
      float baseline_y = -font_metrics.fAscent;
      
      // 如果需要在盒子内居中（盒子高度 > 文本高度）
      if (layout.height > text_height) {
          baseline_y += (layout.height - text_height) / 2.0f;
      }
  }
  ```

#### 验收标准
- [ ] TX-12 vertical-align 测试通过
- [ ] 所有文本测试（TX-01 到 TX-12）通过
- [ ] 其他布局测试无回归

---

### Phase 4: 清理与优化 (预计 1 天)

#### 任务清单

- [ ] **4.1** 移除冗余代码
  - 删除 `IFCLayout::ApplyLayoutResults`（已集成）
  - 删除 `PerformIFCLayout` 函数
  - 合并重复的样式解析逻辑

- [ ] **4.2** 合并缓存机制
  ```cpp
  // 统一缓存结构
  struct LayoutCache {
      // Taffy 缓存
      LayoutOutput output;
      
      // IFC 缓存（合并）
      std::vector<LineBox> line_boxes;
      float content_version;
  };
  ```

- [ ] **4.3** 添加单元测试
  - IFC 测量接口测试
  - 垂直对齐测试
  - 行高计算测试

- [ ] **4.4** 性能验证
  - 对比集成前后的布局时间
  - 验证无内存泄漏

---

## 可复用组件

以下 IFC 组件可直接复用，无需重写：

| 组件 | 文件 | 复用度 |
|------|------|--------|
| `InlineBox` | `inline_box.h` | 100% |
| `LineBox` | `line_box.h/cpp` | 100% |
| `LineBreaker` | `line_breaker.h/cpp` | 100% |
| `VerticalAligner` | `vertical_aligner.h/cpp` | 100% |
| `MeasureTextForIFC` | `ifc_layout.cpp` | 90% (需调整接口) |
| `CollectInlineContent` | `ifc_layout.cpp` | 80% (需适配 LayoutNode) |

---

## 风险与对策

| 风险 | 概率 | 影响 | 对策 |
|------|------|------|------|
| 现有功能回归 | 中 | 高 | 每个 Phase 完成后运行完整测试套件 |
| inline-block 嵌套问题 | 中 | 中 | 保持递归布局机制，添加嵌套测试用例 |
| 性能变化 | 低 | 中 | 集成前后性能对比测试 |
| 边界情况遗漏 | 中 | 中 | 参考 Chrome/WebKit 实现，添加边界测试 |

---

## 时间线

| 阶段 | 预计时间 | 依赖 |
|------|----------|------|
| Phase 1: 接口统一 | 1-2 天 | 无 |
| Phase 2: Block 集成 | 2-3 天 | Phase 1 |
| Phase 3: 位置统一 | 1-2 天 | Phase 2 |
| Phase 4: 清理优化 | 1 天 | Phase 3 |
| **总计** | **5-8 天** | |

---

## 验收标准

### 功能验收
- [ ] 所有 layout_compare_test 测试用例通过
- [ ] TX-12 vertical-align 正确渲染
- [ ] 无其他布局回归

### 性能验收
- [ ] 布局时间减少 ≥ 20%
- [ ] 内存使用无增加

### 代码质量
- [ ] 无编译警告
- [ ] 关键函数有文档注释
- [ ] 单元测试覆盖核心逻辑

---

## 参考资料

- [CSS Inline Layout Module Level 3](https://www.w3.org/TR/css-inline-3/)
- [CSS Text Module Level 3](https://www.w3.org/TR/css-text-3/)
- [Taffy Layout Library](https://github.com/DioxusLabs/taffy)
- Chrome Blink IFC 实现：`third_party/blink/renderer/core/layout/inline/`

