/**
 * @file test_html_image_element.cpp
 * @brief HTMLImageElement单元测试
 */

#include <gtest/gtest.h>
#include "core/dom/html_image_element.h"
#include "core/dom/document.h"
#include "core/lexbor/lexbor_document.h"
#include <chrono>

using namespace lightui;

// ========== 基础测试 ==========

TEST(HTMLImageElement, Construction) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->GetTagName(), "img");
    EXPECT_EQ(img->GetSrc(), "");
    EXPECT_EQ(img->GetAlt(), "");
    EXPECT_EQ(img->GetWidth(), 0u);
    EXPECT_EQ(img->GetHeight(), 0u);
    EXPECT_EQ(img->GetNaturalWidth(), 0u);
    EXPECT_EQ(img->GetNaturalHeight(), 0u);
    EXPECT_FALSE(img->GetComplete());
}

// ========== 属性测试 ==========

TEST(HTMLImageElement, SrcProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 设置src
    img->SetSrc("https://example.com/image.png");
    EXPECT_EQ(img->GetSrc(), "https://example.com/image.png");
    EXPECT_EQ(img->GetAttribute("src"), "https://example.com/image.png");
    
    // 修改src
    img->SetSrc("https://example.com/image2.png");
    EXPECT_EQ(img->GetSrc(), "https://example.com/image2.png");
}

TEST(HTMLImageElement, AltProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 设置alt
    img->SetAlt("A beautiful image");
    EXPECT_EQ(img->GetAlt(), "A beautiful image");
    EXPECT_EQ(img->GetAttribute("alt"), "A beautiful image");
}

TEST(HTMLImageElement, WidthProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 设置width
    img->SetWidth(800);
    EXPECT_EQ(img->GetWidth(), 800u);
    EXPECT_EQ(img->GetAttribute("width"), "800");
}

TEST(HTMLImageElement, HeightProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 设置height
    img->SetHeight(600);
    EXPECT_EQ(img->GetHeight(), 600u);
    EXPECT_EQ(img->GetAttribute("height"), "600");
}

TEST(HTMLImageElement, CrossOriginProperty) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 设置crossOrigin
    img->SetCrossOrigin("anonymous");
    EXPECT_EQ(img->GetCrossOrigin(), "anonymous");
    EXPECT_EQ(img->GetAttribute("crossorigin"), "anonymous");
}

// ========== SetAttribute/RemoveAttribute测试 ==========

TEST(HTMLImageElement, SetAttributeSrc) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    img->SetAttribute("src", "https://example.com/image.png");
    EXPECT_EQ(img->GetSrc(), "https://example.com/image.png");
}

TEST(HTMLImageElement, SetAttributeAlt) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    img->SetAttribute("alt", "Test image");
    EXPECT_EQ(img->GetAlt(), "Test image");
}

TEST(HTMLImageElement, SetAttributeWidth) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    img->SetAttribute("width", "800");
    EXPECT_EQ(img->GetWidth(), 800u);
}

TEST(HTMLImageElement, SetAttributeHeight) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    img->SetAttribute("height", "600");
    EXPECT_EQ(img->GetHeight(), 600u);
}

TEST(HTMLImageElement, RemoveAttributeSrc) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    img->SetSrc("https://example.com/image.png");
    
    img->RemoveAttribute("src");
    EXPECT_EQ(img->GetSrc(), "");
    EXPECT_FALSE(img->GetComplete());
}

TEST(HTMLImageElement, RemoveAttributeWidth) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    img->SetWidth(800);
    
    img->RemoveAttribute("width");
    EXPECT_EQ(img->GetWidth(), 0u);
}

// ========== 图片加载测试 ==========

TEST(HTMLImageElement, LoadImageEvents) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 监听事件
    bool loadstart_fired = false;
    bool load_fired = false;
    bool loadend_fired = false;
    
    img->AddEventListener("loadstart", [&](std::shared_ptr<Event> event) {
        loadstart_fired = true;
    });
    
    img->AddEventListener("load", [&](std::shared_ptr<Event> event) {
        load_fired = true;
    });
    
    img->AddEventListener("loadend", [&](std::shared_ptr<Event> event) {
        loadend_fired = true;
    });
    
    // 设置src触发加载
    img->SetSrc("https://example.com/image.png");
    
    EXPECT_TRUE(loadstart_fired);
    EXPECT_TRUE(load_fired);
    EXPECT_TRUE(loadend_fired);
    EXPECT_TRUE(img->GetComplete());
}

TEST(HTMLImageElement, SetImageData) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 模拟设置图片数据
    void* fake_data = reinterpret_cast<void*>(0x12345678);
    img->SetImageData(fake_data, 1920, 1080);
    
    EXPECT_EQ(img->GetNaturalWidth(), 1920u);
    EXPECT_EQ(img->GetNaturalHeight(), 1080u);
    EXPECT_TRUE(img->GetComplete());
    EXPECT_EQ(img->GetImageData(), fake_data);
    
    // 如果没有设置显示尺寸，应该使用原始尺寸
    EXPECT_EQ(img->GetWidth(), 1920u);
    EXPECT_EQ(img->GetHeight(), 1080u);
}

TEST(HTMLImageElement, SetImageDataWithDisplaySize) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 先设置显示尺寸
    img->SetWidth(800);
    img->SetHeight(600);
    
    // 再设置图片数据
    void* fake_data = reinterpret_cast<void*>(0x12345678);
    img->SetImageData(fake_data, 1920, 1080);
    
    // 显示尺寸应该保持不变
    EXPECT_EQ(img->GetWidth(), 800u);
    EXPECT_EQ(img->GetHeight(), 600u);
    
    // 原始尺寸应该是图片尺寸
    EXPECT_EQ(img->GetNaturalWidth(), 1920u);
    EXPECT_EQ(img->GetNaturalHeight(), 1080u);
}

// ========== React兼容性测试 ==========

TEST(HTMLImageElement, ReactAttributeUpdate) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
    
    // 模拟React更新属性
    img->SetAttribute("src", "image1.png");
    EXPECT_EQ(img->GetSrc(), "image1.png");
    
    img->SetAttribute("src", "image2.png");
    EXPECT_EQ(img->GetSrc(), "image2.png");
    
    img->SetAttribute("alt", "Image 1");
    EXPECT_EQ(img->GetAlt(), "Image 1");
    
    img->SetAttribute("alt", "Image 2");
    EXPECT_EQ(img->GetAlt(), "Image 2");
    
    img->SetAttribute("width", "400");
    EXPECT_EQ(img->GetWidth(), 400u);
    
    img->SetAttribute("width", "800");
    EXPECT_EQ(img->GetWidth(), 800u);
}

// ========== 性能测试 ==========

TEST(HTMLImageElement, PerformanceCreation) {
    auto doc = std::make_shared<Document>();
    doc->Initialize();
    
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto img = std::dynamic_pointer_cast<HTMLImageElement>(doc->CreateElement("img"));
        img->SetSrc("https://example.com/image.png");
        img->SetAlt("Test image");
        img->SetWidth(800);
        img->SetHeight(600);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double avg_time = static_cast<double>(duration.count()) / iterations;
    
    std::cout << "Average img creation time: " << avg_time << " microseconds" << std::endl;

    // 性能要求：平均创建时间应该小于100微秒
    EXPECT_LT(avg_time, 100.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

