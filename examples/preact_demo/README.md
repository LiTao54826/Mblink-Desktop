# Preact 桌面应用示例

这是一个使用 Preact.js 构建的 MBink 桌面应用示例。

## 运行方法

```bash
# 在项目根目录下运行
build/bin/Release/app_loader.exe examples/preact_demo/app.js

# 或者指定窗口大小和标题
build/bin/Release/app_loader.exe examples/preact_demo/app.js --width 800 --height 700 --title "Preact Demo"
```

## 功能演示

- **实时时钟**: 使用 `useEffect` 实现每秒更新
- **计数器**: 使用 `useState` 管理状态
- **待办事项**: 完整的 CRUD 操作示例
- **颜色选择器**: 交互式 UI 组件

## 技术要点

1. 使用 `Preact.h()` 创建虚拟 DOM 元素
2. 使用 `PreactHooks.useState()` 管理组件状态
3. 使用 `PreactHooks.useEffect()` 处理副作用
4. 事件处理 (`onClick`, `onInput`, `onChange`)
5. 列表渲染和 key 属性
6. 内联样式对象
