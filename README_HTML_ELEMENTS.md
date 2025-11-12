# MBink HTML元素实现计划总结

**生成时间**: 2025-11-12  
**状态**: 📋 计划阶段  
**目标**: 实现完整的HTML元素支持，确保React能正常驱动，所有效果与标准浏览器一致

---

## 📊 当前状态

### ✅ 已完成 (2个元素)
- **HTMLInputElement** - 18种输入类型，完整实现 ⭐⭐⭐⭐⭐
- **HTMLTextAreaElement** - 多行文本输入，完整实现 ⭐⭐⭐⭐⭐

### 🔴 待实现 (P0 - 关键表单元素，1周)
1. **HTMLButtonElement** - 按钮元素
2. **HTMLSelectElement** - 下拉选择
3. **HTMLOptionElement** - 选项元素
4. **HTMLFormElement** - 表单元素

### 🟡 待实现 (P1 - 常用交互元素，1周)
5. **HTMLAnchorElement** - 超链接
6. **HTMLLabelElement** - 标签元素
7. **HTMLImageElement** - 图片元素

### 🟢 待实现 (P2 - 媒体和高级元素，2周)
8. **HTMLCanvasElement** - 画布
9. **HTMLVideoElement** - 视频
10. **HTMLAudioElement** - 音频
11. **HTMLIFrameElement** - 内联框架
12. **HTMLTableElement** - 表格及相关元素

---

## 📁 生成的文档

### 1. HTML_ELEMENTS_IMPLEMENTATION_PLAN.md
**内容**: 详细的实现计划
- 📊 当前状态分析
- 🎯 优先级分级 (P0/P1/P2)
- 📋 每个元素的详细接口定义
- 🗓️ 实施时间表 (按周划分)
- ✅ 验收标准

**关键内容**:
- P0元素 (1周): Button, Select, Option, Form
- P1元素 (1周): Anchor, Label, Image
- P2元素 (2周): Canvas, Video, Audio, IFrame, Table

### 2. HTML_ELEMENTS_STANDARDS_REFERENCE.md
**内容**: 标准参考文档
- 📚 官方规范链接 (WHATWG, W3C, MDN)
- 🏷️ HTML5标准元素分类 (10大类)
- 📖 关键接口详细说明 (Web IDL)
- 🎨 默认CSS样式参考 (Chromium User Agent Stylesheet)
- ✅ 实现检查清单

**关键内容**:
- 75个HTML5标准元素分类
- HTMLButtonElement Web IDL完整定义
- HTMLSelectElement Web IDL完整定义
- HTMLImageElement Web IDL完整定义
- 浏览器默认样式参考

### 3. HTML_ELEMENTS_DEVELOPMENT_GUIDE.md
**内容**: 开发指南和规范
- 🎯 核心原则 (标准一致性、React兼容性、性能要求)
- 📁 文件结构规范 (头文件/实现文件/测试文件模板)
- ✅ 开发检查清单
- 🚫 常见错误和最佳实践

**关键内容**:
- 完整的头文件模板 (HTMLButtonElement示例)
- 完整的实现文件模板 (包含所有细节)
- 完整的测试文件模板 (10+测试用例)
- 性能要求: 创建<1ms, 更新<0.1ms, 渲染>=60fps

---

## 🎯 核心目标

### 1. React驱动支持
- ✅ 所有元素必须支持React Virtual DOM操作
- ✅ 属性变更必须触发重新渲染
- ✅ 事件系统必须符合React合成事件规范
- ✅ 生命周期钩子必须正确触发

### 2. 浏览器标准一致性
- ✅ 所有行为与Chrome/Firefox/Safari一致
- ✅ 遵循WHATWG HTML Living Standard
- ✅ 实现所有标准IDL属性和方法
- ✅ CSS渲染效果与标准浏览器相同

### 3. 性能要求
- ✅ 元素创建 < 1ms
- ✅ 属性更新 < 0.1ms
- ✅ 样式计算 < 5ms
- ✅ 渲染帧率 >= 60fps

---

## 📋 实施计划

### 第1周: P0关键表单元素 (React必需)

#### Day 1-2: HTMLButtonElement
```cpp
class HTMLButtonElement : public Element {
    // type: "submit" | "reset" | "button"
    // disabled: 是否禁用
    // form: 关联的表单
    // 默认样式: 按钮外观
    // 伪类: :hover, :active, :disabled
};
```

