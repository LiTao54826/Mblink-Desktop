# Layer 系统开发计划

## 项目概述

**目标**：实现企业级 Layer 合成系统，达到 Chrome 级别的渲染性能

**总工期**：3 周 (15 个工作日)

**预期收益**：
- Resize 性能提升 50-100 倍
- 滚动达到 60fps
- 动画流畅无卡顿
- 支持复杂 UI 特效

---

## 里程碑规划

```
Week 1: 基础架构
├── M1: Layer 基类 (Day 1-2)
├── M2: LayerTreeHost (Day 3-4)
└── M3: 基础合成 (Day 5)

Week 2: 核心功能
├── M4: 分层策略 (Day 6-7)
├── M5: 集成替换 (Day 8-9)
└── M6: 脏区域追踪 (Day 10)

Week 3: 优化完善
├── M7: 动画优化 (Day 11-12)
├── M8: 滚动优化 (Day 13)
├── M9: 测试调优 (Day 14)
└── M10: 文档收尾 (Day 15)
```

---

## 详细开发计划

### Week 1: 基础架构

#### Day 1-2: M1 - Layer 基类

**目标**：实现 Layer 核心类

**新增文件**：
```
core/render/layer/
├── layer.h
├── layer.cpp
├── layer_type.h
└── compositing_reasons.h
```

**任务清单**：
- [ ] Layer 类定义 (属性、方法)
- [ ] LayerType 枚举
- [ ] SkSurface 创建和管理
- [ ] SkPicture 录制和回放
- [ ] 基本的 Paint() 实现
- [ ] 单元测试

**验收标准**：
```cpp
// 能够创建 Layer 并绘制内容
auto layer = std::make_unique<Layer>(LayerType::kNormal);
layer->SetBounds(SkRect::MakeWH(200, 100));
layer->BeginPaint();
// ... 绘制操作
layer->EndPaint();
// surface_ 包含绘制结果
```

---

#### Day 3-4: M2 - LayerTreeHost

**目标**：实现图层树管理

**新增文件**：
```
core/render/layer/
├── layer_tree_host.h
├── layer_tree_host.cpp
└── layer_tree_builder.h
```

**任务清单**：
- [ ] LayerTreeHost 类
- [ ] 层的父子关系管理
- [ ] z-order 排序
- [ ] 层的增删改接口
- [ ] BuildLayerTree() 基础实现
- [ ] 单元测试

**验收标准**：
```cpp
LayerTreeHost host;
host.SetRootLayer(root);
host.AddLayer(child, parent);
host.RemoveLayer(child);
// 层树结构正确
```

---

#### Day 5: M3 - 基础合成

**目标**：实现层合成到最终画布

**修改文件**：
```
core/render/layer/
├── compositor.h (新增)
├── compositor.cpp (新增)
└── layer.cpp (添加 Composite 方法)
```

**任务清单**：
- [ ] Compositor 类
- [ ] 按 z-order 合成所有层
- [ ] 应用 transform 矩阵
- [ ] 应用 opacity
- [ ] 应用 clip
- [ ] 集成测试

**验收标准**：
```cpp
Compositor compositor;
compositor.Composite(layer_tree, output_canvas);
// output_canvas 包含所有层的合成结果
```

---

### Week 2: 核心功能

#### Day 6-7: M4 - 分层策略

**目标**：实现智能分层决策

**新增/修改文件**：
```
core/render/layer/
├── compositing_reasons.cpp (新增)
├── layer_tree_builder.cpp (修改)
```

**任务清单**：
- [ ] CompositingReason 枚举完善
- [ ] GetCompositingReasons() 实现
- [ ] box-shadow 触发分层
- [ ] transform 触发分层
- [ ] opacity 触发分层
- [ ] overflow:scroll 触发分层
- [ ] position:fixed 触发分层
- [ ] 重叠检测 (overlap)
- [ ] 层合并优化 (避免层爆炸)

**验收标准**：
```cpp
// 有 box-shadow 的元素自动分层
auto reasons = GetCompositingReasons(render_obj);
EXPECT_TRUE(reasons & CompositingReason::kBoxShadow);
```

---

#### Day 8-9: M5 - 集成替换

**目标**：用 Layer 系统替换现有 Paint 流程

**修改文件**：
```
core/window/window.h
core/window/window.cpp
core/render/render_object.h
core/render/render_object.cpp
```

**任务清单**：
- [ ] Window 持有 LayerTreeHost
- [ ] Render() 改为调用 LayerTreeHost
- [ ] RenderObject 关联 Layer
- [ ] Paint 流程重构
- [ ] 保持现有功能兼容
- [ ] Feature flag 控制 (可回退)
- [ ] 集成测试

**验收标准**：
```
- 所有现有 demo 正常运行
- box-shadow resize 性能显著提升
- 无视觉回归
```

---

#### Day 10: M6 - 脏区域追踪

