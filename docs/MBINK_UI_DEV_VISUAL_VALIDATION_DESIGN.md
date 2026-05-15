# MBink UI Dev 视觉验证能力设计文档

> 状态：Draft v1
> 目标：为 `mbink-ui-dev` 提供面向 AI 的视觉观测与视觉验证能力，支持后续稳定开发与迭代。

---

## 1. 背景

当前 `mbink-ui-dev snapshot` 已能返回 DOM tree、`rect`、`attrs`、`visible`、`interactive` 等结构化信息，但 `screenshot_base64` 仍为空。现状足以支撑结构理解，不足以支撑视觉验收。

仅靠真实截图，AI 能看到最终效果，但对布局层级、交互目标、局部问题定位不稳定；仅靠骨架图，AI 能理解结构，但无法判断颜色、阴影、字体、透明度等真实视觉效果。

因此需要建设一套多层视觉观测系统，而不是单一截图能力。

---

## 2. 设计目标

1. 让 AI 能基于真实 UI 进行视觉验收，而不只是结构推断。
2. 让 AI 能稳定理解布局结构、语义角色、局部元素状态。
3. 让系统能输出可机器消费的视觉断言结果，降低纯视觉推理的不稳定性。
4. 在现有 `mbink-ui-dev` 架构上最小增量落地，优先复用 snapshot/export/inspect/query 能力。

## 3. 非目标

1. 不做通用 CV 模型训练或复杂图像识别系统。
2. 不在首期实现完整视觉回归平台。
3. 不替代现有 `query_element` / `inspect` / `highlight`，而是在其上增强。

---

## 4. 总体方案

视觉验证能力采用四层观测模型：

1. 像素层：真实截图，用于最终视觉验收。
2. 结构层：骨架图 / layout overlay，用于稳定理解布局结构。
3. 语义层：语义树 / style brief，用于理解元素角色、关系与局部样式。
4. 约束层：自动视觉断言，用于直接产出可验证结论。

结论：最优组合不是“截图或骨架图”，而是“截图 + 语义树 + 局部裁剪 + 自动断言”，骨架图作为增强层。

---

## 5. 能力边界与优先级

### P1 必做

1. 为 `snapshot_ui` 补齐 `screenshot_base64`。
2. 为 snapshot tree 增加 `semantic_role` 与 `computed_style_brief`。
3. 提供元素级 crop 导出能力。

### P2 增强

1. 提供 `skeleton_base64`。
2. 提供页面级与元素级 `visual_assertions`。
3. 提供 before/after diff 图与 changed node 摘要。

### P3 高级

1. 组件基线比对。
2. 视觉评分与回归等级。
3. IDE 侧可视化预览联动。

---

## 6. 对外接口设计

### 6.1 `snapshot_ui`

返回字段建议：

```json
{
  "ok": true,
  "timestamp": "2026-01-01T12:00:00Z",
  "viewport": { "width": 800, "height": 600, "dpr": 1.0 },
  "screenshot_base64": "...",
  "skeleton_base64": "...",
  "tree": {},
  "visual_assertions": []
}
```

说明：
- `screenshot_base64`：真实截图，PNG base64。
- `skeleton_base64`：基于 tree 绘制的结构图，首期可选返回。
- `tree`：语义增强后的 UI 树。
- `visual_assertions`：页面级断言结果。

### 6.2 `inspect_element`

返回字段建议：

```json
{
  "ok": true,
  "node_id": "node@123",
  "crop_base64": "...",
  "semantic_role": "primary_button",
  "style_brief": {},
  "relations": {},
  "visual_assertions": []
}
```

### 6.3 `compare_ui`

返回字段建议：

```json
{
  "ok": true,
  "before_screenshot_base64": "...",
  "after_screenshot_base64": "...",
  "diff_base64": "...",
  "changed_nodes": [],
  "visual_regressions": []
}
```

---

## 7. 数据模型设计

### 7.1 语义树节点

每个节点在现有字段基础上新增：

