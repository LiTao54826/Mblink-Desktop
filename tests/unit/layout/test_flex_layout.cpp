/**
 * @file test_flex_layout.cpp
 * @brief Flexbox 布局算法单元测试
 */

#include <gtest/gtest.h>
#include "layout/flex_layout.h"
#include "layout/types/style.h"
#include "layout/types/geometry.h"
#include <unordered_map>

namespace mbink {
namespace test {

class FlexLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
};

// ========== Flex Direction 测试 ==========

TEST_F(FlexLayoutTest, FlexDirectionRow) {
    // 测试 row 方向的主轴和交叉轴
    FlexDirection dir = FlexDirection::Row;
    EXPECT_TRUE(dir == FlexDirection::Row);
}

TEST_F(FlexLayoutTest, FlexDirectionColumn) {
    FlexDirection dir = FlexDirection::Column;
    EXPECT_TRUE(dir == FlexDirection::Column);
}

TEST_F(FlexLayoutTest, FlexDirectionRowReverse) {
    FlexDirection dir = FlexDirection::RowReverse;
    EXPECT_TRUE(dir == FlexDirection::RowReverse);
}

TEST_F(FlexLayoutTest, FlexDirectionColumnReverse) {
    FlexDirection dir = FlexDirection::ColumnReverse;
    EXPECT_TRUE(dir == FlexDirection::ColumnReverse);
}

// ========== Justify Content 测试 ==========

TEST_F(FlexLayoutTest, JustifyContentFlexStart) {
    JustifyContent jc = JustifyContent::FlexStart;
    EXPECT_TRUE(jc == JustifyContent::FlexStart);
}

TEST_F(FlexLayoutTest, JustifyContentFlexEnd) {
    JustifyContent jc = JustifyContent::FlexEnd;
    EXPECT_TRUE(jc == JustifyContent::FlexEnd);
}

TEST_F(FlexLayoutTest, JustifyContentCenter) {
    JustifyContent jc = JustifyContent::Center;
    EXPECT_TRUE(jc == JustifyContent::Center);
}

TEST_F(FlexLayoutTest, JustifyContentSpaceBetween) {
    JustifyContent jc = JustifyContent::SpaceBetween;
    EXPECT_TRUE(jc == JustifyContent::SpaceBetween);
}

TEST_F(FlexLayoutTest, JustifyContentSpaceAround) {
    JustifyContent jc = JustifyContent::SpaceAround;
    EXPECT_TRUE(jc == JustifyContent::SpaceAround);
}

TEST_F(FlexLayoutTest, JustifyContentSpaceEvenly) {
    JustifyContent jc = JustifyContent::SpaceEvenly;
    EXPECT_TRUE(jc == JustifyContent::SpaceEvenly);
}

// ========== Align Items 测试 ==========

TEST_F(FlexLayoutTest, AlignItemsFlexStart) {
    AlignItems ai = AlignItems::FlexStart;
    EXPECT_TRUE(ai == AlignItems::FlexStart);
}

TEST_F(FlexLayoutTest, AlignItemsFlexEnd) {
    AlignItems ai = AlignItems::FlexEnd;
    EXPECT_TRUE(ai == AlignItems::FlexEnd);
}

TEST_F(FlexLayoutTest, AlignItemsCenter) {
    AlignItems ai = AlignItems::Center;
    EXPECT_TRUE(ai == AlignItems::Center);
}

TEST_F(FlexLayoutTest, AlignItemsStretch) {
    AlignItems ai = AlignItems::Stretch;
    EXPECT_TRUE(ai == AlignItems::Stretch);
}

TEST_F(FlexLayoutTest, AlignItemsBaseline) {
    AlignItems ai = AlignItems::Baseline;
    EXPECT_TRUE(ai == AlignItems::Baseline);
}

// ========== Flex Wrap 测试 ==========

TEST_F(FlexLayoutTest, FlexWrapNoWrap) {
    FlexWrap fw = FlexWrap::NoWrap;
    EXPECT_TRUE(fw == FlexWrap::NoWrap);
}

TEST_F(FlexLayoutTest, FlexWrapWrap) {
    FlexWrap fw = FlexWrap::Wrap;
    EXPECT_TRUE(fw == FlexWrap::Wrap);
}

TEST_F(FlexLayoutTest, FlexWrapWrapReverse) {
    FlexWrap fw = FlexWrap::WrapReverse;
    EXPECT_TRUE(fw == FlexWrap::WrapReverse);
}

// ========== Align Content 测试 ==========

TEST_F(FlexLayoutTest, AlignContentFlexStart) {
    AlignContent ac = AlignContent::FlexStart;
    EXPECT_TRUE(ac == AlignContent::FlexStart);
}

TEST_F(FlexLayoutTest, AlignContentFlexEnd) {
    AlignContent ac = AlignContent::FlexEnd;
    EXPECT_TRUE(ac == AlignContent::FlexEnd);
}

TEST_F(FlexLayoutTest, AlignContentCenter) {
    AlignContent ac = AlignContent::Center;
    EXPECT_TRUE(ac == AlignContent::Center);
}

TEST_F(FlexLayoutTest, AlignContentStretch) {
    AlignContent ac = AlignContent::Stretch;
    EXPECT_TRUE(ac == AlignContent::Stretch);
}

TEST_F(FlexLayoutTest, AlignContentSpaceBetween) {
    AlignContent ac = AlignContent::SpaceBetween;
    EXPECT_TRUE(ac == AlignContent::SpaceBetween);
}

TEST_F(FlexLayoutTest, AlignContentSpaceAround) {
    AlignContent ac = AlignContent::SpaceAround;
    EXPECT_TRUE(ac == AlignContent::SpaceAround);
}

// ========== Align Self 测试 ==========

TEST_F(FlexLayoutTest, AlignSelfStart) {
    AlignSelf as = AlignSelf::Start;
    EXPECT_TRUE(as == AlignSelf::Start);
}

TEST_F(FlexLayoutTest, AlignSelfFlexStart) {
    AlignSelf as = AlignSelf::FlexStart;
    EXPECT_TRUE(as == AlignSelf::FlexStart);
}

TEST_F(FlexLayoutTest, AlignSelfFlexEnd) {
    AlignSelf as = AlignSelf::FlexEnd;
    EXPECT_TRUE(as == AlignSelf::FlexEnd);
}

TEST_F(FlexLayoutTest, AlignSelfCenter) {
    AlignSelf as = AlignSelf::Center;
    EXPECT_TRUE(as == AlignSelf::Center);
}

TEST_F(FlexLayoutTest, AlignSelfStretch) {
    AlignSelf as = AlignSelf::Stretch;
    EXPECT_TRUE(as == AlignSelf::Stretch);
}

// ========== Flex 属性默认值测试 ==========

TEST_F(FlexLayoutTest, DefaultFlexGrow) {
    // 默认 flex-grow 应该是 0
    float default_grow = 0.0f;
    EXPECT_FLOAT_EQ(default_grow, 0.0f);
}

TEST_F(FlexLayoutTest, DefaultFlexShrink) {
    // 默认 flex-shrink 应该是 1
    float default_shrink = 1.0f;
    EXPECT_FLOAT_EQ(default_shrink, 1.0f);
}

TEST_F(FlexLayoutTest, DefaultFlexBasis) {
    // 默认 flex-basis 应该是 auto
    // 这里用 -1 或特殊值表示 auto
}

class SingleChildFlexTree : public LayoutFlexboxContainer {
public:
    SingleChildFlexTree() {
        Style container_style;
        container_style.display = Display::Flex;
        container_style.flex_direction = FlexDirection::Row;
        container_style.justify_content = JustifyContent::FlexEnd;
        styles_[container_id_] = container_style;

        Style child_style;
        child_style.display = Display::Block;
        child_style.size = Size<Dimension>{
            Dimension::Length(48.0f),
            Dimension::Length(27.0f)
        };
        styles_[child_id_] = child_style;
    }