**目标**：实现精确的重绘区域追踪

**修改文件**：
```
core/render/layer/
├── dirty_tracker.h (新增)
├── dirty_tracker.cpp (新增)
├── layer.cpp (添加 Invalidate 方法)
```

**任务清单**：
- [ ] DirtyTracker 类
- [ ] 层级别脏标记
- [ ] 矩形级别脏区域
- [ ] InvalidateRect() 实现
- [ ] 脏区域合并优化
- [ ] 只重绘脏层

**验收标准**：
```cpp
layer->InvalidateRect(SkRect::MakeXYWH(10, 10, 50, 50));
// 只有该矩形区域被重绘
```

---

### Week 3: 优化完善

#### Day 11-12: M7 - 动画优化

**目标**：transform/opacity 动画不触发重绘

**修改文件**：
```
core/render/animation_controller.cpp
core/render/layer/layer.cpp
core/render/layer/compositor.cpp
```

**任务清单**：
- [ ] 动画更新 Layer.transform_ 而非重绘
- [ ] 动画更新 Layer.opacity_ 而非重绘
- [ ] 只触发 Composite，不触发 Paint
- [ ] GPU 加速合成 (如果可用)
- [ ] 动画性能测试

**验收标准**：
```
- transform 动画期间 Paint 调用次数 = 0
- opacity 动画期间 Paint 调用次数 = 0
- 动画帧率 >= 60fps
```

---

#### Day 13: M8 - 滚动优化

**目标**：滚动不触发内容重绘

**修改文件**：
```
core/render/layer/
├── scroll_layer.h (新增)
├── scroll_layer.cpp (新增)
```

**任务清单**：
- [ ] ScrollLayer 特化类
- [ ] 滚动只更新 scroll offset
- [ ] 内容层缓存复用
- [ ] 滚动条独立层
- [ ] 滚动性能测试

**验收标准**：
```
- 滚动期间 Paint 调用次数 = 0 (内容不变时)
- 滚动帧率 >= 60fps
```

---

#### Day 14: M9 - 测试调优

**目标**：全面测试和性能调优

**任务清单**：
- [ ] 单元测试覆盖率 > 80%
- [ ] 集成测试所有 demo
- [ ] 性能基准测试
- [ ] 内存泄漏检测
- [ ] 边界情况处理
- [ ] 性能瓶颈分析和优化

**验收标准**：
```
- 所有测试通过
- 无内存泄漏
- 性能达到设计目标
```

---

#### Day 15: M10 - 文档收尾

**目标**：完善文档和代码清理

**任务清单**：
- [ ] API 文档
- [ ] 架构文档更新
- [ ] 使用示例
- [ ] 调试工具文档
- [ ] 代码审查
- [ ] 合并到主分支

---

## 文件结构

```
core/render/layer/
├── layer.h                    # Layer 基类
├── layer.cpp
├── layer_type.h               # 层类型枚举
├── compositing_reasons.h      # 分层原因
├── compositing_reasons.cpp
├── layer_tree_host.h          # 层树管理
├── layer_tree_host.cpp
├── layer_tree_builder.h       # 层树构建
├── layer_tree_builder.cpp
├── compositor.h               # 合成器
├── compositor.cpp
├── dirty_tracker.h            # 脏区域追踪
├── dirty_tracker.cpp
├── scroll_layer.h             # 滚动层
└── scroll_layer.cpp

tests/layer/
├── layer_test.cpp
├── layer_tree_host_test.cpp
├── compositor_test.cpp
└── performance_test.cpp
```

---

## 风险管理

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 集成导致功能回归 | 中 | 高 | Feature flag，充分测试 |
| 内存占用过高 | 中 | 中 | 层合并，按需创建 Surface |
| 性能未达预期 | 低 | 高 | 分阶段验证，及时调整 |
| 工期延误 | 中 | 中 | 预留缓冲，优先核心功能 |

---

## 验收标准

### 功能验收
- [ ] 所有现有 demo 正常运行
- [ ] box-shadow 元素自动分层
- [ ] transform/opacity 动画流畅
- [ ] 滚动流畅

### 性能验收
| 指标 | 当前 | 目标 |
|------|------|------|
| Resize (有 shadow) | 2000ms+ | < 50ms |
| 滚动帧率 | 卡顿 | >= 60fps |
| 动画帧率 | 掉帧 | >= 60fps |
| 内存增加 | - | < 50% |

### 质量验收
- [ ] 单元测试覆盖率 > 80%
- [ ] 无内存泄漏
- [ ] 代码审查通过
- [ ] 文档完整

---

## 后续迭代

Layer 系统完成后，可继续优化：

1. **GPU 合成**：使用 OpenGL/Vulkan 合成
2. **多线程光栅化**：后台线程绘制层
3. **层缓存策略**：LRU 淘汰不活跃层
4. **调试工具**：层边界可视化、性能面板
