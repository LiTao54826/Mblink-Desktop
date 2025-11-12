# Preact Hello World Example

这是一个简单的Preact Hello World示例，展示了MBink + Preact的基本集成。

## 功能

- ✅ Preact函数组件
- ✅ Virtual DOM渲染
- ✅ 嵌套元素
- ✅ Props传递
- ✅ C++ ↔ JavaScript集成

## 构建

```bash
cmake --build build --config Debug --target preact_hello_world
```

## 运行

```bash
.\build\bin\Debug\preact_hello_world.exe
```

## 预期输出

```
========================================
  MBink + Preact Hello World Example    
========================================

[1/6] Creating QuickJS runtime...       
[2/6] Creating document...
[3/6] Creating Preact renderer...       
[4/6] Initializing bindings...
[5/6] Loading Preact library...
  - preact.js loaded
  - hooks.js loaded
[6/6] Testing direct DOM creation...
Direct DOM creation successful!     
[6/6] Loading and running app...        

========================================
  Application Rendered Successfully!
========================================

DOM Tree:
----------------------------------------
<body>
  <div>
    <div>
      <h1>
        "Hello from Preact!"
      </h1>
      <p>
        "This is running on MBink - a lightweight desktop framework"
      </p>
    </div>
    <hr>
    </hr>
    <p>
      "MBink + Preact integration successful!"
    </p>
  </div>
</body>
----------------------------------------

Statistics:
  - Body children: 1
  - Total elements: 7

✅ Example completed successfully!
```

## 代码说明

### app.js

```javascript
// 主应用组件
function App() {
    return Preact.h('div', { className: 'app' },
        Preact.h('div', { className: 'container' },
            Preact.h('h1', null, 'Hello from Preact!'),
            Preact.h('p', null, 'This is running on MBink - a lightweight desktop framework')
        ),
        Preact.h('hr'),
        Preact.h('p', null, 'MBink + Preact integration successful!')
    );
}

// 渲染应用
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);
```

### main.cpp

C++程序负责：
1. 创建QuickJS运行时
2. 创建MBink Document
3. 创建PreactRenderer
4. 注册DOM和Preact绑定
5. 加载Preact库
6. 运行app.js
7. 打印渲染后的DOM树

## 技术细节

- **JavaScript引擎**: QuickJS
- **Virtual DOM**: Preact (自定义实现)
- **DOM**: MBink DOM (基于Lexbor)
- **渲染**: PreactRenderer (C++)

## 已知问题

- 程序退出时可能出现QuickJS assertion (`p->ref_count > 0`)
- 这是一个已知的内存管理问题，不影响核心功能
- 正在调查修复方案

## 下一步

- [ ] 实现事件处理 (onclick, onchange等)
- [ ] 实现Hooks (useState, useEffect等)
- [ ] 实现Virtual DOM diffing
- [ ] 创建更复杂的示例应用

