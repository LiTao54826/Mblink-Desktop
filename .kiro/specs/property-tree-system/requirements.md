# 需求文档

## 简介

本文档定义了参考 Chromium Blink 渲染引擎设计的完整属性树系统。该系统旨在实现高效的 GPU 增量渲染，通过将渲染属性（变换、裁剪、效果、滚动）组织成独立的树结构，支持直接属性更新和智能层合并，从而大幅降低 CPU/GPU 占用率。

## 术语表

- **属性树（Property Tree）**：将渲染属性组织成树形结构，每个节点代表一个属性值，子节点继承父节点的属性
- **变换树（Transform Tree）**：管理所有 2D/3D 变换（translate、rotate、scale、matrix）
- **裁剪树（Clip Tree）**：管理所有裁剪区域（overflow、clip-path）
- **效果树（Effect Tree）**：管理视觉效果（opacity、filter、blend-mode、mask）
- **滚动树（Scroll Tree）**：管理滚动容器和滚动偏移
- **属性树状态（PropertyTreeState）**：一个元素在四棵属性树中的节点引用组合
- **绘制块（PaintChunk）**：具有相同 PropertyTreeState 的连续绘制指令集合
- **层化（Layerization）**：将 PaintChunks 分配到合成层的过程
- **直接属性更新（Direct Property Update）**：不触发重新光栅化，只更新属性树节点值
- **光栅化失效（Raster Invalidation）**：标记需要重新光栅化的区域
- **几何映射器（Geometry Mapper）**：在不同属性树状态之间转换坐标和区域

## 需求

### 需求 1：变换树

**用户故事**：作为渲染引擎，我需要一个变换树来管理所有元素的变换属性，以便高效计算最终变换矩阵并支持直接更新。

#### 验收标准

1. WHEN 创建变换节点 THEN 系统 SHALL 存储 4x4 变换矩阵、变换原点、是否扁平化标志
2. WHEN 查询元素的最终变换 THEN 系统 SHALL 通过遍历祖先节点累乘矩阵计算结果
3. WHEN 变换属性变化且不影响层结构 THEN 系统 SHALL 支持直接更新节点值而不触发重新光栅化
4. WHEN 滚动发生 THEN 系统 SHALL 将滚动偏移作为变换节点的一部分处理
5. WHEN 存在 3D 变换 THEN 系统 SHALL 正确处理透视和 preserve-3d

### 需求 2：裁剪树

**用户故事**：作为渲染引擎，我需要一个裁剪树来管理所有裁剪区域，以便正确计算可见区域并优化绘制。

#### 验收标准

1. WHEN 创建裁剪节点 THEN 系统 SHALL 存储裁剪矩形、圆角半径、关联的变换节点
2. WHEN 存在 clip-path THEN 系统 SHALL 支持任意路径裁剪
3. WHEN 计算元素可见区域 THEN 系统 SHALL 遍历祖先裁剪节点求交集
4. WHEN 裁剪区域变化 THEN 系统 SHALL 正确计算需要重新光栅化的区域
5. WHEN overflow 属性为 hidden/scroll/auto THEN 系统 SHALL 创建对应的裁剪节点

### 需求 3：效果树

**用户故事**：作为渲染引擎，我需要一个效果树来管理视觉效果，以便支持 opacity 和 filter 的直接更新。

#### 验收标准

1. WHEN 创建效果节点 THEN 系统 SHALL 存储 opacity、filter、blend-mode、mask 等属性
2. WHEN opacity 变化且不影响层结构 THEN 系统 SHALL 支持直接更新而不触发重新光栅化
3. WHEN 存在 filter 效果 THEN 系统 SHALL 正确计算效果的输入输出边界
4. WHEN 存在 blend-mode THEN 系统 SHALL 创建隔离的合成层
5. WHEN 效果需要隔离 THEN 系统 SHALL 创建独立的渲染表面

### 需求 4：滚动树

**用户故事**：作为渲染引擎，我需要一个滚动树来管理滚动容器，以便支持高效的滚动处理和合成器线程滚动。

#### 验收标准

1. WHEN 创建滚动节点 THEN 系统 SHALL 存储滚动容器尺寸、内容尺寸、滚动偏移、滚动方向
2. WHEN 滚动发生 THEN 系统 SHALL 更新对应变换节点的滚动偏移
3. WHEN 滚动容器有 overflow: scroll/auto THEN 系统 SHALL 创建滚动节点
4. WHEN 查询滚动范围 THEN 系统 SHALL 返回可滚动的最大偏移
5. WHEN 滚动偏移变化 THEN 系统 SHALL 支持直接更新而不触发重新光栅化

