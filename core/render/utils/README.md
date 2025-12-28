# Render Utils 模块

渲染工具类集合，提供基础渲染原语和辅助功能。

## 文件列表

| 文件 | 描述 |
|------|------|
| color.h/cpp | 颜色解析和转换 |
| paint.h/cpp | 画笔封装 |
| transform.h/cpp | CSS Transform 解析 |
| shapes.h/cpp | 基础图形绘制 |
| gradient_renderer.h/cpp | 渐变渲染 |
| shadow_renderer.h/cpp | 阴影渲染 |
| dirty_region.h/cpp | 脏区域管理 |
| dirty_region_collector.h/cpp | 脏区域收集 |
| clip_optimizer.h/cpp | 裁剪优化 |
| filter_cache.h/cpp | 滤镜缓存 |
| performance_monitor.h/cpp | 性能监控 |
| object_pool.h | 对象池模板 |

## 依赖关系

- shapes 依赖 paint
- gradient_renderer/shadow_renderer 依赖 css/css_value
- dirty_region_collector 依赖 dirty_region, dom/node
