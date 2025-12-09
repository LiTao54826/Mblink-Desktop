# 增量布局与增量渲染修正计划

> **版本**: 1.0  
> **创建日期**: 2025-12-09  
> **目标**: 在现有架构基础上，真正实现 **正确且高效** 的增量布局 + 增量绘制

---

## 1. 背景与现状分析

### 1.1 当前问题

当前分支的增量渲染系统存在以下问题：

| 现象 | 根本原因 |
|------|----------|
| 全量渲染正常，增量渲染后 UI 混乱 | `DirtyRegionCollector::GetNodeLayout()` 返回假数据（硬编码 100x100 @ 0,0） |
| 滚动后出现残影、控件消失 | 滚动仅标记 `RenderObject::needs_paint_`，未接入 DOM 脏区域系统 |
| 按钮/输入框看起来"不工作" | 视觉帧与命中测试数据不同步，用户看到旧画面但点击命中新位置 |
| 伪类状态（hover/focus）切换异常 | 局部裁剪区域计算错误，导致状态变化后只重绘错误区域 |

### 1.2 现有架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         DOM 层                                   │
│  Node::MarkDirty(DirtyType) ──► 向父级传播脏标记                  │
│  Node::dirty_rect_ ──► 记录节点脏区域                            │
│  Element::SetAttribute() ──► 触发 MarkDirty                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      渲染层 (Render Tree)                        │
│  RenderObject::needs_layout_ / needs_paint_                      │
│  RenderObject::GetBoundingRect() ──► 获取绝对坐标边界框           │
│  cached_render_tree_ ──► 缓存的渲染树                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Window 渲染管线                               │
│  全量路径: 重建渲染树 → 全量布局 → clear → Paint 整树             │
│  增量路径: 收集脏区域 → 同步脏标记 → 增量布局 → 裁剪重绘          │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 关键问题代码位置

| 问题 | 文件 | 行号 | 说明 |
|------|------|------|------|
| 假布局信息 | `core/render/dirty_region_collector.cpp` | 118-165 | `GetNodeLayout()` 未使用真实布局 |
| 滚动未接入脏系统 | `core/render/render_object.cpp` | 187-202 | `ScrollTo()` 仅调用 `MarkNeedsPaint()` |
| 增量渲染入口 | `core/window/window.cpp` | 1245-1318 | 增量渲染分支逻辑 |
| DOM 脏标记同步 | `core/window/window.cpp` | 1330-1465 | `MarkRenderObjectsDirty()` |

---

## 2. 设计目标与原则

### 2.1 设计目标

1. **正确性优先**
   - 屏幕显示必须与内部渲染树状态一致
   - 不允许视觉帧与命中测试/布局数据不同步

2. **渐进式增量**
   - 最差情况下能安全退化为稳定的全量重绘
   - 逐步细化脏区域，让局部重绘生效

3. **高效增量布局**
   - 利用 `NativeLayoutEngine` 的增量接口
   - 减少不必要的全树重排与全屏重绘

4. **可观测 & 易调试**
   - 明确的开关、统计和调试日志
   - 能快速判断当前帧的渲染模式

### 2.2 设计原则

- **单一真实信息源**: 布局信息只信任 `RenderObject::GetBoundingRect()`
- **分层职责清晰**: DOM 负责语义脏，RenderObject 负责精确边界
- **保守退化**: 不确定时向安全方向退化（全屏重绘）

---

## 3. 渲染模式定义

| 模式 | 触发条件 | 行为 | 性能 |
|------|----------|------|------|
| **A: 全量重建** | `!render_tree_valid_` 或 `!cached_render_tree_` | 重建渲染树 → 全量布局 → 全屏重绘 | 最慢 |
| **B: 缓存全屏** | `force_full_repaint_` 或 `!enable_incremental_render_` | 复用渲染树 → 全屏重绘 | 中等 |
| **C: 局部重绘** | 有有效脏区域 | 复用渲染树 → 增量布局 → 裁剪重绘 | 最快 |

---

## 4. 分阶段实施计划

### 阶段 0: 安全开关 & 调试可观测性

**目标**: 确保开发过程中可随时回退到稳定全量重绘

#### 任务清单

- [ ] **0.1** 在 `Window` 类中增加配置开关
  - 文件: `core/window/window.h`
  - 新增字段:
    ```cpp
    bool enable_incremental_render_ = true;   // 是否启用增量渲染
    bool force_full_repaint_ = false;         // 强制全屏重绘（调试用）
    ```

- [ ] **0.2** 修改 `Window::Render()` 增加模式选择逻辑
  - 文件: `core/window/window.cpp`
  - 位置: 增量分支开头（约 1246 行）
  - 逻辑:
    ```cpp
    if (force_full_repaint_ || !enable_incremental_render_) {
        // 模式 B: 不做局部裁剪，直接 clear + Paint
        canvas->clear(clear_color);
        cached_render_tree_->Paint(canvas);
        // 跳转到公共清理逻辑
    }
    ```

