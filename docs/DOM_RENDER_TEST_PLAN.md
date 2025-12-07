# MBink DOM 元素渲染验证测试计划

> **版本**: 1.0
> **日期**: 2025-12-07
> **技术栈**: Preact + QuickJS

## 概述

本文档定义了 MBink 所有 DOM 元素渲染效果的验证测试计划。通过与真实浏览器的一对一对比，确保 MBink 的渲染结果与标准浏览器行为一致。

### 核心设计理念

**同源 Preact 测试**：浏览器和 MBink 加载完全相同的 `app.js` 文件（使用 Preact 编写），通过 Preact 组件创建 DOM 元素，确保测试环境完全一致。

```
┌────────────────┐
│    app.js      │  ◄── 同一份 Preact 代码
│  (Preact 组件)  │
└───────┬────────┘
        │
   ┌────┴────┐
   ▼         ▼
┌──────┐  ┌──────┐
│浏览器 │  │MBink │
│Preact│  │Preact│
└──┬───┘  └──┬───┘
   │         │
   ▼         ▼
┌──────┐  ┌──────┐
│ JSON │  │ JSON │  ◄── 提取渲染数据
└──┬───┘  └──┬───┘
   │         │
   └────┬────┘
        ▼
   ┌─────────┐
   │  比较   │  ◄── 差异分析 & 报告
   └─────────┘
```

## 1. 测试目标

### 1.1 核心目标
- **渲染一致性**：所有 DOM 元素的视觉渲染与 Chrome 浏览器一致
- **布局正确性**：元素位置、尺寸计算误差 ≤ 2px
- **样式准确性**：CSS 属性应用效果与浏览器一致
- **回归防护**：确保代码修改不会破坏现有渲染功能

### 1.2 测试范围

| 类别 | 元素 | 优先级 |
|------|------|--------|
| 文本元素 | h1-h6, p, span, strong, em, code, pre | P0 |
| 容器元素 | div, section, article, header, footer, main, nav, aside | P0 |
| 列表元素 | ul, ol, li, dl, dt, dd | P1 |
| 表格元素 | table, thead, tbody, tr, th, td | P1 |
| 表单元素 | form, input, button, select, textarea, label | P1 |
| 媒体元素 | img, canvas, svg | P2 |
| 行内元素 | a, br, hr, small, sub, sup | P2 |

---

## 2. 测试方法论

### 2.1 核心原则：同源 app.js

**关键设计**：浏览器和 MBink 加载同一个 `app.js` 文件来创建 DOM 元素，确保环境条件完全一致。

```
┌─────────────────────────────────────────────────────────────────┐
│                    同源 app.js 测试架构                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│                      ┌──────────────┐                           │
│                      │   app.js     │                           │
│                      │  (测试用例)   │                           │
│                      └──────┬───────┘                           │
│                             │                                    │
│              ┌──────────────┴──────────────┐                    │
│              ▼                              ▼                    │
│    ┌──────────────────┐          ┌──────────────────┐          │
│    │     浏览器        │          │      MBink       │          │
│    │  index.html      │          │  QuickJS 运行时   │          │
│    │  + app.js        │          │  + app.js        │          │
│    └────────┬─────────┘          └────────┬─────────┘          │
│             │                              │                    │
│             ▼                              ▼                    │
│    ┌──────────────────┐          ┌──────────────────┐          │
│    │  提取渲染数据     │          │  提取渲染数据     │          │
│    │  browser.json    │          │  mbink.json      │          │
│    └────────┬─────────┘          └────────┬─────────┘          │
│             │                              │                    │
│             └──────────────┬───────────────┘                    │
│                            ▼                                    │
│                  ┌──────────────────┐                           │
│                  │    比较 & 报告    │                           │
│                  └──────────────────┘                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 文件结构

```
test_cases/
├── basic/
│   ├── app.js              # 测试用例 JavaScript（共享）
│   ├── index.html          # 浏览器加载入口
│   └── expected.json       # 预期结果（可选）
├── layout/
│   ├── app.js
│   └── index.html
└── ...
```

### 2.3 使用 Preact 编写测试用例

使用 Preact 组件化方式编写测试用例，更接近实际应用场景：

```javascript
// app.js - 使用 Preact 的测试用例
import { h, render, Component } from 'preact';

