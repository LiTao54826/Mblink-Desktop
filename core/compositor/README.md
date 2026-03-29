# Compositor Subsystem | 合成器子系统

## Overview | 概览

The compositor is responsible for layer composition and render-side optimization.
合成器负责图层合成以及渲染阶段的优化。

## Main Files | 主要文件

- `compositor.h/cpp` — compositor entry / 合成器入口
- `compositor_layer.h/cpp` — compositor layers / 合成层
- `layer_tree_builder.h/cpp` — builds layer trees from render objects / 从渲染对象构建层树
- `layer_tree_manager.h/cpp` — coordinates incremental updates / 协调增量更新
- `layer_tree_types.h` — shared layer-tree types / 层树公共类型
- `rasterizer.h/cpp` — rasterization / 光栅化
- `scroll_layer_manager.h/cpp` — scroll layer handling / 滚动图层管理

## Subdirectories | 子目录

- `animation/` — animation / layer bridge logic
  动画与合成层桥接逻辑
- `property_tree/` — transform, clip, and effect property trees
  变换、裁剪、效果等属性树

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/render`
- `core/dom`
- `Skia`

Used by | 被依赖：

- `core/window`
- `core/render`

## Pipeline Role | 流程中的作用

Typical flow | 典型流程：

1. build layer trees / 构建层树
2. manage incremental state / 管理增量状态
3. compute property trees / 计算属性树
4. rasterize layers / 光栅化图层
5. composite final output / 合成最终输出

## Notes | 说明

- independent layers may be created for scrollable, animated, transformed, or fixed-position content
  可滚动、动画、变换或 fixed 元素可能被提升为独立层
- exact behavior should follow the implementation in source files
  具体行为应以源码实现为准

