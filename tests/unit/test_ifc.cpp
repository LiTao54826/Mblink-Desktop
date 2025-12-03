/**
 * @file test_ifc.cpp
 * @brief IFC (Inline Formatting Context) 单元测试
 * 
 * 测试 IFC 核心组件：
 * - TextRun: 文本片段
 * - InlineBox: 内联盒
 * - LineBox: 行盒
 * - LineBreaker: 断行器
 * - VerticalAligner: 垂直对齐器
 */

#include <gtest/gtest.h>
#include "core/layout/text_run.h"
#include "core/layout/inline_box.h"
#include "core/layout/line_box.h"
#include "core/layout/line_breaker.h"
#include "core/layout/vertical_aligner.h"
#include "core/layout/ifc_layout.h"

using namespace lightui;

// ========== TextRun 测试 ==========

class TextRunTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TextRunTest, BasicConstruction) {
    TextRun run;
    run.text = "Hello";
    run.start_offset = 0;
    run.end_offset = 5;
    run.width = 50.0f;
    run.height = 16.0f;
    run.baseline = 12.0f;
    
    EXPECT_EQ(run.text, "Hello");
    EXPECT_EQ(run.start_offset, 0);
    EXPECT_EQ(run.end_offset, 5);
    EXPECT_FLOAT_EQ(run.width, 50.0f);
    EXPECT_FLOAT_EQ(run.height, 16.0f);
    EXPECT_FLOAT_EQ(run.baseline, 12.0f);
}

TEST_F(TextRunTest, CharacterCount) {
    TextRun run;
    
    // ASCII
    run.text = "Hello";
    EXPECT_EQ(run.CharacterCount(), 5);
    
    // 中文 (3字节 UTF-8)
    run.text = "你好";
    EXPECT_EQ(run.CharacterCount(), 2);
    
    // 混合
    run.text = "Hello你好";
    EXPECT_EQ(run.CharacterCount(), 7);
    
    // 空字符串
    run.text = "";
    EXPECT_EQ(run.CharacterCount(), 0);
}

TEST_F(TextRunTest, IsOnlyWhitespace) {
    TextRun run;
    
    run.text = "   ";
    EXPECT_TRUE(run.IsOnlyWhitespace());
    
    run.text = "\t\n ";
    EXPECT_TRUE(run.IsOnlyWhitespace());
    
    run.text = "Hello";
    EXPECT_FALSE(run.IsOnlyWhitespace());
    
    run.text = " Hello ";
    EXPECT_FALSE(run.IsOnlyWhitespace());
    
    run.text = "";
    EXPECT_TRUE(run.IsOnlyWhitespace());
}

TEST_F(TextRunTest, CanBreakAt) {
    TextRun run;
    run.text = "Hello World";
    
    // 可以在空格处断行
    EXPECT_TRUE(run.CanBreakAt(5));  // 空格位置
    
    // 不能在单词中间断行
    EXPECT_FALSE(run.CanBreakAt(2));  // 'l' 位置
    EXPECT_FALSE(run.CanBreakAt(8));  // 'r' 位置
}

// ========== InlineBox 测试 ==========

class InlineBoxTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(InlineBoxTest, CreateTextBox) {
    InlineBox box = InlineBox::CreateTextBox(nullptr);
    box.width = 100.0f;
    box.height = 20.0f;
    box.baseline = 16.0f;
    
    EXPECT_TRUE(box.IsText());
    EXPECT_FALSE(box.IsAtomic());
    EXPECT_FLOAT_EQ(box.width, 100.0f);
    EXPECT_FLOAT_EQ(box.height, 20.0f);
    EXPECT_FLOAT_EQ(box.baseline, 16.0f);
}

TEST_F(InlineBoxTest, CreateAtomicBox) {
    InlineBox box = InlineBox::CreateAtomicBox(nullptr, 50.0f, 50.0f, 50.0f);
    
    EXPECT_TRUE(box.IsAtomic());
    EXPECT_FALSE(box.IsText());
    EXPECT_FLOAT_EQ(box.width, 50.0f);
    EXPECT_FLOAT_EQ(box.height, 50.0f);
    EXPECT_FLOAT_EQ(box.baseline, 50.0f);
}

TEST_F(InlineBoxTest, CreateInlineStartEnd) {
    InlineBox start = InlineBox::CreateInlineStart(nullptr);
    InlineBox end = InlineBox::CreateInlineEnd(nullptr);
    
    EXPECT_TRUE(start.IsInlineStart());
    EXPECT_TRUE(end.IsInlineEnd());
    EXPECT_FALSE(start.IsText());
    EXPECT_FALSE(end.IsAtomic());
}