// ============================================
// 测试用例定义
// ============================================

const testCases = [
    {
        id: 'BASIC-001',
        name: '默认 div 渲染',
        component: () => (
            <div id="test-BASIC-001">Hello World</div>
        )
    },
    {
        id: 'BASIC-002',
        name: '嵌套 div 渲染',
        component: () => (
            <div id="test-BASIC-002" style="width: 200px; padding: 10px;">
                <div id="test-BASIC-002-inner" style="background: #eee;">
                    Inner
                </div>
            </div>
        )
    },
    {
        id: 'LAYOUT-001',
        name: 'Flexbox 行布局',
        component: () => (
            <div id="test-LAYOUT-001" style="display: flex; gap: 10px;">
                <div style="width: 100px; height: 50px; background: #f00;">1</div>
                <div style="width: 100px; height: 50px; background: #0f0;">2</div>
                <div style="width: 100px; height: 50px; background: #00f;">3</div>
            </div>
        )
    },
    {
        id: 'TEXT-001',
        name: '文本居中对齐',
        component: () => (
            <div id="test-TEXT-001" style="width: 300px; text-align: center;">
                居中的文本内容
            </div>
        )
    },
    {
        id: 'BOX-001',
        name: '盒模型 margin/padding',
        component: () => (
            <div id="test-BOX-001" style="margin: 20px; padding: 15px; border: 2px solid #333; background: #eee;">
                盒模型测试
            </div>
        )
    }
];

// ============================================
// 测试容器组件
// ============================================

class TestCase extends Component {
    render({ testCase }) {
        const TestComponent = testCase.component;
        return (
            <div class="test-case" data-test-id={testCase.id} data-test-name={testCase.name}>
                <TestComponent />
            </div>
        );
    }
}

class TestApp extends Component {
    componentDidMount() {
        // 渲染完成后提取数据
        setTimeout(() => {
            const data = extractRenderData();
            console.log('__RENDER_DATA__' + JSON.stringify(data));
        }, 100);
    }

    render() {
        return (
            <div id="test-container">
                {testCases.map(tc => (
                    <TestCase key={tc.id} testCase={tc} />
                ))}
            </div>
        );
    }
}

// ============================================
// 渲染数据提取（递归提取所有子元素）
// ============================================

