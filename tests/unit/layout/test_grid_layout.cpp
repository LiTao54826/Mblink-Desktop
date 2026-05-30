/**
 * @file test_grid_layout.cpp
 * @brief CSS Grid 布局算法单元测试
 * 
 * 测试 Grid 布局的核心类型和功能，验证 Style 统一重构后 Grid 布局正常工作。
 * 
 * **Feature: style-unification**
 * **Validates: Requirements 6.1**
 */

#include <gtest/gtest.h>
#include "layout/grid/grid.h"
#include "layout/grid/types.h"
#include "layout/types/style.h"
#include "layout/types/geometry.h"
#include <unordered_map>

namespace mbink {
namespace test {

class GridLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
};

// ========== Display Grid 类型测试 ==========

TEST_F(GridLayoutTest, DisplayGrid) {
    Display d = Display::Grid;
    EXPECT_TRUE(d == Display::Grid);
}

// ========== Grid Auto Flow 测试 ==========

TEST_F(GridLayoutTest, GridAutoFlowRow) {
    GridContainerStyle::GridAutoFlow flow = GridContainerStyle::GridAutoFlow::Row;
    EXPECT_TRUE(flow == GridContainerStyle::GridAutoFlow::Row);
}

TEST_F(GridLayoutTest, GridAutoFlowColumn) {
    GridContainerStyle::GridAutoFlow flow = GridContainerStyle::GridAutoFlow::Column;
    EXPECT_TRUE(flow == GridContainerStyle::GridAutoFlow::Column);
}

TEST_F(GridLayoutTest, GridAutoFlowRowDense) {
    GridContainerStyle::GridAutoFlow flow = GridContainerStyle::GridAutoFlow::RowDense;
    EXPECT_TRUE(flow == GridContainerStyle::GridAutoFlow::RowDense);
}

TEST_F(GridLayoutTest, GridAutoFlowColumnDense) {
    GridContainerStyle::GridAutoFlow flow = GridContainerStyle::GridAutoFlow::ColumnDense;
    EXPECT_TRUE(flow == GridContainerStyle::GridAutoFlow::ColumnDense);
}

// ========== Grid Placement 测试 ==========

TEST_F(GridLayoutTest, GridPlacementAuto) {
    GridPlacement placement = GridPlacement::Auto();
    EXPECT_TRUE(placement.IsAuto());
    EXPECT_FALSE(placement.IsLine());
    EXPECT_FALSE(placement.IsSpan());
}

TEST_F(GridLayoutTest, GridPlacementLine) {
    GridPlacement placement = GridPlacement::Line(2);
    EXPECT_FALSE(placement.IsAuto());
    EXPECT_TRUE(placement.IsLine());
    EXPECT_FALSE(placement.IsSpan());
    EXPECT_EQ(placement.value, 2);
}

TEST_F(GridLayoutTest, GridPlacementLineNegative) {
    GridPlacement placement = GridPlacement::Line(-1);
    EXPECT_TRUE(placement.IsLine());
    EXPECT_EQ(placement.value, -1);
}

TEST_F(GridLayoutTest, GridPlacementSpan) {
    GridPlacement placement = GridPlacement::Span(3);
    EXPECT_FALSE(placement.IsAuto());
    EXPECT_FALSE(placement.IsLine());
    EXPECT_TRUE(placement.IsSpan());
    EXPECT_EQ(placement.value, 3);
}

// ========== Min Track Sizing Function 测试 ==========

TEST_F(GridLayoutTest, MinTrackSizingFunctionFixed) {
    MinTrackSizingFunction func = MinTrackSizingFunction::Fixed(100.0f);
    EXPECT_EQ(func.type, MinTrackSizingFunctionType::Fixed);
    EXPECT_FLOAT_EQ(func.value, 100.0f);
    EXPECT_FALSE(func.is_percent);
    EXPECT_FALSE(func.IsIntrinsic());
}

TEST_F(GridLayoutTest, MinTrackSizingFunctionPercent) {
    MinTrackSizingFunction func = MinTrackSizingFunction::Percent(0.5f);
    EXPECT_EQ(func.type, MinTrackSizingFunctionType::Fixed);
    EXPECT_FLOAT_EQ(func.value, 0.5f);
    EXPECT_TRUE(func.is_percent);
    EXPECT_TRUE(func.UsesPercentage());
}

TEST_F(GridLayoutTest, MinTrackSizingFunctionMinContent) {
    MinTrackSizingFunction func = MinTrackSizingFunction::MinContent();
    EXPECT_EQ(func.type, MinTrackSizingFunctionType::MinContent);
    EXPECT_TRUE(func.IsIntrinsic());
}

TEST_F(GridLayoutTest, MinTrackSizingFunctionMaxContent) {
    MinTrackSizingFunction func = MinTrackSizingFunction::MaxContent();
    EXPECT_EQ(func.type, MinTrackSizingFunctionType::MaxContent);
    EXPECT_TRUE(func.IsIntrinsic());
}

TEST_F(GridLayoutTest, MinTrackSizingFunctionAuto) {
    MinTrackSizingFunction func = MinTrackSizingFunction::Auto();
    EXPECT_EQ(func.type, MinTrackSizingFunctionType::Auto);
    EXPECT_TRUE(func.IsIntrinsic());
}

// ========== Max Track Sizing Function 测试 ==========

TEST_F(GridLayoutTest, MaxTrackSizingFunctionFixed) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::Fixed(200.0f);
    EXPECT_EQ(func.type, MaxTrackSizingFunctionType::Fixed);
    EXPECT_FLOAT_EQ(func.value, 200.0f);
    EXPECT_FALSE(func.is_percent);
    EXPECT_FALSE(func.IsFr());
}

