# QuickJS Bindings

JavaScript 绑定实现，将 C++ 对象暴露给 JavaScript。

## 模块列表

| 文件 | 描述 |
|------|------|
| `js_element.h/cpp` | Element 类绑定 |
| `js_document.h/cpp` | Document 类绑定 |
| `js_node.h/cpp` | Node 类绑定 |
| `js_event.h/cpp` | Event 类绑定 |
| `js_style.h/cpp` | CSSStyleDeclaration 绑定 |
| `js_range.h/cpp` | Range 类绑定 |
| `js_selection.h/cpp` | Selection 类绑定 |

## 架构说明

每个绑定文件负责：
1. 定义 JavaScript 类原型
2. 实现属性 getter/setter
3. 实现方法调用
4. 管理 C++ 对象生命周期

## 使用示例

```javascript
// JavaScript 代码
const div = document.createElement('div');
div.style.backgroundColor = 'red';
document.body.appendChild(div);
```
