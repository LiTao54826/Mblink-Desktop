# MBink 性能优化快速参考

> **快速查阅版** - 5分钟了解核心优化方案

---

## 🎯 一句话总结

**从全量渲染改为局部渲染，性能提升10-100倍**

---

## 📊 核心问题

### 当前状态
```
改变1个按钮 → 重绘1000个节点 → 50ms延迟 → 无法60FPS
```

### 优化后
```
改变1个按钮 → 只重绘1个节点 → 3ms延迟 → 稳定60FPS
```

---

## 🚀 三大核心优化

### 1️⃣ 智能脏标记 (最重要)

**代码位置**: `core/dom/node.h`

```cpp
class Node {
    bool is_layout_dirty_;  // 需要重新布局
    bool is_paint_dirty_;   // 需要重新绘制
    SkRect dirty_rect_;     // 脏矩形区域
    
    void MarkDirty(DirtyType type);
};
```

**关键逻辑**:
```cpp
// 智能判断
if (IsLayoutAttribute(name)) {
    MarkDirty(LAYOUT);  // width, height → 重新布局
} else {
    MarkDirty(PAINT);   // color → 只重绘
}
```

---

### 2️⃣ 渲染树缓存 (性能关键)

**代码位置**: `core/window/window.h`

```cpp
class Window {
    std::shared_ptr<RenderObject> cached_render_tree_;
    
    void RenderDocument() {
        if (render_tree_dirty_) {
            cached_render_tree_ = BuildRenderTree();  // 重建
        } else {
            UpdateDirtyLayout(cached_render_tree_);   // 复用
            PaintDirtyRegions(cached_render_tree_);
        }
    }
};
```

**效果**: 避免90%的渲染树重建

---

### 3️⃣ 批量更新 (Preact集成)

**代码位置**: `core/dom/document.h`

```cpp
class Document {
    void BeginBatch();  // 开始批量
    void EndBatch();    // 结束批量，一次性渲染
};
```

**JavaScript集成**:
```javascript
// Preact Hook
options.__c = (vnode) => {
    document.beginBatch();
    // ... DOM更新 ...
    document.endBatch();  // 只触发一次渲染
};
```

**效果**: 100次setState → 1次渲染 (100倍提升)

---

## 📈 性能对比

| 场景 | 优化前 | 优化后 | 提升 |
|------|--------|--------|------|
| 单节点更新 | 50ms | 3ms | **16倍** |
| 批量更新 | 5000ms | 50ms | **100倍** |
| 列表滚动 | 30 FPS | 60 FPS | **2倍** |

---

## 🔧 实施步骤

### Week 1: 脏标记
```bash
# 1. 增强Node类
core/dom/node.h         # 添加 layout_dirty_, paint_dirty_
core/dom/node.cpp       # 实现 MarkDirty()

# 2. 智能标记
core/dom/element.cpp    # SetAttribute() 智能判断

# 3. 脏区域收集
core/render/dirty_region_collector.h  # 新建
core/render/dirty_region_collector.cpp
```

### Week 2: 增量渲染
```bash
# 1. 渲染树缓存
core/window/window.h    # 添加 cached_render_tree_
core/window/window.cpp  # RenderDocumentIncremental()

# 2. 批量更新
core/dom/document.h     # BeginBatch(), EndBatch()
js/runtime/preact_batch.js  # Preact集成
```

### Week 3: 图层系统
```bash
# 1. 图层提升
core/render/layer_promoter.h   # 新建
core/render/layer_promoter.cpp

# 2. 图层缓存
core/render/layer.cpp   # 增强缓存逻辑
```

---

## ✅ 验收标准

### 性能指标
- [ ] 单节点更新 <5ms
- [ ] 批量更新 <50ms
- [ ] 稳定60FPS
- [ ] 渲染区域 <10%

### 测试覆盖
- [ ] 脏标记测试通过
- [ ] 增量渲染测试通过
- [ ] 批量更新测试通过
- [ ] 基准测试达标

---

## 📚 详细文档

