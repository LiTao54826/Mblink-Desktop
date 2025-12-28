# Property Tree System

属性树系统，用于高效管理渲染属性（变换、裁剪、效果等）。

## 概述

属性树是一种优化技术，将渲染属性从渲染对象中分离出来，
形成独立的树结构，便于增量更新和属性继承计算。

## 模块列表

| 文件 | 描述 |
|------|------|
| `property_trees.h/cpp` | 属性树集合管理 |
| `transform_tree_node.h/cpp` | 变换属性节点 |
| `clip_tree_node.h/cpp` | 裁剪属性节点 |
| `effect_tree_node.h/cpp` | 效果属性节点（透明度、滤镜等） |
| `scroll_tree_node.h/cpp` | 滚动属性节点 |
| `paint_artifact_compositor.h/cpp` | 绘制工件合成器 |
| `property_tree_error.h/cpp` | 错误处理 |

## 架构说明

```
PropertyTrees
├── TransformTree  - 管理所有变换节点
├── ClipTree       - 管理所有裁剪节点
├── EffectTree     - 管理所有效果节点
└── ScrollTree     - 管理所有滚动节点
```

每个渲染对象通过索引引用属性树中的节点，
而不是直接存储属性值，这样可以：
1. 减少内存占用
2. 加速属性继承计算
3. 支持增量更新

## 参考

- Chromium Compositor Property Trees
- WebKit Property Trees
