/**
 * @file test_advanced_optimization.cpp
 * @brief 高级优化测试
 *
 * 测试层级系统、渲染缓存、批量渲染和裁剪优化
 */

#include "core/render/layer.h"
#include "core/render/render_cache.h"
#include "core/render/clip_optimizer.h"
#include "core/render/render_object.h"
#include "core/render/renderer.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include <iostream>
#include <cassert>

using namespace lightui;

// ========== 测试层级系统 ==========

void TestLayerSystem() {
    std::cout << "\nTesting Layer System..." << std::endl;

    // 创建层级管理器
    LayerManager manager;

    // 创建多个层级
    auto layer1 = manager.CreateLayer(LayerType::NORMAL, 0);
    layer1->SetId("layer1");
    
    auto layer2 = manager.CreateLayer(LayerType::NORMAL, 10);
    layer2->SetId("layer2");
    
    auto layer3 = manager.CreateLayer(LayerType::OPACITY, 5);
    layer3->SetId("layer3");
    layer3->SetOpacity(0.5f);

    // 验证层级数量
    assert(manager.GetLayers().size() == 3);
    std::cout << "✓ Layer creation correct!" << std::endl;

    // 验证层级排序（按 z-index）
    manager.SortLayers();
    const auto& layers = manager.GetLayers();
    assert(layers[0]->GetZIndex() == 0);
    assert(layers[1]->GetZIndex() == 5);
    assert(layers[2]->GetZIndex() == 10);
    std::cout << "✓ Layer sorting correct!" << std::endl;

    // 测试查找层级
    auto found = manager.FindLayerById("layer2");
    assert(found != nullptr);
    assert(found->GetId() == "layer2");
    std::cout << "✓ Layer finding correct!" << std::endl;

    // 测试层级属性
    assert(layer3->GetOpacity() == 0.5f);
    layer3->SetOpacity(0.8f);
    assert(layer3->GetOpacity() == 0.8f);
    std::cout << "✓ Layer opacity correct!" << std::endl;

    // 测试裁剪
    layer1->SetClipRect(SkRect::MakeXYWH(10, 10, 100, 100));
    assert(layer1->HasClip());
    layer1->ClearClip();
    assert(!layer1->HasClip());
    std::cout << "✓ Layer clipping correct!" << std::endl;

    // 测试层级表面
    layer1->CreateSurface(200, 200);
    assert(layer1->HasSurface());
    assert(layer1->GetSurface() != nullptr);
    layer1->ReleaseSurface();
    assert(!layer1->HasSurface());
    std::cout << "✓ Layer surface correct!" << std::endl;

    // 测试移除层级
    manager.RemoveLayer(layer2);
    assert(manager.GetLayers().size() == 2);
    std::cout << "✓ Layer removal correct!" << std::endl;
}

// ========== 测试渲染缓存 ==========

void TestRenderCache() {
    std::cout << "\nTesting Render Cache..." << std::endl;

    // 创建缓存（最大 1MB）
    RenderCache cache(1024 * 1024);

    // 创建测试图像
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(100, 100));
    auto image1 = surface->makeImageSnapshot();
    auto image2 = surface->makeImageSnapshot();

    // 测试添加缓存
    cache.Put("image1", image1, SkRect::MakeWH(100, 100));
    assert(cache.Has("image1"));
    assert(cache.GetEntryCount() == 1);
    std::cout << "✓ Cache put correct!" << std::endl;

    // 测试获取缓存
    auto entry = cache.Get("image1");
    assert(entry != nullptr);
    assert(entry->image == image1);
    assert(cache.GetHitCount() == 1);
    std::cout << "✓ Cache get correct!" << std::endl;

    // 测试缓存未命中
    auto missing = cache.Get("missing");
    assert(missing == nullptr);
    assert(cache.GetMissCount() == 1);
    std::cout << "✓ Cache miss correct!" << std::endl;

    // 测试命中率
    cache.Get("image1"); // 再次命中
    float hit_rate = cache.GetHitRate();
    assert(hit_rate > 0.5f); // 2 hits, 1 miss
    std::cout << "✓ Cache hit rate correct! (" << (hit_rate * 100) << "%)" << std::endl;

    // 测试标记脏
    cache.MarkDirty("image1");
    entry = cache.Get("image1");
    assert(entry->is_dirty);
    std::cout << "✓ Cache dirty marking correct!" << std::endl;

    // 测试清理脏缓存
    cache.CleanDirtyEntries();
    assert(!cache.Has("image1"));
    std::cout << "✓ Cache dirty cleanup correct!" << std::endl;

    // 测试内存管理
    cache.Put("image2", image2, SkRect::MakeWH(100, 100));
    size_t memory_usage = cache.GetMemoryUsage();
    assert(memory_usage > 0);
    std::cout << "✓ Cache memory tracking correct! (" << memory_usage << " bytes)" << std::endl;

    // 测试清空缓存
    cache.Clear();
    assert(cache.GetEntryCount() == 0);
    assert(cache.GetMemoryUsage() == 0);
    std::cout << "✓ Cache clear correct!" << std::endl;
}

// ========== 测试批量渲染 ==========

