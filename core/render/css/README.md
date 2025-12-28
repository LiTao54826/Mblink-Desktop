# CSS Value System

CSS 值解析和计算系统。

## 模块列表

| 文件 | 描述 |
|------|------|
| `css_value.h/cpp` | CSS 值基类和派生类 |
| `css_parser.h/cpp` | CSS 值解析器 |
| `css_calc.h/cpp` | calc() 函数支持 |
| `css_color.h/cpp` | 颜色值处理 |

## 功能说明

支持的 CSS 值类型：
- 长度单位（px, em, rem, vw, vh 等）
- 颜色（hex, rgb, rgba, hsl, hsla, 命名颜色）
- 百分比
- calc() 表达式
- 关键字值
