# 第三方库集成计划

## 📋 概述

本文档详细说明如何集成 stb_image 和 Lexbor 两个关键第三方库。

---

## 🎯 集成目标

### 1. stb_image - 图片加载库
- **用途**: 支持 `<img>` 元素，加载 PNG/JPG/BMP/GIF
- **优先级**: P0（Phase 2.5 Task 7.1 需要）
- **预计时间**: 30 分钟

### 2. Lexbor - HTML/CSS 解析器
- **用途**: 实现 querySelector, CSS 选择器，`<style>` 标签
- **优先级**: P0（Phase 2.5 Task 3, 8 需要）
- **预计时间**: 2 小时

---

## 📦 集成步骤

### Step 1: 集成 stb_image

#### 1.1 下载库文件

**方式 A: 使用 curl（推荐）**
```bash
mkdir -p third_party/stb
curl -o third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

**方式 B: 手动下载**
1. 访问 https://github.com/nothings/stb
2. 下载 `stb_image.h`
3. 放到 `third_party/stb/` 目录

#### 1.2 修改 CMakeLists.txt

**文件**: `third_party/CMakeLists.txt`

在文件末尾添加：
```cmake
# ============================================
# stb (header-only)
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/stb/stb_image.h")
    message(STATUS "  Found stb_image")
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${THIRD_PARTY_DIR}/stb")
    set(STB_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  stb_image not found")
    set(STB_FOUND FALSE PARENT_SCOPE)
endif()
```

#### 1.3 创建包装类

**文件**: `core/utils/image_loader.h`
```cpp
#pragma once
#include <string>
#include <memory>

namespace lightui {

struct ImageData {
    int width;
    int height;
    int channels;
    std::unique_ptr<unsigned char[]> data;
};

class ImageLoader {
public:
    static ImageData LoadFromFile(const std::string& path);
    static ImageData LoadFromMemory(const unsigned char* buffer, int len);
};

} // namespace lightui
```

**文件**: `core/utils/image_loader.cpp`
```cpp
#include "image_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <stdexcept>

namespace lightui {

ImageData ImageLoader::LoadFromFile(const std::string& path) {
    ImageData result;
    unsigned char* raw_data = stbi_load(
        path.c_str(), 
        &result.width, 
        &result.height, 
        &result.channels, 
        4  // 强制 RGBA
    );
    
    if (!raw_data) {
        throw std::runtime_error("Failed to load image: " + path);
    }
    
    result.data.reset(raw_data);
    return result;
}

ImageData ImageLoader::LoadFromMemory(const unsigned char* buffer, int len) {
    ImageData result;
    unsigned char* raw_data = stbi_load_from_memory(
        buffer, 
        len, 
        &result.width, 
        &result.height, 
        &result.channels, 
        4
    );
    
    if (!raw_data) {
        throw std::runtime_error("Failed to load image from memory");
    }
    
    result.data.reset(raw_data);
    return result;
}

} // namespace lightui
```

#### 1.4 测试

**文件**: `tests/test_image_loader.cpp`
```cpp
#include <gtest/gtest.h>
#include "core/utils/image_loader.h"

TEST(ImageLoaderTest, LoadPNG) {
    // 创建测试图片或使用现有图片
    auto image = lightui::ImageLoader::LoadFromFile("test.png");
    EXPECT_GT(image.width, 0);
    EXPECT_GT(image.height, 0);
    EXPECT_EQ(image.channels, 4);  // RGBA
    EXPECT_NE(image.data.get(), nullptr);
}
```

---

### Step 2: 集成 Lexbor

#### 2.1 添加为 Git Submodule

```bash
cd third_party
git submodule add https://github.com/lexbor/lexbor.git
cd lexbor
git checkout master
```

#### 2.2 修改 CMakeLists.txt

**文件**: `third_party/CMakeLists.txt`

在文件末尾添加：
```cmake
# ============================================
# Lexbor - HTML/CSS Parser
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/lexbor/CMakeLists.txt")
    message(STATUS "  Found Lexbor")
    
    # 配置 Lexbor 构建选项
    set(LEXBOR_BUILD_SHARED OFF CACHE BOOL "Build Lexbor as static library" FORCE)
    set(LEXBOR_BUILD_STATIC ON CACHE BOOL "Build Lexbor as static library" FORCE)
    set(LEXBOR_BUILD_TESTS OFF CACHE BOOL "Build Lexbor tests" FORCE)
    set(LEXBOR_BUILD_EXAMPLES OFF CACHE BOOL "Build Lexbor examples" FORCE)
    
    add_subdirectory(lexbor)
    
    set(LEXBOR_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  Lexbor not found. Please run: git submodule add https://github.com/lexbor/lexbor.git third_party/lexbor")
    set(LEXBOR_FOUND FALSE PARENT_SCOPE)
endif()
```

#### 2.3 创建 C++ 包装类

**文件**: `core/dom/selector_engine.h`
```cpp
#pragma once
#include <string>
#include <vector>
#include "core/dom/element.h"

namespace lightui {

class SelectorEngine {
public:
    // 查询单个元素
    static Element* QuerySelector(Element* root, const std::string& selector);
    
    // 查询所有匹配元素
    static std::vector<Element*> QuerySelectorAll(Element* root, const std::string& selector);
    
    // 检查元素是否匹配选择器
    static bool Matches(Element* element, const std::string& selector);
};

} // namespace lightui
```

**文件**: `core/dom/selector_engine.cpp`
```cpp
#include "selector_engine.h"
#include <lexbor/html/html.h>
#include <lexbor/selectors/selectors.h>

namespace lightui {

// 内部辅助函数：将我们的 DOM 树转换为 Lexbor DOM 树
// （或者直接在 Lexbor DOM 上操作）

Element* SelectorEngine::QuerySelector(Element* root, const std::string& selector) {
    // TODO: 实现
    // 1. 将 selector 字符串传给 Lexbor
    // 2. 在 root 的子树中查找
    // 3. 返回第一个匹配的元素
    return nullptr;
}

std::vector<Element*> SelectorEngine::QuerySelectorAll(Element* root, const std::string& selector) {
    // TODO: 实现
    return {};
}

bool SelectorEngine::Matches(Element* element, const std::string& selector) {
    // TODO: 实现
    return false;
}

} // namespace lightui
```

#### 2.4 集成到 Document

**文件**: `core/dom/document.h`

添加方法：
```cpp
class Document : public Node {
public:
    // ... 现有方法 ...
    
    // 新增：CSS 选择器查询
    Element* QuerySelector(const std::string& selector);
    std::vector<Element*> QuerySelectorAll(const std::string& selector);
};
```

**文件**: `core/dom/document.cpp`

实现：
```cpp
#include "selector_engine.h"

Element* Document::QuerySelector(const std::string& selector) {
    return SelectorEngine::QuerySelector(GetDocumentElement(), selector);
}

std::vector<Element*> Document::QuerySelectorAll(const std::string& selector) {
    return SelectorEngine::QuerySelectorAll(GetDocumentElement(), selector);
}
```

#### 2.5 JavaScript 绑定

**文件**: `core/quickjs/window_bindings.cpp`

添加绑定：
```cpp
// 在 InitBindings() 中添加

// document.querySelector
runtime_->RegisterFunction("__querySelector", [this](const json& args) -> json {
    auto doc = window_->GetDocument();
    if (!doc || !args.is_array() || args.empty() || !args[0].is_string()) {
        return nullptr;
    }
    
    std::string selector = args[0].get<std::string>();
    auto element = doc->QuerySelector(selector);
    
    if (!element) {
        return nullptr;
    }
    
    // 返回元素信息
    return {
        {"tagName", element->GetTagName()},
        {"id", element->GetId()},
        {"className", element->GetClassName()}
    };
});

// document.querySelectorAll
runtime_->RegisterFunction("__querySelectorAll", [this](const json& args) -> json {
    auto doc = window_->GetDocument();
    if (!doc || !args.is_array() || args.empty() || !args[0].is_string()) {
        return json::array();
    }
    
    std::string selector = args[0].get<std::string>();
    auto elements = doc->QuerySelectorAll(selector);
    
    json result = json::array();
    for (auto* elem : elements) {
        result.push_back({
            {"tagName", elem->GetTagName()},
            {"id", elem->GetId()},
            {"className", elem->GetClassName()}
        });
    }
    
    return result;
});

// 更新 document 对象
std::string document_code = R"(
    globalThis.document = {
        getElementById: function(id) { /* ... */ },
        querySelector: function(selector) {
            return __querySelector(selector);
        },
        querySelectorAll: function(selector) {
            return __querySelectorAll(selector);
        }
    };
)";
```

#### 2.6 测试

**文件**: `tests/test_selector_engine.cpp`
```cpp
#include <gtest/gtest.h>
#include "core/dom/document.h"
#include "core/dom/element.h"

TEST(SelectorEngineTest, QuerySelectorById) {
    auto doc = std::make_shared<Document>();
    auto div = doc->CreateElement("div");
    div->SetId("test-id");
    doc->GetBody()->AppendChild(div);
    
    auto result = doc->QuerySelector("#test-id");
    EXPECT_EQ(result, div.get());
}

TEST(SelectorEngineTest, QuerySelectorByClass) {
    auto doc = std::make_shared<Document>();
    auto div = doc->CreateElement("div");
    div->SetClassName("test-class");
    doc->GetBody()->AppendChild(div);
    
    auto result = doc->QuerySelector(".test-class");
    EXPECT_EQ(result, div.get());
}

TEST(SelectorEngineTest, QuerySelectorAll) {
    auto doc = std::make_shared<Document>();
    
    for (int i = 0; i < 3; i++) {
        auto div = doc->CreateElement("div");
        div->SetClassName("item");
        doc->GetBody()->AppendChild(div);
    }
    
    auto results = doc->QuerySelectorAll(".item");
    EXPECT_EQ(results.size(), 3);
}
```

---

## 🔧 构建和测试

### 重新配置 CMake
```bash
cd build
cmake ..
```

### 编译
```bash
cmake --build . --config Debug
```

### 运行测试
```bash
ctest -C Debug --output-on-failure
```

---

## 📝 验证清单

### stb_image 集成验证
- [ ] `third_party/stb/stb_image.h` 文件存在
- [ ] CMake 配置成功（显示 "Found stb_image"）
- [ ] `ImageLoader` 类编译成功
- [ ] 测试用例通过
- [ ] 能成功加载 PNG/JPG 图片

### Lexbor 集成验证
- [ ] `third_party/lexbor/` 目录存在
- [ ] CMake 配置成功（显示 "Found Lexbor"）
- [ ] `SelectorEngine` 类编译成功
- [ ] `document.querySelector` JavaScript 绑定工作
- [ ] 测试用例通过

---

## ⚠️ 常见问题

### Q1: stb_image 编译错误 "multiple definition"
**原因**: 多个 .cpp 文件都定义了 `STB_IMAGE_IMPLEMENTATION`

**解决**: 只在一个 .cpp 文件中定义
```cpp
// image_loader.cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// 其他文件只需要
#include "stb/stb_image.h"  // 不要定义 IMPLEMENTATION
```

### Q2: Lexbor 链接错误
**原因**: 没有链接 Lexbor 库

**解决**: 在 `core/CMakeLists.txt` 中添加
```cmake
target_link_libraries(lightui PRIVATE lexbor_static)
```

### Q3: Lexbor 头文件找不到
**原因**: 包含路径不正确

**解决**: 使用正确的包含路径
```cpp
#include <lexbor/html/html.h>
#include <lexbor/selectors/selectors.h>
```

---

## 📚 参考文档

- [stb_image 文档](https://github.com/nothings/stb/blob/master/stb_image.h)
- [Lexbor 文档](https://lexbor.com/docs/)
- [Lexbor API 参考](https://lexbor.com/api/)
- [CSS 选择器规范](https://www.w3.org/TR/selectors-4/)

---

**文档创建时间**: 2025-11-11  
**预计完成时间**: 2025-11-12