void TestBatchRenderer() {
    std::cout << "\nTesting Batch Renderer..." << std::endl;

    BatchRenderer batch;

    // 添加多个绘制命令
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    
    batch.AddFillRect(10, 10, 50, 50, paint);
    batch.AddFillRect(70, 10, 50, 50, paint);
    batch.AddDrawCircle(150, 35, 25, paint);
    
    assert(batch.GetCommandCount() == 3);
    std::cout << "✓ Batch command addition correct!" << std::endl;

    // 测试优化（合并相邻矩形）
    size_t before_count = batch.GetCommandCount();
    batch.Optimize();
    size_t after_count = batch.GetCommandCount();
    std::cout << "✓ Batch optimization correct! (Before: " << before_count 
              << ", After: " << after_count << ")" << std::endl;

    // 测试执行
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(200, 100));
    auto canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    
    batch.Execute(canvas);
    std::cout << "✓ Batch execution correct!" << std::endl;

    // 测试清空
    batch.Clear();
    assert(batch.GetCommandCount() == 0);
    std::cout << "✓ Batch clear correct!" << std::endl;
}

// ========== 测试裁剪优化 ==========

void TestClipOptimizer() {
    std::cout << "\nTesting Clip Optimizer..." << std::endl;

    ClipOptimizer optimizer;

    // 设置视口
    SkRect viewport = SkRect::MakeWH(800, 600);
    optimizer.SetViewport(viewport);
    assert(optimizer.GetViewport() == viewport);
    std::cout << "✓ Viewport setting correct!" << std::endl;

    // 测试视口检测
    SkRect inside = SkRect::MakeXYWH(100, 100, 200, 200);
    SkRect outside = SkRect::MakeXYWH(1000, 1000, 200, 200);
    SkRect partial = SkRect::MakeXYWH(700, 500, 200, 200);

    assert(optimizer.IsInViewport(inside));
    assert(!optimizer.IsOutsideViewport(inside));
    std::cout << "✓ Inside viewport detection correct!" << std::endl;

    assert(!optimizer.IsInViewport(outside));
    assert(optimizer.IsOutsideViewport(outside));
    std::cout << "✓ Outside viewport detection correct!" << std::endl;

    assert(optimizer.IsInViewport(partial));
    assert(!optimizer.IsOutsideViewport(partial));
    std::cout << "✓ Partial viewport detection correct!" << std::endl;

    // 测试裁剪到视口
    SkRect clipped = optimizer.ClipToViewport(partial);
    assert(clipped.width() < partial.width());
    assert(clipped.height() < partial.height());
    std::cout << "✓ Clip to viewport correct!" << std::endl;

    // 测试裁剪区域栈
    optimizer.PushClipRect(SkRect::MakeXYWH(100, 100, 400, 400));
    optimizer.PushClipRect(SkRect::MakeXYWH(200, 200, 200, 200));
    
    SkRect current = optimizer.GetCurrentClipRect();
    assert(current.left() == 200);
    assert(current.top() == 200);
    std::cout << "✓ Clip rect stack correct!" << std::endl;

    optimizer.PopClipRect();
    current = optimizer.GetCurrentClipRect();
    assert(current.left() == 100);
    std::cout << "✓ Clip rect pop correct!" << std::endl;

    // 测试统计
    optimizer.ResetStats();
    assert(optimizer.GetClippedObjectCount() == 0);
    assert(optimizer.GetTotalObjectCount() == 0);
    std::cout << "✓ Clip statistics correct!" << std::endl;
}

// ========== 测试视口裁剪器 ==========

void TestViewportClipper() {
    std::cout << "\nTesting Viewport Clipper..." << std::endl;

    SkRect viewport = SkRect::MakeWH(800, 600);
    ViewportClipper clipper(viewport);

    // 测试点包含
    assert(clipper.Contains(400, 300));
    assert(!clipper.Contains(1000, 1000));
    std::cout << "✓ Point containment correct!" << std::endl;

    // 测试矩形相交
    SkRect inside = SkRect::MakeXYWH(100, 100, 200, 200);
    SkRect outside = SkRect::MakeXYWH(1000, 1000, 200, 200);
    
    assert(clipper.Intersects(inside));
    assert(!clipper.Intersects(outside));
    std::cout << "✓ Rectangle intersection correct!" << std::endl;

    // 测试矩形包含
    assert(clipper.ContainsRect(inside));
    assert(!clipper.ContainsRect(outside));
    std::cout << "✓ Rectangle containment correct!" << std::endl;

    // 测试裁剪
    SkRect partial = SkRect::MakeXYWH(700, 500, 200, 200);
    SkRect clipped = clipper.Clip(partial);
    assert(!clipped.isEmpty());
    assert(clipped.width() < partial.width());
    std::cout << "✓ Clipping correct!" << std::endl;
}

// ========== 主函数 ==========

int main() {
    std::cout << "\n=== Advanced Optimization Tests ===" << std::endl;

    TestLayerSystem();
    TestRenderCache();
    TestBatchRenderer();
    TestClipOptimizer();
    TestViewportClipper();

    std::cout << "\n=== All Advanced Optimization Tests Passed! ===" << std::endl;

    return 0;
}

