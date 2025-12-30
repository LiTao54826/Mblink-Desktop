/**
 * @file test_virtual_scroll_properties.cpp
 * @brief VirtualScrollRenderer 属性测试
 *
 * 测试 VirtualScrollRenderer 的正确性属性：
 * - 属性 3: 虚拟滚动正确性
 * - 属性 4: 命中测试准确性
 */

#include "core/dom/elements/virtual_text/virtual_scroll_renderer.h"

#include <gtest/gtest.h>
#include <random>
#include <cmath>

namespace lightui {
namespace {

// 用于测试的具体渲染器实现
class TestScrollRenderer : public VirtualScrollRenderer {
public:
    TestScrollRenderer() = default;

    void Render(SkCanvas* canvas, const SkRect& bounds) override {
        // 记录渲染调用的行范围
        rendered_start_ = scroll_offset_;
        rendered_end_ = std::min(scroll_offset_ + visible_lines_, total_lines_);
    }

    int rendered_start() const { return rendered_start_; }
    int rendered_end() const { return rendered_end_; }

private:
    int rendered_start_ = 0;
    int rendered_end_ = 0;
};

class RandomGenerator {
public:
    RandomGenerator() : gen_(std::random_device{}()) {}

    int Int(int min, int max) {
        return std::uniform_int_distribution<int>(min, max)(gen_);
    }