function extractElementData(element, depth = 0) {
    const rect = element.getBoundingClientRect();
    const style = window.getComputedStyle(element);

    const data = {
        tag: element.tagName.toLowerCase(),
        id: element.id || null,
        className: element.className || null,
        depth: depth,

        // 布局数据
        layout: {
            x: Math.round(rect.x * 100) / 100,
            y: Math.round(rect.y * 100) / 100,
            width: Math.round(rect.width * 100) / 100,
            height: Math.round(rect.height * 100) / 100,
            // 内容区域（不含 padding/border）
            clientWidth: element.clientWidth,
            clientHeight: element.clientHeight,
            scrollWidth: element.scrollWidth,
            scrollHeight: element.scrollHeight
        },

        // 盒模型数据
        box: {
            marginTop: parseFloat(style.marginTop) || 0,
            marginRight: parseFloat(style.marginRight) || 0,
            marginBottom: parseFloat(style.marginBottom) || 0,
            marginLeft: parseFloat(style.marginLeft) || 0,
            paddingTop: parseFloat(style.paddingTop) || 0,
            paddingRight: parseFloat(style.paddingRight) || 0,
            paddingBottom: parseFloat(style.paddingBottom) || 0,
            paddingLeft: parseFloat(style.paddingLeft) || 0,
            borderTopWidth: parseFloat(style.borderTopWidth) || 0,
            borderRightWidth: parseFloat(style.borderRightWidth) || 0,
            borderBottomWidth: parseFloat(style.borderBottomWidth) || 0,
            borderLeftWidth: parseFloat(style.borderLeftWidth) || 0
        },

        // 文本相关样式
        text: {
            textAlign: style.textAlign,
            verticalAlign: style.verticalAlign,
            lineHeight: style.lineHeight,
            fontSize: style.fontSize,
            fontFamily: style.fontFamily,
            fontWeight: style.fontWeight,
            letterSpacing: style.letterSpacing,
            wordSpacing: style.wordSpacing,
            textIndent: style.textIndent,
            whiteSpace: style.whiteSpace,
            textOverflow: style.textOverflow,
            textDecoration: style.textDecoration,
            textTransform: style.textTransform,
            color: style.color
        },

        // 布局模式
        layoutMode: {
            display: style.display,
            position: style.position,
            float: style.float,
            clear: style.clear,
            overflow: style.overflow,
            overflowX: style.overflowX,
            overflowY: style.overflowY,
            zIndex: style.zIndex
        },

        // Flexbox 属性（如果是 flex 容器或项目）
        flexbox: style.display.includes('flex') ? {
            flexDirection: style.flexDirection,
            flexWrap: style.flexWrap,
            justifyContent: style.justifyContent,
            alignItems: style.alignItems,
            alignContent: style.alignContent,
            gap: style.gap,
            flexGrow: style.flexGrow,
            flexShrink: style.flexShrink,
            flexBasis: style.flexBasis,
            alignSelf: style.alignSelf,
            order: style.order
        } : null,

        // Grid 属性（如果是 grid 容器或项目）
        grid: style.display.includes('grid') ? {
            gridTemplateColumns: style.gridTemplateColumns,
            gridTemplateRows: style.gridTemplateRows,
            gridColumn: style.gridColumn,
            gridRow: style.gridRow,
            gridGap: style.gridGap,
            justifyItems: style.justifyItems,
            alignItems: style.alignItems
        } : null,

        // 视觉样式
        visual: {
            backgroundColor: style.backgroundColor,
            backgroundImage: style.backgroundImage,
            borderColor: style.borderColor,
            borderStyle: style.borderStyle,
            borderRadius: style.borderRadius,
            boxShadow: style.boxShadow,
            opacity: style.opacity,
            visibility: style.visibility,
            transform: style.transform,
            transformOrigin: style.transformOrigin
        },

        // 文本内容（如果有直接文本子节点）
        textContent: null,
        textNodes: [],

        // 子元素（递归）
        children: []
    };

    // 提取文本节点信息
    Array.from(element.childNodes).forEach((node, index) => {
        if (node.nodeType === Node.TEXT_NODE) {
            const text = node.textContent.trim();
            if (text) {
                // 使用 Range 获取文本节点的精确位置
                const range = document.createRange();
                range.selectNodeContents(node);
                const textRect = range.getBoundingClientRect();

                data.textNodes.push({
                    index: index,
                    content: text,
                    layout: {
                        x: Math.round(textRect.x * 100) / 100,
                        y: Math.round(textRect.y * 100) / 100,
                        width: Math.round(textRect.width * 100) / 100,
                        height: Math.round(textRect.height * 100) / 100
                    }
                });
            }
        }
    });

    // 获取直接文本内容
    if (data.textNodes.length > 0) {
        data.textContent = data.textNodes.map(t => t.content).join(' ');
    }

    // 递归提取子元素
    Array.from(element.children).forEach(child => {
        data.children.push(extractElementData(child, depth + 1));
    });

    return data;
}

function extractRenderData() {
    const results = [];

    testCases.forEach(tc => {
        const element = document.getElementById(`test-${tc.id}`);
        if (!element) return;

        results.push({
            id: tc.id,
            name: tc.name,
            root: extractElementData(element, 0)
        });
    });

    return results;
}

// ============================================
// 启动
// ============================================

render(<TestApp />, document.body);