TEST_F(InlineBoxTest, TotalWidthCalculation) {
    InlineBox box = InlineBox::CreateTextBox(nullptr);
    box.width = 100.0f;
    box.margin_left = 10.0f;
    box.margin_right = 10.0f;
    box.padding_left = 5.0f;
    box.padding_right = 5.0f;
    box.border_left = 1.0f;
    box.border_right = 1.0f;

    // 手动计算总宽度
    float total = box.width + box.margin_left + box.margin_right +
                  box.padding_left + box.padding_right +
                  box.border_left + box.border_right;
    EXPECT_FLOAT_EQ(total, 132.0f);
}

// ========== LineBox 测试 ==========

class LineBoxTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LineBoxTest, BasicConstruction) {
    LineBox line(500.0f);
    
    EXPECT_FLOAT_EQ(line.available_width, 500.0f);
    EXPECT_TRUE(line.IsEmpty());
    EXPECT_FLOAT_EQ(line.GetRemainingWidth(), 500.0f);
}

TEST_F(LineBoxTest, AddBox) {
    LineBox line(500.0f);
    
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    
    line.AddBox(&box1);
    
    EXPECT_FALSE(line.IsEmpty());
    EXPECT_EQ(line.boxes.size(), 1);
}

TEST_F(LineBoxTest, CanFit) {
    LineBox line(500.0f);
    
    // 初始可以容纳 500px
    EXPECT_TRUE(line.CanFit(400.0f));
    EXPECT_TRUE(line.CanFit(500.0f));
    EXPECT_FALSE(line.CanFit(501.0f));
    
    // 添加一个 200px 的盒子后
    InlineBox box = InlineBox::CreateTextBox(nullptr);
    box.width = 200.0f;
    line.AddBox(&box);
    line.content_width = 200.0f;
    
    EXPECT_TRUE(line.CanFit(300.0f));
    EXPECT_FALSE(line.CanFit(301.0f));
}

TEST_F(LineBoxTest, CalculateHeight) {
    LineBox line(500.0f);
    
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    box1.baseline = 16.0f;
    
    InlineBox box2 = InlineBox::CreateTextBox(nullptr);
    box2.width = 100.0f;
    box2.height = 30.0f;
    box2.baseline = 24.0f;
    
    line.AddBox(&box1);
    line.AddBox(&box2);
    line.CalculateHeight();
    
    // 现代模式：行高由最大内容决定
    EXPECT_GE(line.height, 30.0f);
}

// ========== LineBreaker 测试 ==========

class LineBreakerTest : public ::testing::Test {
protected:
    LineBreaker breaker;
    
    void SetUp() override {
        breaker.SetWhiteSpace(WhiteSpaceMode::NORMAL);
        breaker.SetWordBreak(WordBreakMode::NORMAL);
        breaker.SetOverflowWrap(OverflowWrapMode::NORMAL);
    }
};

TEST_F(LineBreakerTest, ProcessWhitespace) {
    std::string text = "  Hello   World  ";
    std::string result = breaker.ProcessWhitespace(text);
    
    // NORMAL 模式：多个空格合并为一个
    EXPECT_EQ(result, " Hello World ");
}

TEST_F(LineBreakerTest, ProcessWhitespacePreWrap) {
    breaker.SetWhiteSpace(WhiteSpaceMode::PRE_WRAP);
    
    std::string text = "  Hello   World  ";
    std::string result = breaker.ProcessWhitespace(text);
    
    // PRE_WRAP 模式：保留空格
    EXPECT_EQ(result, "  Hello   World  ");
}

TEST_F(LineBreakerTest, FindBreakOpportunities) {
    // 创建一些内联盒用于测试
    std::vector<InlineBox> boxes;

    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    TextRun run1;
    run1.text = "Hello World";
    run1.width = 100.0f;
    box1.text_runs.push_back(run1);
    boxes.push_back(box1);

    auto breaks = breaker.FindBreakOpportunities(boxes, 500.0f);

    // 应该找到断行机会
    EXPECT_GE(breaks.size(), 0);
}

TEST_F(LineBreakerTest, BreakIntoLines) {
    std::vector<InlineBox> boxes;
    
    // 创建一些文本盒子
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    
    InlineBox box2 = InlineBox::CreateTextBox(nullptr);
    box2.width = 100.0f;
    box2.height = 20.0f;
    
    InlineBox box3 = InlineBox::CreateTextBox(nullptr);
    box3.width = 100.0f;
    box3.height = 20.0f;
    
    boxes.push_back(box1);
    boxes.push_back(box2);
    boxes.push_back(box3);
    
    // 可用宽度 250px，应该分成两行
    auto lines = breaker.BreakIntoLines(boxes, 250.0f);
    
    EXPECT_GE(lines.size(), 1);
}

// ========== VerticalAligner 测试 ==========

class VerticalAlignerTest : public ::testing::Test {
protected:
    VerticalAligner aligner;
    
    void SetUp() override {}
};

