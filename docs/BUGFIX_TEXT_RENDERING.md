# 文字渲染问题修复报告

**日期**: 2025-11-11  
**问题**: hello_world 示例中文字不可见、出现黑色条纹  
**状态**: ✅ 已修复  

---

## 📋 问题描述

在运行 `hello_world.exe` 示例时，发现以下问题：

1. **文字不可见** - 窗口中看不到任何文字
2. **黑色条纹** - 窗口内容区域出现黑色横条
3. **h1 样式不生效** - 标题没有变大、没有加粗
4. **行间距太小** - 文字紧贴在一起

用户提供的 Skia 独立测试 (`test_skia_text_simple.cpp`) 证明 Skia 本身工作正常，问题出在渲染管线集成上。

---

## 🔍 问题分析

### Bug 1: 文字基线计算错误

**位置**: `core/render/render_object.cpp:352`

**原始代码**:
```cpp
text_renderer.DrawText(text_, 0, layout.height * 0.8f, font, text_paint);
```

**问题**:
- 使用 `layout.height * 0.8f` 计算基线位置
- 如果 `layout.height` 很小或为 0，文字会被绘制在 (0, 0) 附近
- 文字可能被绘制到可视区域外

**根本原因**: 
- 没有使用字体度量信息计算正确的基线
- Skia 的 `drawSimpleText` 的 Y 坐标是基线位置，不是顶部

---

### Bug 2: 背景渲染绘制黑色矩形

**位置**: `core/render/box_renderer.cpp:445`

**原始代码**:
```cpp
// 总是调用 drawPath，即使没有设置背景
canvas_->drawPath(path, paint.GetSkPaint());
```

**问题**:
- Paint 对象默认颜色是黑色
- 即使元素没有设置背景，也会绘制黑色矩形
- 导致窗口出现黑色条纹

**根本原因**:
- 没有检查元素是否真的有背景样式
- 无条件绘制导致默认黑色覆盖内容

---

### Bug 3: 行间距太小

**位置**: `core/render/render_object.cpp:146`

**原始代码**:
```cpp
current_y += child_layout.height;
```

**问题**:
- 只累加子元素的高度
- 没有考虑子元素的 `margin-top` 和 `margin-bottom`
- 导致元素紧贴在一起

**根本原因**:
- 布局算法不完整，忽略了 CSS 盒模型的 margin

---

### Bug 4: CSS 层叠顺序错误

**位置**: `core/render/style_resolver.cpp:29-49`

**原始代码**:
```cpp
ComputedStyle StyleResolver::ResolveStyle(...) {
    ComputedStyle style;
    
    // 1. 应用默认样式
    ApplyDefaultStyle(style, element->GetTagName());
    
    // 2. 应用继承
    if (parent_style) {
        ApplyInheritance(style, parent_style);  // ❌ 覆盖了默认样式！
    }
    
    // 3. 应用内联样式
    ApplyInlineStyle(style, element);
    
    return style;
}
```

**问题**:
- `ApplyInheritance` 无条件覆盖了 `font_size` 和 `font_weight`
- h1 的默认样式 (32px, bold) 被父元素的继承值 (16px, normal) 覆盖
- 所有文字都变成了 16px 正常字重

**根本原因**:
- CSS 层叠顺序错误
- 没有理解"用户代理样式表优先级高于继承"的规则

---

## 🔧 修复方案

### 修复 1: 使用字体度量计算基线

**文件**: `core/render/render_object.cpp`

```cpp
void RenderText::Paint(SkCanvas* canvas) {
    // ... 前置代码 ...
    
    // 获取字体度量信息
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    
    // 计算基线位置：从顶部，偏移 ascent（ascent 是负值）
    float baseline_y = -font_metrics.fAscent;
    
    // 在正确的基线位置绘制文字
    text_renderer.DrawText(text_, 0, baseline_y, font, text_paint);
    
    // ... 后续代码 ...
}
```

**原理**:
- `fAscent` 是负值，表示基线上方的距离
- `-fAscent` 得到从顶部到基线的距离
- 文字正确绘制在可视区域内

---

### 修复 2: 只在有背景时绘制

**文件**: `core/render/box_renderer.cpp`

```cpp
void BoxRenderer::RenderBackgroundAdvanced(...) {
    Paint paint;
    bool has_background = false;  // 跟踪是否有有效背景
    
    // 检查线性渐变
    if (/* 有线性渐变 */) {
        // ... 设置渐变 shader ...
        has_background = true;
    }
    // 检查径向渐变
    else if (/* 有径向渐变 */) {
        // ... 设置渐变 shader ...
        has_background = true;
    }
    // 检查背景图片
    else if (/* 有背景图片 */) {
        // ... 设置图片 shader ...
        has_background = true;
    }
    // 检查纯色背景
    else if (bg_color_it != styles.end() && !bg_color_it->second.empty() && 
             bg_color_it->second != "transparent") {
        paint.SetColor(Color::Parse(bg_color_it->second));
        has_background = true;
    }
    
    // 只在有有效背景时绘制
    if (has_background) {
        canvas_->drawPath(path, paint.GetSkPaint());
    }
}
```

---

### 修复 3: 布局时考虑 margin

**文件**: `core/render/render_object.cpp`