- [ ] **0.3** 增加调试统计输出
  - 记录: 当前帧模式 (A/B/C)、脏矩形数量、增量布局是否执行

---

### 阶段 1: 修复 DirtyRegionCollector

**目标**: 确保 DirtyRegionCollector 不再产生错误裁剪

#### 任务清单

- [ ] **1.1** 重写 `GetNodeLayout()` 方法
  - 文件: `core/render/dirty_region_collector.cpp`
  - 位置: 118-165 行
  - 新实现:
    ```cpp
    bool DirtyRegionCollector::GetNodeLayout(Node* node,
                                             float& x, float& y,
                                             float& width, float& height) const {
        if (!node) return false;
        if (node->GetNodeType() != NodeType::ELEMENT_NODE) return false;

        auto ro = node->GetRenderObject();
        if (!ro) return false;

        SkRect bounds = ro->GetBoundingRect();
        if (bounds.isEmpty()) return false;

        x      = bounds.left();
        y      = bounds.top();
        width  = bounds.width();
        height = bounds.height();
        return true;
    }
    ```

- [ ] **1.2** 删除旧的 HTML 属性解析逻辑
  - 移除 `GetAttribute("width")` / `GetAttribute("height")` 相关代码

- [ ] **1.3** 验证视口裁剪逻辑
  - 确认 `ComputeNodeBounds()` 中视口裁剪仍然生效

#### 验证场景

- [ ] 简单布局，只改一个元素样式，检查脏区域是否正确
- [ ] 禁用 `dirty_rects_` 直接写入，纯依赖 `CollectFromDOM`

---

### 阶段 2: 渲染树级脏区域收集 & 滚动接入

**目标**: 让所有 `NeedsPaint()` 的更新都能参与增量绘制

#### 任务清单

- [ ] **2.1** 新增渲染树脏区域收集函数
  - 文件: `core/window/window.cpp` / `window.h`
  - 新增方法:
    ```cpp
    void Window::CollectDirtyRectsFromRenderTree(RenderObject* root) {
        if (!root) return;

        std::function<void(RenderObject*)> dfs = [&](RenderObject* obj) {
            if (!obj) return;
            if (obj->NeedsPaint()) {
                SkRect rect = obj->GetBoundingRect();
                if (!rect.isEmpty()) {
                    AddDirtyRect(rect);
                }
            }
            for (const auto& child : obj->GetChildren()) {
                dfs(child.get());
            }
        };
        dfs(root);
    }
    ```

- [ ] **2.2** 在 `Window::Render()` 中调用渲染树收集
  - 位置: 在决定 `combined_dirty_rects` 之前
  - 顺序:
    1. 从渲染树收集 → 合并到 `dirty_rects_`
    2. 若 `dirty_rects_` 非空 → 作为 `combined_dirty_rects`
    3. 否则调用 `DirtyRegionCollector`

- [ ] **2.3** 滚动路径接入脏区域系统
  - 文件: `core/render/render_object.cpp`
  - 方案 A (简单): 在 `ScrollTo()` 中通知 Window 添加脏区域
  - 方案 B (依赖 2.1): 保持 `MarkNeedsPaint()`，依赖渲染树收集

- [ ] **2.4** 审查所有 `MarkNeedsPaint()` 调用点
  - 确保都能被渲染树级收集覆盖

#### 验证场景

- [ ] 滚动容器多次滚动，无残影
- [ ] 动画播放过程中无视觉错误

---

### 阶段 3: 增量布局优化

**目标**: 真正发挥 Taffy/NativeLayoutEngine 的增量能力

#### 任务清单

- [ ] **3.1** 梳理 DOM → RenderObject 脏同步
  - 文件: `core/window/window.cpp` 中 `MarkRenderObjectsDirty()`
  - 确认布局相关属性变化触发 `DirtyType::LAYOUT`

- [ ] **3.2** 审查观察者回调
  - `OnStyleChanged` / `OnAttributeChanged` / `OnTextChanged`
  - 确保会导致重排的变化标记为 LAYOUT

- [ ] **3.3** 优化 `LayoutDirtySubtree()` 调用范围
  - 当前: 从根开始递归检查
  - 优化: 从包含脏节点的最小公共祖先开始

- [ ] **3.4** 验证 `NativeLayoutEngine::ComputeIncrementalLayout()`
  - 确认增量布局真正只计算脏子树
  - 确认调用前 `MarkNeedsLayout` 正确设置

#### 性能观测

- [ ] 增加统计: `LayoutDirtySubtree` 耗时、脏节点数量

---

### 阶段 3.5: 修复脏标记清理遗漏

**目标**: 确保渲染后 DOM 和 RenderObject 的脏标记都被正确清理

#### 问题发现

当前 `ClearDirtyFlags(body.get())` 只清除 DOM 节点的脏标记，**没有清除 RenderObject 的 `needs_paint_` / `needs_layout_`**。

这会导致：
- 下一帧渲染时，`CollectDirtyRectsFromRenderTree` 会重复收集已绘制的脏区域
- RenderObject 永远处于 `NeedsPaint()` 状态