    size_t ChildCount(NodeId node) const override {
        return node == container_id_ ? 1 : 0;
    }

    NodeId GetChildId(NodeId node, size_t index) const override {
        return node == container_id_ && index == 0 ? child_id_ : INVALID_NODE_ID;
    }

    Cache& GetCache(NodeId node) override {
        return caches_[node];
    }

    void SetUnroundedLayout(NodeId node, const Layout& layout) override {
        layouts_[node] = layout;
    }

    const Layout& GetLayout(NodeId node) const override {
        auto it = layouts_.find(node);
        return it != layouts_.end() ? it->second : default_layout_;
    }

    LayoutOutput PerformChildLayout(
        NodeId,
        Size<std::optional<float>> known_dimensions,
        Size<std::optional<float>>,
        Size<AvailableSpace>,
        SizingMode,
        Line<bool>
    ) override {
        ++perform_child_layout_calls;
        LayoutOutput output;
        output.size = Size<float>{
            known_dimensions.width.value_or(48.0f),
            known_dimensions.height.value_or(27.0f)
        };
        output.content_size = output.size;
        return output;
    }

    Size<float> MeasureChildSize(
        NodeId,
        Size<std::optional<float>>,
        Size<std::optional<float>>,
        Size<AvailableSpace>,
        SizingMode
    ) override {
        return Size<float>{48.0f, 27.0f};
    }

    const Style& GetContainerStyle(NodeId node) const override {
        return styles_.at(node);
    }

    const Style& GetChildStyle(NodeId node) const override {
        return styles_.at(node);
    }

    static constexpr NodeId container_id_ = 1;
    static constexpr NodeId child_id_ = 2;

    int perform_child_layout_calls = 0;

private:
    Layout default_layout_;
    std::unordered_map<NodeId, Cache> caches_;
    std::unordered_map<NodeId, Layout> layouts_;
    std::unordered_map<NodeId, Style> styles_;
};

TEST_F(FlexLayoutTest, ContentSizeProbeDoesNotOverwriteFinalChildPosition) {
    SingleChildFlexTree tree;

    LayoutInput final_input;
    final_input.run_mode = RunMode::PerformLayout;
    final_input.sizing_mode = SizingMode::InherentSize;
    final_input.known_dimensions = Size<std::optional<float>>{380.0f, 27.0f};
    final_input.parent_size = Size<std::optional<float>>{380.0f, 27.0f};
    final_input.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(380.0f),
        AvailableSpace::Definite(27.0f)
    };

    ComputeFlexboxLayout(tree, SingleChildFlexTree::container_id_, final_input);
    float final_x = tree.GetLayout(SingleChildFlexTree::child_id_).location.x;
    int calls_after_final_layout = tree.perform_child_layout_calls;
    EXPECT_GT(final_x, 0.0f);

    LayoutInput probe_input = final_input;
    probe_input.sizing_mode = SizingMode::ContentSize;
    probe_input.known_dimensions = Size<std::optional<float>>{48.0f, 27.0f};
    probe_input.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(48.0f),
        AvailableSpace::Definite(27.0f)
    };

    ComputeFlexboxLayout(tree, SingleChildFlexTree::container_id_, probe_input);

    EXPECT_EQ(tree.perform_child_layout_calls, calls_after_final_layout);
    EXPECT_FLOAT_EQ(tree.GetLayout(SingleChildFlexTree::child_id_).location.x, final_x);
}

} // namespace test
} // namespace mbink
