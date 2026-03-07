# LightUI 无边框 Preact 应用开发经验总结

> 基于 `sysmon` 示例的踩坑记录，适用于所有使用 `load_preact()` + `borderless=True` 的应用。

---

## 1. HTML 模板必须包含 CSS Reset

**问题**：`load_preact()` 前调用 `load_html()` 时，如果 HTML 模板没有任何 CSS，`html`/`body` 保留浏览器默认的 `margin` 和 `overflow: auto`，会导致**无论 JS 层如何设置都出现滚动条**。

**错误写法**：
```python
app.load_html("""<!DOCTYPE html>
<html><head><meta charset="utf-8"></head>
<body><div id="root"></div></body>
</html>""")
```

**正确写法**（参考 `borderless_demo.py`）：
```python
app.load_html("""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; overflow: hidden; }
    #root { width: 100%; height: 100%; }
  </style>
</head>
<body><div id="root"></div></body>
</html>""")
```

**原因**：`html`/`body`/`#root` 必须都设置 `height: 100%` 才能形成完整的高度传递链，Preact 根组件才能用 `height: 100%` 精确填满窗口。

---

## 2. 根组件用 `100%` 而非 `100vw/100vh`

**问题**：`100vw` 在某些 WebView 实现中不含滚动条宽度，内容稍微超出时会触发水平滚动条。

**错误写法**（JS inline style）：
```js
style: { width: '100vw', height: '100vh', overflow: 'hidden' }
```

**正确写法**：
```js
style: { width: '100%', height: '100%', overflow: 'hidden' }
```

---

## 3. `-webkit-app-region` 必须用 CSS 类，不能用 JS inline style

**问题**：`-webkit-app-region` 和 `-webkit-window-control` 是 WebKit 私有扩展属性，在这个 WebView 环境里**只认 CSS 属性名**（连字符形式），JS 的驼峰式 `WebkitAppRegion` **无效**，导致标题栏无法拖动、窗口控制按钮无效。

**错误写法**（JS inline style，不生效）：
```js
// ❌ 驼峰式在此 WebView 中无效
h('div', { style: { WebkitAppRegion: 'drag' } })
h('button', { style: { WebkitWindowControl: 'close' } })
```

**正确写法**：在 HTML `<style>` 中用 CSS 类定义，JS 中用 `class` 属性引用：

```html
<!-- HTML <style> 里定义 -->
<style>
  .drag    { -webkit-app-region: drag; }
  .no-drag { -webkit-app-region: no-drag; }

  .wc-btn      { width: 13px; height: 13px; border-radius: 50%; border: none; cursor: pointer; padding: 0; }
  .wc-pin      { background: #4f8ef7; -webkit-window-control: pin; }
  .wc-minimize { background: #64748b; -webkit-window-control: minimize; }
  .wc-maximize { background: #34d399; -webkit-window-control: maximize; }
  .wc-close    { background: #f87171; -webkit-window-control: close; }
</style>
```

```js
// ✅ Preact 组件里用 class 属性
h('div',    { class: 'drag' }, ...)
h('div',    { class: 'no-drag', style: { ... } }, ...)
h('button', { class: 'wc-btn wc-close', title: '关闭' })
```

> **注意**：Preact 里用 `class`（不是 `className`），因为这里运行的是原生 `h()` 而非 JSX 转译后的代码。

---

## 4. SVG 折线图用 `viewBox` 实现自适应宽度

**问题**：如果 SVG 用固定像素宽度（如 `width: 260`），当容器比 SVG 窄时（Grid 4列，每列约 209px < 260px），SVG 会水平溢出，触发滚动条。

**错误写法**：
```js
function Sparkline({ data, color, w = 260, h: height = 56 }) {
  const xStep = w / (data.length - 1);
  // ...
  return h('svg', { width: w, height })  // ❌ 固定宽度
}
```

**正确写法**：内部用固定坐标系（`viewBox`），外部用 `width: '100%'` 自适应：
```js
const SPARK_VW = 300;  // 内部坐标系，固定宽度
function Sparkline({ data, color, h: height = 44 }) {
  const xStep = SPARK_VW / (data.length - 1);  // 基于内部坐标系计算
  // ...
  return h('svg', {
    viewBox: '0 0 ' + SPARK_VW + ' ' + height,
    width: '100%',          // 外部自适应容器
    height: height,
    preserveAspectRatio: 'none',  // 允许非等比缩放以填满宽度
    style: { display: 'block' },  // 消除 SVG 默认 inline 底部间距
  })
}
```

---

## 5. Flex 布局防溢出：`minHeight: 0` 是关键

**问题**：Flex 子项默认 `min-height: auto`，内容高度超过 Flex 容器时不会收缩，导致溢出滚动条。

**正确写法**：所有 Flex 列方向的容器和可伸缩子项都加 `minHeight: 0`：
```js
// 内容区容器
{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden', minHeight: 0 }

// 可填充剩余空间的子项
{ flex: 1, overflow: 'hidden', minHeight: 0 }

// 固定高度、不伸缩的子项
{ flex: '0 0 auto' }
```

---

## 6. 完整无边框 Preact 应用模板

综合以上所有经验，最小可用模板如下：

**`main.py`**：
```python
from lightui import App

app = App("My App", 900, 600, borderless=True, resizable=True)
data = app.shared("data")

app.load_html("""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; overflow: hidden; }
    #root { width: 100%; height: 100%; }
    .drag    { -webkit-app-region: drag; }
    .no-drag { -webkit-app-region: no-drag; }
    .wc-btn      { width: 13px; height: 13px; border-radius: 50%; border: none; cursor: pointer; padding: 0; }
    .wc-minimize { background: #febc2e; -webkit-window-control: minimize; }
    .wc-maximize { background: #28c840; -webkit-window-control: maximize; }
    .wc-close    { background: #ff5f57; -webkit-window-control: close; }
  </style>
</head>
<body><div id="root"></div></body>
</html>""")

app.load_preact("ui/app.js")
app.run()
```

**`ui/app.js`**：
```js
const { h, render } = Preact;

function TitleBar() {
  return h('div', { class: 'drag', style: { height: '38px', display: 'flex', alignItems: 'center', padding: '0 12px' } },
    h('span', null, 'My App'),
    h('div', { class: 'no-drag', style: { marginLeft: 'auto', display: 'flex', gap: '6px' } },
      h('button', { class: 'wc-btn wc-minimize' }),
      h('button', { class: 'wc-btn wc-maximize' }),
      h('button', { class: 'wc-btn wc-close' }),
    )
  );
}

function App() {
  return h('div', { style: { width: '100%', height: '100%', display: 'flex', flexDirection: 'column', overflow: 'hidden' } },
    h(TitleBar),
    h('div', { style: { flex: 1, overflow: 'hidden', minHeight: 0 } },
      h('p', null, 'Hello, LightUI!')
    )
  );
}

var _root = document.getElementById('root');
globalThis.__onSharedUpdate = function() { render(h(App), _root); };
render(h(App), _root);
```