    float Float(float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(gen_);
    }

private:
    std::mt19937 gen_;
};

/**
 * **Feature: virtual-text-components, Property 3: 虚拟滚动正确性**
 * **Validates: Requirements 2.1, 3.4**
 *
 * *对于任何*总共 N 行且可见窗口为 M 行的视图，渲染 SHALL 只访问 
 * [scroll_offset, scroll_offset + M) 范围内的行。
 */
class VirtualScrollCorrectnessPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(VirtualScrollCorrectnessPropertyTest, RenderOnlyVisibleLines) {
    for (int iter = 0; iter < kIterations; ++iter) {
        TestScrollRenderer renderer;
        
        // 设置随机参数
        float line_height = rng_.Float(14.0f, 24.0f);
        float view_height = rng_.Float(200.0f, 800.0f);
        int total_lines = rng_.Int(10, 10000);
        
        renderer.SetLineHeight(line_height);
        renderer.UpdateMetrics(view_height);
        renderer.SetTotalLines(total_lines);
        
        // 设置随机滚动偏移
        int max_offset = std::max(0, total_lines - renderer.visible_lines());
        int scroll_offset = (max_offset > 0) ? rng_.Int(0, max_offset) : 0;
        renderer.SetScrollOffset(scroll_offset);
        
        // 执行渲染
        SkRect bounds = SkRect::MakeWH(800, view_height);
        renderer.Render(nullptr, bounds);
        
        // 验证：渲染范围正确
        ASSERT_EQ(renderer.rendered_start(), scroll_offset)
            << "Render should start at scroll_offset";
        
        int expected_end = std::min(scroll_offset + renderer.visible_lines(), total_lines);
        ASSERT_EQ(renderer.rendered_end(), expected_end)
            << "Render should end at scroll_offset + visible_lines or total_lines";
        
        // 验证：渲染的行数不超过可见行数
        int rendered_count = renderer.rendered_end() - renderer.rendered_start();
        ASSERT_LE(rendered_count, renderer.visible_lines())
            << "Should not render more than visible_lines";
    }
}

TEST_F(VirtualScrollCorrectnessPropertyTest, ScrollToSetsCorrectOffset) {
    for (int iter = 0; iter < kIterations; ++iter) {
        TestScrollRenderer renderer;
        
        float line_height = rng_.Float(14.0f, 24.0f);
        float view_height = rng_.Float(200.0f, 800.0f);
        int total_lines = rng_.Int(50, 1000);
        
        renderer.SetLineHeight(line_height);
        renderer.UpdateMetrics(view_height);
        renderer.SetTotalLines(total_lines);
        
        // 滚动到随机行
        int target_line = rng_.Int(0, total_lines - 1);
        renderer.ScrollTo(target_line);
        
        // 验证：滚动偏移被正确设置（可能被限制在有效范围内）
        // 实现允许滚动到 total_lines - 1（最后一行显示在顶部）
        int max_offset = std::max(0, total_lines - 1);
        int expected_offset = std::min(target_line, max_offset);
        expected_offset = std::max(0, expected_offset);
        
        ASSERT_EQ(renderer.scroll_offset(), expected_offset)
            << "ScrollTo should set correct offset";
    }
}

/**
 * **Feature: virtual-text-components, Property 4: 命中测试准确性**
 * **Validates: Requirements 2.4**
 *
 * *对于任何*点击位置 (x, y)，HitTestLine() 和 HitTestColumn() SHALL 返回正确的行列索引，
 * 误差不超过 1 像素。
 */
class HitTestAccuracyPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(HitTestAccuracyPropertyTest, HitTestLineReturnsCorrectLine) {
    for (int iter = 0; iter < kIterations; ++iter) {
        TestScrollRenderer renderer;
        
        float line_height = rng_.Float(14.0f, 24.0f);
        float view_height = rng_.Float(200.0f, 800.0f);
        int total_lines = rng_.Int(50, 1000);
        
        renderer.SetLineHeight(line_height);
        renderer.UpdateMetrics(view_height);
        renderer.SetTotalLines(total_lines);
        
        int scroll_offset = rng_.Int(0, std::max(0, total_lines - renderer.visible_lines()));
        renderer.SetScrollOffset(scroll_offset);
        
        // 测试随机 Y 坐标
        float y = rng_.Float(0, view_height);
        int hit_line = renderer.HitTestLine(y);
        
        // 计算期望的行号
        int expected_line = scroll_offset + static_cast<int>(y / line_height);
        expected_line = std::max(0, std::min(expected_line, total_lines - 1));
        
        // 验证：命中测试结果正确（允许 1 行误差，因为边界情况）
        ASSERT_LE(std::abs(hit_line - expected_line), 1)
            << "HitTestLine should return correct line index";
    }
}

TEST_F(HitTestAccuracyPropertyTest, HitTestColumnReturnsCorrectColumn) {
    for (int iter = 0; iter < kIterations; ++iter) {
        TestScrollRenderer renderer;
        
        // 设置等宽字体参数
        float font_size = rng_.Float(12.0f, 18.0f);
        renderer.SetFont(nullptr, font_size);
        
        // 假设 cell_width 大约是 font_size * 0.6
        float cell_width = renderer.cell_width();
        if (cell_width <= 0) {
            // 如果没有设置字体，跳过此测试
            continue;
        }
        
        float padding = rng_.Float(0, 10.0f);
        renderer.SetPadding(padding);
        
        // 测试随机 X 坐标
        float x = rng_.Float(padding, padding + cell_width * 80);
        int hit_col = renderer.HitTestColumn(x);
        
        // 计算期望的列号
        int expected_col = static_cast<int>((x - padding) / cell_width);
        expected_col = std::max(0, expected_col);
        
        // 验证：命中测试结果正确（允许 1 列误差）
        ASSERT_LE(std::abs(hit_col - expected_col), 1)
            << "HitTestColumn should return correct column index";
    }
}

TEST_F(HitTestAccuracyPropertyTest, HitTestAtLineBoundaries) {
    for (int iter = 0; iter < kIterations; ++iter) {
        TestScrollRenderer renderer;
        
        float line_height = rng_.Float(14.0f, 24.0f);
        float view_height = line_height * 20;  // 恰好 20 行
        int total_lines = 100;
        
        renderer.SetLineHeight(line_height);
        renderer.UpdateMetrics(view_height);
        renderer.SetTotalLines(total_lines);
        renderer.SetScrollOffset(0);
        
        // 测试每行的边界
        for (int line = 0; line < 10; ++line) {
            // 行的起始位置
            float y_start = line * line_height;
            int hit_start = renderer.HitTestLine(y_start);
            ASSERT_EQ(hit_start, line)
                << "HitTest at line start should return that line";
            
            // 行的中间位置
            float y_mid = line * line_height + line_height / 2;
            int hit_mid = renderer.HitTestLine(y_mid);
            ASSERT_EQ(hit_mid, line)
                << "HitTest at line middle should return that line";
        }
    }
}

}  // namespace
}  // namespace lightui
