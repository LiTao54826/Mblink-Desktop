# Compositor Subsystem

合成器子系统，负责图层合成和渲染优化。

## 模块列表

| 文件 | 描述 |
|------|------|
| `compositor.h/cpp` | 主合成器，管理图层合成 |
| `compositor_layer.h/cpp` | 合成器图层 |
| `layer_tree_builder.h/cpp` | 图层树构建器 |
| `rasterizer.h/cpp` | 光栅化器 |
| `scroll_layer_manager.h/cpp` | 滚动图层管理器 |
| `animation_bounds_calculator.h/cpp` | 动画边界计算器 |
| `animation_layer_bridge.h/cpp` | 动画图层桥接 |

### property_tree/ 子目录
属性树系统，用于高效管理变换、裁剪、效果等属性。

## 依赖关系

### 依赖的模块
- `core/render` - 渲染对象
- `core/dom` - DOM 元素
- `Skia` - 图形渲染

### 被依赖的模块
- `core/window` - 窗口渲染
- `core/render` - 渲染管线

## 架构说明

合成器采用分层架构：
1. 构建图层树 (LayerTreeBuilder)
2. 计算属性树 (PropertyTrees)
3. 光栅化图层 (Rasterizer)
4. 合成输出 (Compositor)
