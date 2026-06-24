# Browser Compatibility | 浏览器兼容性

> 以当前仓库可见代码为准；如果文档与源码冲突，以源码为准。

## Summary | 总览

当前项目不是已验证完成的多浏览器兼容实现。
更准确的状态是：**实现了一套面向桌面端、偏 Chrome/Blink 风格的基础浏览器运行时兼容层**。

已知边界：
- 运行时 `navigator.userAgent` 伪装为 Chrome 120 on Windows
- Windows 平台构建/验证证据最完整
- 仓库中没有足够证据宣称完整兼容 Chrome / Firefox / Safari / Edge
- 仓库中没有足够证据宣称完整 Web 平台一致性

## Implemented Features | 已明确实现功能

### 1. Window / Navigator 基础环境
来源：`core/quickjs/window_bindings.cpp`

已实现：
- `window.innerWidth`
- `window.innerHeight`
- `window.devicePixelRatio`
- `window.title` 读写
- `window.minimize()`
- `window.maximize()`
- `window.restore()`
- `window.close()`
- `navigator.platform`
- `navigator.userAgent`
- `navigator.vendor`
- `navigator.language`
- `navigator.languages`
- `navigator.onLine`
- `navigator.cookieEnabled`
- `navigator.clipboard`（存在字段，当前为 `undefined`）
- `navigator.maxTouchPoints`

### 2. JavaScript Runtime 基础 API
来源：`js/runtime/bootstrap.js`

已提供：
- `console.log / error / warn / info / debug`
- `setTimeout / clearTimeout`
- `setInterval / clearInterval`
- `requestAnimationFrame / cancelAnimationFrame`
- `window` / `self` / `globalThis` 指向统一全局对象

说明：当前实现以兼容壳为主，多个能力仍是占位或有限支持，不是完整浏览器级行为。

### 3. DOM 查询与选择器
来源：`core/quickjs/document_bindings_impl.cpp`、`core/dom/bindings/dom_bindings.cpp`、`core/dom/selection/README.md`

已明确实现：
- `document.querySelector()`
- `document.querySelectorAll()`
- `element.querySelector()`
- CSS 选择器引擎模块
- `Selection API` 模块
- `Range` 模块

### 4. DOM Polyfills / 兼容补丁
来源：`js/polyfills/dom.js`

已实现或补充：
- `classList`（基于原型补充）
- `Element.prototype.matches`
- `Element.prototype.closest`
- `Element.prototype.append`
- `Element.prototype.prepend`
- `Element.prototype.remove`
- `Document.prototype.getElementById`
- `Document.prototype.getElementsByClassName`
- `Document.prototype.getElementsByTagName`
- `Event` 构造函数与 `preventDefault()`（缺失时补充）
- `Intl.NumberFormat` 最小兼容支持（用于图表场景，见 `js/polyfills` 相关实现）

### 5. Form 基础行为
来源：`core/dom/elements/html_form_element.cpp`

已实现：
- `form.method` 解析与规范化，仅接受 `get` / `post`
- `form.enctype` 解析与规范化
- 收集表单控件集合
- `submit` 事件触发
- `reset` 事件触发
- 基础 `checkValidity()` 流程
- 表单数据收集流程

### 6. Anchor 基础行为
来源：`core/dom/elements/html_anchor_element.cpp`

已实现：
- `href` / `target` / `download` / `rel` 属性同步
- `click` 事件触发
- `navigate` 自定义事件触发
- `download` 自定义事件触发
- `:link` / `:visited` 相关状态更新

### 7. CSS / Layout / Render 相关基础能力
来源：`core/layout/README.md`、`tests/css_compare/*`、`core/event/input/hit_test_controller.h`

已明确存在：
- Block layout
- Flexbox layout
- Grid layout
- Inline formatting context
- CSS 默认样式表：`css/default.css`
- 与 Chrome 的 CSS 对比测试基础设施
- 命中测试支持的场景：
  - static / relative / absolute / fixed
  - transform
  - clipping
  - stacking context

### 8. Network 基础能力
来源：`core/network/http_client.h`、`core/network/http_client.cpp`

已实现：
- HTTP method / headers / body / timeout
- redirect 跟随选项
- 基础 URL 解析
- `http` / `https`

## Partially Implemented / Limited | 部分实现或有限支持

- `console.*`：当前主要通过 `print()` 兜底，原生日志桥接尚未完整打通
- Timer API：当前 JS 侧有兼容实现，原生定时器桥接尚未完整打通
- `requestAnimationFrame`：当前是基于 `setTimeout(16)` 的临时实现
- `Element.prototype.matches`：当前只支持简单选择器（`#id`、`.class`、tag）
- `getElementById / getElementsByClassName / getElementsByTagName`：当前通过 `querySelector` / `querySelectorAll` 兜底，不是独立原生实现
- `navigator.clipboard`：仅保留字段，未提供可用能力
- 布局正确性：`core/layout/README.md` 明确提示仍依赖平台验证，不能据此宣称完整浏览器一致性

## Unsupported / Not Implemented Yet | 明确不支持或尚未实现

### 1. 完整浏览器导航行为
来源：`core/dom/elements/html_anchor_element.cpp`

当前未实现：
- `href` 的完整解析（相对路径 / 绝对路径 / 锚点）
- `target` 的完整打开行为（`_self` / `_blank` / `_parent` / `_top`）
- `rel` 语义处理（如 `noopener`、`noreferrer`）
- 浏览历史更新
- 下载资源获取与文件保存流程

### 2. 浏览器原生表单提交
来源：`core/dom/elements/html_form_element.cpp`

当前未实现：
- 按浏览器方式自动发起 HTTP 表单提交
- 完整的 `action` 导航处理
- 完整浏览器级校验与提交后导航行为

### 3. 完整 DOM / Runtime 浏览器环境
来源：`js/runtime/bootstrap.js`、`js/polyfills/dom.js`

当前不能视为已完整支持：
- 完整 `document` 初始化流程
- 完整原生 Console / Timer / RAF 桥接
- 完整 Element / Node / Document 扩展 API
- 完整 CSS 选择器匹配语义

### 4. 浏览器兼容声明范围
当前没有代码或仓库级验证证据支持以下声明：
- 完整兼容 Chrome
- 完整兼容 Firefox
- 完整兼容 Safari
- 完整兼容 Edge
- 已支持移动端浏览器环境
- 已实现完整 Web 标准平台

