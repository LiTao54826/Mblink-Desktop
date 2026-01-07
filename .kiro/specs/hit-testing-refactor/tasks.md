# 命中测试系统重构 - 开发任务

## 概述

本计划实现 LightUI 统一的命中测试系统，采用**视口坐标缓存策略**（ViewportBounds）。

**核心设计**：
- 使用 `ViewportBounds` 缓存替代动态坐标累积
- 命中测试时间复杂度从 O(depth) 降低到 O(1)
- 支持所有定位类型和嵌套场景

---

## 阶段 1：基础架构搭建（3-4 天）

### 1.1 创建 ViewportBounds 结构体
- [x] 在 render_object.h 中定义结构体
- [x] 添加 Contains() 和 ToLocalCoordinates() 方法

### 1.2 在 RenderObject 中添加缓存
- [x] 添加 viewport_bounds_ 成员
- [x] 添加 Get/Invalidate 方法

### 1.3 实现 UpdateViewportBounds 基础方法
- [x] static/relative 定位计算
- [x] 累加祖先偏移，减去滚动偏移

### 1.4 实现 FindContainingBlock 方法
- [x] 查找最近的定位祖先

### 1.5 基础单元测试
- [x] 测试 ViewportBounds 方法
- [x] 测试缓存更新和失效

---

## 阶段 2：定位类型支持（3-4 天）

### 2.1 实现 absolute 定位计算
- [x] 相对于包含块计算视口坐标

### 2.2 实现 fixed 定位计算
- [x] 直接使用 layout 坐标，不受滚动影响

### 2.3 实现多重滚动偏移累积
- [x] IsScrollContainer() 判断
- [x] 累积滚动偏移，Fixed 打断累积

### 2.4 实现嵌套独立层坐标计算
- [x] 累加祖先独立层变换

### 2.5 定位类型单元测试
- [x] absolute/fixed/嵌套滚动/嵌套独立层测试

---

## 阶段 3：HitTestController 实现（4-5 天）

### 3.1 创建 HitTestController 类
- [x] HitTestRequest 结构体
- [x] HitTest() 主入口

### 3.2 实现独立合成层优先测试
- [x] 按 z-order 测试独立层

### 3.3 实现 z-order 遍历
- [x] 正 z-index → 自身 → 负 z-index

### 3.4 实现 HitTestLayer 方法
- [x] ViewportBounds 边界检查
- [x] pointer-events 检查

### 3.5 HitTestController 单元测试
- [x] 创建测试用例

---

## 阶段 4：CSS Transform 支持（2-3 天）

### 4.1 实现 ApplyTransformToBounds
- [x] 计算变换后的包围盒

### 4.2 实现变换后的边界检查
- [x] 使用 transformed_bounds 进行边界检查

### 4.3 实现 ToLocalCoordinates 坐标转换
- [x] 应用逆变换计算局部坐标

### 4.4 处理不可逆变换
- [x] transform_invertible 标志处理

### 4.5 Transform 单元测试
- [ ] 创建 Transform 测试用例

---

## 阶段 5：裁剪与边界检查（2-3 天）

### 5.1 实现 overflow:hidden 裁剪检查
- [x] IsClipped() 方法实现
### 5.2 实现 clip-path 形状裁剪
- [ ] 待实现
### 5.3 实现 border-radius 圆角裁剪
- [ ] 待实现
### 5.4 裁剪单元测试
- [ ] 创建裁剪测试用例

---

## 阶段 6：层叠上下文处理（2-3 天）

### 6.1 实现 CreatesStackingContext 判断
- [x] PaintLayer::IsStackingContext() 已存在
### 6.2 实现 z-order 列表排序
- [x] PaintLayer::UpdateZOrderLists() 已存在
### 6.3 处理独立层逃逸裁剪
- [x] HitTestController 中已处理
### 6.4 层叠上下文单元测试
- [ ] 待创建

---

## 阶段 7：DevTools 集成（2-3 天）

### 7.1 实现 HitTestResult 结构体
- [x] HitTestResultEx 已实现
### 7.2 添加 DevTools 元数据
- [x] FillDevToolsInfo() 已实现
### 7.3 实现 ExplainMiss 调试方法
- [x] ExplainMiss() 已实现
### 7.4 添加命中测试可视化（可选）
- [ ] 待实现

---

## 阶段 8：迁移与集成（2-3 天）

### 8.1 修改 MouseEventDispatcher 使用新系统
- [x] 集成 HitTestController
### 8.2 保留旧系统作为回退（功能开关）
- [x] USE_NEW_HIT_TEST_SYSTEM 开关
### 8.3 集成测试
- [x] 基础测试通过

---

## 阶段 9：性能优化（2-3 天）

### 9.1 实现缓存失效优化
### 9.2 实现滚动时的增量更新
### 9.3 实现动画帧的快速路径
### 9.4 性能基准测试（目标 < 100μs）

---

## 阶段 10：稳定化与文档（2-3 天）

### 10.1 属性测试覆盖
### 10.2 边界情况测试
### 10.3 代码审查
### 10.4 更新开发文档

---

**总计：约 25-35 天**