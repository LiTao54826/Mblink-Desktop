# Layer 子系统

本目录包含 LightUI 的统一层系统实现。

## 文件说明

| 文件 | 说明 |
|------|------|
| `paint_layer.h/cpp` | **统一绘制层**，整合 z-order 排序、compositing 判断、绘制、hit testing |
| `fbo_manager.h/cpp` | FBO（帧缓冲对象）管理器，用于 GPU 增量渲染 |

## PaintLayer 架构

PaintLayer 是 LightUI 统一层系统的核心类，参考 Blink 的 PaintLayer 设计，整合了：

- **Stacking Context 管理**：判断元素是否创建 stacking context
- **Z-index 排序**：维护正负 z-index 子层列表
- **Compositing 判断**：决定是否需要独立的 CompositorLayer
- **绘制 (Paint)**：按 CSS stacking context 规则绘制
- **Hit Testing**：按 z-order 逆序测试点击

### 设计原则

1. **单一类整合所有功能**：减少类数量，简化代码结构
2. **复用现有 CompositorLayer**：GPU 纹理管理由 CompositorLayer 负责
3. **与 RenderObject 紧密关联**：每个需要特殊绘制处理的 RenderObject 有一个 PaintLayer

### Stacking Context 创建条件

根据 CSS 规范，以下情况会创建 stacking context：

- `position: absolute/relative/fixed/sticky` 且 `z-index != auto`
- `opacity < 1`
- `transform != none`
- `filter != none`
- `will-change: transform/opacity`
- `position: fixed`（总是创建）

### 绘制顺序

按 CSS stacking context 规则绘制：

1. 背景和边框
2. 负 z-index 子层（按 z-index 升序）
3. 正常流内容
4. 正 z-index 子层（按 z-index 升序）

### Hit Testing 顺序

按 z-order 逆序测试（从高到低）：

1. 正 z-index 子层（从高到低）
2. 自身
3. 负 z-index 子层（从高到低）

## 使用示例

```cpp
#include "core/render/layer/paint_layer.h"

// 获取 RenderObject 的 PaintLayer
PaintLayer* layer = render_object->GetPaintLayer();

// 检查是否是 stacking context
if (layer->IsStackingContext()) {
    // 更新 z-order 列表
    layer->UpdateZOrderLists();
}

// 绘制
layer->Paint(canvas);

// Hit Testing
HitTestResult result;
if (layer->HitTest(x, y, result)) {
    // 命中了元素
}
```

## 相关文档

- [层系统设计文档](../../../docs/LAYER_SYSTEM_DESIGN.md)
- [统一层系统规范](../../../.kiro/specs/unified-layer-system/design.md)
