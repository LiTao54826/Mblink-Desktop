# MBink Fluent Design 组件库

基于 [Microsoft Fluent UI](https://fluent2.microsoft.design/) 设计规范实现的轻量级组件库，专为 MBink 桌面应用框架优化。

## 特性

- 🎨 **Fluent Design** - 完全遵循 Microsoft Fluent UI 设计规范
- 🪶 **轻量级** - 纯 JavaScript 实现，无额外依赖
- ⚡ **高性能** - 针对 MBink 渲染引擎优化
- 🔧 **易于使用** - 简洁的 API，与 Preact 无缝集成
- 📦 **模块化** - 按需导入，减少打包体积

## 安装

组件库已内置于 MBink，无需额外安装。

## 快速开始

```javascript
import { h, render } from 'preact';
import { Button, Input, Card, Text } from './fluent/index.js';

function App() {
  return h(Card, { appearance: 'filled' }, [
    h(Text, { size: 500, weight: 'semibold' }, 'Hello Fluent!'),
    h(Input, { placeholder: 'Enter your name' }),
    h(Button, { appearance: 'primary' }, 'Submit'),
  ]);
}

render(h(App), document.body);
```

## 组件列表

### 按钮组件

| 组件 | 描述 |
|------|------|
| `Button` | 基础按钮，支持多种外观和尺寸 |
| `CompoundButton` | 复合按钮，带主文本和次要文本 |
| `ToggleButton` | 切换按钮 |

```javascript
// Button 示例
h(Button, { appearance: 'primary', size: 'medium' }, 'Primary')
h(Button, { appearance: 'secondary' }, 'Secondary')
h(Button, { appearance: 'outline' }, 'Outline')
h(Button, { appearance: 'subtle' }, 'Subtle')
h(Button, { appearance: 'transparent' }, 'Transparent')
```

### 输入组件

| 组件 | 描述 |
|------|------|
| `Input` | 单行文本输入框 |
| `SearchBox` | 搜索框 |
| `Textarea` | 多行文本输入框 |
| `Select` | 下拉选择框 |
| `Checkbox` | 复选框 |
| `Switch` | 开关 |
| `Radio` / `RadioGroup` | 单选框 |

```javascript
// Input 示例
h(Input, { 
  appearance: 'outline',
  placeholder: 'Enter text...',
  contentBefore: h('span', null, '🔍'),
})

// Select 示例
h(Select, {
  options: [
    { value: 'a', label: 'Option A' },
    { value: 'b', label: 'Option B' },
  ],
  placeholder: 'Select...',
})

// Checkbox 示例
h(Checkbox, { label: 'Remember me' })

// Switch 示例
h(Switch, { label: 'Dark mode', labelPosition: 'before' })
```

### 数据展示组件

| 组件 | 描述 |
|------|------|
| `Text` | 文本组件，支持多种尺寸和样式 |
| `Title` / `Subtitle` / `Body` / `Caption` | 预设文本样式 |
| `Badge` / `CounterBadge` / `PresenceBadge` | 徽章组件 |
| `Avatar` / `AvatarGroup` | 头像组件 |
| `Card` / `CardHeader` / `CardFooter` | 卡片组件 |
| `Divider` | 分割线 |

```javascript
// Text 示例
h(Text, { size: 500, weight: 'semibold' }, 'Large Title')
h(Caption, null, 'Small caption text')

// Badge 示例
h(Badge, { color: 'brand' }, 'New')
h(CounterBadge, { count: 99 })
h(PresenceBadge, { status: 'available' })

// Avatar 示例
h(Avatar, { name: 'John Doe', size: 48 })
h(Avatar, { image: '/avatar.jpg', size: 32 })

// Card 示例
h(Card, { appearance: 'filled' }, [
  h(CardHeader, { 
    header: 'Card Title',
    description: 'Card description',
  }),
  h('p', null, 'Card content...'),
  h(CardFooter, null, [
    h(Button, { appearance: 'primary' }, 'Action'),
  ]),
])
```

### 反馈组件

| 组件 | 描述 |
|------|------|
| `Spinner` | 加载指示器 |
| `LoadingDots` | 加载点动画 |

```javascript
h(Spinner, { size: 'medium', label: 'Loading...' })
h(LoadingDots, { size: 'medium' })
```

### 导航组件

| 组件 | 描述 |
|------|------|
| `Link` | 链接组件 |

```javascript
h(Link, { href: '#' }, 'Click me')
h(Link, { appearance: 'subtle', inline: true }, 'Subtle link')
```

## 主题定制

组件库提供完整的主题 token，可以通过修改 `theme.js` 来定制主题：

```javascript
import { colors, borderRadius, shadow } from './fluent/theme.js';

// 使用主题 token
const customStyle = {
  backgroundColor: colors.colorBrandBackground,
  borderRadius: borderRadius.medium,
  boxShadow: shadow.shadow4,
};
```

### 主要 Token

- **colors** - 颜色系统（品牌色、中性色、状态色）
- **borderRadius** - 圆角（none, small, medium, large, xLarge, circular）
- **shadow** - 阴影（shadow2, shadow4, shadow8, shadow16, shadow28, shadow64）
- **spacing** - 间距（xxs, xs, s, m, l, xl, xxl, xxxl）
- **fontSize** - 字体大小（base100-600, hero700-1000）
- **fontWeight** - 字重（regular, medium, semibold, bold）

## 注意事项

### CSS 动画限制

MBink 目前对 CSS `@keyframes` 动画的支持有限，因此：
- `Spinner` 组件显示为静态状态
- 建议使用 `transition` 实现简单动画效果

### 与原生 Fluent UI 的差异

1. **简化实现** - 部分复杂组件进行了简化
2. **无 CSS-in-JS** - 使用内联样式而非 Griffel
3. **无 Portal** - 弹出层直接渲染在组件内部

## 文件结构

```
js/fluent/
├── index.js          # 入口文件
├── theme.js          # 主题系统
├── utils.js          # 工具函数
├── Button.js         # 按钮组件
├── Input.js          # 输入框组件
├── Textarea.js       # 多行输入组件
├── Select.js         # 下拉选择组件
├── Checkbox.js       # 复选框组件
├── Switch.js         # 开关组件
├── Radio.js          # 单选框组件
├── Text.js           # 文本组件
├── Badge.js          # 徽章组件
├── Avatar.js         # 头像组件
├── Card.js           # 卡片组件
├── Divider.js        # 分割线组件
├── Spinner.js        # 加载组件
├── Link.js           # 链接组件
└── README.md         # 文档
```

## 参考资源

- [Fluent UI React](https://react.fluentui.dev/)
- [Fluent 2 Design System](https://fluent2.microsoft.design/)
- [Fluent UI Design Tokens](https://react.fluentui.dev/?path=/docs/concepts-developer-design-tokens--page)

## License

MIT