// 导出供外部调用
window.DOMRenderTest = {
    cases: testCases,
    extract: extractRenderData
};
```

### 2.4 index.html 模板（浏览器端）

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=800, height=600">
    <title>DOM Render Test</title>
    <style>
        /* 重置样式，确保浏览器和 MBink 环境一致 */
        * { margin: 0; padding: 0; box-sizing: border-box; }
        html, body { width: 800px; height: 600px; overflow: hidden; }
        body { font-family: Arial, sans-serif; font-size: 16px; color: #000; }
        .test-case { margin-bottom: 10px; }
    </style>
    <!-- Preact CDN -->
    <script src="https://unpkg.com/preact@10/dist/preact.umd.js"></script>
    <script src="https://unpkg.com/preact@10/hooks/dist/hooks.umd.js"></script>
</head>
<body>
    <script>
        // 设置 Preact 全局变量
        const { h, render, Component } = preact;
    </script>
    <script src="app.js"></script>
</body>
</html>
```

### 2.5 MBink 端运行

#### 2.5.1 C++ 程序设计（仅 -q 参数）

C++ 程序只需支持 `-q` 参数实现自动退出，其他逻辑由 Python 脚本处理：

```bash
# 用法
dom_render_test.exe          # 正常运行，手动关闭窗口
dom_render_test.exe -q       # 渲染完成后立即退出（用于自动化测试）
```

#### 2.5.2 日志输出格式

使用特殊标记 `__RENDER_DATA__` 便于 Python 脚本提取 JSON 数据：

```
[INFO] MBink DOM Render Test
[INFO] Creating window...
[INFO] Loading Preact library...
[INFO] Rendering...

__RENDER_DATA__[{"id":"BASIC-001","name":"默认 div 渲染","root":{...}},{"id":"BASIC-002",...}]

[INFO] Render complete.
```

Python 脚本通过正则匹配 `__RENDER_DATA__` 后的 JSON，忽略其他日志。

#### 2.5.3 C++ 实现框架

```cpp
// dom_render_test.cpp
int main(int argc, char* argv[]) {
    // 只解析 -q 参数
    bool quick_exit = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-q" || arg == "--quick") {
            quick_exit = true;
        }
    }

    std::cout << "[INFO] MBink DOM Render Test" << std::endl;
    if (quick_exit) {
        std::cout << "[INFO] Quick exit mode" << std::endl;
    }

    // 1. 创建窗口 (800x600 固定尺寸)
    WindowConfig config{800, 600, "DOM Render Test"};
    auto window = std::make_shared<Window>(config);

    // 2. 初始化 Document + QuickJS + Preact
    auto document = std::make_shared<Document>();
    document->Initialize();
    auto runtime = std::make_unique<QuickJSRuntime>();
    DOMBindings::Init(runtime->GetContext());
    DOMBindings::SetGlobalDocument(runtime->GetContext(), document);

    // 3. 加载 Preact
    runtime->EvalFile("js/preact/preact.js");
    runtime->EvalFile("js/preact/hooks.js");

    // 4. 加载测试用例 app.js
    runtime->EvalFile("app.js");

    // 5. 执行渲染
    window->SetDocument(document);
    window->Show();
    window->RenderDocument();
    window->SwapBuffers();

    // 6. 调用 JS 提取渲染数据并输出
    // app.js 中定义 window.DOMRenderTest.extract() 返回 JSON 字符串
    JSValue result = runtime->CallGlobal("extractAndPrintRenderData");
    // extractAndPrintRenderData() 内部 console.log("__RENDER_DATA__" + JSON.stringify(data))

    std::cout << "[INFO] Render complete." << std::endl;

    // 7. 根据 -q 参数决定是否等待
    if (!quick_exit) {
        std::cout << "[INFO] Close window to exit." << std::endl;
        EventLoop event_loop;
        event_loop.Run();
    }

    return 0;
}
```

#### 2.5.4 测试流程

```bash
# 1. 运行 MBink 测试，输出 JSON
dom_render_test.exe -q > output/mbink.json

# 2. 比较浏览器数据和 MBink 数据
python compare_render.py reference_data/browser.json output/mbink.json

# 3. 按 ID 筛选比较
python compare_render.py browser.json mbink.json --test BASIC-001

# 4. 按类别筛选比较
python compare_render.py browser.json mbink.json --category text
```

### 2.6 数据输出格式

浏览器和 MBink 都输出相同格式的 JSON 数据，包含完整的嵌套结构：