TEST_F(GridLayoutTest, MaxTrackSizingFunctionPercent) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::Percent(0.75f);
    EXPECT_EQ(func.type, MaxTrackSizingFunctionType::Fixed);
    EXPECT_FLOAT_EQ(func.value, 0.75f);
    EXPECT_TRUE(func.is_percent);
    EXPECT_TRUE(func.UsesPercentage());
}

TEST_F(GridLayoutTest, MaxTrackSizingFunctionFraction) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::Fraction(1.0f);
    EXPECT_EQ(func.type, MaxTrackSizingFunctionType::Fraction);
    EXPECT_FLOAT_EQ(func.value, 1.0f);
    EXPECT_TRUE(func.IsFr());
}

TEST_F(GridLayoutTest, MaxTrackSizingFunctionFitContent) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::FitContentPx(300.0f);
    EXPECT_EQ(func.type, MaxTrackSizingFunctionType::FitContent);
    EXPECT_FLOAT_EQ(func.value, 300.0f);
    EXPECT_TRUE(func.IsIntrinsic());
}

TEST_F(GridLayoutTest, MaxTrackSizingFunctionAuto) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::Auto();
    EXPECT_EQ(func.type, MaxTrackSizingFunctionType::Auto);
    EXPECT_TRUE(func.IsIntrinsic());
}

// ========== Non-Repeated Track Sizing Function 测试 ==========

TEST_F(GridLayoutTest, NonRepeatedTrackSizingFunctionAuto) {
    NonRepeatedTrackSizingFunction func = NonRepeatedTrackSizingFunction::Auto();
    EXPECT_EQ(func.min.type, MinTrackSizingFunctionType::Auto);
    EXPECT_EQ(func.max.type, MaxTrackSizingFunctionType::Auto);
}

TEST_F(GridLayoutTest, NonRepeatedTrackSizingFunctionFixed) {
    NonRepeatedTrackSizingFunction func = NonRepeatedTrackSizingFunction::Fixed(150.0f);
    EXPECT_EQ(func.min.type, MinTrackSizingFunctionType::Fixed);
    EXPECT_EQ(func.max.type, MaxTrackSizingFunctionType::Fixed);
    EXPECT_FLOAT_EQ(func.min.value, 150.0f);
    EXPECT_FLOAT_EQ(func.max.value, 150.0f);
}

TEST_F(GridLayoutTest, NonRepeatedTrackSizingFunctionFlex) {
    NonRepeatedTrackSizingFunction func = NonRepeatedTrackSizingFunction::Flex(2.0f);
    EXPECT_EQ(func.min.type, MinTrackSizingFunctionType::Auto);
    EXPECT_EQ(func.max.type, MaxTrackSizingFunctionType::Fraction);
    EXPECT_FLOAT_EQ(func.max.value, 2.0f);
}

class CountingGridTree : public LayoutGridContainer {
public:
    explicit CountingGridTree(const GridContainerStyle& grid_style) : grid_style_(grid_style) {
        Style root_style;
        root_style.display = Display::Grid;
        root_style.size = Size<Dimension>{
            Dimension::Length(400.0f),
            Dimension::Length(200.0f)
        };
        styles_[root_id_] = root_style;

        Style child_style;
        child_style.display = Display::Block;
        styles_[child_id_] = child_style;
        child_styles_[child_id_] = GridItemStyle{};
    }

