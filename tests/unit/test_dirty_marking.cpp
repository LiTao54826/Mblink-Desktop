/**
 * @file test_dirty_marking.cpp
 * @brief Week 1 脏标记系统单元测试
 * 
 * 测试内容:
 * - Task 1.1: Node脏标记系统
 * - Task 1.2: Element智能属性脏标记判断
 * - Task 1.3: DirtyRegionCollector脏区域收集器
 */

#include <gtest/gtest.h>
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/render/dirty_region_collector.h"
#include <memory>

using namespace lightui;

// ============================================================================
// Task 1.1: Node脏标记系统测试
// ============================================================================

TEST(DirtyMarkingTest, DirtyTypeEnum) {
    // 测试DirtyType枚举值
    EXPECT_EQ(static_cast<uint32_t>(DirtyType::NONE), 0);
    EXPECT_EQ(static_cast<uint32_t>(DirtyType::LAYOUT), 1);
    EXPECT_EQ(static_cast<uint32_t>(DirtyType::PAINT), 2);
    EXPECT_EQ(static_cast<uint32_t>(DirtyType::STYLE), 4);
    EXPECT_EQ(static_cast<uint32_t>(DirtyType::ALL), 7);
}

TEST(DirtyMarkingTest, DirtyTypeBitwiseOperators) {
    // 测试位运算符
    DirtyType combined = DirtyType::LAYOUT | DirtyType::PAINT;
    EXPECT_EQ(static_cast<uint32_t>(combined), 3);
    
    combined = DirtyType::LAYOUT | DirtyType::STYLE;
    EXPECT_EQ(static_cast<uint32_t>(combined), 5);
    
    combined = DirtyType::PAINT | DirtyType::STYLE;
    EXPECT_EQ(static_cast<uint32_t>(combined), 6);
}

TEST(DirtyMarkingTest, MarkDirtyLayout) {
    auto element = std::make_shared<Element>("div");

    // 清除初始脏标记（新创建的节点默认是脏的）
    element->ClearDirty(DirtyType::ALL);

    // 初始状态应该是干净的
    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_FALSE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());

    // 标记为布局脏
    element->MarkDirty(DirtyType::LAYOUT);

    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_FALSE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, MarkDirtyPaint) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);

    // 标记为绘制脏
    element->MarkDirty(DirtyType::PAINT);

    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, MarkDirtyStyle) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);

    // 标记为样式脏
    element->MarkDirty(DirtyType::STYLE);

    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_FALSE(element->IsPaintDirty());
    EXPECT_TRUE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, MarkDirtyCombined) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);

    // 标记为布局+绘制脏
    element->MarkDirty(DirtyType::LAYOUT | DirtyType::PAINT);

    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, MarkDirtyAll) {
    auto element = std::make_shared<Element>("div");
    
    // 标记为全部脏
    element->MarkDirty(DirtyType::ALL);
    
    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_TRUE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, ClearDirtyLayout) {
    auto element = std::make_shared<Element>("div");
    
    // 标记为全部脏
    element->MarkDirty(DirtyType::ALL);
    
    // 清除布局脏标记
    element->ClearDirty(DirtyType::LAYOUT);
    
    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_TRUE(element->IsStyleDirty());
    EXPECT_TRUE(element->IsDirty());
}

TEST(DirtyMarkingTest, ClearDirtyAll) {
    auto element = std::make_shared<Element>("div");
    
    // 标记为全部脏
    element->MarkDirty(DirtyType::ALL);
    
    // 清除所有脏标记
    element->ClearDirty(DirtyType::ALL);
    
    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_FALSE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
    EXPECT_FALSE(element->IsDirty());
}

TEST(DirtyMarkingTest, DirtyPropagationToParent) {
    auto parent = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    
    parent->AppendChild(child);
    
    // 清除父节点的脏标记（AppendChild会标记脏）
    parent->ClearDirty(DirtyType::ALL);
    
    // 标记子节点为布局脏
    child->MarkDirty(DirtyType::LAYOUT);
    
    // 父节点应该也被标记为脏
    EXPECT_TRUE(parent->IsLayoutDirty());
    EXPECT_TRUE(parent->IsDirty());
}

