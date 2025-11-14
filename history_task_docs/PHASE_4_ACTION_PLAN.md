# Phase 4: Preact 生态系统完善 - 行动计划（更新版）

> **日期**: 2025-11-13（更新）
> **分支**: `feature/phase4-preact-native-hooks`
> **预计时间**: 2-3周
> **目标**: 完善 Preact 生态系统，创建更多示例应用

---

## 📊 当前状态（2025-11-13）

### ✅ 已完成的功能
- ✅ **Preact 核心库** - 原生 Preact 10.19.3 集成
- ✅ **Hooks 支持** - useState, useEffect, useRef 等完全工作
- ✅ **事件处理** - onClick, onChange 等事件正常工作
- ✅ **Virtual DOM** - 完整的 Virtual DOM diffing 和渲染
- ✅ **DOM 绑定** - 完整的 DOM API 暴露给 JavaScript
- ✅ **重渲染机制** - setState 触发组件重新渲染和 DOM 更新
- ✅ **伪类支持** - :hover, :active, :focus-visible 等伪类
- ✅ **基础示例** - Counter, Hello World 示例完全工作

### 🎯 架构说明

**当前架构**（原生 Preact + DOM 绑定）：

```
JavaScript 层:
  - Preact 10.19.3 (原生库，未修改)
  - Hooks (原生库，未修改)
  - 应用代码 (app.js)
       ↓
  DOM API (通过 DOMBindings 暴露)
       ↓
C++ 层:
  - Document, Element, Text (core/dom/)
  - EventLoop, FocusManager (core/event/)
  - StyleResolver, RenderTree (core/render/)
  - Window, Skia 渲染 (core/window/)
```

**优势**：
- ✅ 使用原生 Preact，无需维护自定义实现
- ✅ 自动获得 Preact 生态系统的所有更新
- ✅ 代码更简洁，更易维护
- ✅ 性能更好（Preact 团队的优化）

---

## 🚀 解决方案（3个任务）

### Task 1: 实现组件重渲染机制 (2天)

#### 目标
让 `setState()` 触发组件重新渲染并更新 DOM

#### 实现步骤

**Step 1.1: 在 PreactRenderer 中存储组件实例**

```cpp
// core/quickjs/preact_renderer.h
class PreactRenderer {
private:
    // 组件实例映射：component_id -> {vnode, container, props}
    struct ComponentInstance {
        JSValueWrapper vnode;
        JSValueWrapper container;
        JSValueWrapper props;
        std::shared_ptr<Element> dom_root;
    };
    std::unordered_map<int, ComponentInstance> components_;
    int next_component_id_ = 1;
};
```

**Step 1.2: 渲染组件时注册 __rerender 函数**

```cpp
// core/quickjs/preact_renderer.cpp
JSValue PreactRenderer::RenderComponent(JSValue component_func, JSValue props_val) {
    // 创建组件实例对象
    JSValue component = JS_NewObject(ctx_);
    int component_id = next_component_id_++;
    JS_SetPropertyStr(ctx_, component, "__id", JS_NewInt32(ctx_, component_id));
    
    // 注册 __rerender 函数
    JSValue rerender_func = JS_NewCFunction(ctx_, js_rerender_component, "rerender", 0);
    JS_SetPropertyStr(ctx_, component, "__rerender", rerender_func);
    
    // 设置当前组件上下文
    SetCurrentComponent(component);
    
    // 调用组件函数
    JSValue vnode = JS_Call(ctx_, component_func, JS_UNDEFINED, 1, &props_val);
    
    // 存储组件实例
    components_[component_id] = {
        JSValueWrapper(ctx_, vnode),
        JSValueWrapper(ctx_, container),
        JSValueWrapper(ctx_, props_val),
        nullptr  // dom_root 稍后设置
    };
    
    return vnode;
}
```

**Step 1.3: 实现 js_rerender_component 回调**

```cpp
// core/quickjs/preact_renderer.cpp
static JSValue js_rerender_component(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    // 获取 component_id
    JSValue id_val = JS_GetPropertyStr(ctx, this_val, "__id");
    int component_id;
    JS_ToInt32(ctx, &component_id, id_val);
    JS_FreeValue(ctx, id_val);
    
    // 获取 PreactRenderer 实例（通过全局变量或 opaque）
    PreactRenderer* renderer = GetRendererFromContext(ctx);
    
    // 重新渲染组件
    renderer->RerenderComponent(component_id);
    
    return JS_UNDEFINED;
}
```

**Step 1.4: 实现 RerenderComponent 方法**

```cpp
void PreactRenderer::RerenderComponent(int component_id) {
    auto it = components_.find(component_id);
    if (it == components_.end()) return;
    
    auto& instance = it->second;
    
    // 重新调用组件函数生成新 VNode
    JSValue component_func = GetProperty(instance.vnode.Get(), "type");
    JSValue new_vnode = RenderComponent(component_func, instance.props.Get());
    
    // Diff 并更新 DOM
    if (instance.dom_root) {
        UpdateElement(instance.dom_root, new_vnode);
    }
    
    // 更新存储的 vnode
    instance.vnode = JSValueWrapper(ctx_, new_vnode);
}
```

**验收标准**:
- [ ] setState 调用后触发重渲染
- [ ] DOM 正确更新
- [ ] Counter 示例可以点击按钮增减

---

### Task 2: 实现事件处理 (1天)

#### 目标
让 onClick、onChange 等事件正常工作

#### 实现步骤

**Step 2.1: 在 ApplyProps 中处理事件属性**

