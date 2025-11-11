/**
 * @file test_style_cache.cpp
 * @brief StyleCache 类的单元测试
 */

#include <gtest/gtest.h>
#include "core/lexbor/style_cache.h"
#include "core/lexbor/lexbor_document.h"
#include "core/dom/document.h"
#include "core/dom/element.h"

using namespace lightui;

// ========== 基本功能测试 ==========

TEST(StyleCacheTest, Constructor) {
    StyleCache cache;
    
    EXPECT_EQ(cache.GetCacheSize(), 0);
    EXPECT_EQ(cache.GetHits(), 0);
    EXPECT_EQ(cache.GetMisses(), 0);
    EXPECT_EQ(cache.GetHitRate(), 0.0);
    EXPECT_EQ(cache.GetMaxCacheSize(), 0); // 0表示无限制
}

TEST(StyleCacheTest, SetAndGetCachedStyle) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    style["font-size"] = "14px";
    
    cache.SetCachedStyle(div.get(), style);
    
    EXPECT_TRUE(cache.HasCachedStyle(div.get()));
    EXPECT_EQ(cache.GetCacheSize(), 1);
    
    auto cached = cache.GetCachedStyle(div.get());
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->at("color"), "red");
    EXPECT_EQ(cached->at("font-size"), "14px");
}

TEST(StyleCacheTest, GetNonExistentCache) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    EXPECT_FALSE(cache.HasCachedStyle(div.get()));
    EXPECT_EQ(cache.GetCachedStyle(div.get()), nullptr);
}

TEST(StyleCacheTest, OverwriteCache) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::map<std::string, std::string> style1;
    style1["color"] = "red";
    cache.SetCachedStyle(div.get(), style1);
    
    std::map<std::string, std::string> style2;
    style2["color"] = "blue";
    cache.SetCachedStyle(div.get(), style2);
    
    EXPECT_EQ(cache.GetCacheSize(), 1);
    
    auto cached = cache.GetCachedStyle(div.get());
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->at("color"), "blue");
}

// ========== 缓存失效测试 ==========

TEST(StyleCacheTest, InvalidateElement) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    cache.SetCachedStyle(div.get(), style);
    
    EXPECT_TRUE(cache.HasCachedStyle(div.get()));
    
    cache.InvalidateElement(div.get());
    
    EXPECT_FALSE(cache.HasCachedStyle(div.get()));
    EXPECT_EQ(cache.GetCacheSize(), 0);
}

TEST(StyleCacheTest, InvalidateSubtree) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto parent = std::make_shared<Element>("div");
    doc->AppendChild(parent);
    
    auto child1 = std::make_shared<Element>("span");
    parent->AppendChild(child1);
    
    auto child2 = std::make_shared<Element>("p");
    parent->AppendChild(child2);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    
    cache.SetCachedStyle(parent.get(), style);
    cache.SetCachedStyle(child1.get(), style);
    cache.SetCachedStyle(child2.get(), style);
    
    EXPECT_EQ(cache.GetCacheSize(), 3);
    
    cache.InvalidateSubtree(parent.get());
    
    EXPECT_EQ(cache.GetCacheSize(), 0);
    EXPECT_FALSE(cache.HasCachedStyle(parent.get()));
    EXPECT_FALSE(cache.HasCachedStyle(child1.get()));
    EXPECT_FALSE(cache.HasCachedStyle(child2.get()));
}

TEST(StyleCacheTest, InvalidateAll) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div1 = std::make_shared<Element>("div");
    doc->AppendChild(div1);
    
    auto div2 = std::make_shared<Element>("div");
    doc->AppendChild(div2);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    
    cache.SetCachedStyle(div1.get(), style);
    cache.SetCachedStyle(div2.get(), style);
    
    EXPECT_EQ(cache.GetCacheSize(), 2);
    
    cache.InvalidateAll();
    
    EXPECT_EQ(cache.GetCacheSize(), 0);
}

// ========== 统计信息测试 ==========

