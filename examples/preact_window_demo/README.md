# Preact Window Demo

这是一个**带GUI窗口**的Preact示例，展示了MBink + Preact的完整集成。

## 功能

- ✅ GUI窗口渲染
- ✅ Preact函数组件
- ✅ Virtual DOM渲染
- ✅ 样式支持
- ✅ C++ ↔ JavaScript集成

## 构建

```bash
cmake --build build --config Debug --target preact_window_demo
```

## 运行

```bash
.\build\bin\Debug\preact_window_demo.exe
```

## 预期效果

程序会打开一个600x400的窗口，显示：

- **标题**: "Hello from Preact!"
- **欢迎区域**: 灰色背景，包含欢迎信息
- **成功提示**: 绿色背景，显示集成成功

窗口内容完全由Preact渲染，使用Virtual DOM技术。

## 技术栈

- **窗口**: SDL3
- **渲染**: Skia
- **JavaScript引擎**: QuickJS
- **Virtual DOM**: Preact (自定义实现)
- **DOM**: MBink DOM (基于Lexbor)

## 代码说明

### app.js

```javascript
function App() {
    return Preact.h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif;' 
    },
        Preact.h('h1', { 
            style: 'color: #333; margin-bottom: 20px;' 
        }, 'Hello from Preact!'),
        
        Preact.h('div', { 
            style: 'background: #f0f0f0; padding: 15px; border-radius: 8px;' 
        },
            Preact.h('h2', { 
                style: 'color: #666; margin-top: 0;' 
            }, 'Welcome to MBink'),
            Preact.h('p', { 
                style: 'color: #444; line-height: 1.6;' 
            }, 'This is a lightweight desktop application framework')
        ),
        
        Preact.h('hr', { 
            style: 'border: none; border-top: 2px solid #ddd; margin: 20px 0;' 
        }),
        
        Preact.h('div', { 
            style: 'background: #e8f5e9; padding: 15px; border-radius: 8px;' 
        },
            Preact.h('p', { 
                style: 'color: #2e7d32; margin: 0; font-weight: bold;' 
            }, '✅ MBink + Preact integration successful!')
        )
    );
}

Preact.render(Preact.h(App), document.body);
```

### main.cpp

C++程序负责：
1. 创建SDL窗口
2. 创建QuickJS运行时
3. 创建MBink Document
4. 创建PreactRenderer
5. 注册DOM和Preact绑定
6. 加载Preact库
7. 运行app.js
8. 启动事件循环
9. 渲染DOM到窗口

## 下一步

- [ ] 实现事件处理 (onclick, onchange等)
- [ ] 实现Hooks (useState, useEffect等)
- [ ] 创建交互式示例 (Counter, TodoList等)
- [ ] 实现Virtual DOM diffing