    size_t ChildCount(NodeId node) const override {
        return node == root_id_ ? 1 : 0;
    }

    NodeId GetChildId(NodeId node, size_t index) const override {
        return node == root_id_ && index == 0 ? child_id_ : INVALID_NODE_ID;
    }

    Cache& GetCache(NodeId node) override {
        return caches_[node];
    }

    void SetUnroundedLayout(NodeId node, const Layout& layout) override {
        set_unrounded_layout_calls++;
        last_set_layout_node = node;
        last_set_layout = layout;
        layouts_[node] = layout;
    }

    const Layout& GetLayout(NodeId node) const override {
        auto it = layouts_.find(node);
        return it != layouts_.end() ? it->second : default_layout_;
    }

    LayoutOutput PerformChildLayout(
        NodeId,
        Size<std::optional<float>>,
        Size<std::optional<float>>,
        Size<AvailableSpace> available_space,
        SizingMode,
        Line<bool>
    ) override {
        perform_child_layout_calls++;
        if (available_space.width.IsMinContent()) min_content_width_calls++;
        if (available_space.width.IsMaxContent()) max_content_width_calls++;
        if (available_space.height.IsMinContent()) min_content_height_calls++;
        if (available_space.height.IsMaxContent()) max_content_height_calls++;
        LayoutOutput output;
        output.size = Size<float>{120.0f, 40.0f};
        output.content_size = output.size;
        return output;
    }

    Size<float> MeasureChildSize(
        NodeId,
        Size<std::optional<float>>,
        Size<std::optional<float>>,
        Size<AvailableSpace> available_space,
        SizingMode
    ) override {
        measure_child_size_calls++;
        if (available_space.width.IsMinContent()) min_content_width_calls++;
        if (available_space.width.IsMaxContent()) max_content_width_calls++;
        if (available_space.height.IsMinContent()) min_content_height_calls++;
        if (available_space.height.IsMaxContent()) max_content_height_calls++;
        return Size<float>{120.0f, 40.0f};
    }

    const Style& GetContainerStyle(NodeId node) const override {
        return styles_.at(node);
    }

    const Style& GetChildStyle(NodeId node) const override {
        return styles_.at(node);
    }

    const GridContainerStyle& GetGridContainerStyle(NodeId node) const override {
        return node == root_id_ ? grid_style_ : default_grid_style_;
    }

    const GridItemStyle& GetGridItemStyle(NodeId node) const override {
        auto it = child_styles_.find(node);
        return it != child_styles_.end() ? it->second : default_grid_item_style_;
    }

    bool IsTextNode(NodeId) const override {
        return false;
    }

    int perform_child_layout_calls = 0;
    int measure_child_size_calls = 0;
    int set_unrounded_layout_calls = 0;
    int min_content_width_calls = 0;
    int max_content_width_calls = 0;
    int min_content_height_calls = 0;
    int max_content_height_calls = 0;
    NodeId last_set_layout_node = INVALID_NODE_ID;
    Layout last_set_layout;

private:
    static constexpr NodeId root_id_ = 1;
    static constexpr NodeId child_id_ = 2;

    GridContainerStyle grid_style_;
    GridContainerStyle default_grid_style_;
    GridItemStyle default_grid_item_style_;
    Layout default_layout_;
    std::unordered_map<NodeId, Style> styles_;
    std::unordered_map<NodeId, GridItemStyle> child_styles_;
    std::unordered_map<NodeId, Cache> caches_;
    std::unordered_map<NodeId, Layout> layouts_;
};

