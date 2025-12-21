# LightUI 组件库设计规范

## 设计原则

1. **简洁** - 最小化 props，合理默认值
2. **一致** - 统一的命名、尺寸、颜色规范
3. **轻量** - 无外部依赖，体积小
4. **可组合** - 组件可自由嵌套组合

## 技术栈

- Preact (3KB) - 渲染
- 无 CSS 框架 - 内联样式 + CSS 变量

## 命名约定

### Props 命名

```javascript
// 布尔值：is/has/can/show 前缀，或直接形容词
disabled, loading, checked, visible, readonly

// 尺寸：size 统一用 sm/md/lg
size="sm" | "md" | "lg"

// 变体：variant
variant="primary" | "secondary" | "outline" | "ghost" | "link"

// 状态：status
status="success" | "warning" | "error" | "info"

// 事件：on 前缀
onClick, onChange, onFocus, onBlur, onSubmit
```

### 尺寸规范

```javascript
// 统一尺寸 token
const sizes = {
  sm: { height: 28, fontSize: 12, padding: '4px 8px' },
  md: { height: 36, fontSize: 14, padding: '8px 16px' },
  lg: { height: 44, fontSize: 16, padding: '12px 24px' },
};
```

## 主题系统

```javascript
// theme.js - CSS 变量
const theme = {
  // 颜色
  colors: {
    primary: '#3b82f6',
    primaryHover: '#2563eb',
    secondary: '#64748b',
    success: '#22c55e',
    warning: '#f59e0b',
    error: '#ef4444',
    info: '#3b82f6',
    
    bg: '#ffffff',
    bgSecondary: '#f8fafc',
    bgHover: '#f1f5f9',
    
    text: '#1e293b',
    textSecondary: '#64748b',
    textDisabled: '#94a3b8',
    
    border: '#e2e8f0',
    borderHover: '#cbd5e1',
    borderFocus: '#3b82f6',
  },
  
  // 圆角
  radius: {
    sm: '4px',
    md: '6px',
    lg: '8px',
    full: '9999px',
  },
  
  // 阴影
  shadow: {
    sm: '0 1px 2px rgba(0,0,0,0.05)',
    md: '0 4px 6px rgba(0,0,0,0.1)',
    lg: '0 10px 15px rgba(0,0,0,0.1)',
  },
  
  // 间距
  space: {
    xs: '4px',
    sm: '8px',
    md: '16px',
    lg: '24px',
    xl: '32px',
  },
};
```


## 组件 API 设计

### Button

```javascript
// Props
{
  variant: 'primary' | 'secondary' | 'outline' | 'ghost' | 'link',  // 默认 'primary'
  size: 'sm' | 'md' | 'lg',      // 默认 'md'
  disabled: boolean,              // 默认 false
  loading: boolean,               // 默认 false
  block: boolean,                 // 全宽，默认 false
  icon: VNode,                    // 图标
  iconPosition: 'left' | 'right', // 默认 'left'
  onClick: (e) => void,
}

// 使用
h(Button, { variant: 'primary', onClick: handleClick }, 'Submit')
h(Button, { variant: 'outline', size: 'sm', loading: true }, 'Loading...')
h(Button, { variant: 'ghost', icon: h(Icon, { name: 'plus' }) }, 'Add')
```

### Input

```javascript
// Props
{
  type: 'text' | 'password' | 'number' | 'email' | 'tel' | 'url',  // 默认 'text'
  size: 'sm' | 'md' | 'lg',
  value: string | number,
  placeholder: string,
  disabled: boolean,
  readonly: boolean,
  maxLength: number,
  prefix: VNode | string,         // 前缀图标/文字
  suffix: VNode | string,         // 后缀图标/文字
  status: 'success' | 'warning' | 'error',
  onChange: (value, e) => void,
  onFocus: (e) => void,
  onBlur: (e) => void,
  onEnter: (value, e) => void,    // 回车事件
}

// 使用
h(Input, { placeholder: 'Enter name', onChange: setName })
h(Input, { type: 'password', prefix: h(Icon, { name: 'lock' }) })
h(Input, { type: 'number', suffix: '元', status: 'error' })
```

### Textarea

```javascript
// Props
{
  value: string,
  placeholder: string,
  rows: number,                   // 默认 3
  maxLength: number,
  showCount: boolean,             // 显示字数统计
  autoSize: boolean | { minRows, maxRows },
  disabled: boolean,
  readonly: boolean,
  onChange: (value, e) => void,
}
```

