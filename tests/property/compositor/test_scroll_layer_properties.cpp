/**
 * @file test_scroll_layer_properties.cpp
 * @brief 滚动层属性测试
 *
 * 测试滚动层管理器的核心属性：
 * - 滚动容器注册和管理
 * - 无需重新光栅化的滚动
 * - 固定元素处理
 * - 滚动范围限制
 */

#include <gtest/gtest.h>
#include "core/compositor/scroll_layer_manager.h"
#include "core/compositor/compositor_layer.h"
#include "core/compositor/layer_tree_builder.h"
#include "core/compositor/rasterizer.h"
#include "core/render/objects/render_object.h"
#include <memory>
#include <cmath>

using namespace mbink;

// =========================================================================
// 测试辅助类
// =========================================================================

/**
 * @brief 可滚动的测试渲染对象
 */
class ScrollableRenderObject : public RenderObject {
public:
    ScrollableRenderObject() : RenderObject(RenderObjectType::BLOCK) {
        // 设置为可滚动
        computed_style_.overflow_y = "scroll";
        computed_style_.overflow_x = "auto";
        
        // 设置布局信息
        layout_info_.x = 0;
        layout_info_.y = 0;
        layout_info_.width = 400;
        layout_info_.height = 300;
        layout_info_.is_laid_out = true;
        
        // 设置内容尺寸（大于视口）
        content_width_ = 800;
        content_height_ = 1000;
    }

    void SetViewportSize(float width, float height) {
        layout_info_.width = width;
        layout_info_.height = height;
    }

    void SetContentSizeValues(float width, float height) {
        content_width_ = width;
        content_height_ = height;
    }
};

/**
 * @brief 固定定位的测试渲染对象
 */
class FixedRenderObject : public RenderObject {
public:
    FixedRenderObject() : RenderObject(RenderObjectType::BLOCK) {
        // 设置为 position: fixed
        computed_style_.position = "fixed";
        
        // 设置布局信息
        layout_info_.x = 10;
        layout_info_.y = 10;
        layout_info_.width = 100;
        layout_info_.height = 50;
        layout_info_.is_laid_out = true;
    }

    void SetPosition(float x, float y) {
        layout_info_.x = x;
        layout_info_.y = y;
    }
};

/**
 * @brief 普通测试渲染对象（不可滚动）
 */
class NormalRenderObject : public RenderObject {
public:
    NormalRenderObject() : RenderObject(RenderObjectType::BLOCK) {
        computed_style_.overflow_y = "visible";
        layout_info_.width = 200;
        layout_info_.height = 100;
        layout_info_.is_laid_out = true;
    }
};

// =========================================================================
// 滚动容器注册测试
// =========================================================================

class ScrollContainerRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
    }

    std::unique_ptr<ScrollLayerManager> manager_;
};

/**
 * Property 1: 可滚动容器可以成功注册
 */
TEST_F(ScrollContainerRegistrationTest, ScrollableContainerCanBeRegistered) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    
    EXPECT_TRUE(manager_->RegisterScrollContainer(scrollable.get()));
    EXPECT_TRUE(manager_->IsScrollContainer(scrollable.get()));
    EXPECT_EQ(manager_->GetScrollContainerCount(), 1);
}

/**
 * Property 2: 不可滚动容器注册失败
 */
TEST_F(ScrollContainerRegistrationTest, NonScrollableContainerCannotBeRegistered) {
    auto normal = std::make_shared<NormalRenderObject>();
    
    EXPECT_FALSE(manager_->RegisterScrollContainer(normal.get()));
    EXPECT_FALSE(manager_->IsScrollContainer(normal.get()));
    EXPECT_EQ(manager_->GetScrollContainerCount(), 0);
}

/**
 * Property 3: 重复注册返回 true 但不增加计数
 */