TEST_F(GridLayoutTest, FixedMinmaxFlexTracksSkipIntrinsicProbeLayouts) {
    GridContainerStyle grid_style;
    grid_style.grid_template_columns.push_back(TrackSizingFunction::Repeat(
        UINT16_MAX,
        {NonRepeatedTrackSizingFunction::MinMax(
            MinTrackSizingFunction::Fixed(150.0f),
            MaxTrackSizingFunction::Fraction(1.0f)
        )}
    ));
    grid_style.grid_auto_rows.push_back(NonRepeatedTrackSizingFunction::MinMax(
        MinTrackSizingFunction::Fixed(104.0f),
        MaxTrackSizingFunction::Auto()
    ));

    CountingGridTree tree(grid_style);
    LayoutInput input;
    input.run_mode = RunMode::PerformLayout;
    input.known_dimensions = Size<std::optional<float>>{400.0f, 200.0f};
    input.parent_size = Size<std::optional<float>>{400.0f, 200.0f};
    input.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(400.0f),
        AvailableSpace::Definite(200.0f)
    };

    ComputeGridLayout(tree, 1, input);

    EXPECT_EQ(tree.measure_child_size_calls, 1);
    EXPECT_EQ(tree.min_content_width_calls, 0);
    EXPECT_EQ(tree.max_content_width_calls, 0);
    EXPECT_EQ(tree.min_content_height_calls, 0);
}

TEST_F(GridLayoutTest, PerformLayoutCommitsGridItemLayout) {
    GridContainerStyle grid_style;
    grid_style.grid_template_columns.push_back(TrackSizingFunction::Single(
        NonRepeatedTrackSizingFunction::Flex(1.0f)
    ));

    CountingGridTree tree(grid_style);
    LayoutInput input;
    input.run_mode = RunMode::PerformLayout;
    input.sizing_mode = SizingMode::InherentSize;
    input.known_dimensions = Size<std::optional<float>>{260.0f, 100.0f};
    input.parent_size = Size<std::optional<float>>{260.0f, 100.0f};
    input.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(260.0f),
        AvailableSpace::Definite(100.0f)
    };

    ComputeGridLayout(tree, 1, input);

    EXPECT_EQ(tree.measure_child_size_calls, 1);
    EXPECT_EQ(tree.perform_child_layout_calls, 1);
    EXPECT_EQ(tree.set_unrounded_layout_calls, 1);
    EXPECT_EQ(tree.last_set_layout_node, 2);
    EXPECT_FLOAT_EQ(tree.last_set_layout.size.width, 260.0f);
}

TEST_F(GridLayoutTest, ContentSizeProbeDoesNotCommitGridItemLayout) {
    GridContainerStyle grid_style;
    grid_style.grid_template_columns.push_back(TrackSizingFunction::Single(
        NonRepeatedTrackSizingFunction::Flex(1.0f)
    ));

    CountingGridTree tree(grid_style);
    LayoutInput input;
    input.run_mode = RunMode::PerformLayout;
    input.sizing_mode = SizingMode::ContentSize;
    input.known_dimensions = Size<std::optional<float>>{260.0f, 100.0f};
    input.parent_size = Size<std::optional<float>>{260.0f, 100.0f};
    input.available_space = Size<AvailableSpace>{
        AvailableSpace::Definite(260.0f),
        AvailableSpace::Definite(100.0f)
    };

    ComputeGridLayout(tree, 1, input);

    EXPECT_EQ(tree.measure_child_size_calls, 1);
    EXPECT_EQ(tree.perform_child_layout_calls, 0);
    EXPECT_EQ(tree.set_unrounded_layout_calls, 0);
    EXPECT_EQ(tree.last_set_layout_node, INVALID_NODE_ID);
}

TEST_F(GridLayoutTest, NonRepeatedTrackSizingFunctionMinMax) {
    NonRepeatedTrackSizingFunction func = NonRepeatedTrackSizingFunction::MinMax(
        MinTrackSizingFunction::Fixed(50.0f),
        MaxTrackSizingFunction::Fraction(1.0f)
    );
    EXPECT_EQ(func.min.type, MinTrackSizingFunctionType::Fixed);
    EXPECT_EQ(func.max.type, MaxTrackSizingFunctionType::Fraction);
    EXPECT_FLOAT_EQ(func.min.value, 50.0f);
    EXPECT_FLOAT_EQ(func.max.value, 1.0f);
}

// ========== Track Sizing Function 测试 ==========

TEST_F(GridLayoutTest, TrackSizingFunctionSingle) {
    TrackSizingFunction tsf = TrackSizingFunction::Single(
        NonRepeatedTrackSizingFunction::Fixed(100.0f)
    );
    EXPECT_EQ(tsf.type, TrackSizingFunction::Type::Single);
    EXPECT_FLOAT_EQ(tsf.single.min.value, 100.0f);
}