### Select

```javascript
// Props
{
  value: any,
  options: Array<{ value, label, disabled? }>,
  placeholder: string,
  size: 'sm' | 'md' | 'lg',
  disabled: boolean,
  multiple: boolean,              // 多选
  searchable: boolean,            // 可搜索
  clearable: boolean,             // 可清除
  onChange: (value) => void,
}

// 使用
h(Select, {
  value: selected,
  options: [
    { value: 1, label: 'Option 1' },
    { value: 2, label: 'Option 2' },
    { value: 3, label: 'Option 3', disabled: true },
  ],
  onChange: setSelected,
})
```

### Checkbox

```javascript
// Props
{
  checked: boolean,
  indeterminate: boolean,         // 半选状态
  disabled: boolean,
  onChange: (checked, e) => void,
}

// Checkbox.Group
{
  value: Array,
  options: Array<{ value, label, disabled? }>,
  direction: 'horizontal' | 'vertical',
  onChange: (values) => void,
}
```

### Radio

```javascript
// Radio.Group
{
  value: any,
  options: Array<{ value, label, disabled? }>,
  direction: 'horizontal' | 'vertical',
  onChange: (value) => void,
}
```

### Switch

```javascript
// Props
{
  checked: boolean,
  disabled: boolean,
  size: 'sm' | 'md',
  checkedText: string,
  uncheckedText: string,
  onChange: (checked) => void,
}
```


### 布局组件

#### Row / Column

```javascript
// Row - 水平布局
{
  gap: number | string,           // 间距，默认 0
  align: 'start' | 'center' | 'end' | 'stretch',  // 垂直对齐
  justify: 'start' | 'center' | 'end' | 'between' | 'around',  // 水平分布
  wrap: boolean,                  // 换行，默认 false
}

// Column - 垂直布局
{
  gap: number | string,
  align: 'start' | 'center' | 'end' | 'stretch',  // 水平对齐
}

// 使用
h(Row, { gap: 16, align: 'center' }, [
  h(Button, {}, 'Button 1'),
  h(Button, {}, 'Button 2'),
])

h(Column, { gap: 8 }, [
  h(Input, { placeholder: 'Name' }),
  h(Input, { placeholder: 'Email' }),
])
```

#### Grid

```javascript
// Props
{
  columns: number | string,       // 列数或 grid-template-columns
  rows: number | string,          // 行数或 grid-template-rows
  gap: number | string,
  rowGap: number | string,
  columnGap: number | string,
}

// 使用
h(Grid, { columns: 3, gap: 16 }, [
  h(Card, {}, 'Card 1'),
  h(Card, {}, 'Card 2'),
  h(Card, {}, 'Card 3'),
])

h(Grid, { columns: 'repeat(auto-fill, minmax(200px, 1fr))', gap: 16 }, [...])
```

#### Stack

```javascript
// 层叠布局
{
  direction: 'horizontal' | 'vertical',
  gap: number | string,
  align: 'start' | 'center' | 'end' | 'stretch',
}
```

#### Spacer

```javascript
// 弹性空白
{
  size: number | string,          // 固定大小，不设置则自动填充
}

// 使用
h(Row, {}, [
  h(Text, {}, 'Left'),
  h(Spacer),                      // 自动填充中间空白
  h(Button, {}, 'Right'),
])
```

### 数据展示

#### Text

```javascript
// Props
{
  size: 'xs' | 'sm' | 'md' | 'lg' | 'xl',
  weight: 'normal' | 'medium' | 'semibold' | 'bold',
  color: string,                  // 颜色名或色值
  align: 'left' | 'center' | 'right',
  truncate: boolean | number,     // 截断，number 为行数
}
```

#### Card

```javascript
// Props
{
  title: string | VNode,
  extra: VNode,                   // 右上角额外内容
  padding: number | string,
  shadow: 'none' | 'sm' | 'md' | 'lg',
  bordered: boolean,
  hoverable: boolean,             // hover 效果
  onClick: () => void,
}

// 使用
h(Card, { title: 'User Info', extra: h(Button, { size: 'sm' }, 'Edit') }, [
  h(Text, {}, 'Name: John'),
  h(Text, {}, 'Email: john@example.com'),
])
```

#### List