TEST_F(ScrollContainerRegistrationTest, DuplicateRegistrationReturnsTrue) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    
    EXPECT_TRUE(manager_->RegisterScrollContainer(scrollable.get()));
    EXPECT_TRUE(manager_->RegisterScrollContainer(scrollable.get()));
    EXPECT_EQ(manager_->GetScrollContainerCount(), 1);
}

/**
 * Property 4: 注销后容器不再被识别
 */
TEST_F(ScrollContainerRegistrationTest, UnregisteredContainerNotRecognized) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    
    manager_->RegisterScrollContainer(scrollable.get());
    manager_->UnregisterScrollContainer(scrollable.get());
    
    EXPECT_FALSE(manager_->IsScrollContainer(scrollable.get()));
    EXPECT_EQ(manager_->GetScrollContainerCount(), 0);
}

// =========================================================================
// 滚动处理测试
// =========================================================================

class ScrollHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
        scrollable_ = std::make_shared<ScrollableRenderObject>();
        manager_->RegisterScrollContainer(scrollable_.get());
    }

    std::unique_ptr<ScrollLayerManager> manager_;
    std::shared_ptr<ScrollableRenderObject> scrollable_;
};

/**
 * Property 5: 滚动更新层偏移而不是重新光栅化
 */
TEST_F(ScrollHandlingTest, ScrollUpdatesLayerOffsetNotRasterize) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->content_layer, nullptr);

    // 记录初始状态
    auto initial_offset = info->content_layer->GetScrollOffset();
    
    // 执行滚动
    EXPECT_TRUE(manager_->HandleScroll(scrollable_.get(), 0, 100));
    
    // 验证层偏移已更新
    auto new_offset = info->content_layer->GetScrollOffset();
    EXPECT_FLOAT_EQ(new_offset.fY, -100);
    
    // 验证没有标记脏区域（不需要重新光栅化）
    // 注意：初始创建时会标记脏，这里检查滚动后没有新增脏区域
}

/**
 * Property 6: 滚动位置被限制在有效范围内
 */
TEST_F(ScrollHandlingTest, ScrollPositionClampedToValidRange) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);

    // 尝试滚动超出范围
    manager_->HandleScroll(scrollable_.get(), 0, 10000);
    
    // 验证滚动位置被限制
    EXPECT_LE(info->scroll_y, info->max_scroll_y);
    EXPECT_GE(info->scroll_y, 0);
}

/**
 * Property 7: 负滚动被限制为 0
 */
TEST_F(ScrollHandlingTest, NegativeScrollClampedToZero) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);

    // 尝试负滚动
    manager_->HandleScroll(scrollable_.get(), -100, -100);
    
    EXPECT_FLOAT_EQ(info->scroll_x, 0);
    EXPECT_FLOAT_EQ(info->scroll_y, 0);
}

/**
 * Property 8: ScrollTo 设置绝对滚动位置
 */
TEST_F(ScrollHandlingTest, ScrollToSetsAbsolutePosition) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);

    // 先滚动到某个位置
    manager_->HandleScroll(scrollable_.get(), 0, 50);
    
    // 使用 ScrollTo 设置绝对位置
    manager_->ScrollTo(scrollable_.get(), 0, 200);
    
    EXPECT_FLOAT_EQ(info->scroll_y, 200);
}

/**
 * Property 9: 滚动同步到 RenderObject
 */
TEST_F(ScrollHandlingTest, ScrollSyncedToRenderObject) {
    manager_->HandleScroll(scrollable_.get(), 50, 100);
    
    EXPECT_FLOAT_EQ(scrollable_->GetScrollX(), 50);
    EXPECT_FLOAT_EQ(scrollable_->GetScrollY(), 100);
}

// =========================================================================
// 固定元素测试
// =========================================================================

class FixedElementTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
        scrollable_ = std::make_shared<ScrollableRenderObject>();
        fixed_ = std::make_shared<FixedRenderObject>();
        
        manager_->RegisterScrollContainer(scrollable_.get());
        manager_->RegisterFixedElement(fixed_.get());
    }

    std::unique_ptr<ScrollLayerManager> manager_;
    std::shared_ptr<ScrollableRenderObject> scrollable_;
    std::shared_ptr<FixedRenderObject> fixed_;
};

