# MBink Window Demo

## 📖 简介

这是一个完整的 MBink 窗口应用示例，展示了如何使用 MBink 框架创建交互式的 GUI 应用程序。

## ✨ 功能特性

### 1. 交互式计数器
- ➕ 增加计数
- ➖ 减少计数
- 🔄 重置计数
- 实时更新显示

### 2. 表单输入
- 文本输入框
- 输入验证
- 动态问候消息
- Enter 键提交

### 3. Todo List
- ➕ 添加待办事项
- ✓ 标记完成/未完成
- 🗑️ 删除待办事项
- 动态列表渲染

### 4. 定时器
- ▶️ 启动/暂停
- 实时秒数显示
- 空格键快捷操作

### 5. UI 元素展示
- Checkbox（复选框）
- Radio（单选按钮）
- Select（下拉选择）
- Table（表格）
- 各种按钮样式

## 🚀 快速开始

### 编译

```bash
# 从项目根目录
cmake --build build --config Release --target window_demo
```

### 运行

```bash
# Windows
.\build\bin\Release\window_demo.exe

# Linux/macOS
./build/bin/Release/window_demo
```

## 📁 文件结构

```
window_demo/
├── README.md          # 本文件
├── main.cpp          # C++ 主程序
├── index.html        # HTML 界面
├── app.js            # JavaScript 应用逻辑
└── CMakeLists.txt    # 构建配置
```

## 🎯 技术栈

- **C++ 核心**: Window、Document、Element、Event 等
- **JavaScript 引擎**: QuickJS
- **HTML 解析**: Lexbor
- **渲染引擎**: Skia
- **窗口系统**: SDL3
- **事件循环**: 自定义 EventLoop

## 💡 代码示例

### C++ 主程序

```cpp
// 创建窗口
WindowConfig config;
config.title = "MBink Window Demo";
config.width = 800;
config.height = 600;
auto window = std::make_shared<Window>(config);

// 创建文档
auto document = std::make_shared<Document>();
document->Initialize();

// 加载 HTML
document->LoadHTML(html);

// 关联文档到窗口
window->SetDocument(document);

// 显示窗口
window->Show();

// 运行事件循环
EventLoop event_loop;
event_loop.Run();
```

### JavaScript 应用逻辑

```javascript
// 计数器
let counter = 0;

function incrementCounter() {
    counter++;
    document.getElementById('counter').textContent = counter;
}

// 事件监听
document.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        submitForm();
    }
});
```

### HTML 界面

```html
<div class="section">
    <h2>Interactive Counter</h2>
    <div class="counter" id="counter">0</div>
    <button onclick="incrementCounter()">Increment</button>
</div>
```

## 🎨 样式特性

### CSS 支持
- ✅ 渐变背景 (`linear-gradient`)
- ✅ 圆角边框 (`border-radius`)
- ✅ 阴影效果 (`box-shadow`)
- ✅ 过渡动画 (`transition`)
- ✅ Flexbox 布局
- ✅ 表格样式
- ✅ 伪类选择器 (`:hover`)

### 响应式设计
- 最大宽度限制
- 居中布局
- 自适应内容

## 🔧 自定义开发

### 添加新功能

1. **修改 HTML** (`index.html`)
   ```html
   <div class="section">
       <h2>My New Feature</h2>
       <button onclick="myFunction()">Click Me</button>
   </div>
   ```

2. **添加 JavaScript** (`app.js`)
   ```javascript
   function myFunction() {
       console.log('Button clicked!');
       // 你的逻辑
   }
   
   // 导出到全局
   globalThis.myFunction = myFunction;
   ```

3. **重新编译运行**
   ```bash
   cmake --build build --config Release --target window_demo
   ./build/bin/Release/window_demo.exe
   ```

### 修改样式

在 `index.html` 的 `<style>` 标签中修改 CSS：

```css
.my-button {
    background: #667eea;
    color: white;
    padding: 10px 20px;
    border-radius: 5px;
}
```

## 🎮 键盘快捷键

- **Enter**: 提交当前焦点的表单
- **Space**: 切换定时器（启动/暂停）

## 📊 应用架构

```
┌─────────────────────────────────────┐
│         Window (SDL3)               │
│  ┌───────────────────────────────┐  │
│  │      Document (DOM Tree)      │  │
│  │  ┌─────────────────────────┐  │  │
│  │  │   HTML Elements         │  │  │
│  │  │   - Buttons             │  │  │
│  │  │   - Inputs              │  │  │
│  │  │   - Lists               │  │  │
│  │  └─────────────────────────┘  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
           ↕
┌─────────────────────────────────────┐
│    QuickJS Runtime                  │
│  ┌───────────────────────────────┐  │
│  │   JavaScript Code (app.js)    │  │
│  │   - Event Handlers            │  │
│  │   - Business Logic            │  │
│  │   - DOM Manipulation          │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
           ↕
┌─────────────────────────────────────┐
│      Event Loop                     │
│  - Process Events                   │
│  - Execute Timers                   │
│  - Render Frames                    │
└─────────────────────────────────────┘
```

## 🐛 调试技巧

### 1. 查看控制台输出

应用会在控制台输出详细的日志：

```
========================================
  MBink Window Demo
  Version: 1.0
========================================

[1/7] Creating window...
  ✓ Window created: 800x600
  ✓ Window registered
...
```

### 2. JavaScript 调试

在 `app.js` 中添加 `console.log`：

```javascript
function myFunction() {
    console.log('Debug: myFunction called');
    console.log('Counter value:', counter);
}
```

### 3. DOM 检查

在 C++ 中打印 DOM 结构：

```cpp
auto body = document->GetBody();
auto children = body->GetChildNodes();
std::cout << "Body has " << children.size() << " children" << std::endl;
```

## 📚 相关文档

- [MBink 框架文档](../../docs/)
- [DOM API 文档](../../core/dom/README.md)
- [Window API 文档](../../core/window/README.md)
- [Event 系统文档](../../core/event/README.md)

## 🎯 学习路径

1. **初学者**: 
   - 运行示例应用
   - 修改 HTML 内容
   - 修改 CSS 样式

2. **进阶**:
   - 添加新的 JavaScript 函数
   - 实现新的交互功能
   - 使用定时器和事件

3. **高级**:
   - 修改 C++ 主程序
   - 集成外部库
   - 优化性能

## ✅ 功能清单

- [x] 窗口创建和显示
- [x] HTML 加载和解析
- [x] CSS 样式渲染
- [x] JavaScript 执行
- [x] 事件处理（点击、键盘）
- [x] DOM 操作
- [x] 定时器功能
- [x] 表单输入
- [x] 动态列表
- [x] 响应式布局

## 🚧 未来改进

- [ ] 添加动画效果
- [ ] 支持拖拽功能
- [ ] 添加更多 UI 组件
- [ ] 支持主题切换
- [ ] 添加数据持久化
- [ ] 支持多窗口

## 💬 反馈

如果你有任何问题或建议，欢迎：
- 查看项目文档
- 提交 Issue
- 贡献代码

---

**MBink Window Demo** - 展示 MBink 框架的强大功能！ 🚀

