# LightUI 测试套件修复建议

## 基于优先级的修复指南

本文档提供了修复全部 52 个失败测试的详细、可操作的建议，按优先级和影响程度组织。

---

## 🔴 关键优先级（修复 35 个测试 - 占失败的 67%）

### 1. 修复 JavaScript Document 绑定（30 个测试）

**影响：** 修复全部 30 个 DOMBindingsTest 失败

**根本原因：** `document` 对象未在 QuickJS JavaScript 运行时中注册，导致所有 JavaScript DOM 测试失败，报错 `ReferenceError: document is not defined`。

**位置：** `core/quickjs/window_bindings.cpp` 或类似的绑定初始化文件

**修复步骤：**

1. 定位 QuickJS 绑定初始化代码，即注册全局对象的位置
2. 添加 document 绑定注册：

```cpp
// In window bindings or similar initialization
void RegisterDocumentBindings(JSContext* ctx, Document* doc) {
    JSValue document_obj = JS_NewObject(ctx);
    
    // Register document methods
    JS_SetPropertyStr(ctx, document_obj, "createElement", 
        JS_NewCFunction(ctx, js_document_createElement, "createElement", 1));
    JS_SetPropertyStr(ctx, document_obj, "createTextNode",
        JS_NewCFunction(ctx, js_document_createTextNode, "createTextNode", 1));
    JS_SetPropertyStr(ctx, document_obj, "getElementById",
        JS_NewCFunction(ctx, js_document_getElementById, "getElementById", 1));
    JS_SetPropertyStr(ctx, document_obj, "querySelector",
        JS_NewCFunction(ctx, js_document_querySelector, "querySelector", 1));
    JS_SetPropertyStr(ctx, document_obj, "querySelectorAll",
        JS_NewCFunction(ctx, js_document_querySelectorAll, "querySelectorAll", 1));
    
    // Register document properties
    JSValue body = /* get body element */;
    JS_SetPropertyStr(ctx, document_obj, "body", body);
    
    JSValue head = /* get head element */;
    JS_SetPropertyStr(ctx, document_obj, "head", head);
    
    JSValue documentElement = /* get document element */;
    JS_SetPropertyStr(ctx, document_obj, "documentElement", documentElement);
    
    // Set as global
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "document", document_obj);
    JS_FreeValue(ctx, global);
}
```

3. 在 QuickJS 运行时初始化期间调用此注册函数
4. 确保 Document 指针被正确存储和访问

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=DOMBindingsTest.*
```

---

### 2. 修复元素删除时的 ID 缓存（2 个测试）

**影响：** 修复 DocumentTest.GetElementByIdAfterRemove、JavaScriptIntegrationTest.RemoveElement

**根本原因：** 当元素从 DOM 中删除时，其 ID 未从文档的 ID-元素缓存中移除，导致 getElementById 返回已删除的元素。

**位置：** `core/dom/document.cpp` - `Document::RemoveChild` 或类似方法

**修复步骤：**

1. 定位 Document 类的元素删除逻辑
2. 添加 ID 缓存清理：

```cpp
void Document::RemoveChild(Node* child) {
    if (!child) return;
    
    // If it's an element with an ID, remove from cache
    if (Element* elem = dynamic_cast<Element*>(child)) {
        std::string id = elem->GetAttribute("id");
        if (!id.empty()) {
            // Remove from ID cache
            id_cache_.erase(id);
        }
        
        // Recursively remove IDs from all descendants
        RemoveDescendantIdsFromCache(elem);
    }
    
    // Continue with normal removal
    // ... existing removal code ...
}

