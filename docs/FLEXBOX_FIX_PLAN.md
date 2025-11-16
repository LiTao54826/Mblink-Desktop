# Flexbox 布局修复完整计划

## 问题分析

### 当前状态
- ✅ Taffy CSS 布局引擎已编译并集成
- ✅ LayoutEngine 已实现并能调用 Taffy API
- ✅ ComputedStyle 已包含所有 Flexbox 属性
- ✅ StyleResolver 已支持解析 Flexbox 属性（flex-direction, justify-content 等）
- ✅ 文本节点尺寸测量已修复
- ❌ **`<style>` 标签中的 CSS 规则没有被应用到元素上**

### 根本原因
`StyleResolver::ResolveStyle()` 只应用了以下样式：
1. 默认样式（ApplyDefaultStyle）
2. 继承样式（ApplyInheritance）
3. 元素特定样式（ApplyElementSpecificStyle）
4. 伪类样式（ApplyPseudoClassStyles）
5. 内联样式（ApplyInlineStyle）

**缺少了从 `<style>` 标签解析的 CSS 规则！**

虽然 `StyleManager` 类已经实现并能解析 CSS 规则，但是：
- Document 没有 StyleManager 成员
- StyleResolver 没有使用 StyleManager
- `<style>` 标签的内容没有被解析和应用

---

## 完整修复方案

### 阶段 1: 为 Document 添加 StyleManager

#### 1.1 修改 `core/dom/document.h`
```cpp
// 在 Document 类中添加：
private:
    std::shared_ptr<Element> document_element_;
    std::shared_ptr<Element> body_;
    std::unordered_map<std::string, std::weak_ptr<Element>> id_map_;
    DOMObserverManager observer_manager_;

    // Lexbor 集成
    std::unique_ptr<LexborDocument> lexbor_doc_;
    bool lexbor_dirty_;

    // 批量更新
    int batch_depth_ = 0;
    
    // ✅ 新增：样式管理器
    std::unique_ptr<StyleManager> style_manager_;

public:
    // ✅ 新增：获取样式管理器
    StyleManager* GetStyleManager() const { return style_manager_.get(); }
```

#### 1.2 修改 `core/dom/document.cpp`
```cpp
// 在构造函数中初始化 StyleManager
Document::Document()
    : Node(NodeType::DOCUMENT_NODE)
    , document_element_(nullptr)
    , body_(nullptr)
    , id_map_()
    , lexbor_doc_(std::make_unique<LexborDocument>())
    , lexbor_dirty_(false)
    , style_manager_(std::make_unique<StyleManager>(this)) {  // ✅ 新增
}

// 在 LoadHTML 方法中解析 <style> 标签
void Document::LoadHTML(const std::string& html) {
    // ... 现有的 HTML 解析代码 ...
    
    // ✅ 新增：解析所有 <style> 标签
    auto style_elements = QuerySelectorAll("style");
    for (auto& style_elem : style_elements) {
        if (style_manager_) {
            style_manager_->ParseStyleElement(style_elem.get());
        }
    }
}
```

#### 1.3 添加头文件引用
在 `core/dom/document.h` 顶部添加：
```cpp
#include "core/lexbor/style_manager.h"
```

---

### 阶段 2: 修改 StyleResolver 使用 StyleManager

#### 2.1 修改 `core/render/style_resolver.h`
```cpp
class StyleResolver {
public:
    StyleResolver();
    ~StyleResolver();

    // ✅ 新增：设置 StyleManager
    void SetStyleManager(StyleManager* manager) { style_manager_ = manager; }

    ComputedStyle ResolveStyle(std::shared_ptr<Element> element,
                              const ComputedStyle* parent_style = nullptr);

private:
    void ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name, bool is_root);
    void ApplyInheritance(ComputedStyle& style, const ComputedStyle* parent_style);
    void ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name, 
                                   std::shared_ptr<Element> element);
    void ApplyPseudoClassStyles(ComputedStyle& style, std::shared_ptr<Element> element);
    void ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element);
    
    // ✅ 新增：应用 CSS 规则
    void ApplyCSSRules(ComputedStyle& style, std::shared_ptr<Element> element);
    
    void ParseStyleProperty(ComputedStyle& style, const std::string& property, 
                           const std::string& value);
    RenderObjectType ParseDisplay(const std::string& value);
    bool IsCustomProperty(const std::string& property) const;

    // ✅ 新增：StyleManager 指针
    StyleManager* style_manager_ = nullptr;
};
```

