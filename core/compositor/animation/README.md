# Compositor Animation 模块

动画与合成层系统的桥接模块。

## 文件说明

| 文件 | 职责 |
|------|------|
| `animation_bounds_calculator.*` | 计算动画过程中元素的完整边界范围，解决旋转/缩放动画被裁剪问题 |
| `animation_layer_bridge.*` | 连接动画系统和合成层系统，实现 transform/opacity 动画的层优化 |

## 设计原理

- transform/opacity 动画不需要重新光栅化，直接更新层属性
- 动画开始时触发层提升，动画结束时可能触发层降级
- 预计算动画边界，避免动画过程中元素被裁剪
