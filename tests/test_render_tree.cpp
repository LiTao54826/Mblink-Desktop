/**
 * @file test_render_tree.cpp
 * @brief 渲染树测试
 */

#include "core/render/style_resolver.h"
#include "core/render/render_object.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/render/renderer.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"
#include <iostream>
#include <cassert>

using namespace lightui;

// ========== 样式解析测试 ==========

void TestStyleResolver() {
    std::cout << "\nTesting Style Resolver..." << std::endl;
    
    StyleResolver resolver;
    
    // 测试默认样式
    {
        auto div_style = resolver.GetDefaultStyle("div");
        assert(div_style.display == RenderObjectType::BLOCK);
        assert(div_style.font_size == 16.0f);
        std::cout << "✓ Default div style correct!" << std::endl;
    }
    
    // 测试标题默认样式
    {
        auto h1_style = resolver.GetDefaultStyle("h1");
        assert(h1_style.font_size == 32.0f);
        assert(h1_style.font_weight == "bold");
        std::cout << "✓ Default h1 style correct!" << std::endl;
    }
    
    // 测试内联元素默认样式
    {
        auto span_style = resolver.GetDefaultStyle("span");
        assert(span_style.display == RenderObjectType::INLINE);
        std::cout << "✓ Default span style correct!" << std::endl;
    }
    
    // 测试样式解析
    {
        auto element = std::make_shared<Element>("div");
        element->SetStyle("width", "200px");
        element->SetStyle("height", "100px");
        element->SetStyle("background-color", "#FF0000");
        element->SetStyle("padding", "10px");
        element->SetStyle("margin", "20px");
        
        auto style = resolver.ResolveStyle(element);
        
        assert(style.width.value == 200.0f);
        assert(style.width.unit == CSSUnit::PX);
        assert(style.height.value == 100.0f);
        assert(style.background_color == "#FF0000");
        assert(style.padding.top.value == 10.0f);
        assert(style.margin.top.value == 20.0f);
        
        std::cout << "✓ Style parsing correct!" << std::endl;
    }
    
    // 测试样式继承
    {
        auto parent = std::make_shared<Element>("div");
        parent->SetStyle("color", "#0000FF");
        parent->SetStyle("font-size", "20px");
        parent->SetStyle("font-family", "Arial");
        
        auto parent_style = resolver.ResolveStyle(parent);
        
        auto child = std::make_shared<Element>("span");
        auto child_style = resolver.ResolveStyle(child, &parent_style);
        
        assert(child_style.color == "#0000FF");
        assert(child_style.font_size == 20.0f);
        assert(child_style.font_family == "Arial");
        
        std::cout << "✓ Style inheritance correct!" << std::endl;
    }
}

// ========== 渲染树构建测试 ==========

void TestRenderTreeBuilder() {
    std::cout << "\nTesting Render Tree Builder..." << std::endl;
    
    RenderTreeBuilder builder;
    
    // 创建简单的 DOM 树
    auto doc = std::make_shared<Document>();
    auto root = doc->CreateElement("div");
    root->SetStyle("width", "400px");
    root->SetStyle("height", "300px");
    root->SetStyle("background-color", "#EEEEEE");
    root->SetStyle("padding", "20px");
    
    auto child1 = doc->CreateElement("div");
    child1->SetStyle("width", "100px");
    child1->SetStyle("height", "50px");
    child1->SetStyle("background-color", "#FF0000");
    child1->SetStyle("margin", "10px");
    
    auto child2 = doc->CreateElement("span");
    child2->SetStyle("color", "#0000FF");
    
    auto text = doc->CreateTextNode("Hello World");
    
    root->AppendChild(child1);
    root->AppendChild(child2);
    child2->AppendChild(text);
    
    // 构建渲染树
    auto render_tree = builder.BuildRenderTree(root);
    
    assert(render_tree != nullptr);
    assert(render_tree->GetType() == RenderObjectType::BLOCK);
    assert(render_tree->GetChildren().size() == 2);
    
    std::cout << "✓ Render tree built successfully!" << std::endl;
    
    // 检查第一个子元素
    auto render_child1 = render_tree->GetChildren()[0];
    assert(render_child1->GetType() == RenderObjectType::BLOCK);
    assert(render_child1->GetComputedStyle().width.value == 100.0f);
    assert(render_child1->GetComputedStyle().background_color == "#FF0000");
    
    std::cout << "✓ First child render object correct!" << std::endl;
    
    // 检查第二个子元素
    auto render_child2 = render_tree->GetChildren()[1];
    assert(render_child2->GetType() == RenderObjectType::INLINE);
    assert(render_child2->GetChildren().size() == 1);
    
    std::cout << "✓ Second child render object correct!" << std::endl;
    
    // 检查文本节点
    auto render_text = render_child2->GetChildren()[0];
    assert(render_text->GetType() == RenderObjectType::TEXT);
    auto text_obj = std::static_pointer_cast<RenderText>(render_text);
    assert(text_obj->GetText() == "Hello World");
    
    std::cout << "✓ Text render object correct!" << std::endl;
}