void Document::RemoveDescendantIdsFromCache(Element* parent) {
    for (Node* child : parent->GetChildren()) {
        if (Element* elem = dynamic_cast<Element*>(child)) {
            std::string id = elem->GetAttribute("id");
            if (!id.empty()) {
                id_cache_.erase(id);
            }
            RemoveDescendantIdsFromCache(elem);
        }
    }
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=DocumentTest.GetElementByIdAfterRemove
build\bin\Release\lightui_integration_tests.exe --gtest_filter=JavaScriptIntegrationTest.RemoveElement
```

---

### 3. 修复文本测量（3 个测试）

**影响：** 修复 TextRendererTest.MeasureMixedTextWidthSimple、MeasureMixedTextWidthLonger、MeasureMixedTextWidthDifferentSizes

**根本原因：** 文本测量返回 0 宽度，表明字体系统或 Skia 文本测量未正确初始化。

**位置：** `core/render/text_renderer.cpp` - `MeasureMixedTextWidth` 函数

**修复步骤：**

1. 检查 TextRenderer 构造函数中的字体初始化：

```cpp
TextRenderer::TextRenderer() {
    // Ensure default font is loaded
    if (!default_font_) {
        default_font_ = SkTypeface::MakeDefault();
    }
    
    // Ensure font is valid
    if (!default_font_) {
        // Log error or load fallback font
    }
}
```

2. 修复 MeasureMixedTextWidth 以正确测量文本：

```cpp
float TextRenderer::MeasureMixedTextWidth(const std::string& text, 
                                          const std::string& font_family,
                                          float font_size) {
    if (text.empty()) return 0.0f;
    
    // Create paint with font
    SkPaint paint;
    SkFont font;
    
    // Set font properties
    if (!font_family.empty()) {
        sk_sp<SkTypeface> typeface = SkTypeface::MakeFromName(
            font_family.c_str(), 
            SkFontStyle::Normal()
        );
        if (typeface) {
            font.setTypeface(typeface);
        }
    }
    
    font.setSize(font_size);
    
    // Measure text
    SkRect bounds;
    font.measureText(text.c_str(), text.length(), 
                     SkTextEncoding::kUTF8, &bounds);
    
    return bounds.width();
}
```

3. 确保在测试运行前正确初始化 Skia

**验证：**
```bash
build\bin\Release\lightui_render_tests.exe --gtest_filter=TextRendererTest.MeasureMixedTextWidth*
```

---

## 🟡 高优先级（修复 9 个测试 - 占失败的 17%）

### 4. 修复多重阴影解析（2 个测试）

**影响：** 修复 ShadowRendererTest.ParseMultipleBoxShadows、ParseMultipleTextShadows

**根本原因：** 阴影解析器在第一个阴影后停止，不处理逗号分隔的列表。

**位置：** `core/render/shadow_renderer.cpp` - `ParseBoxShadow` 和 `ParseTextShadow`

**修复步骤：**

```cpp
std::vector<BoxShadow> ShadowRenderer::ParseBoxShadow(const std::string& value) {
    std::vector<BoxShadow> shadows;
    
    // Split by comma (but not commas inside functions like rgba())
    std::vector<std::string> shadow_strings = SplitShadows(value);
    
    for (const auto& shadow_str : shadow_strings) {
        BoxShadow shadow = ParseSingleBoxShadow(shadow_str);
        if (shadow.IsValid()) {
            shadows.push_back(shadow);
        }
    }
    
    return shadows;
}

std::vector<std::string> ShadowRenderer::SplitShadows(const std::string& value) {
    std::vector<std::string> result;
    std::string current;
    int paren_depth = 0;
    
    for (char c : value) {
        if (c == '(') paren_depth++;
        else if (c == ')') paren_depth--;
        else if (c == ',' && paren_depth == 0) {
            if (!current.empty()) {
                result.push_back(Trim(current));
                current.clear();
            }
            continue;
        }
        current += c;
    }
    
    if (!current.empty()) {
        result.push_back(Trim(current));
    }
    
    return result;
}
```

**验证：**
```bash
build\bin\Release\lightui_render_tests.exe --gtest_filter=ShadowRendererTest.ParseMultiple*
```

---

### 5. 修复内联样式解析（2 个测试）

**影响：** 修复 HTMLLoadingTest.LoadInlineStyles、CSSStyleDeclarationTest.SyncWithElement

**根本原因：** HTML 中的 style 属性未被解析并应用到元素。

**位置：** `core/lexbor/lexbor_document.cpp` - HTML 解析代码

**修复步骤：**

```cpp
void LexborDocument::ProcessElement(lxb_dom_element_t* lxb_elem, Element* elem) {
    // ... existing attribute processing ...
    
    // Check for style attribute
    const lxb_char_t* style_value = lxb_dom_element_get_attribute(
        lxb_elem, 
        (const lxb_char_t*)"style", 
        5
    );
    
    if (style_value) {
        std::string style_str = reinterpret_cast<const char*>(style_value);
        
        // Parse inline style
        auto style_decl = elem->GetStyleDeclaration();
        style_decl->SetCssText(style_str);
        
        // Also set on element for GetStyle() to work
        ParseAndApplyInlineStyle(elem, style_str);
    }
}

void LexborDocument::ParseAndApplyInlineStyle(Element* elem, 
                                               const std::string& style_text) {
    // Split by semicolon
    std::vector<std::string> declarations = Split(style_text, ';');
    
    for (const auto& decl : declarations) {
        size_t colon_pos = decl.find(':');
        if (colon_pos != std::string::npos) {
            std::string property = Trim(decl.substr(0, colon_pos));
            std::string value = Trim(decl.substr(colon_pos + 1));
            
            // Set style on element
            elem->SetStyle(property, value);
        }
    }
}
```

**验证：**
```bash
build\bin\Release\lightui_integration_tests.exe --gtest_filter=HTMLLoadingTest.LoadInlineStyles
build\bin\Release\lightui_unit_tests.exe --gtest_filter=CSSStyleDeclarationTest.SyncWithElement
```

---

### 6. 修复文档 Head 创建（1 个测试）

**影响：** 修复 DocumentTest.HasHead

**根本原因：** 文档初始化期间未创建 head 元素。

**位置：** `core/dom/document.cpp` - Document 构造函数或初始化

**修复步骤：**

```cpp
Document::Document() {
    // Create document structure
    document_element_ = CreateElement("html");
    
    // Create and append head
    head_ = CreateElement("head");
    document_element_->AppendChild(head_);
    
    // Create and append body
    body_ = CreateElement("body");
    document_element_->AppendChild(body_);
}

Element* Document::GetHead() const {
    return head_;
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=DocumentTest.HasHead
```

---

### 7. 修复 HSL 颜色解析（1 个测试）

**影响：** 修复 CSSValueTest.ParseHslColor

**根本原因：** HSL 颜色格式解析器未实现。

**位置：** `core/render/color.cpp` - `Color::FromString` 或类似方法

**修复步骤：**

```cpp
uint32_t Color::ParseHsl(const std::string& hsl_str) {
    // Parse "hsl(120, 100%, 50%)" format
    std::regex hsl_regex(R"(hsl\s*\(\s*(\d+)\s*,\s*(\d+)%\s*,\s*(\d+)%\s*\))");
    std::smatch match;
    
    if (std::regex_match(hsl_str, match, hsl_regex)) {
        float h = std::stof(match[1].str());
        float s = std::stof(match[2].str()) / 100.0f;
        float l = std::stof(match[3].str()) / 100.0f;
        
        return HslToRgb(h, s, l);
    }
    
    return 0;
}

uint32_t Color::HslToRgb(float h, float s, float l) {
    h = h / 360.0f;
    
    auto hue_to_rgb = [](float p, float q, float t) {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1.0f/6.0f) return p + (q - p) * 6 * t;
        if (t < 1.0f/2.0f) return q;
        if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6;
        return p;
    };
    
    float r, g, b;
    
    if (s == 0) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hue_to_rgb(p, q, h + 1.0f/3.0f);
        g = hue_to_rgb(p, q, h);
        b = hue_to_rgb(p, q, h - 1.0f/3.0f);
    }
    
    return MakeRGBA(
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255),
        255
    );
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=CSSValueTest.ParseHslColor
```

---

### 8. 修复 HTML 序列化（1 个测试）

**影响：** 修复 HTMLLoadingTest.SaveHTML

**根本原因：** 修改的属性未包含在 HTML 序列化中。

**位置：** `core/dom/document.cpp` - `SaveHTML` 或序列化代码

**修复步骤：**

```cpp
std::string Document::SaveHTML() const {
    std::stringstream ss;
    SerializeNode(document_element_, ss);
    return ss.str();
}