```cpp
for (auto& child : children_) {
    auto& child_layout = child->GetLayoutInfo();
    auto& child_style = child->GetComputedStyle();

    // 计算子元素的 margin
    float child_margin_top = child_style.margin.top.ToPx(width, child_style.font_size);
    float child_margin_bottom = child_style.margin.bottom.ToPx(width, child_style.font_size);
    float child_margin_left = child_style.margin.left.ToPx(width, child_style.font_size);

    // 设置子元素位置（考虑 margin）
    float new_x = padding_left + border_left + child_margin_left;
    float new_y = current_y + padding_top + border_top + body_top_offset + child_margin_top;

    child_layout.x = new_x;
    child_layout.y = new_y;

    // 累加高度（包括 margin）
    current_y += child_margin_top + child_layout.height + child_margin_bottom;
    max_child_height = std::max(max_child_height, child_layout.height);
}
```

---

### 修复 4: 正确的 CSS 层叠顺序

**文件**: `core/render/style_resolver.cpp`

**新的样式解析流程**:
```cpp
ComputedStyle StyleResolver::ResolveStyle(...) {
    ComputedStyle style;
    
    // 1. 基础默认值（仅根元素）
    ApplyDefaultStyle(style, element->GetTagName(), parent_style == nullptr);
    
    // 2. 继承父元素的可继承属性
    if (parent_style) {
        ApplyInheritance(style, parent_style);
    }
    
    // 3. 元素特定样式（h1 → 32px bold）- 覆盖继承
    ApplyElementSpecificStyle(style, element->GetTagName());
    
    // 4. 内联样式 - 最高优先级
    ApplyInlineStyle(style, element);
    
    return style;
}
```

**新增方法**: `ApplyElementSpecificStyle`
```cpp
void StyleResolver::ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name) {
    // 标题默认样式（覆盖继承的 font-size 和 font-weight）
    if (tag_name == "h1") {
        style.font_size = 32.0f;
        style.font_weight = "bold";
        style.margin.top = CSSLength(21, CSSUnit::PX);
        style.margin.bottom = CSSLength(21, CSSUnit::PX);
    } else if (tag_name == "h2") {
        style.font_size = 24.0f;
        style.font_weight = "bold";
        // ...
    }
    // ... 其他标签 ...
}
```

**修改**: `ApplyDefaultStyle` 只在根元素时设置基础值
```cpp
void StyleResolver::ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name, bool is_root) {
    // 只在根元素时设置基础默认值
    if (is_root) {
        style.color = "#000000";
        style.font_family = "Arial";
        style.font_size = 16.0f;
        style.font_weight = "normal";
        // ...
    }
    
    // 设置 display 属性（不可继承，所有元素都需要）
    style.display = RenderObjectType::BLOCK;
    // ...
}
```

---

## ✅ 验证结果

### 修复前
```
[RenderText] Text: 'Hello, LightUI!'
  Font: Arial, Size: 16, Weight: normal (400)  ❌ 应该是 32px bold
```

### 修复后
```
[RenderText] Text: 'Hello, LightUI!'
  Font: Arial, Size: 32, Weight: bold (700)    ✅ 正确！
```

### 视觉效果
- ✅ h1 标题明显更大（32px）
- ✅ h1 标题是粗体
- ✅ 三行文字之间有合理间距
- ✅ 没有黑色条纹
- ✅ 文字清晰可见

---

## 📚 技术要点

### 1. Skia 文字绘制
- `drawSimpleText` 的 Y 坐标是**基线位置**，不是顶部
- 必须使用 `SkFontMetrics` 计算正确的基线
- `fAscent` 是负值，`fDescent` 是正值

### 2. CSS 盒模型
- 完整的盒模型：`margin + border + padding + content`
- 布局时必须考虑所有部分
- margin 会影响元素之间的间距

### 3. CSS 层叠顺序
正确的优先级（从低到高）：
1. 浏览器默认样式
2. 继承的值
3. 用户代理样式表（如 h1 的默认样式）
4. 用户样式表
5. 内联样式
6. `!important`

### 4. 可继承属性 vs 不可继承属性
- **可继承**: color, font-family, font-size, font-weight, line-height, text-align
- **不可继承**: margin, padding, border, width, height, display

但是：**元素特定的默认样式优先级高于继承！**

---

## 🎯 影响范围

### 修改的文件
1. `core/render/render_object.cpp` - 文字基线、margin 布局
2. `core/render/box_renderer.cpp` - 背景渲染
3. `core/render/style_resolver.cpp` - CSS 层叠顺序
4. `core/render/style_resolver.h` - 新增方法声明

### 测试验证
- ✅ `hello_world.exe` 运行正常
- ✅ 文字清晰可见
- ✅ h1 样式正确（32px bold）
- ✅ 行间距合理
- ✅ 无黑色条纹

---

## 📝 经验教训

1. **字体渲染需要精确计算** - 不能用魔法数字（如 `0.8f`）
2. **绘制前要检查状态** - 不要无条件绘制默认值
3. **CSS 盒模型要完整实现** - margin/padding/border 都要考虑
4. **CSS 层叠顺序很重要** - 继承不应覆盖元素默认样式
5. **测试要覆盖视觉效果** - 单元测试通过不代表渲染正确

---

**修复者**: AI Assistant  
**审核者**: 待审核  
**状态**: ✅ 已修复并验证