TEST_F(GridLayoutTest, TrackSizingFunctionRepeat) {
    std::vector<NonRepeatedTrackSizingFunction> tracks = {
        NonRepeatedTrackSizingFunction::Fixed(100.0f),
        NonRepeatedTrackSizingFunction::Flex(1.0f)
    };
    TrackSizingFunction tsf = TrackSizingFunction::Repeat(3, tracks);
    EXPECT_EQ(tsf.type, TrackSizingFunction::Type::Repeat);
    EXPECT_EQ(tsf.repeat_count, 3);
    EXPECT_EQ(tsf.repeat_tracks.size(), 2);
}

// ========== Grid Track 测试 ==========

TEST_F(GridLayoutTest, GridTrackNew) {
    GridTrack track = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(200.0f)
    );
    EXPECT_EQ(track.kind, GridTrackKind::Track);
    EXPECT_FALSE(track.is_collapsed);
    EXPECT_EQ(track.min_track_sizing_function.type, MinTrackSizingFunctionType::Fixed);
    EXPECT_EQ(track.max_track_sizing_function.type, MaxTrackSizingFunctionType::Fixed);
}

TEST_F(GridLayoutTest, GridTrackGutter) {
    GridTrack track = GridTrack::Gutter(10.0f);
    EXPECT_EQ(track.kind, GridTrackKind::Gutter);
    EXPECT_FLOAT_EQ(track.min_track_sizing_function.value, 10.0f);
    EXPECT_FLOAT_EQ(track.max_track_sizing_function.value, 10.0f);
}

TEST_F(GridLayoutTest, GridTrackIsFlexible) {
    GridTrack flexTrack = GridTrack::New(
        MinTrackSizingFunction::Auto(),
        MaxTrackSizingFunction::Fraction(1.0f)
    );
    EXPECT_TRUE(flexTrack.IsFlexible());
    
    GridTrack fixedTrack = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(100.0f)
    );
    EXPECT_FALSE(fixedTrack.IsFlexible());
}

TEST_F(GridLayoutTest, GridTrackFlexFactor) {
    GridTrack track = GridTrack::New(
        MinTrackSizingFunction::Auto(),
        MaxTrackSizingFunction::Fraction(2.5f)
    );
    EXPECT_FLOAT_EQ(track.FlexFactor(), 2.5f);
    
    GridTrack nonFlexTrack = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(100.0f)
    );
    EXPECT_FLOAT_EQ(nonFlexTrack.FlexFactor(), 0.0f);
}

TEST_F(GridLayoutTest, GridTrackCollapse) {
    GridTrack track = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(200.0f)
    );
    EXPECT_FALSE(track.is_collapsed);
    
    track.Collapse();
    EXPECT_TRUE(track.is_collapsed);
    EXPECT_FLOAT_EQ(track.min_track_sizing_function.value, 0.0f);
    EXPECT_FLOAT_EQ(track.max_track_sizing_function.value, 0.0f);
}

TEST_F(GridLayoutTest, GridTrackUsesPercentage) {
    GridTrack percentTrack = GridTrack::New(
        MinTrackSizingFunction::Percent(0.5f),
        MaxTrackSizingFunction::Auto()
    );
    EXPECT_TRUE(percentTrack.UsesPercentage());
    
    GridTrack fixedTrack = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(100.0f)
    );
    EXPECT_FALSE(fixedTrack.UsesPercentage());
}

TEST_F(GridLayoutTest, GridTrackHasIntrinsicSizingFunction) {
    GridTrack autoTrack = GridTrack::New(
        MinTrackSizingFunction::Auto(),
        MaxTrackSizingFunction::Auto()
    );
    EXPECT_TRUE(autoTrack.HasIntrinsicSizingFunction());
    
    GridTrack fixedTrack = GridTrack::New(
        MinTrackSizingFunction::Fixed(100.0f),
        MaxTrackSizingFunction::Fixed(100.0f)
    );
    EXPECT_FALSE(fixedTrack.HasIntrinsicSizingFunction());
}

// ========== Grid Line 坐标转换测试 ==========

TEST_F(GridLayoutTest, GridLinePositiveToOriginZero) {
    // Line 1 (first line) should become 0 in origin-zero coordinates
    GridLine line1(1);
    OriginZeroLine oz1 = line1.IntoOriginZeroLine(3);  // 3 explicit tracks
    EXPECT_EQ(oz1.value, 0);
    
    // Line 2 should become 1
    GridLine line2(2);
    OriginZeroLine oz2 = line2.IntoOriginZeroLine(3);
    EXPECT_EQ(oz2.value, 1);
    
    // Line 4 (last line for 3 tracks) should become 3
    GridLine line4(4);
    OriginZeroLine oz4 = line4.IntoOriginZeroLine(3);
    EXPECT_EQ(oz4.value, 3);
}

