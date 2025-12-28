# 代码结构整理重构计划 - 下一阶段

## 当前状态总结

### 已完成的重构
| 模块 | 原始行数 | 当前行数 | 状态 |
|------|----------|----------|------|
| event_loop.cpp | 3920 | 1149 | ✅ 完成 |
| render_object.cpp | 5016 | 1304 | ✅ 完成 |
| window.cpp | 3368 | 1723 | ✅ 完成 |

### 待解决的问题

1. **core/event/ 根目录文件过多** - 28 个源文件，超过 15 个限制
2. **core/render/ 根目录文件过多** - 约 60+ 个文件
3. **render_block.cpp 超标** - 1940 行，需要评审说明
4. **临时/备份文件清理** - event_loop_original.cpp, temp_original.cpp 等

---

## Phase 1: 清理临时文件 (优先级: 高, 风险: 低)

- [ ] 1.1 删除 `core/event/event_loop_original.cpp`
- [ ] 1.2 删除根目录 `temp_original.cpp`
- [ ] 1.3 删除根目录 `et GIT_PAGER=cat` (误创建的文件)
- [ ] 1.4 删除根目录 `tash` (误创建的文件)
- [ ] 1.5 Checkpoint - git commit

---

## Phase 2: core/event/ 目录重组 (优先级: 高)

### 目标结构
```
core/event/
├── CMakeLists.txt
├── README.md
├── dispatch/           # 事件分发器 (已有)
│   ├── mouse_event_dispatcher.*
│   ├── keyboard_event_dispatcher.*
│   └── wheel_event_dispatcher.*
├── loop/               # 事件循环核心
│   ├── event_loop.*
│   ├── frame_controller.*
│   └── task_scheduler.*
├── types/              # 事件类型定义
│   ├── event.*
│   ├── event_types.*
│   ├── mouse_event.*
│   ├── keyboard_event.*
│   └── data_transfer.*
└── input/              # 输入处理
    ├── input_handler.*
    ├── hit_testing.*
    ├── focus_manager.*
    └── keyboard_utils.*
```

### 2.1 迁移 loop/ 子目录 (事件循环核心)

- [ ] 2.1.1 移动 event_loop.h 到 loop/
- [ ] 2.1.2 移动 event_loop.cpp 到 loop/
- [ ] 2.1.3 移动 frame_controller.h 到 loop/
- [ ] 2.1.4 移动 frame_controller.cpp 到 loop/
- [ ] 2.1.5 移动 task_scheduler.h 到 loop/
- [ ] 2.1.6 移动 task_scheduler.cpp 到 loop/
- [ ] 2.1.7 更新 loop/ 内文件的 include 路径
- [ ] 2.1.8 更新 CMakeLists.txt 中 loop/ 文件路径
- [ ] 2.1.9 更新 loop/README.md
- [ ] 2.1.10 Checkpoint - 编译验证

### 2.2 迁移 types/ 子目录 (事件类型定义)

- [ ] 2.2.1 移动 event.h 到 types/
- [ ] 2.2.2 移动 event.cpp 到 types/
- [ ] 2.2.3 移动 event_types.h 到 types/
- [ ] 2.2.4 移动 event_types.cpp 到 types/
- [ ] 2.2.5 移动 mouse_event.h 到 types/
- [ ] 2.2.6 移动 mouse_event.cpp 到 types/
- [ ] 2.2.7 移动 keyboard_event.h 到 types/
- [ ] 2.2.8 移动 keyboard_event.cpp 到 types/
- [ ] 2.2.9 移动 data_transfer.h 到 types/
- [ ] 2.2.10 移动 data_transfer.cpp 到 types/
- [ ] 2.2.11 更新 types/ 内文件的 include 路径
- [ ] 2.2.12 更新 CMakeLists.txt 中 types/ 文件路径
- [ ] 2.2.13 更新 types/README.md
- [ ] 2.2.14 Checkpoint - 编译验证

### 2.3 迁移 input/ 子目录 (输入处理)