```json
[
    {
        "id": "BASIC-002",
        "name": "嵌套 div 渲染",
        "root": {
            "tag": "div",
            "id": "test-BASIC-002",
            "depth": 0,
            "layout": {
                "x": 0, "y": 10,
                "width": 200, "height": 54,
                "clientWidth": 180, "clientHeight": 34
            },
            "box": {
                "marginTop": 0, "marginRight": 0, "marginBottom": 0, "marginLeft": 0,
                "paddingTop": 10, "paddingRight": 10, "paddingBottom": 10, "paddingLeft": 10,
                "borderTopWidth": 0, "borderRightWidth": 0, "borderBottomWidth": 0, "borderLeftWidth": 0
            },
            "text": {
                "textAlign": "left",
                "verticalAlign": "baseline",
                "lineHeight": "normal",
                "fontSize": "16px",
                "color": "rgb(0, 0, 0)"
            },
            "layoutMode": {
                "display": "block",
                "position": "static"
            },
            "textContent": null,
            "textNodes": [],
            "children": [
                {
                    "tag": "div",
                    "id": "test-BASIC-002-inner",
                    "depth": 1,
                    "layout": {
                        "x": 10, "y": 20,
                        "width": 180, "height": 24
                    },
                    "text": {
                        "textAlign": "left",
                        "fontSize": "16px"
                    },
                    "textContent": "Inner",
                    "textNodes": [
                        {
                            "index": 0,
                            "content": "Inner",
                            "layout": {
                                "x": 10, "y": 20,
                                "width": 35.5, "height": 24
                            }
                        }
                    ],
                    "children": []
                }
            ]
        }
    }
]
```

### 2.7 比较维度

| 维度 | 说明 | 容差 |
|------|------|------|
| **元素位置** | x, y 坐标 | ≤ 2px |
| **元素尺寸** | width, height | ≤ 2px |
| **盒模型** | margin, padding, border | 精确匹配 |
| **文本位置** | 文本节点的 x, y, width, height | ≤ 2px |
| **文本对齐** | textAlign, verticalAlign | 精确匹配 |
| **行高** | lineHeight | ≤ 2px |
| **字体大小** | fontSize | 精确匹配 |
| **布局模式** | display, position, float | 精确匹配 |
| **Flexbox** | 所有 flex 属性 | 精确匹配 |
| **Grid** | 所有 grid 属性 | 精确匹配 |
| **颜色值** | RGB 分量 | ≤ 5 |
| **子元素数量** | children.length | 精确匹配 |
| **子元素递归** | 每层子元素都需要比较 | 同上规则 |

---

## 3. 测试用例设计

### 3.1 测试用例分类

#### 3.1.1 基础渲染测试 (BASIC-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| BASIC-001 | 默认 div 渲染 | 无样式 div 的默认渲染 | 位置、尺寸、默认样式 |
| BASIC-002 | 嵌套 div 渲染 | 多层嵌套 div (3层以上) | 每层位置、相对位置 |
| BASIC-003 | 文本内容渲染 | 纯文本内容的布局 | 文本位置、行高、宽度 |
| BASIC-004 | 多段落渲染 | 多个 p 元素 | 间距、margin 合并 |
| BASIC-005 | 深度嵌套 | 5层以上嵌套结构 | 所有子元素位置递归验证 |
| BASIC-006 | 混合内容 | 文本 + 元素混合 | 文本节点与元素节点位置 |

#### 3.1.2 布局模式测试 (LAYOUT-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| LAYOUT-001 | Block 布局 | display: block 堆叠 | 垂直位置、宽度继承 |
| LAYOUT-002 | Inline 布局 | display: inline 排列 | 水平位置、换行 |
| LAYOUT-003 | Inline-Block | inline-block 混合 | 水平排列、保持尺寸 |
| LAYOUT-004 | Flex 行布局 | flex-direction: row | 子元素 x 坐标、间距 |
| LAYOUT-005 | Flex 列布局 | flex-direction: column | 子元素 y 坐标、间距 |
| LAYOUT-006 | Flex 对齐 | justify-content/align-items | 子元素对齐位置 |
| LAYOUT-007 | Flex 换行 | flex-wrap: wrap | 多行位置 |
| LAYOUT-008 | Grid 基础 | 简单 grid 网格 | 单元格位置 |
| LAYOUT-009 | Grid 间距 | grid-gap | 单元格间距 |
| LAYOUT-010 | Grid 跨列 | grid-column: span | 跨列元素尺寸 |