- `semantic_role`：如 `page_title` / `primary_button` / `input` / `card` / `dialog`
- `computed_style_brief`：仅保留 AI 判断高价值字段
- `state`：如 `disabled` / `focused` / `hovered` / `selected`
- `relations`：如 `inside` / `aligned_with` / `label_for`

### 7.2 `computed_style_brief` 建议字段

- `display`
- `position`
- `color`
- `backgroundColor`
- `fontSize`
- `fontWeight`
- `textAlign`
- `borderRadius`
- `opacity`
- `zIndex`
- `overflow`
- `boxShadow`
- `visibility`

原则：只保留高价值字段，不返回完整 computed style，避免 payload 失控。

---

## 8. 渲染与导出链路

### 8.1 真实截图导出

推荐在 `tools/esm_loader/ui_dev_snapshot.cpp` 的 `ExportUiDevSnapshot()` 中完成：

1. 从 `Window` 获取当前渲染 surface。
2. 使用 `makeImageSnapshot()` 获取当前帧图像。
3. 编码为 PNG。
4. base64 写入 `screenshot_base64`。
5. 与 tree 同次导出，保证时序一致。

若 GPU surface 直接读取不稳定，可增加 CPU readback 或统一离屏导出路径作为 fallback。

### 8.2 骨架图导出

首期不依赖真实像素渲染，直接根据 snapshot tree 的 `rect` 绘制：

- 容器：矩形边框
- 文本：灰条占位
- 图片：占位框
- button / input / dialog：按语义类型上色或加粗
- 可交互元素：高亮描边

### 8.3 元素级 crop

基于 `node_id + rect` 对真实截图裁剪得到 `crop_base64`，供局部校验使用。该能力应优先服务 `inspect_element`。

---

## 9. 自动视觉断言

首期断言建议：

1. `overlap`：元素异常重叠
2. `overflow`：文本或内容超出容器
3. `offscreen`：元素超出 viewport
4. `misalignment`：同组元素未对齐
5. `tiny_hit_target`：点击区域过小
6. `invisible_interactive`：可交互元素不可见
7. `modal_not_centered`：弹层未居中

断言结果格式：

```json
{ "type": "misalignment", "target": ["btn_ok", "btn_cancel"], "status": "fail", "message": "底边差值 6px" }
```

原则：断言结果优先由几何和样式规则直接计算，AI 负责复核和解释，不把全部判断压给模型。

---

## 10. 开发落地顺序

### Phase 1

- 补齐 `screenshot_base64`
- 保证 snapshot 与截图时序一致
- 为 `inspect` 增加 `crop_base64`

### Phase 2

- 增加 `semantic_role`
- 增加 `computed_style_brief`
- 增加首批 `visual_assertions`

### Phase 3

- 增加 `skeleton_base64`
- 增加 `compare_ui`
- 增加 diff 导出与回归摘要

---

## 11. 风险与对策

1. 截图与 DOM 时序不一致：统一在同一渲染稳定点导出。
2. GPU readback 不稳定：增加 CPU fallback。
3. payload 过大：默认返回简化 tree，图片支持按需开关。
4. AI 误判：引入断言层与元素 crop 降低纯视觉猜测。

---

## 12. 测试建议

1. 单元测试：style brief 提取、断言规则计算。
2. 集成测试：open → build → snapshot → inspect → compare 全链路。
3. 回归测试：按钮对齐、弹窗居中、文本溢出、滚动区裁切等典型场景。
4. 性能测试：截图导出、骨架图生成、diff 计算耗时。

---

## 13. 最终结论

面向 AI 的视觉验证能力应建设为“多层视觉观测系统”，核心顺序为：

1. 真实截图
2. 语义树
3. 元素级 crop
4. 自动断言
5. 骨架图与 diff 增强

这一路线兼顾实现成本、AI 理解稳定性与后续扩展性，适合作为 `mbink-ui-dev` 视觉能力的正式开发基线。