// ========== display: none 测试 ==========

void TestDisplayNone() {
    std::cout << "\nTesting display: none..." << std::endl;
    
    RenderTreeBuilder builder;
    
    auto doc = std::make_shared<Document>();
    auto root = doc->CreateElement("div");
    
    auto visible = doc->CreateElement("div");
    visible->SetStyle("width", "100px");
    
    auto hidden = doc->CreateElement("div");
    hidden->SetStyle("display", "none");
    hidden->SetStyle("width", "200px");
    
    auto visible2 = doc->CreateElement("div");
    visible2->SetStyle("width", "150px");
    
    root->AppendChild(visible);
    root->AppendChild(hidden);
    root->AppendChild(visible2);
    
    auto render_tree = builder.BuildRenderTree(root);
    
    // display: none 的元素不应该出现在渲染树中
    assert(render_tree->GetChildren().size() == 2);
    assert(render_tree->GetChildren()[0]->GetComputedStyle().width.value == 100.0f);
    assert(render_tree->GetChildren()[1]->GetComputedStyle().width.value == 150.0f);
    
    std::cout << "✓ display: none filtering correct!" << std::endl;
}

// ========== 渲染测试 ==========

void TestRendering() {
    std::cout << "\nTesting Rendering..." << std::endl;
    
    // 创建 DOM 树
    auto doc = std::make_shared<Document>();
    auto root = doc->CreateElement("div");
    root->SetStyle("width", "400px");
    root->SetStyle("height", "300px");
    root->SetStyle("background-color", "#F0F0F0");
    root->SetStyle("padding", "20px");
    
    auto box1 = doc->CreateElement("div");
    box1->SetStyle("width", "150px");
    box1->SetStyle("height", "100px");
    box1->SetStyle("background-color", "#FF6B6B");
    box1->SetStyle("margin", "10px");
    box1->SetStyle("padding", "15px");
    box1->SetStyle("border-width", "3px");
    box1->SetStyle("border-style", "solid");
    box1->SetStyle("border-radius", "10px");
    
    auto box2 = doc->CreateElement("div");
    box2->SetStyle("width", "200px");
    box2->SetStyle("height", "80px");
    box2->SetStyle("background-color", "linear-gradient(90deg, #4ECDC4, #44A08D)");
    box2->SetStyle("margin", "10px");
    box2->SetStyle("border-radius", "15px");
    box2->SetStyle("box-shadow", "5px 5px 10px rgba(0,0,0,0.3)");
    
    root->AppendChild(box1);
    root->AppendChild(box2);
    
    // 构建渲染树
    RenderTreeBuilder builder;
    auto render_tree = builder.BuildRenderTree(root);
    
    assert(render_tree != nullptr);
    
    // 执行布局
    render_tree->Layout(800, 600);
    
    assert(render_tree->GetLayoutInfo().is_laid_out);
    assert(render_tree->GetLayoutInfo().width == 400.0f);
    assert(render_tree->GetLayoutInfo().height > 0);
    
    std::cout << "✓ Layout completed!" << std::endl;

    // 创建 Skia 表面
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(800, 600));
    if (!surface) {
        std::cerr << "Failed to create surface" << std::endl;
        return;
    }

    // 创建渲染器
    Renderer renderer(surface);
    auto canvas = renderer.GetCanvas();

    if (canvas) {
        // 清空画布
        canvas->clear(SK_ColorWHITE);

        // 绘制渲染树
        render_tree->Paint(canvas);

        // 保存图片
        auto image = surface->makeImageSnapshot();
        if (image) {
            auto data = SkPngEncoder::Encode(nullptr, image.get(), {});
            if (data) {
                SkFILEWStream stream("test_render_tree.png");
                stream.write(data->data(), data->size());
                std::cout << "✓ Rendering completed! Output: test_render_tree.png" << std::endl;
            }
        }
    }
}

// ========== 主函数 ==========

int main() {
    std::cout << "\n=== Render Tree Tests ===" << std::endl;
    
    try {
        TestStyleResolver();
        TestRenderTreeBuilder();
        TestDisplayNone();
        TestRendering();
        
        std::cout << "\n=== All Render Tree Tests Passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