#### 3.1.3 盒模型测试 (BOX-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| BOX-001 | Margin 计算 | 各方向 margin | 元素偏移位置 |
| BOX-002 | Padding 计算 | 各方向 padding | 内容区位置、clientWidth |
| BOX-003 | Border 计算 | border 宽度 | 边框影响尺寸 |
| BOX-004 | Content-box | box-sizing: content-box | 总尺寸计算 |
| BOX-005 | Border-box | box-sizing: border-box | 内容区尺寸 |
| BOX-006 | Margin 合并 | 垂直 margin 折叠 | 实际间距 |
| BOX-007 | 嵌套盒模型 | 嵌套元素的盒模型 | 子元素相对位置 |

#### 3.1.4 文本渲染测试 (TEXT-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| TEXT-001 | 左对齐 | text-align: left | 文本节点 x 坐标 |
| TEXT-002 | 居中对齐 | text-align: center | 文本节点居中位置 |
| TEXT-003 | 右对齐 | text-align: right | 文本节点 x 坐标 |
| TEXT-004 | 两端对齐 | text-align: justify | 文本节点分布 |
| TEXT-005 | 行高-数值 | line-height: 24px | 行高、文本 y 坐标 |
| TEXT-006 | 行高-倍数 | line-height: 1.5 | 计算后行高 |
| TEXT-007 | 垂直对齐 | vertical-align | 行内元素垂直位置 |
| TEXT-008 | 字体大小 | font-size 各种单位 | 文本高度 |
| TEXT-009 | 文本换行 | 长文本自动换行 | 多行文本位置 |
| TEXT-010 | 文本溢出 | overflow: hidden | 裁剪后尺寸 |
| TEXT-011 | 首行缩进 | text-indent | 首行文本 x 偏移 |
| TEXT-012 | 字间距 | letter-spacing | 文本宽度 |

#### 3.1.5 定位测试 (POS-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| POS-001 | Relative 定位 | position: relative | 偏移后位置、子元素位置 |
| POS-002 | Absolute 定位 | position: absolute | 相对定位祖先的位置 |
| POS-003 | Fixed 定位 | position: fixed | 相对视口位置 |
| POS-004 | Sticky 定位 | position: sticky | 滚动阈值位置 |
| POS-005 | Z-Index 层叠 | z-index 顺序 | 层叠上下文 |
| POS-006 | 嵌套定位 | 多层定位嵌套 | 各层相对位置 |

#### 3.1.6 视觉效果测试 (VISUAL-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| VISUAL-001 | 背景颜色 | background-color | 颜色值 RGB |
| VISUAL-002 | 边框样式 | border 各种样式 | 边框宽度、颜色 |
| VISUAL-003 | 圆角边框 | border-radius | 圆角值 |
| VISUAL-004 | 阴影效果 | box-shadow | 阴影参数 |
| VISUAL-005 | 透明度 | opacity | 透明度值 |
| VISUAL-006 | Transform | translate/scale | 变换后位置 |

#### 3.1.7 嵌套组合测试 (NEST-xxx)
| ID | 名称 | 描述 | 验证点 |
|----|------|------|--------|
| NEST-001 | Flex 嵌套 | flex 容器嵌套 flex | 所有层级子元素位置 |
| NEST-002 | Grid 嵌套 | grid 内嵌套 flex | 混合布局位置 |
| NEST-003 | 定位嵌套 | relative 内 absolute | 定位上下文 |
| NEST-004 | 文本嵌套 | span 内嵌 span + 文本 | 行内元素位置 |
| NEST-005 | 复杂组合 | 多种布局混合 | 完整递归验证 |

---

## 4. CLI 测试工具设计

### 4.1 工具架构

