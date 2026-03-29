# Sysmon Notes | Sysmon 说明

## Overview | 概览

This note records practical lessons visible from the `sysmon` example and related borderless Preact usage.  
本文档记录 `sysmon` 示例及相关无边框 Preact 用法中的实操经验。

## Main Takeaways | 主要结论

- provide a CSS reset in the HTML template  
  在 HTML 模板中提供 CSS Reset
- prefer `width: 100%` / `height: 100%` over `100vw` / `100vh` for the root app container  
  根容器优先使用 `100%` 宽高而不是 `100vw` / `100vh`
- use CSS classes rather than JS inline style for WebKit-specific window-region properties when required by the runtime  
  当运行时要求时，应通过 CSS 类而不是 JS inline style 设置 WebKit 窗口区域属性
- use responsive SVG techniques such as `viewBox` to avoid overflow  
  使用 `viewBox` 等响应式 SVG 技术避免溢出
- in flex layouts, `min-height: 0` can be critical for preventing nested overflow  
  在 Flex 布局中，`min-height: 0` 往往是防止嵌套溢出的关键

## Usage Guidance | 使用建议

Treat this file as implementation notes for example maintenance, not as a formal platform guarantee.  
本文件更适合作为示例维护笔记，而不是正式的平台保证。

## Related Files | 相关文件

- `bindings/python/examples/sysmon/`
- `examples/modern_desktop_demo/`
- `docs/KNOWN_ISSUES.md`

