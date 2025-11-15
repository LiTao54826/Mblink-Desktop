/**
 * @file test_performance_optimization.cpp
 * @brief 性能优化测试
 */

#include "core/render/animation_optimizer.h"
#include "core/render/filter_cache.h"
#include "core/render/object_pool.h"
#include "core/render/css_filters.h"
#include "core/render/transform.h"
#include <iostream>
#include <chrono>
#include <string>

using namespace lightui;

// 测试计数器
static int total_tests = 0;
static int passed_tests = 0;

// 测试宏
#define TEST(name) \
    std::cout << "\n[TEST] " << name << std::endl;

#define ASSERT(condition, message) \
    total_tests++; \
    if (condition) { \
        std::cout << "  ✓ " << message << std::endl; \
        passed_tests++; \
    } else { \
        std::cout << "  ✗ " << message << " (FAILED)" << std::endl; \
    }

// ============================================================================
// 关键帧插值缓存测试
// ============================================================================

void TestKeyframeInterpolationCache() {
    TEST("KeyframeInterpolationCache - Basic Operations");
    
    KeyframeInterpolationCache cache(10);
    
    // 测试缓存未命中
    auto result1 = cache.Get("fade-in", 0.5f);
    ASSERT(!result1.has_value(), "Cache miss on first access");
    
    // 添加到缓存
    std::map<std::string, std::string> props;
    props["opacity"] = "0.5";
    cache.Put("fade-in", 0.5f, props);
    
    // 测试缓存命中
    auto result2 = cache.Get("fade-in", 0.5f);
    ASSERT(result2.has_value(), "Cache hit after put");
    ASSERT(result2->at("opacity") == "0.5", "Cached value is correct");
    
    // 测试缓存失效
    cache.Invalidate("fade-in");
    auto result3 = cache.Get("fade-in", 0.5f);
    ASSERT(!result3.has_value(), "Cache miss after invalidation");
    
    TEST("KeyframeInterpolationCache - Hit Rate");
    
    cache.Clear();
    cache.ResetStats();
    
    // 添加多个条目
    for (int i = 0; i < 5; ++i) {
        float progress = i * 0.2f;
        std::map<std::string, std::string> p;
        p["opacity"] = std::to_string(progress);
        cache.Put("test-anim", progress, p);
    }
    
    // 访问缓存
    for (int i = 0; i < 5; ++i) {
        cache.Get("test-anim", i * 0.2f);
    }
    
    float hit_rate = cache.GetHitRate();
    ASSERT(hit_rate > 0.0f, "Hit rate is positive");
    ASSERT(hit_rate <= 1.0f, "Hit rate is <= 1.0");
    
    TEST("KeyframeInterpolationCache - LRU Eviction");
    
    KeyframeInterpolationCache small_cache(3);
    
    // 填满缓存
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> p;
        p["value"] = std::to_string(i);
        small_cache.Put("anim", i * 0.1f, p);
    }
    
    // 缓存应该只保留最后 3 个
    auto result = small_cache.Get("anim", 0.0f);
    ASSERT(!result.has_value(), "Oldest entry was evicted");
}

// ============================================================================
// 动画脏标记测试
// ============================================================================

