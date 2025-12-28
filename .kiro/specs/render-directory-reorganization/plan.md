# core/render/ 目录重组计划

## 当前状态分析

### 根目录文件统计
- 源文件 (.cpp): 36 个
- 头文件 (.h): 32 个
- 总计: 68 个文件 (严重超标，规范要求 ≤15)

### 已有子目录
| 目录 | 文件数 | 描述 |
|------|--------|------|
| animation/ | 18 | 动画系统 |
| canvas/ | 10 | Canvas 2D API |
| css/ | 8 | CSS 相关 |
| image/ | 8 | 图片处理 |
| painters/ | 8 | 绘制器 |
| text/ | 4 | 文本/字体 |

### 根目录文件分类

#### 渲染对象 (objects/) - 14 个文件
- render_object.h/cpp - 渲染对象基类
- render_block.cpp - 块级元素
- render_inline.cpp - 行内元素
- render_inline_block.h/cpp - 行内块元素
- render_text.cpp - 文本渲染
- render_table.cpp - 表格渲染
- render_svg.h/cpp - SVG 渲染
- scrollbar_controller.h/cpp - 滚动条控制

#### 图层系统 (layer/) - 6 个文件
- layer.h/cpp - 图层
- layer_manager.h/cpp - 图层管理器
- fbo_manager.h/cpp - FBO 管理

#### 渲染管线 (pipeline/) - 14 个文件
- renderer.h/cpp - 渲染器
- render_pipeline.h/cpp - 渲染管线
- unified_renderer.h/cpp - 统一渲染器
- render_context.h/cpp - 渲染上下文
- render_cache.h/cpp - 渲染缓存
- render_tree_synchronizer.h/cpp - 渲染树同步

#### 工具类 (utils/) - 26 个文件
- color.h/cpp - 颜色
- transform.h/cpp - 变换
- shapes.h/cpp - 形状
- paint.h/cpp - 画笔
- gradient_renderer.h/cpp - 渐变渲染
- shadow_renderer.h/cpp - 阴影渲染
- dirty_region.h/cpp - 脏区域
- dirty_region_collector.h/cpp - 脏区域收集
- clip_optimizer.h/cpp - 裁剪优化
- filter_cache.h/cpp - 滤镜缓存
- performance_monitor.h/cpp - 性能监控
- object_pool.h - 对象池

#### 其他 - 8 个文件
- box_renderer.h/cpp - 盒子渲染器
- list_marker.h/cpp - 列表标记
- select_dropdown.h/cpp - 下拉菜单
- style_resolver.h/cpp - 样式解析 → css/
- svg_path_parser.h/cpp - SVG 路径解析 → objects/
- text_renderer.h/cpp - 文本渲染器 → text/
- text_transform.h/cpp - 文本变换 → text/

---

## 目标结构

```
core/render/
├── CMakeLists.txt
├── README.md
├── animation/          # 动画 (已有, 18 文件)
├── canvas/             # Canvas (已有, 10 文件)
├── css/                # CSS 相关 (已有, 8 文件)
│   └── style_resolver.* (新增)
├── image/              # 图片 (已有, 8 文件)
├── painters/           # 绘制器 (已有, 8 文件)
│   └── box_renderer.* (新增)
├── text/               # 文本 (已有, 4 文件)
│   ├── text_renderer.* (新增)
│   └── text_transform.* (新增)
├── objects/            # 渲染对象 (新建, ~16 文件)
│   ├── render_object.*
│   ├── render_block.cpp
│   ├── render_inline.cpp
│   ├── render_inline_block.*
│   ├── render_text.cpp
│   ├── render_table.cpp
│   ├── render_svg.*
│   ├── svg_path_parser.*
│   ├── scrollbar_controller.*
│   ├── list_marker.*
│   └── select_dropdown.*
├── layer/              # 图层系统 (新建, 6 文件)
│   ├── layer.*
│   ├── layer_manager.*
│   └── fbo_manager.*
├── pipeline/           # 渲染管线 (新建, 14 文件)
│   ├── renderer.*
│   ├── render_pipeline.*
│   ├── unified_renderer.*
│   ├── render_context.*
│   ├── render_cache.*
│   └── render_tree_synchronizer.*
└── utils/              # 工具类 (新建, ~20 文件)
    ├── color.*
    ├── transform.*
    ├── shapes.*
    ├── paint.*
    ├── gradient_renderer.*
    ├── shadow_renderer.*
    ├── dirty_region.*
    ├── dirty_region_collector.*
    ├── clip_optimizer.*
    ├── filter_cache.*
    ├── performance_monitor.*
    └── object_pool.h
```