```javascript
// Props
{
  data: Array,
  renderItem: (item, index) => VNode,
  header: VNode,
  footer: VNode,
  bordered: boolean,
  split: boolean,                 // 分割线
  loading: boolean,
  empty: VNode,                   // 空状态
}

// 使用
h(List, {
  data: users,
  renderItem: (user) => h(List.Item, {
    title: user.name,
    description: user.email,
    extra: h(Button, { size: 'sm' }, 'View'),
  }),
})
```

#### Table

```javascript
// Props
{
  columns: Array<{
    key: string,
    title: string,
    width: number | string,
    align: 'left' | 'center' | 'right',
    render: (value, row, index) => VNode,
    sorter: boolean | (a, b) => number,
  }>,
  data: Array,
  rowKey: string | (row) => string,
  bordered: boolean,
  striped: boolean,               // 斑马纹
  hoverable: boolean,
  loading: boolean,
  empty: VNode,
  onRowClick: (row, index) => void,
  
  // 分页
  pagination: false | {
    current: number,
    pageSize: number,
    total: number,
    onChange: (page, pageSize) => void,
  },
}

// 使用
h(Table, {
  columns: [
    { key: 'name', title: 'Name', sorter: true },
    { key: 'age', title: 'Age', width: 80, align: 'center' },
    { key: 'email', title: 'Email' },
    { 
      key: 'actions', 
      title: 'Actions',
      render: (_, row) => h(Button, { size: 'sm', onClick: () => edit(row) }, 'Edit'),
    },
  ],
  data: users,
  rowKey: 'id',
  pagination: { current: 1, pageSize: 10, total: 100, onChange: handlePage },
})
```


### 反馈组件

#### Modal

```javascript
// Props
{
  visible: boolean,
  title: string | VNode,
  width: number | string,         // 默认 480
  closable: boolean,              // 显示关闭按钮，默认 true
  maskClosable: boolean,          // 点击遮罩关闭，默认 true
  footer: VNode | null,           // null 隐藏 footer
  onClose: () => void,
  onOk: () => void,
  okText: string,                 // 默认 'OK'
  cancelText: string,             // 默认 'Cancel'
  okLoading: boolean,
}

// 使用
h(Modal, {
  visible: showModal,
  title: 'Confirm',
  onClose: () => setShowModal(false),
  onOk: handleSubmit,
}, [
  h(Text, {}, 'Are you sure?'),
])

// 快捷方法
Modal.confirm({ title: 'Delete?', content: 'This cannot be undone', onOk: handleDelete })
Modal.alert({ title: 'Success', content: 'Operation completed' })
```

#### Drawer

```javascript
// Props
{
  visible: boolean,
  title: string | VNode,
  placement: 'left' | 'right' | 'top' | 'bottom',  // 默认 'right'
  width: number | string,         // left/right 时生效
  height: number | string,        // top/bottom 时生效
  closable: boolean,
  maskClosable: boolean,
  onClose: () => void,
}
```

#### Toast

```javascript
// 命令式调用
Toast.show('Message')
Toast.success('Success!')
Toast.error('Error!')
Toast.warning('Warning!')
Toast.loading('Loading...')

// 配置
Toast.show({
  content: 'Message',
  duration: 3000,                 // 默认 3000ms，0 为不自动关闭
  position: 'top' | 'center' | 'bottom',  // 默认 'top'
})

// 关闭
const hide = Toast.loading('Loading...')
// ... 
hide()
```

#### Tooltip

```javascript
// Props
{
  content: string | VNode,
  placement: 'top' | 'bottom' | 'left' | 'right',  // 默认 'top'
  trigger: 'hover' | 'click',     // 默认 'hover'
  delay: number,                  // 延迟显示，默认 100ms
}

// 使用
h(Tooltip, { content: 'This is a tooltip' }, 
  h(Button, {}, 'Hover me')
)
```

#### Popover

```javascript
// Props
{
  content: VNode,
  title: string | VNode,
  placement: 'top' | 'bottom' | 'left' | 'right',
  trigger: 'hover' | 'click',
  visible: boolean,               // 受控模式
  onVisibleChange: (visible) => void,
}
```

#### Progress

```javascript
// Props
{
  percent: number,                // 0-100
  status: 'normal' | 'success' | 'error',
  showInfo: boolean,              // 显示百分比，默认 true
  strokeWidth: number,            // 进度条高度
  strokeColor: string,
}
```