#### 2.2 修改 `core/render/style_resolver.cpp`

**修改 ResolveStyle 方法：**
```cpp
ComputedStyle StyleResolver::ResolveStyle(std::shared_ptr<Element> element,
                                         const ComputedStyle* parent_style) {
    if (!element) {
        return ComputedStyle();
    }

    ComputedStyle style;

    // CSS 层叠顺序（从低到高优先级）：
    // 1. 用户代理样式表（默认样式）- 最低优先级
    ApplyDefaultStyle(style, element->GetTagName(), parent_style == nullptr);

    // 2. 继承的值 - 从父元素继承可继承属性
    if (parent_style) {
        ApplyInheritance(style, parent_style);
    }

    // 3. 元素特定的默认样式 - 覆盖继承（如 h1 的 font-size, strong 的 bold）
    ApplyElementSpecificStyle(style, element->GetTagName(), element);

    // ✅ 新增：4. CSS 规则（<style> 标签和外部样式表）
    ApplyCSSRules(style, element);

    // 5. 伪类样式（如 :hover, :active, :focus）
    ApplyPseudoClassStyles(style, element);

    // 6. 内联样式（最高优先级）- 覆盖所有
    ApplyInlineStyle(style, element);

    return style;
}
```

**新增 ApplyCSSRules 方法：**
```cpp
void StyleResolver::ApplyCSSRules(ComputedStyle& style, std::shared_ptr<Element> element) {
    if (!style_manager_ || !element) {
        return;
    }

    // 从 StyleManager 获取匹配的 CSS 规则
    auto css_properties = style_manager_->ComputeStyle(element.get());

    // 应用每个 CSS 属性
    for (const auto& [property, value] : css_properties) {
        ParseStyleProperty(style, property, value);
    }
}
```

---

### 阶段 3: 修改 RenderTreeBuilder 传递 StyleManager

#### 3.1 修改 `core/render/style_resolver.h`
```cpp
class RenderTreeBuilder {
public:
    RenderTreeBuilder();
    ~RenderTreeBuilder();

    // ✅ 新增：设置 Document（用于获取 StyleManager）
    void SetDocument(Document* doc) { document_ = doc; }

    std::shared_ptr<RenderObject> BuildRenderTree(std::shared_ptr<Node> node,
                                                  const ComputedStyle* parent_style = nullptr);

private:
    std::shared_ptr<RenderObject> CreateRenderObjectForElement(
        std::shared_ptr<Element> element,
        const ComputedStyle* parent_style);
    
    std::shared_ptr<RenderObject> CreateRenderObjectForText(
        std::shared_ptr<Text> text,
        const ComputedStyle* parent_style);
    
    std::shared_ptr<RenderObject> CreateRenderObjectByType(RenderObjectType type);

    StyleResolver style_resolver_;
    
    // ✅ 新增：Document 指针
    Document* document_ = nullptr;
};
```

#### 3.2 修改 `core/render/style_resolver.cpp`

**修改 BuildRenderTree 方法：**
```cpp
std::shared_ptr<RenderObject> RenderTreeBuilder::BuildRenderTree(
    std::shared_ptr<Node> node,
    const ComputedStyle* parent_style) {
    
    if (!node) {
        return nullptr;
    }

    // ✅ 新增：设置 StyleManager（如果有 Document）
    if (document_ && document_->GetStyleManager()) {
        style_resolver_.SetStyleManager(document_->GetStyleManager());
    }

    std::shared_ptr<RenderObject> render_obj;

    // 根据节点类型创建渲染对象
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        render_obj = CreateRenderObjectForElement(element, parent_style);
    }
    else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        render_obj = CreateRenderObjectForText(text, parent_style);
    }

    if (!render_obj) {
        return nullptr;
    }

    // 如果是 display: none，不创建渲染对象
    if (render_obj->GetComputedStyle().display == RenderObjectType::NONE) {
        return nullptr;
    }

    // 递归构建子树
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        auto child_render_obj = BuildRenderTree(child, &render_obj->GetComputedStyle());
        if (child_render_obj) {
            render_obj->AppendChild(child_render_obj);
        }
    }

    return render_obj;
}
```

---

### 阶段 4: 修改 Window 传递 Document 给 RenderTreeBuilder

#### 4.1 修改 `core/window/window.cpp`