void TestAnimationDirtyTracker() {
    TEST("AnimationDirtyTracker - Basic Operations");
    
    AnimationDirtyTracker tracker;
    RenderObject* obj1 = reinterpret_cast<RenderObject*>(0x1000);
    RenderObject* obj2 = reinterpret_cast<RenderObject*>(0x2000);
    
    // 初始状态不脏
    ASSERT(!tracker.IsDirty(obj1, "anim1"), "Initially not dirty");
    
    // 标记为脏
    tracker.MarkDirty(obj1, "anim1");
    ASSERT(tracker.IsDirty(obj1, "anim1"), "Marked as dirty");
    
    // 清除脏标记
    tracker.ClearDirty(obj1, "anim1");
    ASSERT(!tracker.IsDirty(obj1, "anim1"), "Cleared dirty flag");
    
    TEST("AnimationDirtyTracker - Multiple Objects");
    
    tracker.Clear();
    
    tracker.MarkDirty(obj1, "anim1");
    tracker.MarkDirty(obj1, "anim2");
    tracker.MarkDirty(obj2, "anim1");
    
    ASSERT(tracker.IsDirty(obj1, "anim1"), "Object 1 animation 1 is dirty");
    ASSERT(tracker.IsDirty(obj1, "anim2"), "Object 1 animation 2 is dirty");
    ASSERT(tracker.IsDirty(obj2, "anim1"), "Object 2 animation 1 is dirty");
    ASSERT(!tracker.IsDirty(obj2, "anim2"), "Object 2 animation 2 is not dirty");
    
    TEST("AnimationDirtyTracker - Clear All");
    
    tracker.ClearAllDirty(obj1);
    ASSERT(!tracker.IsDirty(obj1, "anim1"), "Object 1 all cleared");
    ASSERT(!tracker.IsDirty(obj1, "anim2"), "Object 1 all cleared");
    ASSERT(tracker.IsDirty(obj2, "anim1"), "Object 2 still dirty");
}

// ============================================================================
// 批量动画更新测试
// ============================================================================

void TestBatchAnimationUpdater() {
    TEST("BatchAnimationUpdater - Basic Operations");
    
    BatchAnimationUpdater updater;
    RenderObject* obj = reinterpret_cast<RenderObject*>(0x1000);
    
    ASSERT(updater.GetRequestCount() == 0, "Initially no requests");
    
    updater.AddUpdateRequest(obj, "anim1", 1.0);
    updater.AddUpdateRequest(obj, "anim2", 1.0);
    
    ASSERT(updater.GetRequestCount() == 2, "Two requests added");
    
    updater.Clear();
    ASSERT(updater.GetRequestCount() == 0, "Requests cleared");
    
    TEST("BatchAnimationUpdater - Enable/Disable");
    
    updater.SetEnabled(false);
    ASSERT(!updater.IsEnabled(), "Updater disabled");
    
    updater.AddUpdateRequest(obj, "anim1", 1.0);
    ASSERT(updater.GetRequestCount() == 0, "No requests when disabled");
    
    updater.SetEnabled(true);
    ASSERT(updater.IsEnabled(), "Updater enabled");
}

// ============================================================================
// 滤镜缓存测试
// ============================================================================

void TestFilterCache() {
    TEST("FilterCache - Basic Operations");
    
    FilterCache cache(10);
    
    // 创建滤镜列表
    CSSFilterList filter_list;
    filter_list.AddFilter(CSSFilter::Blur(10.0f));
    filter_list.AddFilter(CSSFilter::Brightness(1.5f));
    
    // 测试缓存未命中
    auto result1 = cache.Get(filter_list);
    ASSERT(result1 == nullptr, "Cache miss on first access");
    
    // 创建并缓存 Skia 滤镜
    auto skia_filter = filter_list.CreateSkiaFilter();
    cache.Put(filter_list, skia_filter);
    
    // 测试缓存命中
    auto result2 = cache.Get(filter_list);
    ASSERT(result2 != nullptr, "Cache hit after put");
    
    TEST("FilterCache - Hit Rate");
    
    cache.Clear();
    cache.ResetStats();
    
    // 添加多个滤镜
    for (int i = 1; i <= 5; ++i) {
        CSSFilterList fl;
        fl.AddFilter(CSSFilter::Blur(i * 5.0f));
        auto sf = fl.CreateSkiaFilter();
        cache.Put(fl, sf);
    }
    
    // 访问缓存
    for (int i = 1; i <= 5; ++i) {
        CSSFilterList fl;
        fl.AddFilter(CSSFilter::Blur(i * 5.0f));
        cache.Get(fl);
    }
    
    float hit_rate = cache.GetHitRate();
    ASSERT(hit_rate > 0.0f, "Hit rate is positive");
    ASSERT(hit_rate <= 1.0f, "Hit rate is <= 1.0");
}