- [ ] 2.3.1 移动 input_handler.h 到 input/
- [ ] 2.3.2 移动 input_handler.cpp 到 input/
- [ ] 2.3.3 移动 hit_testing.h 到 input/
- [ ] 2.3.4 移动 hit_testing.cpp 到 input/
- [ ] 2.3.5 移动 focus_manager.h 到 input/
- [ ] 2.3.6 移动 focus_manager.cpp 到 input/
- [ ] 2.3.7 移动 keyboard_utils.h 到 input/
- [ ] 2.3.8 移动 keyboard_utils.cpp 到 input/
- [ ] 2.3.9 更新 input/ 内文件的 include 路径
- [ ] 2.3.10 更新 CMakeLists.txt 中 input/ 文件路径
- [ ] 2.3.11 更新 input/README.md
- [ ] 2.3.12 Checkpoint - 编译验证

### 2.4 更新外部依赖的 include 路径

- [ ] 2.4.1 搜索并更新 core/dom/ 中的 event include 路径
- [ ] 2.4.2 搜索并更新 core/window/ 中的 event include 路径
- [ ] 2.4.3 搜索并更新 core/quickjs/ 中的 event include 路径
- [ ] 2.4.4 搜索并更新 core/editing/ 中的 event include 路径
- [ ] 2.4.5 搜索并更新 tools/ 中的 event include 路径
- [ ] 2.4.6 搜索并更新其他模块中的 event include 路径
- [ ] 2.4.7 Checkpoint - 全项目编译验证

### 2.5 清理和文档

- [ ] 2.5.1 更新 core/event/README.md 反映新结构
- [ ] 2.5.2 删除 event_system.h/cpp (如果已废弃)
- [ ] 2.5.3 验证根目录文件数 ≤ 10
- [ ] 2.5.4 git commit

---

## Phase 3: core/render/ 目录重组 (优先级: 中)

### 目标结构
```
core/render/
├── CMakeLists.txt
├── README.md
├── animation/          # 动画 (已有)
├── canvas/             # Canvas (已有)
├── css/                # CSS 相关 (已有)
├── image/              # 图片 (已有)
├── painters/           # 绘制器 (已有)
├── text/               # 文本 (已有)
├── objects/            # 渲染对象 (新建)
├── layer/              # 图层系统 (新建)
├── pipeline/           # 渲染管线 (新建)
└── utils/              # 工具类 (新建)
```

### 3.1 创建 objects/ 子目录 (渲染对象)

- [ ] 3.1.1 创建 objects/ 目录
- [ ] 3.1.2 移动 render_object.h 到 objects/
- [ ] 3.1.3 移动 render_object.cpp 到 objects/
- [ ] 3.1.4 移动 render_block.cpp 到 objects/
- [ ] 3.1.5 移动 render_inline.cpp 到 objects/
- [ ] 3.1.6 移动 render_text.cpp 到 objects/
- [ ] 3.1.7 移动 render_table.cpp 到 objects/
- [ ] 3.1.8 移动 render_svg.h/cpp 到 objects/
- [ ] 3.1.9 移动 render_inline_block.h/cpp 到 objects/
- [ ] 3.1.10 更新 objects/ 内文件的 include 路径
- [ ] 3.1.11 更新 CMakeLists.txt
- [ ] 3.1.12 创建 objects/README.md
- [ ] 3.1.13 Checkpoint - 编译验证

### 3.2 创建 layer/ 子目录 (图层系统)

- [ ] 3.2.1 创建 layer/ 目录
- [ ] 3.2.2 移动 layer.h/cpp 到 layer/
- [ ] 3.2.3 移动 layer_manager.h/cpp 到 layer/
- [ ] 3.2.4 移动 fbo_manager.h/cpp 到 layer/
- [ ] 3.2.5 更新 layer/ 内文件的 include 路径
- [ ] 3.2.6 更新 CMakeLists.txt
- [ ] 3.2.7 创建 layer/README.md
- [ ] 3.2.8 Checkpoint - 编译验证

### 3.3 创建 pipeline/ 子目录 (渲染管线)

- [ ] 3.3.1 创建 pipeline/ 目录
- [ ] 3.3.2 移动 renderer.h/cpp 到 pipeline/
- [ ] 3.3.3 移动 render_pipeline.h/cpp 到 pipeline/
- [ ] 3.3.4 移动 unified_renderer.h/cpp 到 pipeline/
- [ ] 3.3.5 移动 render_context.h/cpp 到 pipeline/
- [ ] 3.3.6 移动 render_cache.h/cpp 到 pipeline/
- [ ] 3.3.7 移动 render_tree_synchronizer.h/cpp 到 pipeline/
- [ ] 3.3.8 更新 pipeline/ 内文件的 include 路径
- [ ] 3.3.9 更新 CMakeLists.txt
- [ ] 3.3.10 创建 pipeline/README.md
- [ ] 3.3.11 Checkpoint - 编译验证