在 `RenderDocument()` 方法中：
```cpp
void Window::RenderDocument() {
    if (!document_ || !canvas_) {
        return;
    }

    // ... 现有代码 ...

    // 构建渲染树
    RenderTreeBuilder builder;
    
    // ✅ 新增：设置 Document（用于访问 StyleManager）
    builder.SetDocument(document_.get());
    
    auto root_render = builder.BuildRenderTree(document_->GetDocumentElement());

    // ... 其余代码 ...
}
```

---

### 阶段 5: 确保 CMakeLists.txt 包含 StyleManager

#### 5.1 检查 `core/lexbor/CMakeLists.txt`
确保包含：
```cmake
set(LEXBOR_SOURCES
    lexbor_document.cpp
    lexbor_stylesheet.cpp
    style_manager.cpp      # ✅ 确保存在
    cascade_engine.cpp
)
```

---

## 实施步骤

### Step 1: 修改 Document 类
1. 编辑 `core/dom/document.h` - 添加 StyleManager 成员和方法
2. 编辑 `core/dom/document.cpp` - 初始化 StyleManager 并解析 `<style>` 标签
3. 添加必要的头文件引用

### Step 2: 修改 StyleResolver 类
1. 编辑 `core/render/style_resolver.h` - 添加 StyleManager 指针和 ApplyCSSRules 方法
2. 编辑 `core/render/style_resolver.cpp` - 实现 ApplyCSSRules 并修改 ResolveStyle

### Step 3: 修改 RenderTreeBuilder 类
1. 编辑 `core/render/style_resolver.h` - 添加 Document 指针和 SetDocument 方法
2. 编辑 `core/render/style_resolver.cpp` - 在 BuildRenderTree 中设置 StyleManager

### Step 4: 修改 Window 类
1. 编辑 `core/window/window.cpp` - 在 RenderDocument 中调用 builder.SetDocument()

### Step 5: 编译和测试
1. 编译项目：`cmake --build build --config Release --target flexbox_test -j 8`
2. 运行测试：`build/bin/Release/flexbox_test.exe`
3. 验证 Flexbox 布局是否正确显示

---

## 预期结果

修复后，flexbox_test.html 应该显示：

### 第一个容器（紫色背景，row + space-between）
```
[Item 1]        [Item 2]        [Item 3]
```
三个项目水平排列，两端对齐

### 第二个容器（绿色背景，column）
```
[Column Item 1]
[Column Item 2]
[Column Item 3]
```
三个项目垂直排列

### 第三个容器（红色背景，wrap）
```
[Wrap 1] [Wrap 2] [Wrap 3]
[Wrap 4] [Wrap 5]
```
五个项目自动换行

---

## 调试检查点

如果修复后仍有问题，检查以下内容：

1. **StyleManager 是否被正确初始化？**
   - 在 Document 构造函数中添加日志
   - 检查 `style_manager_` 是否为 nullptr

2. **`<style>` 标签是否被解析？**
   - 在 `Document::LoadHTML` 中添加日志
   - 检查 `QuerySelectorAll("style")` 是否找到元素
   - 检查 `ParseStyleElement` 是否成功

3. **CSS 规则是否被匹配？**
   - 在 `StyleManager::GetMatchingRules` 中添加日志
   - 检查返回的规则数量

4. **CSS 属性是否被应用？**
   - 在 `StyleResolver::ApplyCSSRules` 中添加日志
   - 打印 `css_properties` 的内容

5. **Flexbox 属性是否传递给 Taffy？**
   - 在 `LayoutEngine::ApplyStyle` 中添加日志
   - 检查 `style.display == RenderObjectType::FLEX` 是否为 true
   - 检查 `style.flex_direction`, `style.justify_content` 等值

---

## 文件清单

需要修改的文件：
1. `core/dom/document.h`
2. `core/dom/document.cpp`
3. `core/render/style_resolver.h`
4. `core/render/style_resolver.cpp`
5. `core/window/window.cpp`

需要检查的文件：
1. `core/lexbor/CMakeLists.txt`
2. `core/lexbor/style_manager.h`
3. `core/lexbor/style_manager.cpp`

---

## 时间估计

- 阶段 1: 15 分钟
- 阶段 2: 20 分钟
- 阶段 3: 10 分钟
- 阶段 4: 5 分钟
- 阶段 5: 5 分钟
- 编译和测试: 10 分钟

**总计: 约 65 分钟**