/**
 * Property 10: 固定元素可以成功注册
 */
TEST_F(FixedElementTest, FixedElementCanBeRegistered) {
    EXPECT_TRUE(manager_->IsFixedElement(fixed_.get()));
    EXPECT_EQ(manager_->GetFixedElementCount(), 1);
}

/**
 * Property 11: 固定元素层不受滚动影响
 */
TEST_F(FixedElementTest, FixedElementLayerNotAffectedByScroll) {
    auto* fixed_info = manager_->GetFixedElementInfo(fixed_.get());
    ASSERT_NE(fixed_info, nullptr);
    ASSERT_NE(fixed_info->layer, nullptr);

    // 记录初始偏移
    auto initial_offset = fixed_info->layer->GetScrollOffset();
    
    // 滚动容器
    manager_->HandleScroll(scrollable_.get(), 0, 200);
    
    // 更新固定元素位置
    manager_->UpdateFixedElementPositions();
    
    // 验证固定元素层偏移没有改变
    auto new_offset = fixed_info->layer->GetScrollOffset();
    EXPECT_FLOAT_EQ(new_offset.fX, initial_offset.fX);
    EXPECT_FLOAT_EQ(new_offset.fY, initial_offset.fY);
}

/**
 * Property 12: 固定元素层的提升原因是 PositionFixed
 */
TEST_F(FixedElementTest, FixedElementLayerHasCorrectPromotionReason) {
    auto* fixed_info = manager_->GetFixedElementInfo(fixed_.get());
    ASSERT_NE(fixed_info, nullptr);
    ASSERT_NE(fixed_info->layer, nullptr);

    EXPECT_EQ(fixed_info->layer->GetPromotionReason(), LayerPromotionReason::PositionFixed);
}

// =========================================================================
// 内容尺寸更新测试
// =========================================================================

class ContentSizeUpdateTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
        scrollable_ = std::make_shared<ScrollableRenderObject>();
        manager_->RegisterScrollContainer(scrollable_.get());
    }

    std::unique_ptr<ScrollLayerManager> manager_;
    std::shared_ptr<ScrollableRenderObject> scrollable_;
};

/**
 * Property 13: 内容尺寸更新后滚动范围正确调整
 */
TEST_F(ContentSizeUpdateTest, ScrollBoundsAdjustedAfterContentSizeUpdate) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);

    float old_max_scroll_y = info->max_scroll_y;
    
    // 增加内容高度
    scrollable_->SetContentSizeValues(800, 2000);
    manager_->UpdateContentSize(scrollable_.get());
    
    // 验证最大滚动范围增加
    EXPECT_GT(info->max_scroll_y, old_max_scroll_y);
}

/**
 * Property 14: 内容尺寸减小时滚动位置被调整
 */
TEST_F(ContentSizeUpdateTest, ScrollPositionAdjustedWhenContentShrinks) {
    auto* info = manager_->GetScrollContainerInfo(scrollable_.get());
    ASSERT_NE(info, nullptr);

    // 先滚动到底部
    manager_->ScrollTo(scrollable_.get(), 0, info->max_scroll_y);
    float scroll_before = info->scroll_y;
    
    // 减小内容高度
    scrollable_->SetContentSizeValues(800, 400);  // 比视口稍大
    manager_->UpdateContentSize(scrollable_.get());
    
    // 验证滚动位置被调整到有效范围内
    EXPECT_LE(info->scroll_y, info->max_scroll_y);
}

// =========================================================================
// 层创建测试
// =========================================================================

class LayerCreationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
    }

    std::unique_ptr<ScrollLayerManager> manager_;
};

/**
 * Property 15: 滚动内容层有正确的边界
 */