### 3.4 创建 utils/ 子目录 (工具类)

- [ ] 3.4.1 创建 utils/ 目录
- [ ] 3.4.2 移动 color.h/cpp 到 utils/
- [ ] 3.4.3 移动 transform.h/cpp 到 utils/
- [ ] 3.4.4 移动 shapes.h/cpp 到 utils/
- [ ] 3.4.5 移动 paint.h/cpp 到 utils/
- [ ] 3.4.6 移动 gradient_renderer.h/cpp 到 utils/
- [ ] 3.4.7 移动 shadow_renderer.h/cpp 到 utils/
- [ ] 3.4.8 移动 dirty_region.h/cpp 到 utils/
- [ ] 3.4.9 移动 dirty_region_collector.h/cpp 到 utils/
- [ ] 3.4.10 移动 clip_optimizer.h/cpp 到 utils/
- [ ] 3.4.11 移动 filter_cache.h/cpp 到 utils/
- [ ] 3.4.12 移动 performance_monitor.h/cpp 到 utils/
- [ ] 3.4.13 移动 object_pool.h 到 utils/
- [ ] 3.4.14 更新 utils/ 内文件的 include 路径
- [ ] 3.4.15 更新 CMakeLists.txt
- [ ] 3.4.16 创建 utils/README.md
- [ ] 3.4.17 Checkpoint - 编译验证

### 3.5 处理剩余文件

- [ ] 3.5.1 移动 scrollbar_controller.h/cpp 到 objects/ 或保留
- [ ] 3.5.2 移动 select_dropdown.h/cpp 到合适位置
- [ ] 3.5.3 移动 box_renderer.h/cpp 到 painters/
- [ ] 3.5.4 移动 text_renderer.h/cpp 到 text/
- [ ] 3.5.5 移动 text_transform.h/cpp 到 text/
- [ ] 3.5.6 移动 list_marker.h/cpp 到合适位置
- [ ] 3.5.7 移动 svg_path_parser.h/cpp 到 objects/ 或新建 svg/
- [ ] 3.5.8 移动 style_resolver.h/cpp 到 css/
- [ ] 3.5.9 Checkpoint - 编译验证

### 3.6 更新外部依赖的 include 路径

- [ ] 3.6.1 搜索并更新 core/dom/ 中的 render include 路径
- [ ] 3.6.2 搜索并更新 core/window/ 中的 render include 路径
- [ ] 3.6.3 搜索并更新 core/event/ 中的 render include 路径
- [ ] 3.6.4 搜索并更新 core/quickjs/ 中的 render include 路径
- [ ] 3.6.5 搜索并更新其他模块中的 render include 路径
- [ ] 3.6.6 Checkpoint - 全项目编译验证

### 3.7 清理和文档

- [ ] 3.7.1 更新 core/render/README.md 反映新结构
- [ ] 3.7.2 验证根目录文件数 ≤ 15
- [ ] 3.7.3 git commit

---

## Phase 4: 文档补充 (优先级: 低)

- [ ] 4.1 为 render_block.cpp 添加大文件说明注释 (1940 行)
- [ ] 4.2 检查所有新子目录都有 README.md
- [ ] 4.3 更新项目根目录文档引用
- [ ] 4.4 git commit

---

## 执行建议

### 推荐顺序
1. **Phase 1** - 立即执行，无风险
2. **Phase 2** - 下一步重点，涉及 include 路径更新
3. **Phase 3** - 可选，工作量较大
4. **Phase 4** - 随时补充

### 风险评估
| Phase | 风险 | 工作量 | 影响范围 |
|-------|------|--------|----------|
| 1 | 低 | 小 | 无 |
| 2 | 中 | 中 | event 模块及其依赖 |
| 3 | 高 | 大 | render 模块及全项目 |
| 4 | 低 | 小 | 无 |

### 注意事项
- 每个 Checkpoint 后立即提交 git
- 文件迁移前确保编译通过
- 批量更新 include 路径时使用搜索替换
- 保持 CMakeLists.txt 同步更新
- 每个子目录迁移完成后单独验证编译