```
tests/dom_render_comparison/
├── run_dom_tests.py           # 主 CLI 入口
├── dom_render_test.cpp        # MBink 渲染测试程序
├── CMakeLists.txt             # CMake 配置
├── compare_render.py          # 渲染结果比较器
├── config.json                # 测试配置
│
├── test_cases/                # 测试用例目录
│   ├── basic/
│   │   ├── app.js             # 基础测试用例（同源）
│   │   └── index.html         # 浏览器入口
│   ├── layout/
│   │   ├── app.js
│   │   └── index.html
│   ├── box/
│   │   ├── app.js
│   │   └── index.html
│   ├── text/
│   │   ├── app.js
│   │   └── index.html
│   ├── position/
│   │   ├── app.js
│   │   └── index.html
│   └── visual/
│       ├── app.js
│       └── index.html
│
├── reference_data/            # 浏览器参考数据
│   ├── basic_browser.json
│   ├── layout_browser.json
│   └── ...
│
└── output/                    # 测试输出
    ├── basic_mbink.json
    ├── comparison_report.json
    └── comparison_report.html
```

### 4.2 命令行工具

#### 4.2.1 MBink 测试程序 (dom_render_test.exe)

```bash
dom_render_test.exe           # 正常运行，手动关闭窗口
dom_render_test.exe -q        # 渲染后立即退出，输出 JSON 到 stdout
dom_render_test.exe -q > output/mbink.json   # 输出到文件
```

#### 4.2.2 比较脚本 (compare_render.py)

```bash
# 比较两个 JSON 文件
python compare_render.py browser.json mbink.json

# 按测试 ID 筛选
python compare_render.py browser.json mbink.json --test BASIC-001
python compare_render.py browser.json mbink.json --test TEXT-002,TEXT-003

# 按类别筛选
python compare_render.py browser.json mbink.json --category basic
python compare_render.py browser.json mbink.json --category text,layout

# 设置容差
python compare_render.py browser.json mbink.json --tolerance 2

# 输出报告
python compare_render.py browser.json mbink.json --report html -o report.html
python compare_render.py browser.json mbink.json --report json -o result.json

# 详细/静默输出
python compare_render.py browser.json mbink.json --verbose
python compare_render.py browser.json mbink.json --quiet
```

#### 4.2.3 典型工作流

```bash
# 1. 浏览器端：打开 index.html，自动输出 browser.json
#    (浏览器控制台复制 JSON 或使用 Puppeteer/Playwright 自动化)

# 2. MBink 端：运行测试
dom_render_test.exe -q > output/mbink.json

# 3. 比较结果
python compare_render.py reference_data/browser.json output/mbink.json

# 4. 快速验证单个测试
python compare_render.py browser.json mbink.json --test TEXT-002

# 5. CI 集成
dom_render_test.exe -q > mbink.json && python compare_render.py browser.json mbink.json --quiet
```

### 4.3 输出示例

