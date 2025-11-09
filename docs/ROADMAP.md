# LightUI 开发路线图

## 总体时间规划

**总计**: 6-8个月完成核心功能
**目标**: 2025年Q4发布v1.0

---

## 阶段1: 核心框架 (3个月)

### 第1-2周: 项目基础设施

**目标**: 搭建开发环境和基础架构

#### 任务清单

- [ ] 创建Git仓库，设置分支策略
- [ ] 配置CMake构建系统
- [ ] 设置CI/CD (GitHub Actions)
- [ ] 集成SDL3库
- [ ] 集成Skia库
- [ ] 创建基础窗口示例
- [ ] 编写构建文档

#### 交付物

- 可编译的基础项目
- 能显示空白窗口的Demo
- 完整的构建文档

#### 技术要点

```cmake
# CMakeLists.txt 结构
project(LightUI)
├── core/           # 核心C++代码
├── third_party/    # 第三方库
│   ├── quickjs/
│   ├── skia/
│   ├── sdl3/
│   └── yoga/
└── bindings/       # 语言绑定
```

---

### 第3-4周: QuickJS集成

**目标**: 集成QuickJS引擎，实现JS执行

#### 任务清单

- [ ] 编译QuickJS为静态库
- [ ] 创建QuickJS包装类
- [ ] 实现JS代码执行
- [ ] 实现JS <-> C++数据转换
- [ ] 实现基础的console API
- [ ] 编写单元测试

#### 交付物

- QuickJS运行时封装
- 能执行JavaScript代码
- 基础的console.log支持

#### 技术要点

```cpp
// quickjs_runtime.h
class QuickJSRuntime {
public:
    QuickJSRuntime();
    ~QuickJSRuntime();
    
    // 执行JS代码
    JSValue Eval(const std::string& code);
    
    // 注册C++函数
    void RegisterFunction(const std::string& name, JSCFunction* func);
    
    // 数据转换
    JSValue ToJSValue(const json& data);
    json FromJSValue(JSValue val);
    
private:
    JSRuntime* rt_;
    JSContext* ctx_;
};
```

---

### 第5-6周: 基础DOM API

**目标**: 实现最小可用的DOM API (15个核心API)

#### 任务清单

- [ ] 设计DOM节点类层次结构
- [ ] 实现document对象
- [ ] 实现Element类
- [ ] 实现Text节点
- [ ] 实现基础DOM操作API
- [ ] 绑定到QuickJS
- [ ] 编写测试用例

#### 核心API列表

```javascript
// P0 - 必须实现
document.createElement(tagName)
document.createTextNode(text)
element.appendChild(child)
element.insertBefore(newNode, refNode)
element.removeChild(child)
element.setAttribute(name, value)
element.getAttribute(name)
element.addEventListener(type, handler)
element.removeEventListener(type, handler)

// P1 - 重要
element.className
element.style.cssText
textNode.data
document.body
document.getElementById(id)
```

#### 交付物

- 完整的DOM节点类
- 15个核心DOM API
- 单元测试覆盖率 > 80%

---

### 第7-8周: Yoga布局引擎集成

**目标**: 集成Yoga，实现Flexbox布局

#### 任务清单

- [ ] 集成Yoga库
- [ ] 创建布局引擎类
- [ ] 实现CSS属性解析（简化版）
- [ ] 实现样式计算
- [ ] DOM节点与Yoga节点映射
- [ ] 实现布局计算
- [ ] 测试各种布局场景

#### 支持的CSS属性

```css
/* 布局属性 */
display: flex | block | none
flex-direction: row | column
justify-content: flex-start | center | flex-end | space-between
align-items: flex-start | center | flex-end | stretch
flex-wrap: nowrap | wrap

/* 尺寸属性 */
width, height, min-width, max-width, min-height, max-height
padding, margin
flex-grow, flex-shrink, flex-basis

/* 位置属性 */
position: relative | absolute
top, right, bottom, left
```

#### 交付物

- Yoga集成完成
- 支持20+个CSS属性
- 布局测试用例

---

### 第9-10周: Skia渲染集成

**目标**: 实现DOM树到Skia的渲染

#### 任务清单

- [ ] 创建渲染器类
- [ ] 实现DOM树遍历渲染
- [ ] 实现基础样式渲染（背景、边框）
- [ ] 实现文本渲染
- [ ] 实现裁剪和滚动
- [ ] 优化渲染性能（脏矩形）
- [ ] 实现60fps渲染循环