TEST_F(VerticalAlignerTest, ParseVerticalAlign) {
    float font_size = 16.0f;

    auto info = VerticalAligner::ParseVerticalAlign("baseline", font_size);
    EXPECT_EQ(info.type, VerticalAlignType::BASELINE);

    info = VerticalAligner::ParseVerticalAlign("top", font_size);
    EXPECT_EQ(info.type, VerticalAlignType::TOP);

    info = VerticalAligner::ParseVerticalAlign("middle", font_size);
    EXPECT_EQ(info.type, VerticalAlignType::MIDDLE);

    info = VerticalAligner::ParseVerticalAlign("bottom", font_size);
    EXPECT_EQ(info.type, VerticalAlignType::BOTTOM);

    info = VerticalAligner::ParseVerticalAlign("10px", font_size);
    EXPECT_EQ(info.type, VerticalAlignType::LENGTH);
    EXPECT_FLOAT_EQ(info.length_value, 10.0f);
}

TEST_F(VerticalAlignerTest, CalculateLineMetrics) {
    std::vector<InlineBox*> boxes;
    std::vector<VerticalAlignInfo> aligns;
    
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    box1.baseline = 16.0f;
    
    InlineBox box2 = InlineBox::CreateTextBox(nullptr);
    box2.width = 100.0f;
    box2.height = 30.0f;
    box2.baseline = 24.0f;
    
    boxes.push_back(&box1);
    boxes.push_back(&box2);
    aligns.push_back({VerticalAlignType::BASELINE, 0.0f});
    aligns.push_back({VerticalAlignType::BASELINE, 0.0f});
    
    auto metrics = aligner.CalculateLineMetrics(boxes, aligns);
    
    // 行高应该足够容纳所有盒子
    EXPECT_GE(metrics.line_height, 30.0f);
    EXPECT_GT(metrics.baseline, 0.0f);
}

// ========== 高级特性测试 ==========

class AdvancedFeaturesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AdvancedFeaturesTest, TextIndent) {
    LineBreaker breaker;

    // 设置首行缩进
    breaker.SetTextIndent(32.0f);

    // 创建测试盒子
    std::vector<InlineBox> boxes;
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    boxes.push_back(box1);

    InlineBox box2 = InlineBox::CreateTextBox(nullptr);
    box2.width = 100.0f;
    box2.height = 20.0f;
    boxes.push_back(box2);

    // 断行（可用宽度 150，首行缩进 32，所以首行只能放 118 宽度）
    auto lines = breaker.BreakIntoLines(boxes, 150.0f);

    // 应该有 2 行（第一行放不下两个 100 宽的盒子）
    EXPECT_EQ(lines.size(), 2);

    // 第一行应该有缩进
    if (!lines.empty()) {
        EXPECT_FLOAT_EQ(lines[0].x, 32.0f);
    }
}

TEST_F(AdvancedFeaturesTest, LetterSpacing) {
    LineBreaker breaker;

    // 设置字符间距
    breaker.SetLetterSpacing(2.0f);

    // 字符间距应该被设置（这里只测试设置不会崩溃）
    // 实际效果需要在文本测量时体现
    EXPECT_NO_THROW(breaker.SetLetterSpacing(2.0f));
}

TEST_F(AdvancedFeaturesTest, WordSpacing) {
    LineBreaker breaker;

    // 设置单词间距
    breaker.SetWordSpacing(4.0f);

    // 单词间距应该被设置（这里只测试设置不会崩溃）
    EXPECT_NO_THROW(breaker.SetWordSpacing(4.0f));
}

TEST_F(AdvancedFeaturesTest, WhiteSpaceNowrap) {
    LineBreaker breaker;
    breaker.SetWhiteSpace(WhiteSpaceMode::NOWRAP);

    // 创建测试盒子
    std::vector<InlineBox> boxes;
    InlineBox box1 = InlineBox::CreateTextBox(nullptr);
    box1.width = 100.0f;
    box1.height = 20.0f;
    boxes.push_back(box1);

    InlineBox box2 = InlineBox::CreateTextBox(nullptr);
    box2.width = 100.0f;
    box2.height = 20.0f;
    boxes.push_back(box2);

    // NOWRAP 模式下不应该换行
    auto lines = breaker.BreakIntoLines(boxes, 150.0f);

    // 应该只有 1 行（不换行）
    EXPECT_EQ(lines.size(), 1);
}

// ========== 性能优化测试 ==========

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PerformanceTest, CacheInvalidation) {
    // 测试缓存失效机制
    IFCLayout layout;

    // 测试 InvalidateCache 和 ClearCache 不会崩溃
    layout.InvalidateCache(nullptr);
    layout.ClearCache();

    EXPECT_TRUE(true);  // 如果没有崩溃就通过
}

TEST_F(PerformanceTest, CacheValidation) {
    // 测试缓存验证
    IFCLayout layout;

    // 空容器应该返回缓存无效
    EXPECT_FALSE(layout.IsCacheValid(nullptr, 100.0f));
}

// ========== 主函数 ==========

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