#### Spinner

```javascript
// Props
{
  size: 'sm' | 'md' | 'lg',
  color: string,
}

// 使用
h(Spinner, { size: 'lg' })
```


### 表单组件

#### Form

```javascript
// Props
{
  initialValues: object,
  onSubmit: (values) => void,
  onValuesChange: (changedValues, allValues) => void,
  layout: 'horizontal' | 'vertical' | 'inline',  // 默认 'vertical'
  labelWidth: number | string,    // horizontal 时生效
}

// Form.Item
{
  name: string,                   // 字段名
  label: string | VNode,
  required: boolean,
  rules: Array<{
    required?: boolean,
    message?: string,
    min?: number,
    max?: number,
    pattern?: RegExp,
    validator?: (value) => boolean | string | Promise,
  }>,
  help: string,                   // 帮助文字
  extra: VNode,                   // 额外内容
}

// 使用
h(Form, { 
  initialValues: { name: '', email: '' },
  onSubmit: handleSubmit,
}, [
  h(Form.Item, { 
    name: 'name', 
    label: 'Name', 
    required: true,
    rules: [{ required: true, message: 'Please enter name' }],
  }, 
    h(Input, { placeholder: 'Enter name' })
  ),
  
  h(Form.Item, { 
    name: 'email', 
    label: 'Email',
    rules: [
      { required: true, message: 'Please enter email' },
      { pattern: /^.+@.+\..+$/, message: 'Invalid email format' },
    ],
  }, 
    h(Input, { type: 'email', placeholder: 'Enter email' })
  ),
  
  h(Form.Item, {},
    h(Button, { type: 'submit', variant: 'primary' }, 'Submit')
  ),
])
```

#### useForm Hook

```javascript
// 表单控制 Hook
const form = useForm();

// 方法
form.getValues()                  // 获取所有值
form.getValue(name)               // 获取单个值
form.setValues(values)            // 设置多个值
form.setValue(name, value)        // 设置单个值
form.reset()                      // 重置表单
form.validate()                   // 验证全部，返回 Promise
form.validateField(name)          // 验证单个字段
form.getErrors()                  // 获取错误信息
form.clearErrors()                // 清除错误

// 使用
h(Form, { form, onSubmit: handleSubmit }, [...])

// 外部控制
const handleReset = () => form.reset();
const handleFill = () => form.setValues({ name: 'John', email: 'john@example.com' });
```

## 文件结构

```
components/
├── src/
│   ├── index.js                  # 导出所有组件
│   ├── theme.js                  # 主题变量
│   ├── utils.js                  # 工具函数
│   │
│   ├── button/
│   │   ├── index.js
│   │   └── styles.js
│   ├── input/
│   │   ├── index.js
│   │   ├── textarea.js
│   │   └── styles.js
│   ├── select/
│   │   └── index.js
│   ├── checkbox/
│   │   └── index.js
│   ├── radio/
│   │   └── index.js
│   ├── switch/
│   │   └── index.js
│   │
│   ├── layout/
│   │   ├── row.js
│   │   ├── column.js
│   │   ├── grid.js
│   │   ├── stack.js
│   │   └── spacer.js
│   │
│   ├── data/
│   │   ├── text.js
│   │   ├── card.js
│   │   ├── list.js
│   │   └── table.js
│   │
│   ├── feedback/
│   │   ├── modal.js
│   │   ├── drawer.js
│   │   ├── toast.js
│   │   ├── tooltip.js
│   │   ├── popover.js
│   │   ├── progress.js
│   │   └── spinner.js
│   │
│   └── form/
│       ├── form.js
│       ├── form-item.js
│       └── use-form.js
│
├── examples/
│   ├── basic.js
│   ├── form-demo.js
│   └── table-demo.js
│
└── build/
    └── components.bc             # 预编译字节码
```

## 实现顺序

```
Week 1:
├── theme.js + utils.js
├── Button
├── Input + Textarea
├── Select
├── Checkbox + Radio + Switch
└── 基础测试

Week 2:
├── Row + Column + Grid + Stack + Spacer
├── Text + Card
├── List
├── Table (基础版)
└── 布局测试

Week 3:
├── Modal + Drawer
├── Toast + Tooltip + Popover
├── Progress + Spinner
├── Form + FormItem + useForm
└── 完整示例 + 文档
```
