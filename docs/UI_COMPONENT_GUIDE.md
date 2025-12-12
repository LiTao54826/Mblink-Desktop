# MBink UI 组件使用指南

## 概述

MBink 当前支持两种方式使用 UI 组件：
1. **自定义轻量组件**（推荐）- 直接使用 Preact h() 函数创建
2. **外部组件库**（实验性）- 需要适配和打包

---

## 方法 1: 自定义轻量组件（推荐）✅

### 优势
- ✅ 无需额外依赖
- ✅ 完全控制样式和行为
- ✅ 体积小，性能好
- ✅ 遵循 JavaScript-first 架构

### 示例：创建 Button 组件

```javascript
function Button(props) {
    const { children, onClick, variant = 'contained', color = 'primary' } = props;
    
    // 定义样式
    const styles = {
        primary: { bg: '#1976d2', text: '#fff' },
        secondary: { bg: '#dc004e', text: '#fff' }
    };
    
    const theme = styles[color] || styles.primary;
    
    return preact.h('button', {
        style: `
            padding: 8px 16px;
            background: ${theme.bg};
            color: ${theme.text};
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        `,
        onclick: onClick
    }, children);
}

// 使用
preact.h(Button, {
    variant: 'contained',
    color: 'primary',
    onClick: () => console.log('clicked')
}, '点击我');
```

### 完整示例

参见：`examples/demo_html/preact_ui_component_test.js`

包含的组件：
- Button（按钮）
- Card（卡片）
- Input（输入框）

---

## 方法 2: 外部组件库（实验性）⚠️

### 当前限制

1. **模块系统限制**
   - QuickJS 对 ES Module 支持有限
   - 主流 UI 库使用 npm/webpack

2. **可尝试的方案**

#### 方案 A: UMD 格式的 Preact 组件（推荐尝试）

```javascript
// 1. 下载 UMD 格式的组件库
// 例如：preact-material-components.umd.js

// 2. 放置到 js/vendor/ 目录
// js/vendor/preact-material-components.umd.js

// 3. 在 HTML 中引入
<script src="../../js/vendor/preact-material-components.umd.js"></script>

// 4. 使用全局变量
const { Button } = window.PreactMaterialComponents;
preact.h(Button, { ... }, 'Click');
```

#### 方案 B: CDN 加载（需要测试）

```javascript
// 动态加载脚本
function loadExternalScript(url) {
    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = url;
        script.onload = resolve;
        script.onerror = reject;
        document.head.appendChild(script);
    });
}

// 使用
loadExternalScript('https://unpkg.com/preact@10.x/dist/preact.umd.js')
    .then(() => console.log('加载成功'));
```

---

## 推荐的 Preact 兼容组件库

### 1. preact-material-components
- **特点**: Material Design 风格
- **体积**: ~200KB
- **格式**: 支持 UMD
- **地址**: https://github.com/prateekbh/preact-material-components

### 2. preact-ui-kit
- **特点**: 轻量简洁
- **体积**: ~50KB
- **格式**: 需要打包

### 3. 自定义组件（推荐）
- **特点**: 完全控制
- **体积**: 可控
- **维护**: 自己维护

---

## 不推荐的方案 ❌

### ❌ 直接使用 Ant Design / Material-UI (React)

**原因**:
1. 依赖 React（不兼容 Preact）
2. 需要复杂的模块打包
3. 体积过大（~2MB）
4. 需要 CSS-in-JS 支持

**替代方案**: 
- 使用 Preact 版本的组件
- 或自定义轻量组件

---

## 最佳实践

### ✅ 推荐做法

1. **优先自定义轻量组件**
   - 针对需求实现
   - 遵循 Material Design 或自定义设计系统
   - 控制体积和性能

2. **组件化开发**
   ```javascript
   // 创建组件库文件
   // js/components/Button.js
   // js/components/Input.js
   // js/components/Card.js
   
   // 在应用中引入
   <script src="../../js/components/Button.js"></script>
   ```

3. **使用主题系统**
   ```javascript
   const theme = {
       colors: {
           primary: '#1976d2',
           secondary: '#dc004e'
       },
       spacing: {
           small: '8px',
           medium: '16px'
       }
   };
   ```

### ❌ 避免做法

1. ❌ 过度依赖外部库
2. ❌ 引入体积过大的组件
3. ❌ 混用 React 和 Preact 组件

---

## 快速开始

### 1. 运行示例

```bash
# 打开示例文件
examples/demo_html/preact_ui_component_test/index.html
```

### 2. 创建自己的组件

```javascript
// 1. 创建组件函数
function MyComponent(props) {
    return preact.h('div', {
        style: '...'
    }, props.children);
}

// 2. 使用组件
function App() {
    return preact.h(MyComponent, {}, 'Hello');
}

// 3. 渲染
preact.render(preact.h(App), document.body);
```

---

## 常见问题

### Q: 能否使用 npm 安装组件？
A: 当前不支持。QuickJS 不支持 npm 模块系统。需要使用 UMD 格式或自定义组件。

### Q: 如何使用 TypeScript？
A: 当前不支持。MBink 使用纯 JavaScript。

### Q: 性能如何？
A: 自定义轻量组件性能最优。外部组件库取决于体积和实现。

### Q: 如何调试组件？
A: 使用 `console.log` 和 QuickJS 调试工具。

---

## 下一步

1. 尝试运行 `preact_ui_component_test.js`
2. 创建自己的组件
3. 测试外部 Preact 组件库（可选）
4. 反馈兼容性问题

---

**更新时间**: 2025-12-12  
**维护者**: MBink Team
