# HTML元素标准参考文档

**目的**: 为MBink HTML元素实现提供标准参考  
**来源**: WHATWG HTML Living Standard, MDN Web Docs  
**更新**: 2025-11-12

---

## 📚 标准文档链接

### 官方规范
- **WHATWG HTML Living Standard**: https://html.spec.whatwg.org/
- **W3C HTML5**: https://www.w3.org/TR/html5/
- **MDN Web Docs**: https://developer.mozilla.org/en-US/docs/Web/HTML

### 接口定义
- **Web IDL**: https://webidl.spec.whatwg.org/
- **DOM Standard**: https://dom.spec.whatwg.org/
- **CSSOM**: https://drafts.csswg.org/cssom/

---

## 🏷️ HTML5标准元素分类

### 1. 文档结构 (Document Structure)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<html>` | HTMLHtmlElement | 根元素 | P3 |
| `<head>` | HTMLHeadElement | 文档头部 | P3 |
| `<body>` | HTMLBodyElement | 文档主体 | P3 |
| `<title>` | HTMLTitleElement | 文档标题 | P3 |
| `<meta>` | HTMLMetaElement | 元数据 | P3 |
| `<link>` | HTMLLinkElement | 外部资源链接 | P2 |
| `<style>` | HTMLStyleElement | 内联样式 | P2 |
| `<script>` | HTMLScriptElement | 脚本 | P1 |

### 2. 文本内容 (Text Content)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<div>` | HTMLDivElement | 通用容器 | ✅ 已实现(Element) |
| `<span>` | HTMLSpanElement | 内联容器 | ✅ 已实现(Element) |
| `<p>` | HTMLParagraphElement | 段落 | ✅ 已实现(Element) |
| `<h1>`-`<h6>` | HTMLHeadingElement | 标题 | ✅ 已实现(Element) |
| `<br>` | HTMLBRElement | 换行 | ✅ 已实现(Element) |
| `<hr>` | HTMLHRElement | 水平线 | ✅ 已实现(Element) |
| `<pre>` | HTMLPreElement | 预格式化文本 | ✅ 已实现(Element) |
| `<blockquote>` | HTMLQuoteElement | 块引用 | ✅ 已实现(Element) |
| `<code>` | HTMLElement | 代码 | ✅ 已实现(Element) |

### 3. 列表 (Lists)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<ul>` | HTMLUListElement | 无序列表 | ✅ 已实现(Element) |
| `<ol>` | HTMLOListElement | 有序列表 | ✅ 已实现(Element) |
| `<li>` | HTMLLIElement | 列表项 | ✅ 已实现(Element) |
| `<dl>` | HTMLDListElement | 定义列表 | ✅ 已实现(Element) |
| `<dt>` | HTMLElement | 定义术语 | ✅ 已实现(Element) |
| `<dd>` | HTMLElement | 定义描述 | ✅ 已实现(Element) |

### 4. 表格 (Tables)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<table>` | HTMLTableElement | 表格 | P2 |
| `<thead>` | HTMLTableSectionElement | 表头 | P2 |
| `<tbody>` | HTMLTableSectionElement | 表体 | P2 |
| `<tfoot>` | HTMLTableSectionElement | 表尾 | P2 |
| `<tr>` | HTMLTableRowElement | 表格行 | P2 |
| `<td>` | HTMLTableCellElement | 数据单元格 | P2 |
| `<th>` | HTMLTableCellElement | 表头单元格 | P2 |
| `<caption>` | HTMLTableCaptionElement | 表格标题 | P2 |
| `<col>` | HTMLTableColElement | 列 | P3 |
| `<colgroup>` | HTMLTableColElement | 列组 | P3 |

