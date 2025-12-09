# 原生增量渲染重构计划

## 设计目标

将现有的 `Window` 类渲染逻辑重构为**原生支持增量渲染**，不引入任何中间层。

---

## 核心原则（开发约束）

### ✅ 必须遵守

1. **单一渲染入口**: `Window::Render()` 是唯一的公开渲染方法
2. **透明决策**: 内部自动判断全量/增量，调用者无需关心
3. **原地重构**: 直接修改现有 `Window` 类代码，不创建新的抽象类
4. **删除冗余**: 删除 `RenderDocument()` 和 `RenderDocumentIncremental()`，合并为 `Render()`
5. **零配置**: EventLoop 只调用 `window->Render()`，无需任何额外设置

### ❌ 禁止行为

1. **禁止创建中间层**: 不得创建 RenderPipeline、RenderManager 等新类
2. **禁止两套系统并存**: 不得保留多个渲染入口让用户选择
3. **禁止手动调用**: 不得要求用户手动调用特定方法切换渲染模式
4. **禁止增加复杂度**: 重构后代码行数应该减少，而非增加

---

## 最终 API 设计

### Window 类公开接口

```cpp
class Window {
public:
    // === 渲染 ===
    void Render();              // 唯一渲染入口（自动决策全量/增量）
    void SetNeedsRepaint();     // 标记需要重绘
    bool NeedsRepaint() const;  // 检查是否需要重绘
    void SwapBuffers();         // 交换缓冲区
    
    // === 脏区域 ===
    void AddDirtyRect(const SkRect& rect);  // 添加脏区域
    void InvalidateRenderTree();             // 使渲染树失效（触发全量重建）
};
```

### Window 类私有方法

```cpp
private:
    void DoFullRender();         // 全量渲染（重建渲染树+完整布局+完整绘制）
    void DoIncrementalRender();  // 增量渲染（复用渲染树+局部布局+局部绘制）
    bool ShouldDoFullRender();   // 判断是否需要全量渲染
```

### EventLoop 调用方式

```cpp
void EventLoop::Render() {
    for (auto& window : windows) {
        if (window->NeedsRepaint()) {
            window->Render();
            window->SwapBuffers();
        }
    }
}
```

---

## 渲染决策逻辑

```
Window::Render() 调用流程：

1. 检查前置条件
   ├─ document_ == nullptr → 返回
   └─ surface_ == nullptr → 返回

2. 更新动画

3. 决策渲染模式
   ├─ ShouldDoFullRender() == true
   │   ├─ 首次渲染（cached_render_tree_ == nullptr）
   │   ├─ 渲染树失效（render_tree_valid_ == false）
   │   ├─ 窗口大小改变
   │   └─ → 调用 DoFullRender()
   │
   └─ ShouldDoFullRender() == false
       └─ → 调用 DoIncrementalRender()

4. 刷新 GPU 上下文

5. 清除 needs_repaint_ 标记
```

---

## 全量渲染流程 (DoFullRender)

```
1. 保存滚动位置
2. 重建渲染树（RenderTreeBuilder）
3. 构建布局树（LayoutEngine::BuildLayoutTree）
4. 计算布局（LayoutEngine::ComputeLayout）
5. 读取布局结果（LayoutEngine::GetLayoutInfo）
6. 清空画布
7. 绘制渲染树
8. 恢复滚动位置
9. 标记 render_tree_valid_ = true
```

---

## 增量渲染流程 (DoIncrementalRender)

```
1. 收集脏区域
   ├─ 使用预收集的 dirty_rects_（优先）
   └─ 或从 DOM 树遍历收集

2. 如果没有脏区域 → 返回

3. 增量布局
   ├─ 标记脏 DOM 对应的 RenderObject 需要布局
   └─ 只重新布局脏子树

4. 局部绘制
   └─ 对每个脏区域：
       ├─ 保存画布状态
       ├─ 清除脏区域（用背景色）
       ├─ 裁剪到脏区域
       ├─ 绘制渲染树
       └─ 恢复画布状态

5. 清除 DOM 脏标记
6. 清除 dirty_rects_
```

---

## 开发阶段

| Phase | 任务 | 描述 | 状态 |
|-------|------|------|------|
| **1** | 创建 `Render()` 方法 | 整合 RenderDocument + RenderDocumentIncremental | ✅ |
| **2** | 实现 `ShouldDoFullRender()` | 判断全量/增量的逻辑（已内置） | ✅ |
| **3** | 重构为私有方法 | DoFullRender/DoIncrementalRender（已内联） | ✅ |
| **4** | 删除旧方法 | 删除公开的 RenderDocument/RenderDocumentIncremental | ✅ |
| **5** | 更新 EventLoop | 使用新的 Render() 接口 | ✅ |
| **6** | 测试验证 | 运行所有测试，验证功能 | ✅ |

---

## 验收标准

1. ✅ `Window` 类只有一个公开渲染方法 `Render()`
2. ✅ `EventLoop::Render()` 只调用 `window->Render()`
3. ✅ 增量渲染正常工作（DOM 修改后只重绘脏区域）
4. ✅ 全量渲染正常工作（首次渲染/窗口大小改变）
5. ✅ 性能不低于现有实现
6. ✅ 所有现有测试通过
7. ✅ 代码行数减少（删除冗余代码）

---

## 现有代码分析

### 需要删除的公开方法

- `Window::RenderDocument()` - 旧版全量渲染
- `Window::RenderDocumentIncremental()` - 旧版增量渲染

### 需要保留并重构的内部逻辑

- 渲染树构建（来自 RenderDocumentIncremental）
- 增量布局（来自 RenderDocumentIncremental）
- 脏区域收集（来自 RenderDocumentIncremental）
- 滚动位置保存/恢复（来自 RenderDocumentIncremental）

### 不需要修改的部分

- `SetNeedsRepaint()` / `NeedsRepaint()`
- `AddDirtyRect()` / `ClearDirtyRects()`
- `InvalidateRenderTree()`
- `SwapBuffers()`