TEST_F(GridLayoutTest, GridLineNegativeToOriginZero) {
    // Line -1 (last line) for 3 tracks should become 3
    GridLine lineNeg1(-1);
    OriginZeroLine ozNeg1 = lineNeg1.IntoOriginZeroLine(3);
    EXPECT_EQ(ozNeg1.value, 3);
    
    // Line -2 for 3 tracks should become 2
    GridLine lineNeg2(-2);
    OriginZeroLine ozNeg2 = lineNeg2.IntoOriginZeroLine(3);
    EXPECT_EQ(ozNeg2.value, 2);
}

// ========== Origin Zero Line 算术测试 ==========

TEST_F(GridLayoutTest, OriginZeroLineArithmetic) {
    OriginZeroLine a(5);
    OriginZeroLine b(3);
    
    OriginZeroLine sum = a + b;
    EXPECT_EQ(sum.value, 8);
    
    OriginZeroLine diff = a - b;
    EXPECT_EQ(diff.value, 2);
    
    OriginZeroLine plusInt = a + static_cast<uint16_t>(2);
    EXPECT_EQ(plusInt.value, 7);
    
    OriginZeroLine minusInt = a - static_cast<uint16_t>(2);
    EXPECT_EQ(minusInt.value, 3);
}

TEST_F(GridLayoutTest, OriginZeroLineComparison) {
    OriginZeroLine a(5);
    OriginZeroLine b(3);
    OriginZeroLine c(5);
    
    EXPECT_TRUE(a > b);
    EXPECT_TRUE(b < a);
    EXPECT_TRUE(a >= c);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
}

// ========== Line Span 测试 ==========

TEST_F(GridLayoutTest, LineSpanCalculation) {
    Line<OriginZeroLine> line{OriginZeroLine(1), OriginZeroLine(4)};
    uint16_t span = LineSpan(line);
    EXPECT_EQ(span, 3);
}

TEST_F(GridLayoutTest, LineSpanZero) {
    Line<OriginZeroLine> line{OriginZeroLine(2), OriginZeroLine(2)};
    uint16_t span = LineSpan(line);
    EXPECT_EQ(span, 0);
}

// ========== Track Counts 测试 ==========

TEST_F(GridLayoutTest, TrackCountsTotal) {
    TrackCounts counts{2, 5, 3};  // 2 negative implicit, 5 explicit, 3 positive implicit
    EXPECT_EQ(counts.Total(), 10);
}

// ========== Origin Zero Line Track Vec Index 测试 ==========

TEST_F(GridLayoutTest, OriginZeroLineToTrackVecIndex) {
    TrackCounts counts{1, 3, 1};  // 1 negative implicit, 3 explicit, 1 positive implicit
    
    // Line 0 (first explicit line) with 1 negative implicit track
    // Should be at index 2 (after negative track and its gutter)
    OriginZeroLine line0(0);
    auto idx0 = line0.TryIntoTrackVecIndex(counts);
    EXPECT_TRUE(idx0.has_value());
    EXPECT_EQ(*idx0, 2);  // 2 * (0 + 1) = 2
    
    // Line 1 should be at index 4
    OriginZeroLine line1(1);
    auto idx1 = line1.TryIntoTrackVecIndex(counts);
    EXPECT_TRUE(idx1.has_value());
    EXPECT_EQ(*idx1, 4);  // 2 * (1 + 1) = 4
}

TEST_F(GridLayoutTest, OriginZeroLineImpliedTracks) {
    OriginZeroLine negLine(-2);
    EXPECT_EQ(negLine.ImpliedNegativeImplicitTracks(), 2);
    
    OriginZeroLine posLine(5);
    EXPECT_EQ(posLine.ImpliedPositiveImplicitTracks(3), 2);  // 5 - 3 = 2
    
    OriginZeroLine withinExplicit(2);
    EXPECT_EQ(withinExplicit.ImpliedPositiveImplicitTracks(3), 0);  // 2 <= 3
}

// ========== Grid Container Style 默认值测试 ==========

