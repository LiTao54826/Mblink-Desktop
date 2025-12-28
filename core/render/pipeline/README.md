# Render Pipeline 模块

渲染管线相关组件，负责渲染流程控制和缓存管理。

## 文件列表

| 文件 | 描述 |
|------|------|
| renderer.h/cpp | 主渲染器 |
| render_pipeline.h/cpp | 渲染管线 |
| render_context.h/cpp | 渲染上下文 |
| render_cache.h/cpp | 渲染缓存 |
| render_tree_synchronizer.h/cpp | 渲染树同步 |
| unified_renderer.h/cpp | 统一渲染器 |

## 渲染流程

1. RenderTreeSynchronizer 同步 DOM 和渲染树
2. RenderPipeline 协调布局和绘制
3. Renderer 执行实际绘制
4. RenderCache 缓存渲染结果