void Document::SerializeNode(Node* node, std::stringstream& ss) const {
    if (!node) return;
    
    if (Element* elem = dynamic_cast<Element*>(node)) {
        ss << "<" << elem->GetTagName();
        
        // Serialize ALL attributes (including modified ones)
        const auto& attributes = elem->GetAllAttributes();
        for (const auto& [name, value] : attributes) {
            ss << " " << name << "=\"" << EscapeHtml(value) << "\"";
        }
        
        ss << ">";
        
        // Serialize children
        for (Node* child : elem->GetChildren()) {
            SerializeNode(child, ss);
        }
        
        ss << "</" << elem->GetTagName() << ">";
    }
    else if (Text* text = dynamic_cast<Text*>(node)) {
        ss << EscapeHtml(text->GetData());
    }
}
```

**验证：**
```bash
build\bin\Release\lightui_integration_tests.exe --gtest_filter=HTMLLoadingTest.SaveHTML
```

---

## 🟢 中等优先级（修复 6 个测试 - 占失败的 12%）

### 9. 修复 Calc 表达式百分比（2 个测试）

**影响：** 修复 CSSValueTest.ParseCalcExpression、ParseCalcAddition

**根本原因：** 百分比值存储为整数（100）而非浮点数（1.0）。

**位置：** `core/render/css_value.cpp` - calc 解析

**修复步骤：**

```cpp
CSSValue CSSValue::ParseCalc(const std::string& calc_str) {
    CSSValue result;
    result.type = CSSValueType::Calc;
    
    // Parse "calc(100% - 20px)" or similar
    // When parsing percentage, convert to 0-1 range
    
    std::regex percent_regex(R"((\d+(?:\.\d+)?)%)");
    std::smatch match;
    
    if (std::regex_search(calc_str, match, percent_regex)) {
        float percent_value = std::stof(match[1].str());
        result.calc_percent = percent_value / 100.0f;  // Convert to 0-1 range
    }
    
    // ... rest of calc parsing ...
    
    return result;
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=CSSValueTest.ParseCalc*
```

---

### 10. 修复十六进制颜色大小写（2 个测试）

**影响：** 修复 ColorTest.ToHex、ToHexWithAlpha

**根本原因：** 十六进制输出使用大写，测试期望小写。

**位置：** `core/render/color.cpp` - `ToHex` 函数

**修复步骤：**

```cpp
std::string Color::ToHex(uint32_t color, bool include_alpha) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;
    
    char buffer[10];
    if (include_alpha) {
        snprintf(buffer, sizeof(buffer), "#%02x%02x%02x%02x", r, g, b, a);
    } else {
        snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", r, g, b);
    }
    
    return std::string(buffer);
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=ColorTest.ToHex*
```

---

### 11. 修复脏矩形处理（1 个测试）

**影响：** 修复 DocumentTest.AddDirtyRect

**根本原因：** 脏矩形被合并，而它们应该保持独立。

**位置：** `core/dom/document.cpp` - `AddDirtyRect`

**修复步骤：**

```cpp
void Document::AddDirtyRect(const SkRect& rect) {
    // Option 1: Don't merge, keep all rects
    dirty_rects_.push_back(rect);
    
    // Option 2: Only merge if they overlap
    bool merged = false;
    for (auto& existing : dirty_rects_) {
        if (existing.intersects(rect)) {
            existing.join(rect);
            merged = true;
            break;
        }
    }
    
    if (!merged) {
        dirty_rects_.push_back(rect);
    }
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=DocumentTest.AddDirtyRect
```

---

### 12. 修复空令牌异常（1 个测试）

**影响：** 修复 DOMTokenListTest.EmptyToken

**根本原因：** 空令牌抛出异常而非优雅处理。

**位置：** `core/dom/dom_token_list.cpp` - `Add` 或验证

**修复步骤：**

```cpp
void DOMTokenList::Add(const std::string& token) {
    // Validate token
    if (token.empty()) {
        // Don't throw, just return or log warning
        return;
    }
    
    if (token.find_first_of(" \t\n\r") != std::string::npos) {
        throw std::invalid_argument("DOMTokenList: Token contains whitespace");
    }
    
    // ... rest of add logic ...
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=DOMTokenListTest.EmptyToken
```

---

## 🔵 低优先级（修复 2 个测试 - 占失败的 4%）

### 13. 修复重复任务（1 个测试）

**影响：** 修复 TaskSchedulerTest.ScheduleRepeatingTask

**根本原因：** 重复任务在执行后未被重新调度。

**位置：** `core/event/task_scheduler.cpp`

**修复步骤：**

```cpp
void TaskScheduler::ScheduleRepeatingTask(std::function<void()> task, 
                                          uint32_t interval_ms) {
    auto repeating_wrapper = [this, task, interval_ms]() {
        task();
        // Reschedule itself
        ScheduleDelayedTask([this, task, interval_ms]() {
            ScheduleRepeatingTask(task, interval_ms);
        }, interval_ms);
    };
    
    ScheduleDelayedTask(repeating_wrapper, interval_ms);
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=TaskSchedulerTest.ScheduleRepeatingTask
```

---

### 14. 修复按键状态跟踪（1 个测试）

**影响：** 修复 InputHandlerTest.HandleKeyEvent

**根本原因：** 按键按下状态未被记录。

**位置：** `core/event/input_handler.cpp`

**修复步骤：**

```cpp
void InputHandler::HandleKeyEvent(const SDL_KeyboardEvent& event) {
    SDL_Scancode scancode = event.keysym.scancode;
    
    if (event.type == SDL_EVENT_KEY_DOWN) {
        key_states_[scancode] = true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        key_states_[scancode] = false;
    }
    
    // ... rest of event handling ...
}

bool InputHandler::IsScancodeDown(SDL_Scancode scancode) const {
    auto it = key_states_.find(scancode);
    return it != key_states_.end() && it->second;
}
```

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=InputHandlerTest.HandleKeyEvent
```

---

### 15. 修复模块注册（1 个测试）

**影响：** 修复 QuickJSRuntimeTest.RegisterModule

**根本原因：** 注册模块时的 JSON 类型转换错误。

**位置：** `core/quickjs/quickjs_runtime.cpp`

**修复步骤：**

需要检查具体的模块注册代码，确保在 C++ 和 JavaScript 值之间转换时正确处理类型。

**验证：**
```bash
build\bin\Release\lightui_unit_tests.exe --gtest_filter=QuickJSRuntimeTest.RegisterModule
```

---

## 总结

### 按优先级的修复影响

| 优先级 | 修复测试数 | 百分比 | 累计 |
|--------|-----------|--------|------|
| 关键 | 35 | 67% | 67% |
| 高 | 9 | 17% | 84% |
| 中等 | 6 | 12% | 96% |
| 低 | 2 | 4% | 100% |

### 推荐修复顺序

1. **第 1 周：** 修复关键问题（JavaScript 绑定、ID 缓存、文本测量）
   - 结果：97.4% 通过率（567/581 测试）

2. **第 2 周：** 修复高优先级问题（阴影、内联样式、head、HSL、序列化）
   - 结果：99.0% 通过率（576/581 测试）

3. **第 3 周：** 修复中等优先级问题（calc、十六进制大小写、脏矩形、空令牌）
   - 结果：99.7% 通过率（580/581 测试）

4. **第 4 周：** 修复低优先级问题（重复任务、按键状态、模块注册）
   - 结果：100% 通过率（581/581 测试）

### 测试策略

每次修复后：
1. 运行特定测试以验证修复
2. 运行该类别的整个测试套件（unit/render/integration）
3. 运行所有测试以确保无回归
4. 提交修复并附上清晰的消息引用测试

```bash
# 运行特定测试
build\bin\Release\lightui_unit_tests.exe --gtest_filter=TestName

# 运行类别
build\bin\Release\lightui_unit_tests.exe

# 运行全部
build\bin\Release\lightui_render_tests.exe
build\bin\Release\lightui_unit_tests.exe
build\bin\Release\lightui_integration_tests.exe
```