#### 任务清单

- [ ] **3.5.1** 修改 `Window::ClearDirtyFlags()` 或新增函数
  - 在清除 DOM 脏标记时，同时清除对应 RenderObject 的脏标记
  - 方案 A: 在 `ClearDirtyFlags` 中通过 `node->GetRenderObject()` 获取并清除
  - 方案 B: 新增 `ClearRenderObjectDirtyFlags(RenderObject* root)` 遍历渲染树清除

- [ ] **3.5.2** 确保全量渲染路径也正确清除脏标记
  - 全量渲染后应清除所有 DOM 和 RenderObject 的脏标记

---

### 阶段 4: 测试与回归保障

**目标**: 建立基础测试场景，防止后续修改破坏正确性

#### 手工测试场景

| 场景 | 检查点 |
|------|--------|
| 静态页面 + 样式修改 | 颜色正确变化，无残影 |
| 文本内容修改 | 文本重排正确，无新旧混杂 |
| 滚动容器 | 滚动无残影，按钮不消失 |
| 伪类状态切换 | hover/focus 切换干净 |

#### 自动化测试

- [ ] **4.1** 对比测试工具
  - 同一状态下: 禁用增量 vs 启用增量
  - 比较 framebuffer hash，确保一致

- [ ] **4.2** 回归用例
  - 滚动、样式变化、文本变化关键路径

---

## 5. 风险与回退策略

### 5.1 已识别风险

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 复杂场景边界 case | transform/clip/overflow 组合下裁剪错误 | 暂时标记为强制全屏重绘 |
| 状态不同步 | 某路径忘记更新脏标记 | 核心修改点封装统一标记函数 |
| 性能退化 | 脏区域过多导致比全量重绘更慢 | 阈值控制，自动退化为全屏 |

### 5.2 回退策略

随时可通过配置回退：

```cpp
// 方式 1: 关闭增量渲染
window->SetEnableIncrementalRender(false);

// 方式 2: 强制全屏重绘
window->SetForceFullRepaint(true);

// 方式 3: 编译时禁用
#define MBINK_DISABLE_INCREMENTAL_RENDER 1
```

---

## 6. 实施顺序与里程碑

```
Week 1: 阶段 0 + 阶段 1
        ├── 安全开关实现
        ├── GetNodeLayout 修复
        └── 基础验证通过

Week 2: 阶段 2
        ├── 渲染树脏区域收集
        ├── 滚动路径接入
        └── 滚动验证通过

Week 3: 阶段 3
        ├── 增量布局优化
        ├── 性能统计
        └── 性能基准测试

Week 4: 阶段 4
        ├── 测试场景完善
        ├── 回归测试建立
        └── 文档更新
```

---

## 7. 相关文件索引

| 文件 | 主要内容 |
|------|----------|
| `core/window/window.h` | Window 类定义，新增开关字段 |
| `core/window/window.cpp` | 渲染管线主逻辑，`Render()`、`MarkRenderObjectsDirty()` |
| `core/render/dirty_region_collector.cpp` | 脏区域收集器，`GetNodeLayout()` 修复点 |
| `core/render/render_object.cpp` | 渲染对象，`ScrollTo()`、`GetBoundingRect()` |
| `core/dom/node.cpp` | DOM 节点，`MarkDirty()` 脏标记传播 |
| `core/dom/element.cpp` | 元素节点，属性变化触发脏标记 |
| `core/event/event_loop.cpp` | 事件处理，滚轮事件入口 |

---

## 附录 A: 代码修改速查

### A.1 阶段 1 核心修改

```cpp
// core/render/dirty_region_collector.cpp
// 替换 GetNodeLayout() 实现

bool DirtyRegionCollector::GetNodeLayout(Node* node,
                                         float& x, float& y,
                                         float& width, float& height) const {
    if (!node) return false;
    if (node->GetNodeType() != NodeType::ELEMENT_NODE) return false;

    auto ro = node->GetRenderObject();
    if (!ro) return false;

    SkRect bounds = ro->GetBoundingRect();
    if (bounds.isEmpty()) return false;

    x      = bounds.left();
    y      = bounds.top();
    width  = bounds.width();
    height = bounds.height();
    return true;
}
```

### A.2 阶段 2 核心修改

```cpp
// core/window/window.h
// 新增方法声明
void CollectDirtyRectsFromRenderTree(RenderObject* root);

// core/window/window.cpp
// 新增方法实现
void Window::CollectDirtyRectsFromRenderTree(RenderObject* root) {
    if (!root) return;

    std::function<void(RenderObject*)> dfs = [&](RenderObject* obj) {
        if (!obj) return;
        if (obj->NeedsPaint()) {
            SkRect rect = obj->GetBoundingRect();
            if (!rect.isEmpty()) {
                AddDirtyRect(rect);
            }
        }
        for (const auto& child : obj->GetChildren()) {
            dfs(child.get());
        }
    };
    dfs(root);
}
```