### 需求 5：属性树状态

**用户故事**：作为渲染引擎，我需要用属性树状态来描述每个绘制块的渲染上下文，以便进行层化决策。

#### 验收标准

1. WHEN 创建属性树状态 THEN 系统 SHALL 包含四棵树的节点引用（transform、clip、effect、scroll）
2. WHEN 比较两个属性树状态 THEN 系统 SHALL 能判断是否可以合并到同一层
3. WHEN 属性树状态变化 THEN 系统 SHALL 能识别变化的属性类型
4. WHEN 遍历属性树 THEN 系统 SHALL 提供高效的祖先查找和公共祖先计算

### 需求 6：几何映射器

**用户故事**：作为渲染引擎，我需要几何映射器来在不同坐标空间之间转换坐标和区域。

#### 验收标准

1. WHEN 给定源和目标属性树状态 THEN 系统 SHALL 计算坐标转换矩阵
2. WHEN 转换矩形区域 THEN 系统 SHALL 正确处理变换和裁剪
3. WHEN 计算可见区域 THEN 系统 SHALL 考虑所有中间裁剪节点
4. WHEN 存在 3D 变换 THEN 系统 SHALL 正确处理透视投影
5. WHEN 缓存转换结果 THEN 系统 SHALL 在属性树变化时正确失效缓存

### 需求 7：层化算法

**用户故事**：作为渲染引擎，我需要智能的层化算法来决定哪些内容需要独立层，以平衡 GPU 内存和更新效率。

#### 验收标准

1. WHEN 元素有 will-change: transform/opacity THEN 系统 SHALL 创建独立合成层
2. WHEN 元素有活动的 transform/opacity 动画 THEN 系统 SHALL 创建独立合成层
3. WHEN 多个绘制块可以合并 THEN 系统 SHALL 合并到同一层以减少 GPU 内存
4. WHEN 绘制块与已有层重叠且无法合并 THEN 系统 SHALL 创建新层
5. WHEN 层化决策变化 THEN 系统 SHALL 触发完整更新而非增量更新

### 需求 8：直接属性更新

**用户故事**：作为渲染引擎，我需要支持直接属性更新，以便 transform/opacity 动画不触发重新光栅化。

#### 验收标准

1. WHEN transform 动画更新 THEN 系统 SHALL 只更新变换树节点，不重新光栅化
2. WHEN opacity 动画更新 THEN 系统 SHALL 只更新效果树节点，不重新光栅化
3. WHEN 滚动偏移变化 THEN 系统 SHALL 只更新滚动树节点，不重新光栅化
4. WHEN 直接更新后 THEN 系统 SHALL 只触发 GPU 重新合成
5. WHEN 属性变化影响层结构 THEN 系统 SHALL 回退到完整更新

### 需求 9：光栅化失效

**用户故事**：作为渲染引擎，我需要精确的光栅化失效机制，以便只重新光栅化真正变化的区域。

#### 验收标准

1. WHEN 绘制内容变化 THEN 系统 SHALL 计算最小失效区域
2. WHEN 绘制块出现或消失 THEN 系统 SHALL 失效对应区域
3. WHEN 绘制块移动 THEN 系统 SHALL 失效旧位置和新位置
4. WHEN 裁剪区域变化 THEN 系统 SHALL 只失效裁剪差异区域
5. WHEN 属性变化不影响光栅化 THEN 系统 SHALL 不触发失效

### 需求 10：性能目标

**用户故事**：作为用户，我需要流畅的动画和低资源占用，以获得良好的使用体验。

#### 验收标准

1. WHEN transform/opacity 动画运行 THEN 系统 SHALL 保持 GPU 占用低于 10%（当前 20-30%）
2. WHEN 静态页面显示 THEN 系统 SHALL 保持 CPU/GPU 接近 0%
3. WHEN 滚动页面 THEN 系统 SHALL 保持 60fps 且不触发重新光栅化
4. WHEN 窗口放大 THEN 系统 SHALL 保持 GPU 占用与窗口大小非线性增长
5. WHEN 多个动画同时运行 THEN 系统 SHALL 保持流畅且资源占用可控
