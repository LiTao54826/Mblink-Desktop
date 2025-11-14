# MBink 局部渲染实施指南

> **版本**: 1.0  
> **日期**: 2025-11-14  
> **目标**: 实现高性能局部渲染系统

---

## 📋 实施概览

本指南提供了在MBink中实现局部渲染的详细步骤，严格遵循项目开发规范。

### 核心原则

1. ✅ **遵循PROJECT_STANDARDS.md** - 所有代码符合规范
2. ✅ **模块边界清晰** - 不违反依赖关系
3. ✅ **测试驱动开发** - 每个功能都有测试
4. ✅ **增量实施** - 分阶段实现，每阶段可验证
5. ✅ **性能可测量** - 每个优化都有基准测试

---

## 🎯 实施路线图

### Week 1: 脏标记系统增强

**目标**: 建立精确的脏标记基础设施

#### Task 1.1: 增强Node脏标记 (2天)

**文件**: `core/dom/node.h`, `core/dom/node.cpp`

```cpp
// core/dom/node.h
namespace lightui {

enum class DirtyType {
    NONE = 0,
    LAYOUT = 1 << 0,    // 需要重新布局
    PAINT = 1 << 1,     // 需要重新绘制
    STYLE = 1 << 2,     // 样式改变
    ALL = LAYOUT | PAINT | STYLE
};

class Node {
public:
    // 标记节点为脏
    void MarkDirty(DirtyType type = DirtyType::ALL);
    
    // 检查是否需要布局
    bool IsLayoutDirty() const { return (dirty_flags_ & DirtyType::LAYOUT) != 0; }
    
    // 检查是否需要绘制
    bool IsPaintDirty() const { return (dirty_flags_ & DirtyType::PAINT) != 0; }
    
    // 清除脏标记
    void ClearDirty(DirtyType type = DirtyType::ALL);
    
    // 获取脏矩形（在布局后计算）
    SkRect GetDirtyRect() const { return dirty_rect_; }
    void SetDirtyRect(const SkRect& rect) { dirty_rect_ = rect; }
    
protected:
    uint32_t dirty_flags_ = 0;
    SkRect dirty_rect_ = SkRect::MakeEmpty();
};

} // namespace lightui
```

**实现**:

```cpp
// core/dom/node.cpp
void Node::MarkDirty(DirtyType type) {
    dirty_flags_ |= static_cast<uint32_t>(type);
    
    // 向上传播脏标记
    if (auto parent = parent_.lock()) {
        parent->MarkDirty(type);
    }
    
    // 通知观察者
    NotifyObservers();
}

void Node::ClearDirty(DirtyType type) {
    dirty_flags_ &= ~static_cast<uint32_t>(type);
    
    if (type & DirtyType::PAINT) {
        dirty_rect_ = SkRect::MakeEmpty();
    }
}
```

**测试**:

```cpp
// tests/unit/test_dirty_marking.cpp
TEST(DirtyMarkingTest, MarkLayoutDirty) {
    auto node = std::make_shared<Element>("div");
    node->MarkDirty(DirtyType::LAYOUT);
    
    EXPECT_TRUE(node->IsLayoutDirty());
    EXPECT_FALSE(node->IsPaintDirty());
}

TEST(DirtyMarkingTest, PropagateToParent) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    parent->AppendChild(child);
    
    child->MarkDirty(DirtyType::PAINT);
    
    EXPECT_TRUE(parent->IsPaintDirty());
}
```

---

#### Task 1.2: 智能脏标记传播 (1天)

**目标**: 根据属性变化类型智能标记