**任务**:
- [ ] 创建 `core/dom/html_button_element.h/cpp`
- [ ] 实现所有IDL属性 (disabled, type, name, value, form)
- [ ] 实现默认样式 (border-style: outset, background: buttonface)
- [ ] 实现伪类状态 (:hover, :active, :disabled)
- [ ] 编写10+单元测试
- [ ] 集成到Element工厂方法

#### Day 3-4: HTMLSelectElement + HTMLOptionElement
```cpp
class HTMLSelectElement : public Element {
    // multiple: 是否多选
    // selectedIndex: 当前选中索引
    // value: 当前选中值
    // options: 所有option元素
    // change事件: 选择改变时触发
};

class HTMLOptionElement : public Element {
    // selected: 是否选中
    // value: 选项值
    // text: 选项文本
};
```

**任务**:
- [ ] 创建两个元素类
- [ ] 实现选择逻辑 (单选/多选)
- [ ] 实现change事件
- [ ] 实现默认样式 (下拉框外观)
- [ ] 编写测试

#### Day 5: HTMLFormElement
```cpp
class HTMLFormElement : public Element {
    // action: 提交URL
    // method: GET | POST
    // elements: 所有表单控件
    // submit(): 提交表单
    // reset(): 重置表单
    // checkValidity(): 验证表单
};
```

**任务**:
- [ ] 创建表单元素类
- [ ] 实现表单控件收集
- [ ] 实现验证逻辑
- [ ] 编写测试

---

### 第2周: P1常用交互元素

#### Day 1-2: HTMLAnchorElement
```cpp
class HTMLAnchorElement : public Element {
    // href: 链接地址
    // target: _blank | _self | _parent | _top
    // download: 下载文件名
    // 伪类: :visited, :hover, :active
};
```

#### Day 3: HTMLLabelElement
```cpp
class HTMLLabelElement : public Element {
    // htmlFor: 关联的表单控件ID
    // control: 关联的表单控件
    // 点击时聚焦到关联控件
};
```

#### Day 4-5: HTMLImageElement
```cpp
class HTMLImageElement : public Element {
    // src: 图片URL
    // alt: 替代文本
    // width, height: 尺寸
    // complete: 加载完成状态
    // 集成Skia图片渲染
};
```

---

## 🔧 技术实现要点

### 1. 元素工厂方法

**文件**: `core/dom/element.cpp`

```cpp
std::shared_ptr<Element> Element::CreateFromLexbor(lxb_dom_node_t* lexbor_node) {
    std::string tag_name = /* 获取标签名 */;
    
    std::shared_ptr<Element> new_elem;
    
    if (tag_name == "input") {
        new_elem = std::make_shared<HTMLInputElement>();
    } else if (tag_name == "textarea") {
        new_elem = std::make_shared<HTMLTextAreaElement>();
    } else if (tag_name == "button") {
        new_elem = std::make_shared<HTMLButtonElement>();
    } else if (tag_name == "select") {
        new_elem = std::make_shared<HTMLSelectElement>();
    } else if (tag_name == "option") {
        new_elem = std::make_shared<HTMLOptionElement>();
    } else if (tag_name == "form") {
        new_elem = std::make_shared<HTMLFormElement>();
    } else if (tag_name == "a") {
        new_elem = std::make_shared<HTMLAnchorElement>();
    } else if (tag_name == "label") {
        new_elem = std::make_shared<HTMLLabelElement>();
    } else if (tag_name == "img") {
        new_elem = std::make_shared<HTMLImageElement>();
    } else {
        new_elem = std::make_shared<Element>(tag_name);
    }
    
    return new_elem;
}
```

### 2. 默认样式系统

**文件**: `core/render/user_agent_stylesheet.h`

```cpp
class UserAgentStylesheet {
public:
    static std::string GetDefaultStyles() {
        return R"(
            button {
                display: inline-block;
                padding: 1px 6px;
                border: 2px outset buttonface;
                background-color: buttonface;
                color: buttontext;
                cursor: pointer;
            }
            
            select {
                display: inline-block;
                padding: 1px 2px;
                border: 1px solid #767676;
                background-color: white;
                cursor: pointer;
            }
            
            a {
                color: #0000EE;
                text-decoration: underline;
                cursor: pointer;
            }
            
            a:visited {
                color: #551A8B;
            }
        )";
    }
};
```