#### 渲染流程

```
1. 布局计算 (Yoga)
   └─> 获取每个元素的位置和尺寸

2. 样式计算
   └─> 计算最终的样式属性

3. 绘制 (Skia)
   ├─> 背景和边框
   ├─> 文本内容
   └─> 子元素（递归）

4. 合成和输出
   └─> 交换缓冲区
```

#### 交付物

- 完整的渲染管线
- 支持基础样式渲染
- 60fps流畅渲染

---

### 第11-12周: 事件系统

**目标**: 实现完整的事件系统

#### 任务清单

- [ ] 实现Event类
- [ ] 实现事件捕获和冒泡
- [ ] SDL事件到DOM事件的转换
- [ ] 实现鼠标事件
- [ ] 实现键盘事件
- [ ] 实现焦点管理
- [ ] 事件测试

#### 支持的事件

```javascript
// 鼠标事件
mousedown, mouseup, mousemove
click, dblclick
mouseenter, mouseleave
wheel

// 键盘事件
keydown, keyup, keypress

// 焦点事件
focus, blur

// 表单事件
input, change, submit
```

#### 交付物

- 完整的事件系统
- 支持10+种事件类型
- 事件测试覆盖率 > 80%

---

## 阶段2: Preact支持 (1个月)

### 第13-14周: 完善DOM API

**目标**: 补充Preact所需的DOM API

#### 任务清单

- [ ] 分析Preact的DOM API依赖
- [ ] 实现剩余的DOM API（约25个）
- [ ] 实现innerHTML/textContent
- [ ] 实现classList API
- [ ] 实现表单元素API
- [ ] 运行Preact Hello World
- [ ] 修复兼容性问题

#### 新增API列表

```javascript
// 文本和HTML
element.innerHTML
element.textContent
element.innerText

// 样式
element.classList.add/remove/toggle/contains
element.style.setProperty/getPropertyValue

// 表单
input.value
input.checked
select.selectedIndex
textarea.value

// 查询
document.querySelector(selector)
document.querySelectorAll(selector)
element.matches(selector)

// 其他
element.focus()
element.blur()
element.scrollIntoView()
```

#### 交付物

- 40+个DOM API
- Preact成功运行
- 兼容性测试通过

---

### 第15-16周: Preact集成和测试

**目标**: 完整支持Preact和基础组件

#### 任务清单

- [ ] 打包Preact运行时
- [ ] 实现模块加载器
- [ ] 测试Preact Hooks
- [ ] 测试Preact Router
- [ ] 创建示例应用
- [ ] 性能测试和优化

#### 测试用例

```javascript
// 1. 基础渲染
import { render } from 'preact';
render(<h1>Hello World</h1>, document.body);

// 2. 状态管理
import { useState } from 'preact/hooks';
function Counter() {
    const [count, setCount] = useState(0);
    return <button onClick={() => setCount(count + 1)}>{count}</button>;
}

// 3. 列表渲染
function List({ items }) {
    return <ul>{items.map(item => <li key={item.id}>{item.text}</li>)}</ul>;
}

// 4. 条件渲染
function App() {
    const [show, setShow] = useState(true);
    return <div>{show && <p>Visible</p>}</div>;
}
```

#### 交付物

- Preact完全支持
- 5+个示例应用
- 性能基准测试

---

## 阶段3: 组件库支持 (1个月)

### 第17-18周: Ant Design集成

**目标**: 支持Ant Design组件库

#### 任务清单

- [ ] 分析Ant Design依赖
- [ ] 补充缺失的DOM API
- [ ] 实现CSS样式系统完善
- [ ] 测试核心组件（Button, Input, Modal等）
- [ ] 修复兼容性问题
- [ ] 创建Ant Design示例

#### 测试组件列表

```
优先级P0:
- Button
- Input
- Select
- Checkbox
- Radio

优先级P1:
- Modal
- Drawer
- Table
- Form
- Tabs

优先级P2:
- DatePicker
- Upload
- Tree
- Menu
```

#### 交付物

- 支持Ant Design核心组件
- 10+个组件示例
- 兼容性文档

---

### 第19-20周: 其他组件库和优化

**目标**: 支持更多组件库，性能优化

#### 任务清单

- [ ] 测试Material-UI
- [ ] 测试Chakra UI
- [ ] 渲染性能优化
- [ ] 内存优化
- [ ] 启动速度优化
- [ ] 编写性能优化文档