```cpp
// core/dom/element.cpp
void Element::SetAttribute(const std::string& name, const std::string& value) {
    // 保存旧值
    std::string old_value = GetAttribute(name);
    
    // 设置新值
    attributes_[name] = value;
    
    // 智能脏标记
    if (IsLayoutAttribute(name)) {
        MarkDirty(DirtyType::LAYOUT);
    } else if (IsStyleAttribute(name)) {
        MarkDirty(DirtyType::PAINT);
    }
    
    // 通知观察者
    NotifyAttributeChanged(name, old_value, value);
}

bool Element::IsLayoutAttribute(const std::string& name) const {
    static const std::unordered_set<std::string> layout_attrs = {
        "width", "height", "padding", "margin", "border",
        "display", "position", "flex", "grid"
    };
    return layout_attrs.count(name) > 0;
}

bool Element::IsStyleAttribute(const std::string& name) const {
    static const std::unordered_set<std::string> style_attrs = {
        "color", "background", "opacity", "box-shadow",
        "border-color", "border-radius"
    };
    return style_attrs.count(name) > 0;
}
```

---

#### Task 1.3: 脏区域收集器 (2天)

**文件**: `core/render/dirty_region_collector.h`

```cpp
// core/render/dirty_region_collector.h
#pragma once

#include "core/render/dirty_region.h"
#include "core/dom/node.h"
#include "core/render/render_object.h"
#include <memory>

namespace lightui {

class DirtyRegionCollector {
public:
    DirtyRegionCollector() = default;
    
    // 从DOM树收集脏区域
    void CollectFromDOM(Node* root, DirtyRegion& region);
    
    // 从渲染树收集脏区域
    void CollectFromRenderTree(RenderObject* root, DirtyRegion& region);
    
    // 计算节点的屏幕边界
    SkRect ComputeNodeBounds(Node* node);
    
    // 计算渲染对象的屏幕边界
    SkRect ComputeRenderObjectBounds(RenderObject* obj);
    
private:
    void CollectFromDOMRecursive(Node* node, DirtyRegion& region);
    void CollectFromRenderTreeRecursive(RenderObject* obj, DirtyRegion& region);
};

} // namespace lightui
```

**实现**:

```cpp
// core/render/dirty_region_collector.cpp
void DirtyRegionCollector::CollectFromDOM(Node* root, DirtyRegion& region) {
    if (!root) return;
    CollectFromDOMRecursive(root, region);
}

void DirtyRegionCollector::CollectFromDOMRecursive(Node* node, DirtyRegion& region) {
    if (!node) return;
    
    // 如果节点是脏的，添加其边界
    if (node->IsPaintDirty() || node->IsLayoutDirty()) {
        SkRect bounds = ComputeNodeBounds(node);
        if (!bounds.isEmpty()) {
            region.AddRect(bounds);
        }
    }
    
    // 递归子节点
    for (auto& child : node->GetChildren()) {
        CollectFromDOMRecursive(child.get(), region);
    }
}

SkRect DirtyRegionCollector::ComputeNodeBounds(Node* node) {
    // 从渲染对象获取布局信息
    if (auto element = dynamic_cast<Element*>(node)) {
        // 查找对应的渲染对象
        // 这需要维护DOM节点到渲染对象的映射
        // 暂时返回节点的脏矩形
        return node->GetDirtyRect();
    }
    return SkRect::MakeEmpty();
}
```

---

### Week 2: 增量渲染管线

**目标**: 实现只渲染脏区域的渲染管线

#### Task 2.1: 渲染树缓存 (2天)

**文件**: `core/window/window.h`, `core/window/window.cpp`

```cpp
// core/window/window.h
class Window {
private:
    // 渲染树缓存
    std::shared_ptr<RenderObject> cached_render_tree_;
    bool render_tree_dirty_ = true;
    
    // 脏区域收集器
    std::unique_ptr<DirtyRegionCollector> dirty_collector_;
    
public:
    // 标记渲染树需要重建
    void InvalidateRenderTree() { render_tree_dirty_ = true; }
    
    // 增量渲染
    void RenderDocumentIncremental();
};
```

**实现**:

```cpp
// core/window/window.cpp
void Window::RenderDocumentIncremental() {
    if (!document_ || !surface_) return;
    
    SkCanvas* canvas = surface_->getCanvas();
    
    // Step 1: 重建渲染树（如果需要）
    if (render_tree_dirty_ || !cached_render_tree_) {
        RenderTreeBuilder builder;
        cached_render_tree_ = builder.BuildRenderTree(document_->GetBody(), nullptr);
        render_tree_dirty_ = false;
        
        // 首次渲染，全量绘制
        canvas->clear(SK_ColorWHITE);
        if (cached_render_tree_) {
            int width, height;
            SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);
            cached_render_tree_->Layout(width, height);
            cached_render_tree_->Paint(canvas);
        }
        needs_repaint_ = false;
        return;
    }
    
    // Step 2: 收集脏区域
    DirtyRegion dirty_region;
    if (!dirty_collector_) {
        dirty_collector_ = std::make_unique<DirtyRegionCollector>();
    }
    dirty_collector_->CollectFromDOM(document_->GetBody().get(), dirty_region);
    
    if (dirty_region.IsEmpty()) {
        needs_repaint_ = false;
        return;
    }
    
    // Step 3: 优化脏区域
    dirty_region.Optimize();
    
    // Step 4: 增量布局
    UpdateDirtyLayout(cached_render_tree_.get());
    
    // Step 5: 只渲染脏区域
    for (const auto& rect : dirty_region.GetRegions()) {
        canvas->save();
        canvas->clipRect(rect);
        
        // 清空脏区域
        canvas->clear(SK_ColorWHITE);
        
        // 渲染与脏区域相交的内容
        RenderRegion(cached_render_tree_.get(), rect, canvas);
        
        canvas->restore();
    }
    
    // Step 6: 清除脏标记
    ClearDirtyFlags(document_->GetBody().get());
    
    needs_repaint_ = false;
}

void Window::UpdateDirtyLayout(RenderObject* obj) {
    if (!obj) return;
    
    // 只重新布局脏节点
    if (obj->IsLayoutDirty()) {
        int width, height;
        SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);
        obj->Layout(width, height);
        obj->ClearLayoutDirty();
    }
    
    // 递归子节点
    for (auto& child : obj->GetChildren()) {
        UpdateDirtyLayout(child.get());
    }
}

void Window::RenderRegion(RenderObject* obj, const SkRect& region, SkCanvas* canvas) {
    if (!obj) return;
    
    // 获取对象边界
    SkRect bounds = obj->GetBounds();
    
    // 如果不相交，跳过
    if (!SkRect::Intersects(bounds, region)) {
        return;
    }
    
    // 如果需要绘制，绘制此对象
    if (obj->NeedsPaint()) {
        obj->Paint(canvas);
    }
    
    // 递归子对象
    for (auto& child : obj->GetChildren()) {
        RenderRegion(child.get(), region, canvas);
    }
}

void Window::ClearDirtyFlags(Node* node) {
    if (!node) return;
    
    node->ClearDirty();
    
    for (auto& child : node->GetChildren()) {
        ClearDirtyFlags(child.get());
    }
}
```

---

#### Task 2.2: 批量更新支持 (1天)

**目标**: 支持Preact批量更新

```cpp
// core/dom/document.h
class Document {
public:
    // 批量更新模式
    void BeginBatch();
    void EndBatch();
    bool IsInBatch() const { return batch_mode_; }
    
private:
    bool batch_mode_ = false;
    std::vector<Node*> batched_dirty_nodes_;
};
```

```cpp
// core/dom/document.cpp
void Document::BeginBatch() {
    batch_mode_ = true;
    batched_dirty_nodes_.clear();
    
    // 通知观察者
    for (auto observer : observers_) {
        if (auto batch_obs = dynamic_cast<BatchObserver*>(observer)) {
            batch_obs->OnBatchBegin();
        }
    }
}

void Document::EndBatch() {
    batch_mode_ = false;
    
    // 批量处理脏节点
    for (auto node : batched_dirty_nodes_) {
        // 标记为脏
        node->MarkDirty();
    }
    batched_dirty_nodes_.clear();
    
    // 通知观察者
    for (auto observer : observers_) {
        if (auto batch_obs = dynamic_cast<BatchObserver*>(observer)) {
            batch_obs->OnBatchEnd();
        }
    }
}
```