---

## 实施计划

### Phase 1: 扩展已有子目录 (风险: 低)

#### 1.1 扩展 css/ 子目录
- [x] 1.1.1 移动 style_resolver.h 到 css/
- [x] 1.1.2 移动 style_resolver.cpp 到 css/
- [x] 1.1.3 更新 css/ 内文件的 include 路径
- [x] 1.1.4 更新 CMakeLists.txt
- [x] 1.1.5 Checkpoint - 编译验证

#### 1.2 扩展 text/ 子目录
- [x] 1.2.1 移动 text_renderer.h 到 text/
- [x] 1.2.2 移动 text_renderer.cpp 到 text/
- [x] 1.2.3 移动 text_transform.h 到 text/
- [x] 1.2.4 移动 text_transform.cpp 到 text/
- [x] 1.2.5 更新 text/ 内文件的 include 路径
- [x] 1.2.6 更新 CMakeLists.txt
- [x] 1.2.7 Checkpoint - 编译验证

#### 1.3 扩展 painters/ 子目录
- [x] 1.3.1 移动 box_renderer.h 到 painters/
- [x] 1.3.2 移动 box_renderer.cpp 到 painters/
- [x] 1.3.3 更新 painters/ 内文件的 include 路径
- [x] 1.3.4 更新 CMakeLists.txt
- [x] 1.3.5 Checkpoint - 编译验证

---

### Phase 2: 创建 layer/ 子目录 (风险: 低)

- [x] 2.1 创建 layer/ 目录
- [x] 2.2 移动 layer.h 到 layer/
- [x] 2.3 移动 layer.cpp 到 layer/
- [x] 2.4 移动 layer_manager.h 到 layer/
- [x] 2.5 移动 layer_manager.cpp 到 layer/
- [x] 2.6 移动 fbo_manager.h 到 layer/
- [x] 2.7 移动 fbo_manager.cpp 到 layer/
- [x] 2.8 更新 layer/ 内文件的 include 路径
- [x] 2.9 更新 CMakeLists.txt
- [x] 2.10 创建 layer/README.md
- [x] 2.11 Checkpoint - 编译验证

---

### Phase 3: 创建 utils/ 子目录 (风险: 中)

#### 3.1 基础工具类
- [x] 3.1.1 创建 utils/ 目录
- [x] 3.1.2 移动 color.h/cpp 到 utils/
- [x] 3.1.3 移动 transform.h/cpp 到 utils/
- [x] 3.1.4 移动 shapes.h/cpp 到 utils/
- [x] 3.1.5 移动 paint.h/cpp 到 utils/
- [x] 3.1.6 Checkpoint - 编译验证

#### 3.2 渲染辅助类
- [x] 3.2.1 移动 gradient_renderer.h/cpp 到 utils/
- [x] 3.2.2 移动 shadow_renderer.h/cpp 到 utils/
- [x] 3.2.3 移动 filter_cache.h/cpp 到 utils/
- [x] 3.2.4 Checkpoint - 编译验证

#### 3.3 脏区域和优化
- [x] 3.3.1 移动 dirty_region.h/cpp 到 utils/
- [x] 3.3.2 移动 dirty_region_collector.h/cpp 到 utils/
- [x] 3.3.3 移动 clip_optimizer.h/cpp 到 utils/
- [x] 3.3.4 移动 performance_monitor.h/cpp 到 utils/
- [x] 3.3.5 移动 object_pool.h 到 utils/
- [x] 3.3.6 更新 utils/ 内文件的 include 路径
- [x] 3.3.7 更新 CMakeLists.txt
- [x] 3.3.8 创建 utils/README.md
- [x] 3.3.9 Checkpoint - 编译验证

---

