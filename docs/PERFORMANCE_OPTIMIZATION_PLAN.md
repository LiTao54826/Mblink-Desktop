# MBink 性能优化与局部渲染技术方案

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **状态**: 技术方案  
> **优先级**: P0 - 核心性能优化

---

## 📋 目录

1. [项目现状分析](#项目现状分析)
2. [核心优化目标](#核心优化目标)
3. [局部渲染技术方案](#局部渲染技术方案)
4. [底层性能优化方案](#底层性能优化方案)
5. [实施计划](#实施计划)
6. [性能指标](#性能指标)

---

## 🎯 项目现状分析

### 当前架构优势

✅ **已完成的优化**:
- ID映射缓存 (GetElementById: 146ns/op)
- 事件监听器Hash Map (1-6M ops/sec)
- 内存泄漏修复 (99.99%改进)
- Timer队列优化 (O(1)删除)
- 脏标记系统 (Node级别)

✅ **现有性能基础设施**:
- `DirtyRegion` - 脏区域检测系统
- `RenderCache` - 渲染缓存 (LRU策略)
- `Layer` - 图层管理系统
- `ClipOptimizer` - 裁剪优化器
- `PerformanceMonitor` - 性能监控
- `FrameController` - 帧率控制 (60 FPS)

### 当前性能瓶颈

⚠️ **需要优化的问题**:

1. **全量渲染问题** (P0)
   - 每次DOM变更都触发全窗口重绘
   - `WindowDOMObserver::OnNodeAdded()` → `SetNeedsRepaint()` → 全量渲染
   - 即使只改变一个按钮文本，也重绘整个窗口

2. **渲染管线效率** (P1)
   - 每帧都重建渲染树 (`RenderTreeBuilder::BuildRenderTree`)
   - 没有渲染树缓存
   - 布局计算未使用增量更新

3. **缓存利用率低** (P1)
   - `RenderCache` 已实现但未充分使用
   - `Layer` 系统未与DOM集成
   - 脏区域检测未应用到实际渲染

4. **Preact集成效率** (P2)
   - Virtual DOM diff后触发大量DOM操作
   - 每个DOM操作都触发重绘标记
   - 批量更新未优化

---

## 🎯 核心优化目标

### 性能目标

| 指标 | 当前 | 目标 | 改进 |
|------|------|------|------|
| **首次渲染** | ~50ms | <30ms | 40%↑ |
| **增量更新** | ~50ms | <5ms | 90%↑ |
| **内存占用** | 未优化 | -30% | 30%↓ |
| **帧率稳定性** | 不稳定 | 稳定60FPS | 100%↑ |
| **渲染区域** | 100% | <10% | 90%↓ |

### 用户体验目标

- ✅ 按钮点击响应 <16ms (60 FPS)
- ✅ 文本输入无延迟
- ✅ 列表滚动流畅 (60 FPS)
- ✅ 动画平滑 (60 FPS)
- ✅ 大型应用 (1000+ DOM节点) 流畅运行

---

## 🚀 局部渲染技术方案

### 方案1: 智能脏区域系统 (推荐)

#### 核心思路

```
DOM变更 → 标记脏节点 → 计算脏区域 → 只渲染脏区域
```

#### 实现步骤

**Step 1: 增强脏标记系统**

```cpp
// core/dom/node.h
class Node {
private:
    bool is_dirty_ = false;              // 已有
    bool is_layout_dirty_ = false;       // 新增：布局脏标记
    bool is_paint_dirty_ = false;        // 新增：绘制脏标记
    SkRect dirty_rect_;                  // 新增：脏矩形区域
    
public:
    // 标记节点为脏（传播到父节点）
    void MarkDirty(DirtyType type = DirtyType::ALL);
    
    // 标记布局脏（需要重新布局）
    void MarkLayoutDirty();
    
    // 标记绘制脏（只需要重绘，不需要重新布局）
    void MarkPaintDirty();
    
    // 获取脏矩形区域
    SkRect GetDirtyRect() const;
    
    // 清除脏标记
    void ClearDirty();
};
```

**Step 2: 优化DOM观察者**

```cpp
// core/window/window.cpp
class WindowDOMObserver : public DOMObserver {
private:
    std::vector<Node*> dirty_nodes_;  // 收集脏节点
    bool batch_mode_ = false;         // 批量模式
    
public:
    void OnNodeAdded(Node* node, Node* parent) override {
        if (batch_mode_) {
            dirty_nodes_.push_back(parent);
        } else {
            parent->MarkDirty(DirtyType::LAYOUT);
            window_->SetNeedsRepaint();
        }
    }
    
    void OnAttributeChanged(Element* element, ...) override {
        // 智能判断：样式属性 vs 非样式属性
        if (IsStyleAttribute(name)) {
            if (AffectsLayout(name)) {
                element->MarkLayoutDirty();
            } else {
                element->MarkPaintDirty();
            }
        }
        window_->SetNeedsRepaint();
    }
    
    // 批量更新模式（用于Preact）
    void BeginBatch() { batch_mode_ = true; }
    void EndBatch() {
        batch_mode_ = false;
        for (auto node : dirty_nodes_) {
            node->MarkDirty();
        }
        dirty_nodes_.clear();
        window_->SetNeedsRepaint();
    }
};
```

**Step 3: 增量渲染管线**

```cpp
// core/window/window.cpp
void Window::RenderDocument() {
    if (!document_ || !surface_ || !needs_repaint_) {
        return;
    }
    
    SkCanvas* canvas = surface_->getCanvas();
    
    // 收集脏区域
    DirtyRegion dirty_region;
    CollectDirtyRegions(document_->GetBody(), dirty_region);
    
    if (dirty_region.IsEmpty()) {
        needs_repaint_ = false;
        return;
    }
    
    // 优化脏区域（合并相邻区域）
    dirty_region.Optimize();
    
    // 只渲染脏区域
    for (const auto& rect : dirty_region.GetRegions()) {
        canvas->save();
        canvas->clipRect(rect);
        
        // 清空脏区域
        canvas->clear(SK_ColorWHITE);
        
        // 只渲染与脏区域相交的节点
        RenderDirtyRegion(rect);
        
        canvas->restore();
    }
    
    // 清除脏标记
    ClearDirtyFlags(document_->GetBody());
    needs_repaint_ = false;
}

void Window::CollectDirtyRegions(Node* node, DirtyRegion& region) {
    if (!node) return;

    if (node->IsDirty()) {
        // 获取节点的渲染边界
        SkRect bounds = GetNodeBounds(node);
        region.AddRect(bounds);
    }

    // 递归子节点
    for (auto child : node->GetChildren()) {
        CollectDirtyRegions(child.get(), region);
    }
}
```

**Step 4: 渲染树缓存**

```cpp
// core/window/window.h
class Window {
private:
    std::shared_ptr<RenderObject> cached_render_tree_;  // 缓存的渲染树
    bool render_tree_dirty_ = true;                     // 渲染树是否需要重建

public:
    void RenderDocument() {
        // 只在必要时重建渲染树
        if (render_tree_dirty_ || !cached_render_tree_) {
            RenderTreeBuilder builder;
            cached_render_tree_ = builder.BuildRenderTree(document_->GetBody(), nullptr);
            render_tree_dirty_ = false;
        }

        // 增量布局更新
        UpdateDirtyLayout(cached_render_tree_.get());

        // 增量绘制
        PaintDirtyRegions(cached_render_tree_.get());
    }

    void InvalidateRenderTree() {
        render_tree_dirty_ = true;
    }
};
```

---

### 方案2: 图层合成系统

#### 核心思路

```
静态内容 → 缓存到图层 → 只重绘动态图层 → 合成到屏幕
```

#### 实现步骤

**Step 1: 自动图层提升**

```cpp
// core/render/layer_manager.h
class LayerManager {
public:
    // 自动判断是否需要提升为独立图层
    bool ShouldPromoteToLayer(RenderObject* obj) {
        // 1. 有CSS动画/过渡
        if (obj->HasAnimation()) return true;

        // 2. 有transform/opacity
        if (obj->HasTransform() || obj->GetOpacity() < 1.0f) return true;

        // 3. 固定定位
        if (obj->GetPosition() == Position::FIXED) return true;

        // 4. 频繁更新（启发式）
        if (obj->GetUpdateFrequency() > 10) return true;

        return false;
    }

    // 智能图层分配
    void UpdateLayers(RenderObject* root) {
        TraverseRenderTree(root, [this](RenderObject* obj) {
            if (ShouldPromoteToLayer(obj)) {
                CreateLayerForObject(obj);
            }
        });
    }
};
```

**Step 2: 图层缓存策略**

```cpp
// core/render/layer.cpp
void Layer::Paint(SkCanvas* canvas) {
    // 如果图层内容未改变，直接使用缓存
    if (surface_ && !needs_repaint_) {
        auto image = surface_->makeImageSnapshot();
        canvas->drawImage(image, bounds_.left(), bounds_.top());
        return;
    }

    // 重绘到图层表面
    if (!surface_) {
        CreateSurface();
    }

    auto layer_canvas = surface_->getCanvas();
    layer_canvas->clear(SK_ColorTRANSPARENT);

    // 绘制图层内容
    for (const auto& obj : render_objects_) {
        obj->Paint(layer_canvas);
    }

    // 绘制到主画布
    auto image = surface_->makeImageSnapshot();
    canvas->drawImage(image, bounds_.left(), bounds_.top());

    needs_repaint_ = false;
}
```

---

## ⚡ 底层性能优化方案

### 优化1: 渲染对象池

**问题**: 频繁创建/销毁RenderObject导致内存碎片

**方案**: 对象池复用

```cpp
// core/render/render_object_pool.h
template<typename T>
class RenderObjectPool {
public:
    std::shared_ptr<T> Acquire() {
        if (!free_list_.empty()) {
            auto obj = free_list_.back();
            free_list_.pop_back();
            obj->Reset();  // 重置状态
            return obj;
        }
        return std::make_shared<T>();
    }

    void Release(std::shared_ptr<T> obj) {
        if (free_list_.size() < MAX_POOL_SIZE) {
            free_list_.push_back(obj);
        }
    }

private:
    std::vector<std::shared_ptr<T>> free_list_;
    static constexpr size_t MAX_POOL_SIZE = 1000;
};
```

**预期效果**:
- 减少内存分配 50%
- 减少内存碎片 70%
- 提升创建速度 30%

---

### 优化2: 批量DOM操作

**问题**: Preact每次setState触发多次DOM操作，每次都重绘

**方案**: 批量更新API

```cpp
// core/dom/document.h
class Document {
public:
    // 开始批量更新
    void BeginBatch() {
        batch_mode_ = true;
        NotifyObservers([](DOMObserver* obs) {
            if (auto batch_obs = dynamic_cast<BatchObserver*>(obs)) {
                batch_obs->BeginBatch();
            }
        });
    }

    // 结束批量更新
    void EndBatch() {
        batch_mode_ = false;
        NotifyObservers([](DOMObserver* obs) {
            if (auto batch_obs = dynamic_cast<BatchObserver*>(obs)) {
                batch_obs->EndBatch();
            }
        });
    }

private:
    bool batch_mode_ = false;
};
```

**JavaScript绑定**:

```javascript
// js/runtime/dom_batch.js
window.__batchUpdate = function(callback) {
    document.beginBatch();
    try {
        callback();
    } finally {
        document.endBatch();
    }
};

// Preact集成
import { options } from 'preact';

// Hook into Preact's render cycle
const oldDiffed = options.diffed;
options.diffed = (vnode) => {
    if (oldDiffed) oldDiffed(vnode);

    // Preact diff完成后，批量应用DOM更新
    if (vnode.type === 'root') {
        window.__batchUpdate(() => {
            // DOM updates happen here
        });
    }
};
```

**预期效果**:
- 减少重绘次数 90%
- 提升Preact更新速度 10倍

---

### 优化3: 智能布局缓存

**问题**: 每次都重新计算整个布局树

**方案**: 增量布局更新

```cpp
// core/layout/layout_engine.h
class LayoutEngine {
public:
    void Layout(RenderObject* root, float width, float height) {
        if (!root) return;

        // 只重新布局脏节点
        if (root->IsLayoutDirty()) {
            ComputeLayout(root, width, height);
            root->ClearLayoutDirty();
        } else {
            // 递归检查子节点
            for (auto child : root->GetChildren()) {
                if (child->IsLayoutDirty()) {
                    Layout(child.get(), width, height);
                }
            }
        }
    }

private:
    // 缓存布局结果
    std::unordered_map<RenderObject*, LayoutInfo> layout_cache_;
};
```

**预期效果**:
- 减少布局计算 80%
- 提升布局速度 5倍

---

### 优化4: Skia绘制优化

**问题**: 重复绘制相同内容

**方案**: 绘制命令缓存

```cpp
// core/render/render_cache.h (已存在，需增强)
class DrawCommandCache {
public:
    void RecordCommands(const std::string& key,
                       std::function<void(SkCanvas*)> draw_func) {
        // 记录绘制命令到SkPicture
        SkPictureRecorder recorder;
        SkCanvas* canvas = recorder.beginRecording(bounds);
        draw_func(canvas);

        auto picture = recorder.finishRecordingAsPicture();
        cache_[key] = picture;
    }

    void PlaybackCommands(const std::string& key, SkCanvas* canvas) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            canvas->drawPicture(it->second);
        }
    }

private:
    std::unordered_map<std::string, sk_sp<SkPicture>> cache_;
};
```

**预期效果**:
- 减少Skia调用 60%
- 提升绘制速度 3倍

---

## 📊 实施计划

### Phase 1: 基础设施 (1周)

**优先级**: P0

- [ ] 增强Node脏标记系统 (layout_dirty, paint_dirty)
- [ ] 实现DirtyRegion收集逻辑
- [ ] 优化WindowDOMObserver (批量模式)
- [ ] 添加性能监控点

**验收标准**:
- 脏区域正确收集
- 批量更新API工作
- 性能监控数据准确

---

### Phase 2: 局部渲染 (1周)

**优先级**: P0

- [ ] 实现增量渲染管线
- [ ] 渲染树缓存机制
- [ ] 增量布局更新
- [ ] 脏区域优化算法

**验收标准**:
- 只渲染脏区域
- 渲染区域减少90%
- 帧率稳定60FPS

---

### Phase 3: 图层系统 (1周)

**优先级**: P1

- [ ] 自动图层提升逻辑
- [ ] 图层缓存策略
- [ ] 图层合成优化
- [ ] 内存管理优化

**验收标准**:
- 静态内容缓存到图层
- 动态内容独立图层
- 内存占用合理

---

### Phase 4: 深度优化 (1周)

**优先级**: P1

- [ ] 渲染对象池
- [ ] 绘制命令缓存
- [ ] Preact批量更新集成
- [ ] 性能基准测试

**验收标准**:
- 所有性能目标达成
- 基准测试通过
- 文档完善

---

## 📈 性能指标

### 基准测试场景

```cpp
// tests/benchmarks/benchmark_rendering.cpp

// 场景1: 单节点更新
TEST(RenderingBenchmark, SingleNodeUpdate) {
    // 1000个节点，只更新1个
    // 目标: <5ms
}

// 场景2: 批量更新
TEST(RenderingBenchmark, BatchUpdate) {
    // 更新100个节点
    // 目标: <20ms
}

// 场景3: 列表滚动
TEST(RenderingBenchmark, ListScroll) {
    // 1000项列表滚动
    // 目标: 稳定60FPS
}

// 场景4: 动画
TEST(RenderingBenchmark, Animation) {
    // 10个元素同时动画
    // 目标: 稳定60FPS
}
```

### 性能监控

```cpp
// core/render/performance_monitor.h (已存在，需增强)
class PerformanceMonitor {
public:
    struct FrameStats {
        float total_time;
        float layout_time;
        float paint_time;
        float composite_time;
        size_t dirty_region_count;
        float dirty_region_area_percent;
        size_t layer_count;
        size_t cache_hit_rate;
    };

    void RecordFrame(const FrameStats& stats);
    void PrintReport();
};
```

---

## 🎯 预期成果

### 性能提升

| 场景 | 优化前 | 优化后 | 提升 |
|------|--------|--------|------|
| 单节点更新 | 50ms | 3ms | **16倍** |
| 批量更新(100节点) | 200ms | 15ms | **13倍** |
| 列表滚动 | 30 FPS | 60 FPS | **2倍** |
| 动画流畅度 | 不稳定 | 稳定60FPS | **100%** |
| 内存占用 | 基准 | -30% | **30%↓** |

### 用户体验

- ✅ 即时响应 (<16ms)
- ✅ 流畅动画 (60 FPS)
- ✅ 大型应用支持 (10000+ 节点)
- ✅ 低内存占用
- ✅ 稳定性提升

---

## 📚 参考资料

### 浏览器渲染原理
- [Chromium Rendering Pipeline](https://www.chromium.org/developers/design-documents/gpu-accelerated-compositing-in-chrome/)
- [WebKit Rendering](https://webkit.org/blog/6161/webkit-rendering-pipeline/)
- [Firefox Quantum](https://hacks.mozilla.org/2017/08/inside-a-super-fast-css-engine-quantum-css-aka-stylo/)

### 性能优化最佳实践
- [React Performance Optimization](https://react.dev/learn/render-and-commit)
- [Skia Performance Tips](https://skia.org/docs/user/tips/)
- [GPU Compositing](https://www.html5rocks.com/en/tutorials/speed/layers/)

---

**维护者**: MBink Team
**最后更新**: 2025-11-14