// ============================================================================
// 变换矩阵缓存测试
// ============================================================================

void TestTransformMatrixCache() {
    TEST("TransformMatrixCache - Basic Operations");

    TransformMatrixCache cache(10);

    // 创建变换字符串
    std::string transform_str = "translate(100px, 50px) rotate(45deg)";

    // 测试缓存未命中
    auto result1 = cache.Get(transform_str);
    ASSERT(!result1.has_value(), "Cache miss on first access");

    // 创建并缓存矩阵
    SkMatrix matrix;
    matrix.setIdentity();
    matrix.postTranslate(100.0f, 50.0f);
    matrix.postRotate(45.0f);
    cache.Put(transform_str, matrix);

    // 测试缓存命中
    auto result2 = cache.Get(transform_str);
    ASSERT(result2.has_value(), "Cache hit after put");

    TEST("TransformMatrixCache - Hit Rate");

    cache.Clear();
    cache.ResetStats();

    // 添加多个变换
    for (int i = 1; i <= 5; ++i) {
        std::string t = "scale(" + std::to_string(i * 0.5f) + ")";
        SkMatrix m;
        m.setScale(i * 0.5f, i * 0.5f);
        cache.Put(t, m);
    }

    // 访问缓存
    for (int i = 1; i <= 5; ++i) {
        std::string t = "scale(" + std::to_string(i * 0.5f) + ")";
        cache.Get(t);
    }

    float hit_rate = cache.GetHitRate();
    ASSERT(hit_rate > 0.0f, "Hit rate is positive");
    ASSERT(hit_rate <= 1.0f, "Hit rate is <= 1.0");
}

// ============================================================================
// 对象池测试
// ============================================================================

struct TestObject {
    int value = 0;
    std::string name;
};

void TestObjectPool() {
    TEST("ObjectPool - Basic Operations");
    
    ObjectPool<TestObject> pool(5, 10);
    
    ASSERT(pool.GetAvailableCount() == 5, "Initial size is 5");
    ASSERT(pool.GetInUseCount() == 0, "No objects in use");
    
    // 获取对象
    TestObject* obj1 = pool.Acquire();
    ASSERT(obj1 != nullptr, "Acquired object");
    ASSERT(pool.GetAvailableCount() == 4, "Available count decreased");
    ASSERT(pool.GetInUseCount() == 1, "In-use count increased");
    
    // 归还对象
    pool.Release(obj1);
    ASSERT(pool.GetAvailableCount() == 5, "Available count restored");
    ASSERT(pool.GetInUseCount() == 0, "In-use count restored");
    
    TEST("ObjectPool - Reuse Rate");
    
    pool.Clear();
    pool.ResetStats();
    
    ObjectPool<TestObject> pool2(2, 10);
    
    // 获取并归还多次
    for (int i = 0; i < 5; ++i) {
        TestObject* obj = pool2.Acquire();
        pool2.Release(obj);
    }
    
    float reuse_rate = pool2.GetReuseRate();
    ASSERT(reuse_rate > 0.0f, "Reuse rate is positive");
    ASSERT(reuse_rate <= 1.0f, "Reuse rate is <= 1.0");
    
    TEST("ObjectPool - RAII Wrapper");
    
    ObjectPool<TestObject> pool3(5);
    
    {
        PooledObject<TestObject> obj(pool3);
        ASSERT(obj.Get() != nullptr, "RAII object acquired");
        ASSERT(pool3.GetInUseCount() == 1, "Object in use");
        
        obj->value = 42;
        ASSERT(obj->value == 42, "Object can be modified");
    }
    
    // 对象应该自动归还
    ASSERT(pool3.GetInUseCount() == 0, "Object auto-released");
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Performance Optimization Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    TestKeyframeInterpolationCache();
    TestAnimationDirtyTracker();
    TestBatchAnimationUpdater();
    TestFilterCache();
    TestTransformMatrixCache();
    TestObjectPool();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results: " << passed_tests << "/" << total_tests << " passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed_tests == total_tests) ? 0 : 1;
}