TEST(StyleCacheTest, RecordHitAndMiss) {
    StyleCache cache;
    
    cache.RecordHit();
    cache.RecordHit();
    cache.RecordMiss();
    
    EXPECT_EQ(cache.GetHits(), 2);
    EXPECT_EQ(cache.GetMisses(), 1);
    EXPECT_DOUBLE_EQ(cache.GetHitRate(), 2.0 / 3.0);
}

TEST(StyleCacheTest, ResetStats) {
    StyleCache cache;
    
    cache.RecordHit();
    cache.RecordMiss();
    
    EXPECT_EQ(cache.GetHits(), 1);
    EXPECT_EQ(cache.GetMisses(), 1);
    
    cache.ResetStats();
    
    EXPECT_EQ(cache.GetHits(), 0);
    EXPECT_EQ(cache.GetMisses(), 0);
    EXPECT_EQ(cache.GetHitRate(), 0.0);
}

TEST(StyleCacheTest, HitRateCalculation) {
    StyleCache cache;
    
    // 初始状态
    EXPECT_EQ(cache.GetHitRate(), 0.0);
    
    // 100%命中率
    cache.RecordHit();
    EXPECT_EQ(cache.GetHitRate(), 1.0);
    
    // 50%命中率
    cache.RecordMiss();
    EXPECT_EQ(cache.GetHitRate(), 0.5);
    
    // 75%命中率
    cache.RecordHit();
    cache.RecordHit();
    EXPECT_DOUBLE_EQ(cache.GetHitRate(), 0.75);
}

// ========== 缓存管理测试 ==========

TEST(StyleCacheTest, Clear) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    auto div = std::make_shared<Element>("div");
    doc->AppendChild(div);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    cache.SetCachedStyle(div.get(), style);
    
    cache.RecordHit();
    cache.RecordMiss();
    
    EXPECT_EQ(cache.GetCacheSize(), 1);
    EXPECT_EQ(cache.GetHits(), 1);
    
    cache.Clear();
    
    EXPECT_EQ(cache.GetCacheSize(), 0);
    // 统计信息应该保留
    EXPECT_EQ(cache.GetHits(), 1);
    EXPECT_EQ(cache.GetMisses(), 1);
}

TEST(StyleCacheTest, MaxCacheSize) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    cache.SetMaxCacheSize(2);
    EXPECT_EQ(cache.GetMaxCacheSize(), 2);
    
    auto div1 = std::make_shared<Element>("div");
    doc->AppendChild(div1);
    auto div2 = std::make_shared<Element>("span");
    doc->AppendChild(div2);
    auto div3 = std::make_shared<Element>("p");
    doc->AppendChild(div3);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    
    cache.SetCachedStyle(div1.get(), style);
    cache.SetCachedStyle(div2.get(), style);
    
    EXPECT_EQ(cache.GetCacheSize(), 2);
    
    // 添加第3个元素应该触发LRU淘汰
    cache.SetCachedStyle(div3.get(), style);
    
    EXPECT_EQ(cache.GetCacheSize(), 2);
    // div1应该被淘汰（最久未访问）
    EXPECT_FALSE(cache.HasCachedStyle(div1.get()));
}

TEST(StyleCacheTest, LRUEviction) {
    auto doc = std::make_shared<Document>();
    StyleCache cache;
    
    cache.SetMaxCacheSize(2);
    
    auto div1 = std::make_shared<Element>("div");
    doc->AppendChild(div1);
    auto div2 = std::make_shared<Element>("span");
    doc->AppendChild(div2);
    auto div3 = std::make_shared<Element>("p");
    doc->AppendChild(div3);
    
    std::map<std::string, std::string> style;
    style["color"] = "red";
    
    cache.SetCachedStyle(div1.get(), style);
    cache.SetCachedStyle(div2.get(), style);
    
    // 访问div1，使其成为最近访问
    cache.GetCachedStyle(div1.get());
    
    // 添加div3应该淘汰div2（最久未访问）
    cache.SetCachedStyle(div3.get(), style);
    
    EXPECT_TRUE(cache.HasCachedStyle(div1.get()));
    EXPECT_FALSE(cache.HasCachedStyle(div2.get()));
    EXPECT_TRUE(cache.HasCachedStyle(div3.get()));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