```cpp
// core/quickjs/preact_renderer.cpp
void PreactRenderer::ApplyProps(std::shared_ptr<Element> element, JSValue props_val) {
    // ... 现有代码 ...
    
    // 遍历所有属性
    JSPropertyEnum* props;
    uint32_t prop_count;
    JS_GetOwnPropertyNames(ctx_, &props, &prop_count, props_val, JS_GPN_STRING_MASK);
    
    for (uint32_t i = 0; i < prop_count; i++) {
        const char* key = JS_AtomToCString(ctx_, props[i].atom);
        JSValue value = JS_GetProperty(ctx_, props_val, props[i].atom);
        
        std::string key_str(key);
        
        // 处理事件（on开头的属性）
        if (key_str.size() > 2 && key_str.substr(0, 2) == "on") {
            std::string event_name = key_str.substr(2);
            // onclick -> click
            std::transform(event_name.begin(), event_name.end(), 
                          event_name.begin(), ::tolower);
            
            AddEventListener(element, event_name, value);
        } else {
            // 普通属性
            element->setAttribute(key_str, GetString(value));
        }
        
        JS_FreeCString(ctx_, key);
        JS_FreeValue(ctx_, value);
    }
    
    js_free(ctx_, props);
}
```

**Step 2.2: 实现 AddEventListener**

```cpp
void PreactRenderer::AddEventListener(std::shared_ptr<Element> element,
                                      const std::string& event_name,
                                      JSValue handler) {
    if (!JS_IsFunction(ctx_, handler)) return;
    
    // 存储 handler 防止被 GC
    auto wrapper = std::make_shared<JSValueWrapper>(ctx_, handler);
    event_handlers_[element].push_back(wrapper);
    
    // 创建 C++ 事件监听器
    element->addEventListener(event_name, [this, wrapper](Event* event) {
        // 调用 JavaScript handler
        JSValue event_obj = CreateEventObject(event);
        JSValue result = JS_Call(ctx_, wrapper->Get(), JS_UNDEFINED, 1, &event_obj);
        JS_FreeValue(ctx_, result);
        JS_FreeValue(ctx_, event_obj);
    });
}
```

**Step 2.3: 创建 Event 对象**

```cpp
JSValue PreactRenderer::CreateEventObject(Event* event) {
    JSValue obj = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, obj, "type", JS_NewString(ctx_, event->type().c_str()));
    JS_SetPropertyStr(ctx_, obj, "target", /* element JSValue */);
    // ... 其他属性
    return obj;
}
```

**验收标准**:
- [ ] onClick 事件触发
- [ ] 事件回调正确执行
- [ ] Counter 示例完全工作

---

### Task 3: 实现 Virtual DOM Diff (2天)

#### 目标
优化更新性能，只更新变化的部分

#### 实现步骤

**Step 3.1: 实现 UpdateElement**

```cpp
void PreactRenderer::UpdateElement(std::shared_ptr<Element> element, JSValue new_vnode) {
    // 获取旧的 props 和 children（需要存储）
    JSValue old_props = /* 从某处获取 */;
    JSValue new_props = GetProperty(new_vnode, "props");
    
    // 更新 props
    UpdateProps(element, old_props, new_props);
    
    // 更新 children
    JSValue old_children = GetProperty(old_props, "children");
    JSValue new_children = GetProperty(new_props, "children");
    UpdateChildren(element, old_children, new_children);
}
```

**Step 3.2: 实现 UpdateProps**

```cpp
void PreactRenderer::UpdateProps(std::shared_ptr<Element> element,
                                 JSValue old_props, JSValue new_props) {
    // 移除旧的属性
    // 添加新的属性
    // 更新变化的属性
}
```

**Step 3.3: 实现 UpdateChildren**

```cpp
void PreactRenderer::UpdateChildren(std::shared_ptr<Element> element,
                                    JSValue old_children, JSValue new_children) {
    // 简化版 Diff 算法
    // 1. 如果长度相同，逐个对比更新
    // 2. 如果长度不同，删除多余的或添加新的
}
```

**验收标准**:
- [ ] 只更新变化的 DOM 节点
- [ ] 性能测试：1000次更新 < 100ms

---

## 📋 开发检查清单

### Day 1-2: 重渲染机制
- [ ] 修改 PreactRenderer.h 添加组件存储
- [ ] 实现 RenderComponent 注册 __rerender
- [ ] 实现 js_rerender_component 回调
- [ ] 实现 RerenderComponent 方法
- [ ] 测试 Counter 示例

### Day 3: 事件处理
- [ ] 修改 ApplyProps 处理 on* 属性
- [ ] 实现 AddEventListener
- [ ] 实现 CreateEventObject
- [ ] 测试所有事件类型

### Day 4-5: Virtual DOM Diff
- [ ] 实现 UpdateElement
- [ ] 实现 UpdateProps
- [ ] 实现 UpdateChildren
- [ ] 性能测试

### Day 6-7: 测试和优化
- [ ] 创建完整测试套件
- [ ] 修复所有 bug
- [ ] 性能优化
- [ ] 文档更新

---

## 🎯 成功标准

### 功能标准
✅ Counter 示例完全工作（点击按钮，数字更新）
✅ Timer 示例工作（useEffect + setInterval）
✅ Todo App 示例工作（综合测试）

### 性能标准
✅ 组件渲染 < 16ms
✅ 状态更新 < 5ms
✅ 无内存泄漏

### 质量标准
✅ 30+ 测试用例通过
✅ 无崩溃
✅ 代码审查通过

---

## 🚦 立即开始

**第一步**: 修改 `core/quickjs/preact_renderer.h`，添加组件存储结构

```bash
# 开始开发
git status
# 确认在 feature/phase4-preact-native-hooks 分支
```

准备好了吗？让我们开始 Task 1！