1. **PERFORMANCE_OPTIMIZATION_SUMMARY.md** - 执行摘要
2. **PERFORMANCE_OPTIMIZATION_PLAN.md** - 完整技术方案
3. **PARTIAL_RENDERING_IMPLEMENTATION_GUIDE.md** - 实施指南
4. **RENDERING_PIPELINE_ARCHITECTURE.md** - 架构设计

---

## 🎯 关键代码片段

### 脏区域收集
```cpp
void Window::RenderDocumentIncremental() {
    // 1. 收集脏区域
    DirtyRegion dirty_region;
    dirty_collector_->CollectFromDOM(document_->GetBody(), dirty_region);
    
    // 2. 优化脏区域
    dirty_region.Optimize();
    
    // 3. 只渲染脏区域
    for (const auto& rect : dirty_region.GetRegions()) {
        canvas->clipRect(rect);
        RenderRegion(rect);
    }
}
```

### 增量布局
```cpp
void Window::UpdateDirtyLayout(RenderObject* obj) {
    if (obj->IsLayoutDirty()) {
        obj->Layout(width, height);  // 只布局脏节点
        obj->ClearLayoutDirty();
    }
    
    for (auto& child : obj->GetChildren()) {
        UpdateDirtyLayout(child.get());  // 递归
    }
}
```

### 批量更新
```cpp
void Document::EndBatch() {
    batch_mode_ = false;
    
    // 批量处理所有脏节点
    for (auto node : batched_dirty_nodes_) {
        node->MarkDirty();
    }
    
    // 触发一次渲染
    NotifyObservers();
}
```

---

## 🔍 调试技巧

### 性能监控
```cpp
// 添加性能日志
auto start = std::chrono::high_resolution_clock::now();
RenderDocument();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Render time: " << duration.count() << " μs" << std::endl;
```

### 脏区域可视化
```cpp
// 绘制脏区域边界（调试用）
void Window::DebugDrawDirtyRegions(const DirtyRegion& region) {
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2);
    
    for (const auto& rect : region.GetRegions()) {
        canvas->drawRect(rect, paint);
    }
}
```

---

## ⚠️ 常见陷阱

### 1. 忘记清除脏标记
```cpp
// ❌ 错误
void RenderDocument() {
    RenderDirtyRegions();
    // 忘记清除脏标记 → 每帧都重绘
}

// ✅ 正确
void RenderDocument() {
    RenderDirtyRegions();
    ClearDirtyFlags();  // 必须清除
}
```

### 2. 脏区域计算错误
```cpp
// ❌ 错误 - 使用局部坐标
SkRect bounds = SkRect::MakeXYWH(0, 0, width, height);

// ✅ 正确 - 使用屏幕坐标
SkRect bounds = ComputeScreenBounds(node);
```

### 3. 批量更新未生效
```cpp
// ❌ 错误 - 忘记调用endBatch
document.beginBatch();
// ... DOM操作 ...
// 忘记调用endBatch → 不会渲染

// ✅ 正确 - 使用RAII
class BatchScope {
    ~BatchScope() { document->EndBatch(); }
};
```

---

## 🎉 预期成果

### 性能提升
- ✅ 单节点更新: **16倍**
- ✅ 批量更新: **100倍**
- ✅ 帧率: **2倍**
- ✅ 内存: **-30%**

### 用户体验
- ✅ 即时响应 (<16ms)
- ✅ 流畅动画 (60 FPS)
- ✅ 大型应用支持 (10000+ 节点)

---

## 📞 获取帮助

- 📖 查看详细文档: `docs/PERFORMANCE_OPTIMIZATION_PLAN.md`
- 🔍 查看实施指南: `docs/PARTIAL_RENDERING_IMPLEMENTATION_GUIDE.md`
- 🏗️ 查看架构设计: `docs/RENDERING_PIPELINE_ARCHITECTURE.md`

---

**开始优化吧！** 🚀

---

**维护者**: MBink Team  
**最后更新**: 2025-11-14

