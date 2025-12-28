# Paint System

绘制系统相关类型，管理绘制指令和绘制产物。

## 文件说明

| 文件 | 职责 |
|------|------|
| `display_item.*` | 绘制指令（DrawRect、DrawText 等） |
| `paint_chunk.*` | 绘制块（具有相同属性状态的连续绘制指令） |
| `paint_artifact.*` | 绘制产物（绘制块集合） |
| `paint_artifact_compositor.*` | 绘制产物合成器（将绘制产物转换为合成层） |
| `pending_layer.*` | 待定层（层化过程中的中间表示） |

## 绘制流程

```
RenderObject.Paint()
    ↓
DisplayItem (绘制指令)
    ↓
PaintChunk (按属性状态分组)
    ↓
PaintArtifact (收集所有绘制块)
    ↓
PaintArtifactCompositor (生成合成层)
```