TEST(DirtyMarkingTest, DirtyRect) {
    auto element = std::make_shared<Element>("div");
    
    // 初始脏矩形应该为空
    SkRect rect = element->GetDirtyRect();
    EXPECT_TRUE(rect.isEmpty());
    
    // 设置脏矩形
    SkRect newRect = SkRect::MakeXYWH(10, 20, 100, 50);
    element->SetDirtyRect(newRect);
    
    rect = element->GetDirtyRect();
    EXPECT_EQ(rect.x(), 10);
    EXPECT_EQ(rect.y(), 20);
    EXPECT_EQ(rect.width(), 100);
    EXPECT_EQ(rect.height(), 50);
}

TEST(DirtyMarkingTest, GetDirtyFlags) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);

    // 清除后flags应该为0
    EXPECT_EQ(element->GetDirtyFlags(), 0);

    // 标记为布局脏
    element->MarkDirty(DirtyType::LAYOUT);
    EXPECT_EQ(element->GetDirtyFlags(), static_cast<uint32_t>(DirtyType::LAYOUT));

    // 再标记为绘制脏
    element->MarkDirty(DirtyType::PAINT);
    EXPECT_EQ(element->GetDirtyFlags(), static_cast<uint32_t>(DirtyType::LAYOUT | DirtyType::PAINT));
}

// ============================================================================
// Task 1.2: Element智能属性脏标记判断测试
// ============================================================================

TEST(SmartAttributeMarkingTest, LayoutAttributeWidth) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置width属性（布局属性）
    element->SetAttribute("width", "100px");
    
    // 应该标记为布局+绘制脏
    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
}

TEST(SmartAttributeMarkingTest, LayoutAttributeHeight) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置height属性（布局属性）
    element->SetAttribute("height", "200px");
    
    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
}

TEST(SmartAttributeMarkingTest, LayoutAttributePadding) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置padding属性（布局属性）
    element->SetAttribute("padding", "10px");
    
    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
}

TEST(SmartAttributeMarkingTest, StyleAttributeColor) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置color属性（样式属性）
    element->SetAttribute("color", "red");
    
    // 应该只标记为绘制脏
    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_FALSE(element->IsStyleDirty());
}

TEST(SmartAttributeMarkingTest, StyleAttributeBackgroundColor) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置background-color属性（样式属性）
    element->SetAttribute("background-color", "#ff0000");
    
    EXPECT_FALSE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
}

TEST(SmartAttributeMarkingTest, UnchangedValueNoMarking) {
    auto element = std::make_shared<Element>("div");
    
    // 设置初始值
    element->SetAttribute("width", "100px");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置相同的值
    element->SetAttribute("width", "100px");
    
    // 不应该标记为脏
    EXPECT_FALSE(element->IsDirty());
}

TEST(SmartAttributeMarkingTest, UnknownAttributeMarksAll) {
    auto element = std::make_shared<Element>("div");
    element->ClearDirty(DirtyType::ALL);
    
    // 设置未知属性
    element->SetAttribute("data-custom", "value");
    
    // 应该标记为全部脏（保守策略）
    EXPECT_TRUE(element->IsLayoutDirty());
    EXPECT_TRUE(element->IsPaintDirty());
    EXPECT_TRUE(element->IsStyleDirty());
}

// ============================================================================
// Task 1.3: DirtyRegionCollector测试
// ============================================================================

TEST(DirtyRegionCollectorTest, EmptyDocument) {
    DirtyRegionCollector collector;
    DirtyRegion dirty_region;
    
    // 空文档应该返回false
    bool has_dirty = collector.CollectFromDOM(nullptr, dirty_region);
    
    EXPECT_FALSE(has_dirty);
    EXPECT_EQ(dirty_region.GetRegions().size(), 0);
}

TEST(DirtyRegionCollectorTest, NoDirtyNodes) {
    auto root = std::make_shared<Element>("div");
    auto child = std::make_shared<Element>("span");
    root->AppendChild(child);
    
    // 清除所有脏标记
    root->ClearDirty(DirtyType::ALL);
    child->ClearDirty(DirtyType::ALL);
    
    DirtyRegionCollector collector;
    DirtyRegion dirty_region;
    
    bool has_dirty = collector.CollectFromDOM(root.get(), dirty_region);
    
    EXPECT_FALSE(has_dirty);
    EXPECT_EQ(dirty_region.GetRegions().size(), 0);
}

// 注意：以下测试需要布局信息，暂时跳过
// 等Week 2集成后可以添加完整的集成测试