### 5. 表单 (Forms) ⭐ 最重要
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<form>` | HTMLFormElement | 表单 | 🔴 P0 |
| `<input>` | HTMLInputElement | 输入框 | ✅ 已实现 |
| `<textarea>` | HTMLTextAreaElement | 多行文本 | ✅ 已实现 |
| `<button>` | HTMLButtonElement | 按钮 | 🔴 P0 |
| `<select>` | HTMLSelectElement | 下拉选择 | 🔴 P0 |
| `<option>` | HTMLOptionElement | 选项 | 🔴 P0 |
| `<label>` | HTMLLabelElement | 标签 | 🟡 P1 |
| `<fieldset>` | HTMLFieldSetElement | 字段集 | P2 |
| `<legend>` | HTMLLegendElement | 字段集标题 | P2 |
| `<datalist>` | HTMLDataListElement | 数据列表 | P3 |
| `<optgroup>` | HTMLOptGroupElement | 选项组 | P3 |
| `<output>` | HTMLOutputElement | 输出 | P3 |
| `<progress>` | HTMLProgressElement | 进度条 | P2 |
| `<meter>` | HTMLMeterElement | 度量 | P3 |

### 6. 语义化元素 (Semantic Elements)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<header>` | HTMLElement | 页眉 | ✅ 已实现(Element) |
| `<footer>` | HTMLElement | 页脚 | ✅ 已实现(Element) |
| `<nav>` | HTMLElement | 导航 | ✅ 已实现(Element) |
| `<main>` | HTMLElement | 主内容 | ✅ 已实现(Element) |
| `<section>` | HTMLElement | 章节 | ✅ 已实现(Element) |
| `<article>` | HTMLElement | 文章 | ✅ 已实现(Element) |
| `<aside>` | HTMLElement | 侧边栏 | ✅ 已实现(Element) |
| `<figure>` | HTMLElement | 图表 | ✅ 已实现(Element) |
| `<figcaption>` | HTMLElement | 图表标题 | ✅ 已实现(Element) |
| `<mark>` | HTMLElement | 标记 | ✅ 已实现(Element) |
| `<time>` | HTMLTimeElement | 时间 | P3 |
| `<address>` | HTMLElement | 地址 | ✅ 已实现(Element) |

### 7. 媒体元素 (Media Elements)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<img>` | HTMLImageElement | 图片 | 🟡 P1 |
| `<video>` | HTMLVideoElement | 视频 | P2 |
| `<audio>` | HTMLAudioElement | 音频 | P2 |
| `<source>` | HTMLSourceElement | 媒体源 | P2 |
| `<track>` | HTMLTrackElement | 字幕轨道 | P3 |
| `<canvas>` | HTMLCanvasElement | 画布 | P2 |

### 8. 链接和嵌入 (Links & Embedding)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<a>` | HTMLAnchorElement | 超链接 | 🟡 P1 |
| `<area>` | HTMLAreaElement | 图像映射区域 | P3 |
| `<iframe>` | HTMLIFrameElement | 内联框架 | P2 |
| `<embed>` | HTMLEmbedElement | 嵌入内容 | P3 |
| `<object>` | HTMLObjectElement | 对象 | P3 |
| `<param>` | HTMLParamElement | 对象参数 | P3 |

### 9. 交互元素 (Interactive Elements)
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<details>` | HTMLDetailsElement | 详情折叠 | P2 |
| `<summary>` | HTMLElement | 详情摘要 | P2 |
| `<dialog>` | HTMLDialogElement | 对话框 | P2 |

### 10. Web Components
| 元素 | 接口 | 说明 | 优先级 |
|------|------|------|--------|
| `<slot>` | HTMLSlotElement | 插槽 | P3 |
| `<template>` | HTMLTemplateElement | 模板 | P2 |

---

## 📖 关键接口详细说明

### HTMLButtonElement

**标准**: https://html.spec.whatwg.org/multipage/form-elements.html#the-button-element

**Web IDL**:
```webidl
[Exposed=Window]
interface HTMLButtonElement : HTMLElement {
  [HTMLConstructor] constructor();

