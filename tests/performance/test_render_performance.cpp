/**
 * @file test_render_performance.cpp
 * @brief 渲染性能测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include <chrono>
#include <iostream>

namespace mblink {
namespace test {

class RenderPerformanceTest : public DOMTestBase {
protected:
    void PrintResult(const std::string& name, double ms, int operations) {
        double opsPerSec = operations / (ms / 1000.0);
        std::cout << "[PERF] " << name << ": " << ms << "ms for " << operations
                  << " ops (" << opsPerSec << " ops/sec)" << std::endl;
    }
};

// ========== 样式计算性能 ==========

TEST_F(RenderPerformanceTest, StyleComputation) {
    const int COUNT = 1000;

    // 添加样式规则
    doc_->LoadHTML(R"(
        <html>
        <head>
            <style>
                .item { color: red; font-size: 16px; }
                .item:hover { color: blue; }
                .container .item { margin: 10px; }
                #special { background: yellow; }
            </style>
        </head>
        <body>
            <div class="container" id="container"></div>
        </body>
        </html>
    )");

    auto container = doc_->GetElementById("container");

    // 创建元素
    for (int i = 0; i < COUNT; i++) {
        auto elem = doc_->CreateElement("div");
        elem->AddClass("item");
        if (i == 0) elem->SetAttribute("id", "special");
        container->AppendChild(elem);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 计算所有元素的样式
    auto items = container->QuerySelectorAll(".item");
    for (const auto& item : items) {
        // auto computed = doc_->GetStyleManager()->ComputeStyle(item.get());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("StyleComputation", ms, COUNT);
}

// ========== 选择器匹配性能 ==========

TEST_F(RenderPerformanceTest, SelectorMatching) {
    const int ELEMENTS = 1000;
    const int QUERIES = 100;

    auto container = doc_->CreateElement("div");
    doc_->GetBody()->AppendChild(container);

    // 创建复杂的 DOM 结构
    for (int i = 0; i < ELEMENTS; i++) {
        auto elem = doc_->CreateElement("div");
        elem->AddClass("item");
        elem->AddClass("item-" + std::to_string(i % 10));
        elem->SetAttribute("data-index", std::to_string(i));

        auto child = doc_->CreateElement("span");
        child->AddClass("text");
        elem->AppendChild(child);

        container->AppendChild(elem);
    }

    std::vector<std::string> selectors = {
        ".item",
        ".item.item-5",
        "div.item > span.text",
        "[data-index]",
        ".item:first-child",
        "div > div.item"
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < QUERIES; i++) {
        for (const auto& selector : selectors) {
            auto results = container->QuerySelectorAll(selector);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("SelectorMatching", ms, QUERIES * selectors.size());
}

// ========== 脏区域计算性能 ==========

TEST_F(RenderPerformanceTest, DirtyRegionCalculation) {
    const int UPDATES = 10000;

    auto container = doc_->CreateElement("div");
    container->SetStyle("width", "800px");
    container->SetStyle("height", "600px");
    doc_->GetBody()->AppendChild(container);

    std::vector<std::shared_ptr<Element>> elements;
    for (int i = 0; i < 100; i++) {
        auto elem = doc_->CreateElement("div");
        elem->SetStyle("width", "50px");
        elem->SetStyle("height", "50px");
        elem->SetStyle("position", "absolute");
        elem->SetStyle("left", std::to_string((i % 10) * 60) + "px");
        elem->SetStyle("top", std::to_string((i / 10) * 60) + "px");
        container->AppendChild(elem);
        elements.push_back(elem);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < UPDATES; i++) {
        auto& elem = elements[i % elements.size()];
        elem->SetStyle("background-color", i % 2 == 0 ? "red" : "blue");

        // 计算脏区域
        // auto dirtyRect = elem->GetDirtyRect();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("DirtyRegionCalculation", ms, UPDATES);
}

// ========== 颜色解析性能 ==========

TEST_F(RenderPerformanceTest, ColorParsing) {
    const int COUNT = 100000;

    std::vector<std::string> colors = {
        "red",
        "#ff0000",
        "#f00",
        "rgb(255, 0, 0)",
        "rgba(255, 0, 0, 0.5)",
        "hsl(0, 100%, 50%)",
        "hsla(0, 100%, 50%, 0.5)"
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        const auto& colorStr = colors[i % colors.size()];
        // auto color = Color::FromString(colorStr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("ColorParsing", ms, COUNT);
}

// ========== 变换矩阵计算性能 ==========

TEST_F(RenderPerformanceTest, TransformCalculation) {
    const int COUNT = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        // auto t = Transform::Identity();
        // t = t.Multiply(Transform::Translate(100, 100));
        // t = t.Multiply(Transform::Rotate(45));
        // t = t.Multiply(Transform::Scale(2, 2));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("TransformCalculation", ms, COUNT);
}

// ========== CSS 值解析性能 ==========

TEST_F(RenderPerformanceTest, CSSValueParsing) {
    const int COUNT = 100000;

    std::vector<std::string> values = {
        "100px",
        "50%",
        "2em",
        "1.5rem",
        "auto",
        "calc(100% - 20px)",
        "var(--main-color)"
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        const auto& value = values[i % values.size()];
        // auto parsed = CSSValue::Parse(value);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("CSSValueParsing", ms, COUNT);
}

// ========== 渐变解析性能 ==========

TEST_F(RenderPerformanceTest, GradientParsing) {
    const int COUNT = 10000;

    std::vector<std::string> gradients = {
        "linear-gradient(red, blue)",
        "linear-gradient(45deg, red, yellow, blue)",
        "radial-gradient(circle, red, blue)",
        "conic-gradient(red, yellow, green, blue, red)"
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        const auto& gradient = gradients[i % gradients.size()];
        // auto parsed = GradientRenderer::ParseLinearGradient(gradient);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("GradientParsing", ms, COUNT);
}

// ========== 阴影解析性能 ==========

TEST_F(RenderPerformanceTest, ShadowParsing) {
    const int COUNT = 100000;

    std::vector<std::string> shadows = {
        "10px 10px black",
        "10px 10px 5px black",
        "10px 10px 5px 2px rgba(0,0,0,0.5)",
        "inset 5px 5px 3px black"
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < COUNT; i++) {
        const auto& shadow = shadows[i % shadows.size()];
        // auto parsed = ShadowRenderer::ParseBoxShadow(shadow);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("ShadowParsing", ms, COUNT);
}

// ========== 动画更新性能 ==========

TEST_F(RenderPerformanceTest, AnimationUpdate) {
    const int ANIMATIONS = 100;
    const int FRAMES = 1000;

    // 创建动画
    // std::vector<std::shared_ptr<Animation>> animations;
    // for (int i = 0; i < ANIMATIONS; i++) {
    //     auto anim = std::make_shared<Animation>();
    //     anim->SetDuration(1000);
    //     animations.push_back(anim);
    // }

    auto start = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < FRAMES; frame++) {
        float deltaTime = 16.67f;  // ~60fps
        // for (auto& anim : animations) {
        //     anim->Update(deltaTime);
        // }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ms = duration.count() / 1000.0;

    PrintResult("AnimationUpdate", ms, ANIMATIONS * FRAMES);
}

} // namespace test
} // namespace mblink