### Phase 4: 创建 pipeline/ 子目录 (风险: 中)

- [x] 4.1 创建 pipeline/ 目录
- [x] 4.2 移动 renderer.h/cpp 到 pipeline/
- [x] 4.3 移动 render_pipeline.h/cpp 到 pipeline/
- [x] 4.4 移动 unified_renderer.h/cpp 到 pipeline/
- [x] 4.5 移动 render_context.h/cpp 到 pipeline/
- [x] 4.6 移动 render_cache.h/cpp 到 pipeline/
- [x] 4.7 移动 render_tree_synchronizer.h/cpp 到 pipeline/
- [x] 4.8 更新 pipeline/ 内文件的 include 路径
- [x] 4.9 更新 CMakeLists.txt
- [x] 4.10 创建 pipeline/README.md
- [x] 4.11 Checkpoint - 编译验证

---

### Phase 5: 创建 objects/ 子目录 (风险: 高)

#### 5.1 核心渲染对象
- [x] 5.1.1 创建 objects/ 目录
- [x] 5.1.2 移动 render_object.h 到 objects/
- [x] 5.1.3 移动 render_object.cpp 到 objects/
- [x] 5.1.4 移动 render_block.cpp 到 objects/
- [x] 5.1.5 移动 render_inline.cpp 到 objects/
- [x] 5.1.6 移动 render_text.cpp 到 objects/
- [x] 5.1.7 Checkpoint - 编译验证

#### 5.2 特殊渲染对象
- [x] 5.2.1 移动 render_inline_block.h/cpp 到 objects/
- [x] 5.2.2 移动 render_table.cpp 到 objects/
- [x] 5.2.3 移动 render_svg.h/cpp 到 objects/
- [x] 5.2.4 移动 svg_path_parser.h/cpp 到 objects/
- [x] 5.2.5 Checkpoint - 编译验证

#### 5.3 辅助组件
- [x] 5.3.1 移动 scrollbar_controller.h/cpp 到 objects/
- [x] 5.3.2 移动 list_marker.h/cpp 到 objects/
- [x] 5.3.3 移动 select_dropdown.h/cpp 到 objects/
- [x] 5.3.4 更新 objects/ 内文件的 include 路径
- [x] 5.3.5 更新 CMakeLists.txt
- [x] 5.3.6 创建 objects/README.md
- [x] 5.3.7 Checkpoint - 编译验证

---

### Phase 6: 更新外部依赖 (风险: 高)

- [x] 6.1 搜索并更新 core/dom/ 中的 render include 路径
- [x] 6.2 搜索并更新 core/window/ 中的 render include 路径
- [x] 6.3 搜索并更新 core/event/ 中的 render include 路径
- [x] 6.4 搜索并更新 core/quickjs/ 中的 render include 路径
- [x] 6.5 搜索并更新 core/editing/ 中的 render include 路径
- [x] 6.6 搜索并更新 tests/ 中的 render include 路径
- [x] 6.7 Checkpoint - 全项目编译验证

---

### Phase 7: 清理和文档

- [x] 7.1 更新 core/render/README.md 反映新结构
- [x] 7.2 验证根目录文件数 ≤ 5
- [x] 7.3 git commit

---

## 风险评估

| Phase | 风险 | 工作量 | 影响范围 |
|-------|------|--------|----------|
| 1 | 低 | 小 | render 模块内部 |
| 2 | 低 | 小 | render 模块内部 |
| 3 | 中 | 中 | render 模块及部分依赖 |
| 4 | 中 | 中 | render 模块及部分依赖 |
| 5 | 高 | 大 | 全项目（render_object 被广泛引用） |
| 6 | 高 | 大 | 全项目 |
| 7 | 低 | 小 | 无 |

## 执行建议

1. 按 Phase 顺序执行，每个 Phase 完成后提交 git
2. Phase 5 和 6 风险最高，建议分多次提交
3. 每次迁移后立即编译验证
4. 保留回滚能力（git reset）

## 预计工作量

- Phase 1-2: 约 1 小时
- Phase 3-4: 约 2 小时
- Phase 5-6: 约 3-4 小时
- Phase 7: 约 30 分钟

总计: 约 6-8 小时