  [CEReactions] attribute boolean disabled;
  readonly attribute HTMLFormElement? form;
  [CEReactions] attribute DOMString formAction;
  [CEReactions] attribute DOMString formEnctype;
  [CEReactions] attribute DOMString formMethod;
  [CEReactions] attribute boolean formNoValidate;
  [CEReactions] attribute DOMString formTarget;
  [CEReactions] attribute DOMString name;
  [CEReactions] attribute DOMString type;
  [CEReactions] attribute DOMString value;

  readonly attribute boolean willValidate;
  readonly attribute ValidityState validity;
  readonly attribute DOMString validationMessage;
  boolean checkValidity();
  boolean reportValidity();
  undefined setCustomValidity(DOMString error);

  readonly attribute NodeList labels;
};
```

**C++实现映射**:
```cpp
class HTMLButtonElement : public Element {
public:
    // IDL属性
    bool GetDisabled() const;
    void SetDisabled(bool disabled);
    
    std::shared_ptr<HTMLFormElement> GetForm() const;
    
    std::string GetFormAction() const;
    void SetFormAction(const std::string& action);
    
    std::string GetFormEnctype() const;
    void SetFormEnctype(const std::string& enctype);
    
    std::string GetFormMethod() const;
    void SetFormMethod(const std::string& method);
    
    bool GetFormNoValidate() const;
    void SetFormNoValidate(bool no_validate);
    
    std::string GetFormTarget() const;
    void SetFormTarget(const std::string& target);
    
    std::string GetName() const;
    void SetName(const std::string& name);
    
    std::string GetType() const;
    void SetType(const std::string& type);
    
    std::string GetValue() const;
    void SetValue(const std::string& value);
    
    // 验证
    bool GetWillValidate() const;
    // ValidityState GetValidity() const;  // TODO: 实现ValidityState
    std::string GetValidationMessage() const;
    bool CheckValidity();
    bool ReportValidity();
    void SetCustomValidity(const std::string& error);
    
    // 标签
    std::vector<std::shared_ptr<HTMLLabelElement>> GetLabels() const;
};
```

---

### HTMLSelectElement

**标准**: https://html.spec.whatwg.org/multipage/form-elements.html#the-select-element

**Web IDL**:
```webidl
[Exposed=Window]
interface HTMLSelectElement : HTMLElement {
  [HTMLConstructor] constructor();

  [CEReactions] attribute DOMString autocomplete;
  [CEReactions] attribute boolean disabled;
  readonly attribute HTMLFormElement? form;
  [CEReactions] attribute boolean multiple;
  [CEReactions] attribute DOMString name;
  [CEReactions] attribute boolean required;
  [CEReactions] attribute unsigned long size;

  readonly attribute DOMString type;

  readonly attribute HTMLOptionsCollection options;
  [CEReactions] attribute unsigned long length;
  getter Element? item(unsigned long index);
  HTMLOptionElement? namedItem(DOMString name);
  [CEReactions] undefined add((HTMLOptionElement or HTMLOptGroupElement) element, optional (HTMLElement or long)? before = null);
  [CEReactions] undefined remove(); // ChildNode overload
  [CEReactions] undefined remove(long index);
  [CEReactions] setter undefined (unsigned long index, HTMLOptionElement? option);

  readonly attribute HTMLCollection selectedOptions;
  attribute long selectedIndex;
  attribute DOMString value;

  readonly attribute boolean willValidate;
  readonly attribute ValidityState validity;
  readonly attribute DOMString validationMessage;
  boolean checkValidity();
  boolean reportValidity();
  undefined setCustomValidity(DOMString error);

  readonly attribute NodeList labels;
};
```

---

### HTMLImageElement

**标准**: https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element

**Web IDL**:
```webidl
[Exposed=Window]
interface HTMLImageElement : HTMLElement {
  [HTMLConstructor] constructor();

