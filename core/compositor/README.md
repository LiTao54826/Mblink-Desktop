# Compositor Subsystem

合成器子系统，负责图层合成和渲染优化。

## 模块列表

| 文件 | 描述 |
|------|------|
| `compositor.h/cpp` | 主合成器，管理图层合成 |
| `compositor_layer.h/cpp` | 合成器图层 |
| `layer_tree_builder.h/cpp` | 图层树构建器 |
| `layer_tree_manager.h/cpp` | 图层树管理器（增量更新核心） |
| `rasterizer.h/cpp` | 光栅化器 |
| `scroll_layer_manager.h/cpp` | 滚动图层管理器 |

### animation/ 子目录
动画与合成层系统的桥接模块。

| 文件 | 描述 |
|------|------|
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

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            RenderPipeline                                    │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                    LayerTreeManager (核心协调器)                        │ │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐ │ │
│  │  │ IncrementalUpdate│  │ ScrollStateStore │  │ CoordinateSystem     │ │ │
│  │  │ Controller       │  │ (SSOT)           │  │ Manager              │ │ │
│  │  │ - 增量更新队列    │  │ - 滚动状态存储    │  │ - 坐标转换           │ │ │
│  │  │ - 批量操作       │  │ - 变化通知        │  │ - 边界计算           │ │ │
│  │  └──────────────────┘  └──────────────────┘  └──────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                         │
│  ┌─────────────────────────────────┼─────────────────────────────────────┐  │
│  │                                 ▼                                     │  │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │  │
│  │  │              LayerTreeBuilder                                    │ │  │
│  │  │  - Build() 完整构建                                              │ │  │
│  │  │  - IncrementalBuild() 增量构建                                   │ │  │
│  │  │  - AddLayerForObject() / RemoveLayerForObject() 单层操作         │ │  │
│  │  └─────────────────────────────────────────────────────────────────┘ │  │
│  │                                 │                                     │  │
│  │  ┌──────────────────────────────┼──────────────────────────────────┐ │  │
│  │  │                              ▼                                  │ │  │
│  │  │  ┌──────────────────┐  ┌──────────────────┐                    │ │  │
│  │  │  │ Rasterizer       │  │ Compositor       │                    │ │  │
│  │  │  │ - 增量光栅化      │  │ - GPU/CPU 合成   │                    │ │  │
│  │  │  └──────────────────┘  └──────────────────┘                    │ │  │
│  │  └────────────────────────────────────────────────────────────────┘ │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 渲染流程

1. **构建图层树** (LayerTreeBuilder)
   - 遍历渲染树，决定哪些元素需要独立层
   - 支持增量更新，只添加/删除变化的层

2. **管理层树状态** (LayerTreeManager)
   - 协调增量更新
   - 维护滚动状态的单一数据源（SSOT）
   - 提供统一的坐标转换

3. **计算属性树** (PropertyTrees)
   - 高效管理变换、裁剪、效果等属性

4. **光栅化图层** (Rasterizer)
   - 将渲染对象绘制到合成层的 CPU 位图
   - 支持增量光栅化

5. **合成输出** (Compositor)
   - 将多个层合成到屏幕
   - 支持 GPU 和 CPU 合成

### 层提升条件

元素在以下情况会被提升为独立合成层：
- `will-change: transform/opacity`
- `position: fixed`
- CSS transform/opacity 动画
- 可滚动容器

### 增量更新

启用 `enable_incremental_layer_tree` 配置后：
- 添加/删除元素只影响对应的层
- 滚动偏移从单一数据源读取
- Fixed 元素直接挂在根层下
- 动画状态在层更新时保持

## 使用示例

```cpp
// 启用增量层树更新
UnifiedPipelineConfig config;
config.enable_incremental_layer_tree = true;

RenderPipeline pipeline;
pipeline.Initialize(width, height, config);

// 获取 LayerTreeManager 进行调试
auto* manager = pipeline.GetLayerTreeManager();
manager->SetDebugLogging(true);
manager->DumpLayerTree();
```
