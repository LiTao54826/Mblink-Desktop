# MBink CSS 属性支持状态

## 已支持的属性

### 盒模型
- `display` (block, inline, inline-block, flex, grid, none, table, table-row, table-cell)
- `box-sizing` (content-box, border-box)
- `width`, `height`, `min-width`, `max-width`, `min-height`, `max-height`
- `margin`, `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `padding`, `padding-top`, `padding-right`, `padding-bottom`, `padding-left`

### 边框
- `border`, `border-width`, `border-style`, `border-color`
- `border-top`, `border-right`, `border-bottom`, `border-left`
- `border-*-width`, `border-*-style`, `border-*-color`
- `border-radius`
- `border-collapse`, `border-spacing` (表格)

### 背景
- `background`, `background-color`
- `background-image` (包括 linear-gradient, radial-gradient)
- `background-repeat`, `background-size`

### 文本
- `color`
- `font-family`, `font-size`, `font-weight`, `font-style`
- `text-align`, `text-decoration`, `text-indent`
- `letter-spacing`, `word-spacing`
- `line-height`
- `white-space`, `word-wrap`, `text-overflow`
- `vertical-align`

### 定位
- `position` (static, relative, absolute, fixed)
- `top`, `right`, `bottom`, `left`
- `z-index`

### Flexbox
- `flex-direction`, `flex-wrap`
- `justify-content`, `align-items`, `align-content`, `align-self`
- `justify-items`, `justify-self`
- `place-items`, `place-self`, `place-content`
- `flex`, `flex-grow`, `flex-shrink`, `flex-basis`
- `order`
- `gap`, `row-gap`, `column-gap`

### Grid
- `grid-template-columns`, `grid-template-rows`
- `grid-auto-columns`, `grid-auto-rows`, `grid-auto-flow`
- `grid-column-gap`, `grid-row-gap`
- `grid-column`, `grid-row`

### 视觉效果
- `opacity`
- `visibility`
- `overflow`, `overflow-x`, `overflow-y`
- `box-shadow` ✅
- `text-shadow` ✅
- `transform`, `transform-origin`
- `transition`
- `filter`, `backdrop-filter`

### 其他
- `cursor`
- CSS 自定义属性 (--custom-property)
- `var()` 函数

## 已知问题

### camelCase 属性名转换 (已修复)
JavaScript 中使用 camelCase 属性名（如 `boxShadow`）现在会正确转换为 CSS 的 kebab-case（如 `box-shadow`）。

## 尚未支持的常用属性

### 动画
- `animation`, `animation-name`, `animation-duration`, `animation-timing-function`
- `animation-delay`, `animation-iteration-count`, `animation-direction`
- `animation-fill-mode`, `animation-play-state`
- `@keyframes`

### 文本高级
- `text-transform` (uppercase, lowercase, capitalize)
- `text-align-last`
- `word-break`
- `hyphens`
- `writing-mode`

### 列表
- `list-style`, `list-style-type`, `list-style-position`, `list-style-image`

### 表格高级
- `table-layout`
- `caption-side`
- `empty-cells`

### 多列布局
- `columns`, `column-count`, `column-width`
- `column-gap`, `column-rule`

### 其他
- `outline`, `outline-width`, `outline-style`, `outline-color`, `outline-offset`
- `resize`
- `user-select`
- `pointer-events`
- `clip-path`
- `mask`, `mask-image`
- `object-fit`, `object-position`
- `aspect-ratio`
- `scroll-behavior`
- `scroll-snap-*`

## 使用建议

1. 在 JavaScript 中设置样式时，可以使用 camelCase 或 kebab-case：
   ```javascript
   element.style.backgroundColor = 'red';  // camelCase
   element.style['background-color'] = 'red';  // kebab-case
   ```

2. 对于不支持的属性，样式会被忽略，不会导致错误。

3. 使用 Preact 时，样式对象中的属性名会自动转换：
   ```javascript
   h('div', { style: { boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } })
   ```