#### 性能目标

```
启动时间: < 500ms
首次渲染: < 100ms
帧率: 60fps (16.6ms/frame)
内存占用: < 100MB (空应用)
```

#### 交付物

- 支持3+个主流组件库
- 性能提升30%+
- 性能测试报告

---

## 阶段4: C API和Python绑定 (1个月)

### 第21-22周: C API设计和实现

**目标**: 设计统一的C API接口

#### 任务清单

- [ ] 设计C API接口
- [ ] 实现窗口管理API
- [ ] 实现UI加载API
- [ ] 实现函数绑定API
- [ ] 实现JS调用API
- [ ] 编写C API文档
- [ ] 创建C/C++示例

#### C API设计

详见 [API_DESIGN.md](API_DESIGN.md)

#### 交付物

- 完整的C API
- C API文档
- 5+个C/C++示例

---

### 第23-24周: Python绑定

**目标**: 完善Python绑定

#### 任务清单

- [ ] 实现ctypes版本
- [ ] 实现pybind11版本
- [ ] 实现高级Python API
- [ ] 实现装饰器语法
- [ ] 编写Python文档
- [ ] 创建Python示例
- [ ] 发布到PyPI

#### Python API设计

详见 [PYTHON_API.md](PYTHON_API.md)

#### 交付物

- 完整的Python绑定
- Python文档
- 10+个Python示例
- PyPI包

---

## 阶段5: 其他语言绑定 (1个月)

### 第25-26周: Rust和Go绑定

**目标**: 支持Rust和Go语言

#### 任务清单

- [ ] Rust绑定（使用bindgen）
- [ ] Go绑定（使用cgo）
- [ ] Node.js绑定（使用N-API）
- [ ] 各语言示例
- [ ] 各语言文档

#### 交付物

- Rust/Go/Node.js绑定
- 各语言文档和示例

---

### 第27-28周: 工具链和CLI

**目标**: 开发工具链

#### 任务清单

- [ ] 创建CLI工具
- [ ] 项目模板生成器
- [ ] 打包工具
- [ ] 热重载支持
- [ ] 开发者工具

#### 交付物

- lightui-cli工具
- 项目模板
- 打包工具

---

## 阶段6: 优化和发布 (1个月)

### 第29-30周: 文档和示例

**目标**: 完善文档和示例

#### 任务清单

- [ ] 完整的API文档
- [ ] 教程和指南
- [ ] 10+个完整示例项目
- [ ] 视频教程
- [ ] 官方网站

#### 交付物

- 完整文档网站
- 丰富的示例
- 教程视频

---

### 第31-32周: 测试和发布

**目标**: 全面测试，发布v1.0

#### 任务清单

- [ ] 单元测试覆盖率 > 80%
- [ ] 集成测试
- [ ] 性能测试
- [ ] 跨平台测试
- [ ] Beta测试
- [ ] 修复关键Bug
- [ ] 发布v1.0

#### 交付物

- LightUI v1.0正式版
- 完整的发布说明
- 社区支持渠道

---

## 里程碑

| 里程碑 | 时间 | 描述 |
|--------|------|------|
| M1: 核心框架 | 第12周 | 基础框架完成，能运行简单UI |
| M2: Preact支持 | 第16周 | 完整支持Preact |
| M3: 组件库支持 | 第20周 | 支持Ant Design等组件库 |
| M4: 多语言绑定 | 第24周 | 支持Python/Rust/Go |
| M5: 工具链 | 第28周 | 完整的开发工具链 |
| M6: v1.0发布 | 第32周 | 正式发布v1.0 |

---

## 风险和应对

### 技术风险

1. **性能不达标**
   - 风险: QuickJS性能可能不足
   - 应对: 早期性能测试，必要时优化或切换引擎

2. **组件库兼容性**
   - 风险: 某些组件库可能无法兼容
   - 应对: 优先支持核心组件，提供替代方案

3. **跨平台问题**
   - 风险: 不同平台行为不一致
   - 应对: 早期多平台测试，建立CI/CD

### 资源风险

1. **开发人力不足**
   - 应对: 合理分配任务，寻求社区贡献

2. **时间延期**
   - 应对: 灵活调整优先级，核心功能优先

---

## 后续规划 (v2.0+)

- WebGL支持
- 高级动画系统
- 插件系统
- 移动平台支持
- 云端UI编辑器