### 3. 伪类状态管理

```cpp
class HTMLButtonElement : public Element {
private:
    void UpdatePseudoClasses() {
        SetPseudoClass(":disabled", disabled_);
        SetPseudoClass(":enabled", !disabled_);
    }
    
public:
    void SetDisabled(bool disabled) {
        disabled_ = disabled;
        UpdatePseudoClasses();
        
        if (disabled && IsFocused()) {
            Blur();  // 禁用时移除焦点
        }
    }
};
```

---

## ✅ 验收标准

### 功能完整性
- [ ] 所有P0元素实现完成 (Button, Select, Option, Form)
- [ ] 所有IDL属性符合WHATWG标准
- [ ] 所有默认样式与Chrome一致
- [ ] 所有伪类状态正确

### React兼容性
- [ ] 可以通过React创建元素
- [ ] 属性变更触发重新渲染
- [ ] 事件处理符合React规范
- [ ] Virtual DOM diff正常工作

### 浏览器一致性
- [ ] 渲染效果与Chrome一致
- [ ] 交互行为与Firefox一致
- [ ] CSS样式与标准浏览器一致

### 测试覆盖
- [ ] 每个元素至少10个单元测试
- [ ] 集成测试覆盖React场景
- [ ] 性能测试达标 (创建<1ms, 更新<0.1ms)

---

## 📊 进度跟踪

| 元素 | 优先级 | 状态 | 完成度 | 预计完成 |
|------|--------|------|--------|----------|
| HTMLInputElement | ✅ | 已完成 | 100% | - |
| HTMLTextAreaElement | ✅ | 已完成 | 100% | - |
| HTMLButtonElement | 🔴 P0 | 未开始 | 0% | Week 1 Day 2 |
| HTMLSelectElement | 🔴 P0 | 未开始 | 0% | Week 1 Day 4 |
| HTMLOptionElement | 🔴 P0 | 未开始 | 0% | Week 1 Day 4 |
| HTMLFormElement | 🔴 P0 | 未开始 | 0% | Week 1 Day 5 |
| HTMLAnchorElement | 🟡 P1 | 未开始 | 0% | Week 2 Day 2 |
| HTMLLabelElement | 🟡 P1 | 未开始 | 0% | Week 2 Day 3 |
| HTMLImageElement | 🟡 P1 | 未开始 | 0% | Week 2 Day 5 |

---

## 🚀 快速开始

### 1. 阅读文档
```bash
# 查看实现计划
cat HTML_ELEMENTS_IMPLEMENTATION_PLAN.md

# 查看标准参考
cat HTML_ELEMENTS_STANDARDS_REFERENCE.md

# 查看开发指南
cat HTML_ELEMENTS_DEVELOPMENT_GUIDE.md
```

### 2. 开始实现第一个元素 (HTMLButtonElement)
```bash
# 创建文件
touch core/dom/html_button_element.h
touch core/dom/html_button_element.cpp
touch tests/test_html_button_element.cpp

# 使用模板开始编码
# 参考: HTML_ELEMENTS_DEVELOPMENT_GUIDE.md
```

### 3. 运行测试
```bash
# 编译
cmake --build build

# 运行测试
./build/tests/test_html_button_element
```

---

## 📚 参考资源

### 标准文档
- [WHATWG HTML Living Standard](https://html.spec.whatwg.org/)
- [MDN HTML Element Reference](https://developer.mozilla.org/en-US/docs/Web/HTML/Element)
- [Web IDL Standard](https://webidl.spec.whatwg.org/)

### 实现参考
- [Chromium Blink](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/html/)
- [WebKit](https://github.com/WebKit/WebKit/tree/main/Source/WebCore/html)
- [Gecko (Firefox)](https://searchfox.org/mozilla-central/source/dom/html)

### 测试参考
- [Web Platform Tests](https://github.com/web-platform-tests/wpt)
- [Chromium Layout Tests](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/web_tests/)

---

## 📞 联系方式

如有问题，请参考：
- 项目文档: `docs/PROJECT_STANDARDS.md`
- React集成计划: `PHASE_3_REACT_ECOSYSTEM_PLAN.md`
- 项目状态: `PROJECT_STATUS_2025.md`