TEST_F(GridLayoutTest, GridContainerStyleDefaults) {
    GridContainerStyle style;
    
    EXPECT_EQ(style.grid_auto_flow, GridContainerStyle::GridAutoFlow::Row);
    EXPECT_TRUE(style.grid_template_rows.empty());
    EXPECT_TRUE(style.grid_template_columns.empty());
    EXPECT_TRUE(style.grid_auto_rows.empty());
    EXPECT_TRUE(style.grid_auto_columns.empty());
    EXPECT_FALSE(style.align_items.has_value());
    EXPECT_FALSE(style.justify_items.has_value());
    EXPECT_FALSE(style.align_content.has_value());
    EXPECT_FALSE(style.justify_content.has_value());
}

// ========== Grid Item Style 默认值测试 ==========

TEST_F(GridLayoutTest, GridItemStyleDefaults) {
    GridItemStyle style;
    
    EXPECT_TRUE(style.grid_row_start.IsAuto());
    EXPECT_TRUE(style.grid_row_end.IsAuto());
    EXPECT_TRUE(style.grid_column_start.IsAuto());
    EXPECT_TRUE(style.grid_column_end.IsAuto());
    EXPECT_FALSE(style.align_self.has_value());
    EXPECT_FALSE(style.justify_self.has_value());
}

// ========== Grid Item 测试 ==========

TEST_F(GridLayoutTest, GridItemPlacement) {
    GridItem item;
    item.row = Line<OriginZeroLine>{OriginZeroLine(0), OriginZeroLine(2)};
    item.column = Line<OriginZeroLine>{OriginZeroLine(1), OriginZeroLine(3)};
    
    // Test Placement method
    auto rowPlacement = item.Placement(AbsoluteAxis::Vertical);
    EXPECT_EQ(rowPlacement.start.value, 0);
    EXPECT_EQ(rowPlacement.end.value, 2);
    
    auto colPlacement = item.Placement(AbsoluteAxis::Horizontal);
    EXPECT_EQ(colPlacement.start.value, 1);
    EXPECT_EQ(colPlacement.end.value, 3);
}

TEST_F(GridLayoutTest, GridItemSpan) {
    GridItem item;
    item.row = Line<OriginZeroLine>{OriginZeroLine(0), OriginZeroLine(3)};
    item.column = Line<OriginZeroLine>{OriginZeroLine(1), OriginZeroLine(2)};
    
    EXPECT_EQ(item.Span(AbsoluteAxis::Vertical), 3);
    EXPECT_EQ(item.Span(AbsoluteAxis::Horizontal), 1);
}

TEST_F(GridLayoutTest, GridItemCrossesFlexibleTrack) {
    GridItem item;
    item.crosses_flexible_row = true;
    item.crosses_flexible_column = false;
    
    EXPECT_TRUE(item.CrossesFlexibleTrack(AbsoluteAxis::Vertical));
    EXPECT_FALSE(item.CrossesFlexibleTrack(AbsoluteAxis::Horizontal));
}

TEST_F(GridLayoutTest, GridItemCrossesIntrinsicTrack) {
    GridItem item;
    item.crosses_intrinsic_row = false;
    item.crosses_intrinsic_column = true;
    
    EXPECT_FALSE(item.CrossesIntrinsicTrack(AbsoluteAxis::Vertical));
    EXPECT_TRUE(item.CrossesIntrinsicTrack(AbsoluteAxis::Horizontal));
}

// ========== Fit Content Limit 测试 ==========

TEST_F(GridLayoutTest, FitContentLimitWithDefiniteSpace) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::FitContentPx(200.0f);
    float limit = func.FitContentLimit(std::optional<float>(500.0f));
    EXPECT_FLOAT_EQ(limit, 200.0f);
}

TEST_F(GridLayoutTest, FitContentLimitWithPercentage) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::FitContentPercent(0.5f);
    float limit = func.FitContentLimit(std::optional<float>(400.0f));
    EXPECT_FLOAT_EQ(limit, 200.0f);  // 50% of 400
}

TEST_F(GridLayoutTest, FitContentLimitWithoutSpace) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::FitContentPercent(0.5f);
    float limit = func.FitContentLimit(std::nullopt);
    EXPECT_TRUE(std::isinf(limit));
}

TEST_F(GridLayoutTest, NonFitContentReturnsInfinity) {
    MaxTrackSizingFunction func = MaxTrackSizingFunction::Auto();
    float limit = func.FitContentLimit(std::optional<float>(500.0f));
    EXPECT_TRUE(std::isinf(limit));
}

} // namespace test
} // namespace mbink
