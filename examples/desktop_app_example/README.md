# 桌面应用示例 (Desktop App Example)

这是一个展示常见桌面应用界面的示例，包含菜单栏、工具栏、侧边栏、多视图切换和状态栏。
该示例使用 Preact 框架构建，并由 MBink 的 `app_loader` 加载运行。

## 功能特性

- **菜单栏 (MenuBar)**: 顶部标准菜单布局
- **工具栏 (Toolbar)**: 常用操作快捷按钮
- **侧边栏 (Sidebar)**: 导航切换不同视图
- **多视图 (MainView)**:
  - **概览 (Dashboard)**: 展示数据卡片
  - **文档 (Documents)**: 展示文件列表
  - **设置 (Settings)**: 表单设置界面
- **状态栏 (StatusBar)**: 底部状态信息和实时时间显示
- **自适应布局**: 使用 Flexbox 实现的响应式界面

## 如何运行

确保你已经构建了 `app_loader` 工具。

在项目根目录下运行以下命令：

```powershell
# 运行示例 (推荐尺寸 1200x800)
.\build\tools\app_loader\Release\app_loader.exe examples\desktop_app_example\app.js --width 1200 --height 800 --title "桌面应用示例"
```

如果是在 Debug 模式下构建的，请使用：

```powershell
.\build\tools\app_loader\Debug\app_loader.exe examples\desktop_app_example\app.js --width 1200 --height 800 --title "桌面应用示例"
```

## 代码结构

- `app.js`: 包含所有逻辑、组件和样式。样式通过 JS 动态注入，无需额外的 CSS 文件加载步骤。