**JavaScript绑定**:

```cpp
// core/quickjs/dom_bindings.cpp
JSValue js_document_begin_batch(JSContext* ctx, JSValueConst this_val,
                                 int argc, JSValueConst* argv) {
    auto doc = GetDocument(ctx);
    if (doc) {
        doc->BeginBatch();
    }
    return JS_UNDEFINED;
}

JSValue js_document_end_batch(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto doc = GetDocument(ctx);
    if (doc) {
        doc->EndBatch();
    }
    return JS_UNDEFINED;
}

// 注册函数
JS_SetPropertyStr(ctx, document_obj, "beginBatch",
                  JS_NewCFunction(ctx, js_document_begin_batch, "beginBatch", 0));
JS_SetPropertyStr(ctx, document_obj, "endBatch",
                  JS_NewCFunction(ctx, js_document_end_batch, "endBatch", 0));
```

**Preact集成**:

```javascript
// js/runtime/preact_batch.js
import { options } from './preact.js';

// Hook into Preact's commit phase
const oldCommit = options.__c;
options.__c = (vnode, commitQueue) => {
    // 开始批量更新
    if (typeof document !== 'undefined' && document.beginBatch) {
        document.beginBatch();
    }
    
    // 调用原始commit
    if (oldCommit) oldCommit(vnode, commitQueue);
    
    // 结束批量更新
    if (typeof document !== 'undefined' && document.endBatch) {
        document.endBatch();
    }
};
```

---

### Week 3: 图层系统

**目标**: 实现智能图层缓存

#### Task 3.1: 图层提升策略 (2天)

```cpp
// core/render/layer_promoter.h
#pragma once

#include "core/render/render_object.h"
#include "core/render/layer.h"
#include <memory>

namespace lightui {

class LayerPromoter {
public:
    // 判断是否应该提升为图层
    bool ShouldPromote(RenderObject* obj);
    
    // 创建图层
    std::shared_ptr<Layer> CreateLayer(RenderObject* obj);
    
private:
    // 检查是否有动画
    bool HasAnimation(RenderObject* obj);
    
    // 检查是否有transform
    bool HasTransform(RenderObject* obj);
    
    // 检查更新频率
    int GetUpdateFrequency(RenderObject* obj);
};

} // namespace lightui
```

---

## 📊 性能验证

### 基准测试

```cpp
// tests/benchmarks/benchmark_partial_rendering.cpp
#include <gtest/gtest.h>
#include "core/window/window.h"
#include "core/dom/document.h"

class PartialRenderingBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        window = std::make_shared<Window>(800, 600, "Benchmark");
        document = std::make_shared<Document>();
        window->SetDocument(document);
    }
    
    std::shared_ptr<Window> window;
    std::shared_ptr<Document> document;
};

TEST_F(PartialRenderingBenchmark, SingleNodeUpdate) {
    // 创建1000个节点
    auto body = document->GetBody();
    for (int i = 0; i < 1000; i++) {
        auto div = document->CreateElement("div");
        div->SetAttribute("id", "node" + std::to_string(i));
        body->AppendChild(div);
    }
    
    // 首次渲染
    window->RenderDocument();
    
    // 更新单个节点
    auto start = std::chrono::high_resolution_clock::now();
    
    auto node = document->GetElementById("node500");
    node->SetAttribute("style", "color: red");
    window->RenderDocument();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Single node update: " << duration.count() << " μs" << std::endl;
    
    // 目标: <5ms (5000μs)
    EXPECT_LT(duration.count(), 5000);
}
```

---

## ✅ 验收标准

### Week 1完成标准
- [ ] 所有脏标记测试通过
- [ ] 脏区域正确收集
- [ ] 智能属性判断工作

### Week 2完成标准
- [ ] 增量渲染工作
- [ ] 批量更新API工作
- [ ] 性能提升10倍以上

### Week 3完成标准
- [ ] 图层系统工作
- [ ] 静态内容缓存
- [ ] 所有基准测试通过

---

**维护者**: MBink Team  
**最后更新**: 2025-11-14