  [CEReactions] attribute DOMString alt;
  [CEReactions] attribute DOMString src;
  [CEReactions] attribute DOMString srcset;
  [CEReactions] attribute DOMString sizes;
  [CEReactions] attribute DOMString? crossOrigin;
  [CEReactions] attribute DOMString useMap;
  [CEReactions] attribute boolean isMap;
  [CEReactions] attribute unsigned long width;
  [CEReactions] attribute unsigned long height;
  readonly attribute unsigned long naturalWidth;
  readonly attribute unsigned long naturalHeight;
  readonly attribute boolean complete;
  readonly attribute DOMString currentSrc;
  [CEReactions] attribute DOMString referrerPolicy;
  [CEReactions] attribute DOMString decoding;
  [CEReactions] attribute DOMString loading;
  [CEReactions] attribute DOMString fetchPriority;

  Promise<undefined> decode();
};
```

---

## 🎨 默认CSS样式参考

### 表单元素默认样式

**来源**: Chromium User Agent Stylesheet

```css
/* Button */
button {
    appearance: auto;
    writing-mode: horizontal-tb !important;
    text-rendering: auto;
    color: buttontext;
    letter-spacing: normal;
    word-spacing: normal;
    line-height: normal;
    text-transform: none;
    text-indent: 0px;
    text-shadow: none;
    display: inline-block;
    text-align: center;
    align-items: flex-start;
    cursor: default;
    box-sizing: border-box;
    background-color: buttonface;
    margin: 0em;
    padding: 1px 6px;
    border-width: 2px;
    border-style: outset;
    border-color: buttonborder;
    border-image: initial;
}

button:hover {
    background-color: buttonhighlight;
}

button:active {
    border-style: inset;
}

button:disabled {
    color: graytext;
}

/* Select */
select {
    appearance: auto;
    box-sizing: border-box;
    align-items: center;
    white-space: pre;
    -webkit-rtl-ordering: logical;
    color: fieldtext;
    background-color: field;
    cursor: default;
    margin: 0em;
    border-width: 1px;
    border-style: solid;
    border-color: -internal-light-dark(rgb(118, 118, 118), rgb(133, 133, 133));
    border-image: initial;
    border-radius: 0px;
}

/* Anchor */
a {
    color: -webkit-link;
    cursor: pointer;
    text-decoration: underline;
}

a:visited {
    color: -webkit-link-visited;
}

a:active {
    color: -webkit-activelink;
}

/* Image */
img {
    display: inline-block;
    overflow: clip;
    overflow-clip-margin: content-box;
}
```

---

## ✅ 实现检查清单

### 每个元素必须实现

- [ ] **构造函数**: 正确初始化所有成员变量
- [ ] **IDL属性**: 所有标准属性的getter/setter
- [ ] **默认样式**: 应用浏览器默认样式
- [ ] **伪类状态**: :hover, :active, :focus, :disabled等
- [ ] **事件处理**: 标准事件的正确触发
- [ ] **表单关联**: 如果是表单控件，正确关联到form
- [ ] **验证**: 如果支持验证，实现checkValidity等
- [ ] **渲染**: 正确渲染到Skia画布
- [ ] **测试**: 至少10个单元测试
- [ ] **文档**: 完整的注释和使用示例

---

## 📚 参考资料

### 标准文档
- [WHATWG HTML Living Standard](https://html.spec.whatwg.org/)
- [MDN HTML Element Reference](https://developer.mozilla.org/en-US/docs/Web/HTML/Element)
- [Web IDL Standard](https://webidl.spec.whatwg.org/)

### 实现参考
- [Chromium Blink](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/html/)
- [WebKit](https://github.com/WebKit/WebKit/tree/main/Source/WebCore/html)
- [Gecko (Firefox)](https://searchfox.org/mozilla-central/source/dom/html)

### 测试参考
- [Web Platform Tests](https://github.com/web-platform-tests/wpt)
- [Chromium Layout Tests](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/web_tests/)


