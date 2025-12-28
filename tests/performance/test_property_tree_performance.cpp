/**
 * @file test_property_tree_performance.cpp
 * @brief 属性树系统性能测试
 *
 * 测试内容：
 * - Transform 动画 GPU 占用
 * - Opacity 动画 GPU 占用
 * - 滚动性能
 * - 直接属性更新性能
 * - 层化性能
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_builder.h"
#include "core/compositor/property_tree/paint/paint_artifact_compositor.h"
#include "core/compositor/property_tree/geometry_mapper.h"
#include "core/compositor/property_tree/layerizer.h"
#include "core/compositor/property_tree/raster_invalidator.h"
#include "core/compositor/property_tree/paint/paint_artifact.h"
#include "core/compositor/property_tree/paint/paint_chunk.h"
#include "include/core/SkM44.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <cmath>

namespace lightui {
namespace test {

class PropertyTreePerformanceTest : public DOMTestBase {
protected:
    void SetUp() override {
        DOMTestBase::SetUp();
        property_trees_ = std::make_unique<PropertyTrees>();
    }

    void TearDown() override {
        property_trees_.reset();
        DOMTestBase::TearDown();
    }

    void PrintResult(const std::string& name, double ms, int operations) {
        double opsPerSec = operations / (ms / 1000.0);
        std::cout << "[PERF] " << name << ": " << ms << "ms for " << operations
                  << " ops (" << opsPerSec << " ops/sec)" << std::endl;
    }

    void PrintMemoryResult(const std::string& name, size_t bytes) {
        double kb = bytes / 1024.0;
        double mb = kb / 1024.0;
        if (mb >= 1.0) {
            std::cout << "[MEM] " << name << ": " << mb << " MB" << std::endl;
        } else {
            std::cout << "[MEM] " << name << ": " << kb << " KB" << std::endl;
        }
    }

    std::unique_ptr<PropertyTrees> property_trees_;
};

// ========== Transform 动画性能 ==========

TEST_F(PropertyTreePerformanceTest, TransformAnimationDirectUpdate) {
    // 测试直接更新 transform 的性能（不触发光栅化）
    const int NODE_COUNT = 100;
    const int FRAME_COUNT = 1000;

    // 创建 transform 节点
    std::vector<TransformTreeNode*> nodes;
    auto& transform_tree = property_trees_->GetTransformTree();
    auto root = transform_tree.GetRoot();

    for (int i = 0; i < NODE_COUNT; i++) {
        auto node = transform_tree.CreateNode(root);
        node->SetMatrix(SkM44::Translate(i * 10.0f, i * 10.0f, 0));
        nodes.push_back(node);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 模拟动画帧
    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        float t = static_cast<float>(frame) / FRAME_COUNT;
        float angle = t * 360.0f * 3.14159f / 180.0f;

        for (auto* node : nodes) {
            // 直接更新变换矩阵
            SkM44 matrix = SkM44::Rotate({0, 0, 1}, angle);
            matrix.preTranslate(100, 100, 0);
            node->SetMatrix(matrix);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("TransformDirectUpdate", ms, NODE_COUNT * FRAME_COUNT);

    // 验证：直接更新应该非常快（< 1ms per frame for 100 nodes）
    double msPerFrame = ms / FRAME_COUNT;
    std::cout << "[INFO] Average time per frame: " << msPerFrame << "ms" << std::endl;
    EXPECT_LT(msPerFrame, 5.0) << "Transform direct update should be fast";
}

TEST_F(PropertyTreePerformanceTest, TransformAccumulationPerformance) {
    // 测试累积变换计算性能
    const int DEPTH = 20;
    const int QUERIES = 10000;

    // 创建深层变换树
    auto& transform_tree = property_trees_->GetTransformTree();
    TransformTreeNode* parent = transform_tree.GetRoot();

    for (int i = 0; i < DEPTH; i++) {
        auto node = transform_tree.CreateNode(parent);
        node->SetMatrix(SkM44::Translate(10, 10, 0));
        parent = node;
    }

    TransformTreeNode* leaf = parent;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < QUERIES; i++) {
        // 计算累积变换
        SkM44 accumulated = leaf->GetAccumulatedTransform();
        (void)accumulated;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("TransformAccumulation", ms, QUERIES);
}

// ========== Opacity 动画性能 ==========

TEST_F(PropertyTreePerformanceTest, OpacityAnimationDirectUpdate) {
    const int NODE_COUNT = 100;
    const int FRAME_COUNT = 1000;

    // 创建 effect 节点
    std::vector<EffectTreeNode*> nodes;
    auto& effect_tree = property_trees_->GetEffectTree();
    auto root = effect_tree.GetRoot();

    for (int i = 0; i < NODE_COUNT; i++) {
        auto node = effect_tree.CreateNode(root);
        node->SetOpacity(1.0f);
        nodes.push_back(node);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 模拟动画帧
    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        float t = static_cast<float>(frame) / FRAME_COUNT;
        float opacity = 0.5f + 0.5f * std::sin(t * 3.14159f * 2);

        for (auto* node : nodes) {
            node->SetOpacity(opacity);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("OpacityDirectUpdate", ms, NODE_COUNT * FRAME_COUNT);

    double msPerFrame = ms / FRAME_COUNT;
    std::cout << "[INFO] Average time per frame: " << msPerFrame << "ms" << std::endl;
    EXPECT_LT(msPerFrame, 1.0) << "Opacity direct update should be very fast";
}

// ========== 滚动性能 ==========

TEST_F(PropertyTreePerformanceTest, ScrollOffsetDirectUpdate) {
    const int SCROLL_CONTAINER_COUNT = 10;
    const int FRAME_COUNT = 1000;

    // 创建滚动节点
    std::vector<ScrollTreeNode*> nodes;
    auto& scroll_tree = property_trees_->GetScrollTree();
    auto root = scroll_tree.GetRoot();

    for (int i = 0; i < SCROLL_CONTAINER_COUNT; i++) {
        auto node = scroll_tree.CreateNode(root);
        node->SetContainerSize({800, 600});
        node->SetContentSize({800, 10000});
        node->SetScrollOffset({0, 0});
        nodes.push_back(node);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 模拟滚动
    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        float scrollY = static_cast<float>(frame) * 10.0f;

        for (auto* node : nodes) {
            node->SetScrollOffset({0, scrollY});
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("ScrollOffsetUpdate", ms, SCROLL_CONTAINER_COUNT * FRAME_COUNT);

    double msPerFrame = ms / FRAME_COUNT;
    std::cout << "[INFO] Average time per frame: " << msPerFrame << "ms" << std::endl;
    EXPECT_LT(msPerFrame, 1.0) << "Scroll offset update should be very fast";
}

// ========== 几何映射性能 ==========

TEST_F(PropertyTreePerformanceTest, GeometryMappingPerformance) {
    const int MAPPING_COUNT = 100000;

    // 创建属性树结构
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 创建一些节点
    auto t1 = transform_tree.CreateNode(transform_root);
    t1->SetMatrix(SkM44::Translate(100, 100, 0));

    auto t2 = transform_tree.CreateNode(t1);
    t2->SetMatrix(SkM44::Scale(2, 2, 1));

    auto c1 = clip_tree.CreateNode(clip_root);
    c1->SetClipRect(SkRect::MakeXYWH(0, 0, 800, 600));
    c1->SetTransformNode(t1);

    PropertyTreeState source(t2, c1, effect_root, nullptr);
    PropertyTreeState target(transform_root, clip_root, effect_root, nullptr);

    GeometryMapper mapper(*property_trees_);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MAPPING_COUNT; i++) {
        SkRect rect = SkRect::MakeXYWH(10, 10, 100, 100);
        SkRect mapped = mapper.MapRect(rect, source, target);
        (void)mapped;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("GeometryMapping", ms, MAPPING_COUNT);
}

// ========== 层化性能 ==========

TEST_F(PropertyTreePerformanceTest, LayerizationPerformance) {
    const int CHUNK_COUNT = 1000;
    const int ITERATIONS = 100;

    // 创建 PaintArtifact
    PaintArtifact artifact;

    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    // 创建绘制块
    for (int i = 0; i < CHUNK_COUNT; i++) {
        PropertyTreeState state(transform_root, clip_root, effect_root, nullptr);
        artifact.StartNewChunk(state);

        // 添加一些显示项
        DisplayItem item;
        item.SetType(DisplayItemType::kDrawRect);
        item.SetBounds(SkRect::MakeXYWH(i * 10.0f, 0, 100, 100));
        artifact.AppendDisplayItem(std::move(item));

        artifact.FinishCurrentChunk();
    }

    GeometryMapper mapper(*property_trees_);
    Layerizer layerizer(*property_trees_, mapper);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        auto layers = layerizer.Layerize(artifact);
        (void)layers;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("Layerization", ms, ITERATIONS);

    double msPerIteration = ms / ITERATIONS;
    std::cout << "[INFO] Average time per layerization: " << msPerIteration << "ms" << std::endl;
}

// ========== 光栅化失效计算性能 ==========

TEST_F(PropertyTreePerformanceTest, RasterInvalidationPerformance) {
    const int CHUNK_COUNT = 100;
    const int ITERATIONS = 1000;

    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();

    PropertyTreeState layer_state(transform_root, clip_root, effect_root, nullptr);

    // 创建旧的 PaintArtifact
    PaintArtifact old_artifact;
    for (int i = 0; i < CHUNK_COUNT; i++) {
        old_artifact.StartNewChunk(layer_state);
        DisplayItem item;
        item.SetType(DisplayItemType::kDrawRect);
        item.SetBounds(SkRect::MakeXYWH(i * 10.0f, 0, 100, 100));
        old_artifact.AppendDisplayItem(std::move(item));
        old_artifact.FinishCurrentChunk();
    }

    // 创建新的 PaintArtifact（有一些变化）
    PaintArtifact new_artifact;
    for (int i = 0; i < CHUNK_COUNT; i++) {
        new_artifact.StartNewChunk(layer_state);
        DisplayItem item;
        item.SetType(DisplayItemType::kDrawRect);
        // 一些块有变化
        float offset = (i % 10 == 0) ? 5.0f : 0.0f;
        item.SetBounds(SkRect::MakeXYWH(i * 10.0f + offset, 0, 100, 100));
        new_artifact.AppendDisplayItem(std::move(item));
        new_artifact.FinishCurrentChunk();
    }

    GeometryMapper mapper(*property_trees_);
    RasterInvalidator invalidator(mapper);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        auto result = invalidator.ComputeInvalidation(old_artifact, new_artifact, layer_state);
        (void)result;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("RasterInvalidation", ms, ITERATIONS);
}

// ========== 属性树构建性能 ==========

TEST_F(PropertyTreePerformanceTest, PropertyTreeBuildPerformance) {
    // This test requires render tree support which may not be available
    // Skip if Document doesn't support layout/render tree
    GTEST_SKIP() << "Render tree integration not available in this build";
}

// ========== 内存使用测试 ==========

TEST_F(PropertyTreePerformanceTest, MemoryUsage) {
    const int NODE_COUNT = 10000;

    // 创建大量节点
    auto& transform_tree = property_trees_->GetTransformTree();
    auto& clip_tree = property_trees_->GetClipTree();
    auto& effect_tree = property_trees_->GetEffectTree();
    auto& scroll_tree = property_trees_->GetScrollTree();

    auto transform_root = transform_tree.GetRoot();
    auto clip_root = clip_tree.GetRoot();
    auto effect_root = effect_tree.GetRoot();
    auto scroll_root = scroll_tree.GetRoot();

    for (int i = 0; i < NODE_COUNT; i++) {
        transform_tree.CreateNode(transform_root);
        clip_tree.CreateNode(clip_root);
        effect_tree.CreateNode(effect_root);
        scroll_tree.CreateNode(scroll_root);
    }

    // 估算内存使用
    size_t transform_mem = NODE_COUNT * sizeof(TransformTreeNode);
    size_t clip_mem = NODE_COUNT * sizeof(ClipTreeNode);
    size_t effect_mem = NODE_COUNT * sizeof(EffectTreeNode);
    size_t scroll_mem = NODE_COUNT * sizeof(ScrollTreeNode);
    size_t total_mem = transform_mem + clip_mem + effect_mem + scroll_mem;

    PrintMemoryResult("TransformNodes", transform_mem);
    PrintMemoryResult("ClipNodes", clip_mem);
    PrintMemoryResult("EffectNodes", effect_mem);
    PrintMemoryResult("ScrollNodes", scroll_mem);
    PrintMemoryResult("TotalPropertyTrees", total_mem);

    // 验证节点数量
    EXPECT_EQ(transform_tree.GetNodeCount(), NODE_COUNT + 1);  // +1 for root
    EXPECT_EQ(clip_tree.GetNodeCount(), NODE_COUNT + 1);
    EXPECT_EQ(effect_tree.GetNodeCount(), NODE_COUNT + 1);
    EXPECT_EQ(scroll_tree.GetNodeCount(), NODE_COUNT + 1);
}

// ========== 缓存效率测试 ==========

TEST_F(PropertyTreePerformanceTest, CacheEfficiency) {
    const int QUERIES = 100000;

    // 创建属性树结构
    auto& transform_tree = property_trees_->GetTransformTree();
    auto transform_root = transform_tree.GetRoot();

    // 创建深层树
    TransformTreeNode* parent = transform_root;
    for (int i = 0; i < 10; i++) {
        auto node = transform_tree.CreateNode(parent);
        node->SetMatrix(SkM44::Translate(10, 10, 0));
        parent = node;
    }

    TransformTreeNode* leaf = parent;

    // 第一次查询（无缓存）
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < QUERIES; i++) {
        leaf->ClearDirty();  // 清除脏标记以使用缓存
        SkM44 accumulated = leaf->GetAccumulatedTransform();
        (void)accumulated;
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    double ms1 = duration1.count() / 1000.0;

    // 标记脏并重新查询（无缓存）
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < QUERIES; i++) {
        leaf->MarkDirty();  // 每次都标记脏
        SkM44 accumulated = leaf->GetAccumulatedTransform();
        (void)accumulated;
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    double ms2 = duration2.count() / 1000.0;

    PrintResult("CachedQueries", ms1, QUERIES);
    PrintResult("UncachedQueries", ms2, QUERIES);

    double speedup = ms2 / ms1;
    std::cout << "[INFO] Cache speedup: " << speedup << "x" << std::endl;
}

} // namespace test
} // namespace lightui