```
============================================================
    MBink DOM Render Comparison Test Suite (Preact)
============================================================

📦 Loading test cases...
   Found 7 categories, 56 test cases

🌐 Step 1: Generate browser reference data
   ✅ Generated test HTML pages
   ✅ Browser data extracted (56 root elements, 234 total nodes)

🔧 Step 2: Run MBink rendering
   ✅ MBink output generated (56 root elements, 234 total nodes)

📊 Step 3: Compare results (递归比较所有子元素)

┌─────────────┬───────┬────────┬────────┬───────────┬──────────┐
│ Category    │ Cases │ Nodes  │ Passed │ Failed    │ Pass Rate│
├─────────────┼───────┼────────┼────────┼───────────┼──────────┤
│ Basic       │    6  │    32  │    32  │     0     │   100.0% │
│ Layout      │   10  │    58  │    56  │     2     │    96.6% │
│ Box         │    7  │    28  │    28  │     0     │   100.0% │
│ Text        │   12  │    48  │    45  │     3     │    93.8% │
│ Position    │    6  │    24  │    24  │     0     │   100.0% │
│ Visual      │    6  │    18  │    18  │     0     │   100.0% │
│ Nest        │    5  │    26  │    24  │     2     │    92.3% │
├─────────────┼───────┼────────┼────────┼───────────┼──────────┤
│ TOTAL       │   56  │   234  │   227  │     7     │    97.0% │
└─────────────┴───────┴────────┴────────┴───────────┴──────────┘

❌ Failed Tests (7 nodes):

📍 TEXT-002: 居中对齐
   └─ root > div#test-TEXT-002
      └─ textNode[0] "居中的文本"
         ├─ x: expected 125.5, got 123.2 (diff: 2.3) ❌
         └─ width: expected 96, got 98 (diff: 2)

📍 LAYOUT-006: Flex 对齐
   └─ root > div#test-LAYOUT-006
      └─ children[1] > div.item
         └─ y: expected 25, got 28 (diff: 3) ❌

📍 NEST-001: Flex 嵌套
   └─ root > div > div.inner-flex
      └─ children[0] > div.cell
         ├─ x: expected 10, got 8 (diff: 2) ❌
         └─ textNode[0] "Cell 1"
            └─ x: expected 10, got 8 (diff: 2) ❌

============================================================
   📈 Overall Node Pass Rate: 97.0% (227/234)
   📈 Overall Case Pass Rate: 87.5% (49/56)
   🏆 Threshold: 95.0%
   🎉 TEST PASSED!
============================================================

📄 Reports generated:
   - output/comparison_report.json
   - output/comparison_report.html
```

---

## 5. 实现计划

### 5.1 阶段一：基础框架 (预计 4h)

| 任务 | 描述 | 状态 |
|------|------|------|
| 创建目录结构 | tests/dom_render_comparison/ | ⬜ |
| 创建配置文件 | test_config.json | ⬜ |
| 创建 CMakeLists.txt | 编译配置 | ⬜ |

### 5.2 阶段二：测试用例 (预计 4h)

| 任务 | 描述 | 状态 |
|------|------|------|
| 基础测试用例 | BASIC-001 ~ BASIC-008 | ⬜ |
| 布局测试用例 | LAYOUT-001 ~ LAYOUT-012 | ⬜ |
| 盒模型测试用例 | BOX-001 ~ BOX-006 | ⬜ |
| 文本测试用例 | TEXT-001 ~ TEXT-008 | ⬜ |
| 定位测试用例 | POS-001 ~ POS-004 | ⬜ |
| 视觉测试用例 | VISUAL-001 ~ VISUAL-005 | ⬜ |

### 5.3 阶段三：工具开发 (预计 6h)

| 任务 | 描述 | 状态 |
|------|------|------|
| HTML 生成器 | 生成测试 HTML 页面 | ⬜ |
| 浏览器提取器 | browser_extractor.js | ⬜ |
| MBink 提取器 | dom_render_test.cpp | ⬜ |
| 结果比较器 | render_comparator.py | ⬜ |
| CLI 运行器 | run_dom_tests.py | ⬜ |

### 5.4 阶段四：报告和集成 (预计 2h)

| 任务 | 描述 | 状态 |
|------|------|------|
| 控制台报告 | 彩色表格输出 | ⬜ |
| HTML 报告 | 可视化对比报告 | ⬜ |
| CI 集成 | GitHub Actions 集成 | ⬜ |

---

## 6. 成功标准

| 指标 | 目标 | 当前 |
|------|------|------|
| 测试用例数 | ≥ 40 | ⬜ |
| 整体通过率 | ≥ 95% | ⬜ |
| 位置误差 | ≤ 2px | ⬜ |
| 尺寸误差 | ≤ 2px | ⬜ |
| CLI 工具可用 | ✅ | ⬜ |
| CI 集成完成 | ✅ | ⬜ |

---

## 7. 附录

### 7.1 参考文档
- [LAYOUT_SYSTEM_TEST_PLAN.md](LAYOUT_SYSTEM_TEST_PLAN.md) - 布局系统测试计划
- [HTML_CSS_API_REFERENCE.md](HTML_CSS_API_REFERENCE.md) - HTML/CSS API 参考

### 7.2 更新日志

| 日期 | 版本 | 更新内容 |
|------|------|---------|
| 2025-12-07 | 1.0 | 初始版本，定义测试计划框架 |

