# 统一渲染管线重构 - 任务清单

## 阶段一：创建统一管线

### Task 1.1: 创建 UnifiedRenderPipeline 基础框架
- [ ] 创建 `core/render/unified_render_pipeline.h`
- [ ] 创建 `core/render/unified_render_pipeline.cpp`
- [ ] 定义 `RenderStage` 枚举
- [ ] 定义 `UnifiedPipelineConfig` 配置结构
- [ ] 定义 `FrameStats` 统计结构
- [ ] 实现构造函数和析构函数
- [ ] 更新 `core/render/CMakeLists.txt`

### Task 1.2: 实现初始化和配置
- [ ] 实现 `Initialize()` 方法
- [ ] 实现 `Shutdown()` 方法
- [ ] 实现 `Resize()` 方法
- [ ] 实现 `SetDocument()` 方法
- [ ] 实现 `SetConfig()` 方法
- [ ] 实现 `SetDpiScale()` 方法

### Task 1.3: 整合 DOM 同步阶段（来自 V1）
- [ ] 集成 `DirtyNodeTracker`
- [ ] 集成 `RenderTreeSynchronizer`
- [ ] 集成 `RenderTreeBuilder`
- [ ] 实现 `DoDOMSync()` 方法

### Task 1.4: 整合样式和布局阶段（来自 V1）
- [ ] 集成 `NativeLayoutEngine`
- [ ] 实现 `DoStyleRecalc()` 方法
- [ ] 实现 `DoLayout()` 方法

### Task 1.5: 整合层树构建阶段（来自 V2）
- [ ] 集成 `LayerTreeBuilder`
- [ ] 集成 `PropertyTreeBuilder`
- [ ] 实现 `DoLayerTreeBuild()` 方法

### Task 1.6: 整合光栅化阶段（来自 V2）
- [ ] 集成 `Rasterizer`
- [ ] 实现 `DoRasterize()` 方法

### Task 1.7: 整合合成阶段（来自 V2）
- [ ] 集成 `Compositor`
- [ ] 实现 `DoComposite()` 方法

### Task 1.8: 实现主渲染入口
- [ ] 实现 `ProcessFrame()` 方法
- [ ] 实现 `NeedsUpdate()` 方法
- [ ] 实现脏标记方法
- [ ] 实现 `ForceFullUpdate()` 方法

### Task 1.9: 整合滚动优化（来自 V2）
- [ ] 集成 `ScrollLayerManager`
- [ ] 实现 `HandleScroll()` 方法
- [ ] 实现 `ScrollTo()` 方法

### Task 1.10: 整合动画优化（来自 V2）
- [ ] 集成 `AnimationLayerBridge`
- [ ] 实现 `BeginAnimationFrame()` 方法
- [ ] 实现 `UpdateAnimationProperty()` 方法
- [ ] 实现 `EndAnimationFrame()` 方法
- [ ] 实现动画生命周期回调

### Task 1.11: 整合属性树系统（来自 V2）
- [ ] 集成 `PropertyTrees`
- [ ] 集成 `PaintArtifactCompositor`
- [ ] 实现直接属性更新方法

### Task 1.12: 实现统计和调试功能
- [ ] 实现帧统计收集
- [ ] 实现 `GetLastFrameStats()` 方法
- [ ] 实现调试选项（层边界显示等）

---

## 阶段二：Window 迁移

### Task 2.1: 添加统一管线到 Window
- [ ] 在 `Window` 类中添加 `unified_pipeline_` 成员
- [ ] 添加 `use_unified_pipeline_` 开关
- [ ] 在 `Window::Initialize()` 中初始化统一管线

### Task 2.2: 迁移渲染逻辑
- [ ] 修改 `Window::Render()` 使用统一管线
- [ ] 保留旧路径作为回退
- [ ] 添加切换开关

### Task 2.3: 迁移滚动处理
- [ ] 修改滚动事件处理使用统一管线
- [ ] 验证滚动优化正常工作

### Task 2.4: 迁移动画处理
- [ ] 修改 `AnimationApplicator` 使用统一管线
- [ ] 验证动画优化正常工作

### Task 2.5: 功能验证
- [ ] 验证基本渲染功能
- [ ] 验证增量更新功能
- [ ] 验证滚动功能
- [ ] 验证动画功能
- [ ] 验证 DevTools 集成

---

## 阶段三：清理冗余代码

### Task 3.1: 移除适配层
- [ ] 删除 `WindowCompositorAdapter` 类
- [ ] 删除 `window_compositor_adapter.h`
- [ ] 删除 `window_compositor_adapter.cpp`
- [ ] 更新 CMakeLists.txt

### Task 3.2: 清理 Window 类
- [ ] 移除 `render_pipeline_` 成员
- [ ] 移除 `compositor_adapter_` 成员
- [ ] 移除散落的组件成员
- [ ] 简化 Window 头文件

### Task 3.3: 废弃旧管线
- [ ] 标记 `RenderPipeline` 为 deprecated
- [ ] 标记 `RenderPipelineV2` 为 deprecated
- [ ] 更新相关测试

### Task 3.4: 更新文档
- [ ] 更新架构文档
- [ ] 更新 API 文档
- [ ] 清理旧的 spec 文档

---

## 阶段四：测试和优化

### Task 4.1: 单元测试
- [ ] 为 `UnifiedRenderPipeline` 添加单元测试
- [ ] 测试各阶段独立功能
- [ ] 测试脏标记传播

### Task 4.2: 集成测试
- [ ] 测试完整渲染流程
- [ ] 测试滚动场景
- [ ] 测试动画场景

### Task 4.3: 性能测试
- [ ] 对比新旧管线性能
- [ ] 优化热点代码
- [ ] 验证内存使用

### Task 4.4: 回归测试
- [ ] 运行现有测试套件
- [ ] 修复发现的问题
- [ ] 确保所有测试通过

---

## 完成标准

- [ ] 所有任务完成
- [ ] 所有测试通过
- [ ] 性能不低于旧实现
- [ ] 代码审查通过
- [ ] 文档更新完成