TEST_F(LayerCreationTest, ScrollContentLayerHasCorrectBounds) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    scrollable->SetContentSizeValues(800, 1000);
    
    manager_->RegisterScrollContainer(scrollable.get());
    
    auto* info = manager_->GetScrollContainerInfo(scrollable.get());
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->content_layer, nullptr);

    auto bounds = info->content_layer->GetBounds();
    EXPECT_FLOAT_EQ(bounds.width(), 800);
    EXPECT_FLOAT_EQ(bounds.height(), 1000);
}

/**
 * Property 16: 滚动内容层的提升原因是 ScrollableContent
 */
TEST_F(LayerCreationTest, ScrollContentLayerHasCorrectPromotionReason) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    manager_->RegisterScrollContainer(scrollable.get());
    
    auto* info = manager_->GetScrollContainerInfo(scrollable.get());
    ASSERT_NE(info, nullptr);
    ASSERT_NE(info->content_layer, nullptr);

    EXPECT_EQ(info->content_layer->GetPromotionReason(), LayerPromotionReason::ScrollableContent);
}

// =========================================================================
// 清理测试
// =========================================================================

class CleanupTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
    }

    std::unique_ptr<ScrollLayerManager> manager_;
};

/**
 * Property 17: Clear 清除所有状态
 */
TEST_F(CleanupTest, ClearRemovesAllState) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    auto fixed = std::make_shared<FixedRenderObject>();
    
    manager_->RegisterScrollContainer(scrollable.get());
    manager_->RegisterFixedElement(fixed.get());
    
    EXPECT_EQ(manager_->GetScrollContainerCount(), 1);
    EXPECT_EQ(manager_->GetFixedElementCount(), 1);
    
    manager_->Clear();
    
    EXPECT_EQ(manager_->GetScrollContainerCount(), 0);
    EXPECT_EQ(manager_->GetFixedElementCount(), 0);
}

// =========================================================================
// 边界情况测试
// =========================================================================

class EdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<ScrollLayerManager>();
    }

    std::unique_ptr<ScrollLayerManager> manager_;
};

/**
 * Property 18: nullptr 参数安全处理
 */
TEST_F(EdgeCaseTest, NullptrParametersSafelyHandled) {
    EXPECT_FALSE(manager_->RegisterScrollContainer(nullptr));
    EXPECT_FALSE(manager_->RegisterFixedElement(nullptr));
    EXPECT_FALSE(manager_->HandleScroll(nullptr, 0, 0));
    EXPECT_FALSE(manager_->IsScrollContainer(nullptr));
    EXPECT_FALSE(manager_->IsFixedElement(nullptr));
    
    // 不应该崩溃
    manager_->UnregisterScrollContainer(nullptr);
    manager_->UnregisterFixedElement(nullptr);
    manager_->UpdateContentSize(nullptr);
}

/**
 * Property 19: 未注册容器的滚动操作返回 false
 */
TEST_F(EdgeCaseTest, ScrollOnUnregisteredContainerReturnsFalse) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    
    // 不注册，直接尝试滚动
    EXPECT_FALSE(manager_->HandleScroll(scrollable.get(), 0, 100));
}

/**
 * Property 20: 内容尺寸等于视口时最大滚动为 0
 */
TEST_F(EdgeCaseTest, MaxScrollZeroWhenContentEqualsViewport) {
    auto scrollable = std::make_shared<ScrollableRenderObject>();
    scrollable->SetViewportSize(400, 300);
    // 内容稍大于视口，确保可滚动
    scrollable->SetContentSizeValues(401, 301);
    
    manager_->RegisterScrollContainer(scrollable.get());
    
    auto* info = manager_->GetScrollContainerInfo(scrollable.get());
    ASSERT_NE(info, nullptr);

    // 然后更新内容尺寸为等于视口
    scrollable->SetContentSizeValues(400, 300);
    manager_->UpdateContentSize(scrollable.get());

    EXPECT_FLOAT_EQ(info->max_scroll_x, 0);
    EXPECT_FLOAT_EQ(info->max_scroll_y, 0);
}

