# MBink 框架已知问题

**更新日期**: 2025-11-15  
**测试通过率**: 90.5% (105/116)

---

## 📊 问题概览

| 问题类别 | 严重程度 | 影响测试数 | 状态 |
|---------|---------|-----------|------|
| querySelector 架构问题 | 🔴 高 | 10/20 失败 | 待修复 |
| innerHTML/outerHTML 问题 | 🟡 中 | 7/17 失败 | 待修复 |
| cloneNode 深克隆 bug | 🟢 低 | 1/19 失败 | 待修复 |

---

## 🔴 问题 1: querySelector 架构问题 (高优先级)

### 问题描述
动态创建的元素无法被 `querySelector`/`querySelectorAll` 等 CSS 选择器 API 查询到。

### 失败的测试 (10个)
```
❌ querySelector - 通过标签名查询
❌ querySelector - 通过 class 查询
❌ querySelector - 通过 ID 查询
❌ querySelector - 复杂选择器
❌ querySelectorAll - 查询所有匹配元素
❌ querySelectorAll - 通过 class 查询
❌ getElementById - 查询元素
❌ getElementsByClassName - 查询元素
❌ getElementsByTagName - 查询元素
❌ matches - 匹配标签名
❌ matches - 复杂选择器
```

### 错误信息
```
错误: 断言失败: 应找到元素
值不应为 null
```

### 根本原因
MBink 有**两个独立的 DOM 系统**:

1. **MBink DOM** (`core/dom/`)
   - 主要的 DOM API 实现
   - 所有 JavaScript 绑定都基于这个系统
   - `document.createElement()` 创建的元素只存在于这个系统

2. **Lexbor DOM** (`third_party/lexbor/`)
   - HTML 解析器
   - CSS 选择器引擎
   - 只有通过 HTML 解析创建的元素才存在于这个系统

**问题**: 
- `querySelector` 等 API 依赖 Lexbor 的 CSS 选择器引擎
- 动态创建的元素 (`document.createElement`) 只在 MBink DOM 中
- Lexbor DOM 中没有这些元素的表示，所以查询返回 null

### 复现步骤
```javascript
// 这个会失败
const div = document.createElement('div');
div.className = 'test';
document.body.appendChild(div);
const found = document.querySelector('.test'); // 返回 null ❌

// 这个会成功 (通过 innerHTML 创建)
document.body.innerHTML = '<div class="test"></div>';
const found2 = document.querySelector('.test'); // 返回元素 ✅
```

### 影响范围
- **查询选择器测试**: 10/20 失败 (50% 通过率)
- **影响功能**: 
  - 动态创建元素后无法通过选择器查询
  - `matches()` 对动态元素返回 false
  - 影响所有依赖 CSS 选择器的功能

### 解决方案

#### 方案 1: 实现 MBink DOM → Lexbor DOM 同步 (推荐)
**优点**: 
- 保持现有架构
- 利用 Lexbor 的高性能 CSS 选择器引擎
- 完全兼容标准

**缺点**: 
- 需要维护两个 DOM 树的同步
- 增加内存开销
- 实现复杂度较高

**实现步骤**:
1. 在 `Element::AppendChild()` 中同步创建 Lexbor 节点
2. 在 `Element::RemoveChild()` 中同步删除 Lexbor 节点
3. 在 `Element::SetAttribute()` 中同步更新 Lexbor 属性
4. 在 `Document::CreateElement()` 中创建对应的 Lexbor 节点

#### 方案 2: 实现独立的 CSS 选择器引擎
**优点**: 
- 不需要维护两个 DOM 树
- 减少内存开销
- 架构更简洁

**缺点**: 
- 需要实现完整的 CSS 选择器解析和匹配
- 性能可能不如 Lexbor
- 开发工作量大

#### 方案 3: 混合方案
**实现思路**:
- 简单选择器 (标签名、class、ID) 使用自己实现的快速查询
- 复杂选择器回退到 Lexbor (仅对 Lexbor DOM 中的元素有效)

### 相关代码
- `core/dom/element.cpp` - `Element::QuerySelector()`
- `core/dom/document.cpp` - `Document::QuerySelector()`
- `core/dom/dom_bindings.cpp` - JavaScript 绑定

---

## 🟡 问题 2: innerHTML/outerHTML 问题 (中优先级)

### 问题描述
`innerHTML` 和 `outerHTML` 的 getter/setter 存在多个问题。

### 失败的测试 (7个)
```
❌ innerHTML - 设置简单 HTML
❌ innerHTML - 设置复杂 HTML
❌ innerHTML - 设置带属性的 HTML
❌ innerHTML - 获取 HTML
❌ innerHTML - 替换内容
❌ outerHTML - 获取外部 HTML
❌ outerHTML - 设置外部 HTML (崩溃)
```

### 错误信息

#### 错误 1: JavaScript 错误
```
错误: cannot read property 'toLowerCase' of undefined
```

**原因**: 
- 可能是 HTML 解析过程中的 JavaScript 错误
- 需要检查 `Element::SetInnerHTML()` 的实现

#### 错误 2: 子节点数量不正确
```
错误: 断言失败: 应有 3 个子节点
期望: 3
实际: 1
```

**原因**: 
- HTML 解析可能只解析了第一个元素
- 或者只创建了第一个子节点

#### 错误 3: outerHTML getter 返回空
```
错误: 断言失败: outerHTML 应包含 <span>Hello</span>
期望: true
实际: false
```

**原因**: 
- `Element::GetOuterHTML()` 可能返回空字符串
- 或者序列化逻辑有问题

#### 错误 4: outerHTML setter 崩溃
```
错误: Cannot set outerHTML without document
```

**原因**: 
- 元素没有正确的 document 引用
- 或者 `Element::SetOuterHTML()` 的前置检查过于严格

### 复现步骤

#### innerHTML setter 问题
```javascript
const div = document.createElement('div');
div.innerHTML = '<span>Hello</span>'; // 可能抛出 JavaScript 错误
console.log(div.childNodes.length); // 期望 1，可能是 0 或其他值
```

#### innerHTML getter 问题
```javascript
const div = document.createElement('div');
const span = document.createElement('span');
span.textContent = 'Hello';
div.appendChild(span);
console.log(div.innerHTML); // 期望 '<span>Hello</span>'，实际可能是空字符串
```

#### outerHTML setter 崩溃
```javascript
const div = document.createElement('div');
document.body.appendChild(div);
div.outerHTML = '<p>New</p>'; // 崩溃: Cannot set outerHTML without document
```

### 影响范围
- **HTML 内容测试**: 7/17 失败 (41.2% 通过率)
- **影响功能**: 
  - 无法通过 innerHTML 设置复杂 HTML
  - 无法正确获取元素的 HTML 表示
  - outerHTML setter 完全不可用

### 解决方案

#### 修复 1: innerHTML setter
1. 检查 `Element::SetInnerHTML()` 实现
2. 确保 HTML 解析器正确解析所有子元素
3. 检查 JavaScript 错误的来源 (可能是 QuickJS 绑定问题)

#### 修复 2: innerHTML getter
1. 检查 `Element::GetInnerHTML()` 实现
2. 确保正确序列化所有子节点
3. 检查标签名、属性、文本内容的序列化

#### 修复 3: outerHTML setter
1. 检查 `Element::SetOuterHTML()` 的 document 检查逻辑
2. 确保元素有正确的 document 引用
3. 或者放宽检查条件

### 相关代码
- `core/dom/element.cpp` - `Element::SetInnerHTML()`, `Element::GetInnerHTML()`
- `core/dom/element.cpp` - `Element::SetOuterHTML()`, `Element::GetOuterHTML()`
- `core/dom/dom_bindings.cpp` - JavaScript 绑定

---

## 🟢 问题 3: cloneNode 深克隆 bug (低优先级)

### 问题描述
`cloneNode(true)` 深克隆时只克隆了部分子节点。

### 失败的测试 (1个)
```
❌ cloneNode - 深克隆(包含子节点)
```

### 错误信息
```
错误: 断言失败: 应包含 2 个子节点
期望: 2
实际: 1
```

### 复现步骤
```javascript
const parent = document.createElement('div');
const child1 = document.createElement('span');
const child2 = document.createElement('p');
parent.appendChild(child1);
parent.appendChild(child2);

const clone = parent.cloneNode(true);
console.log(clone.childNodes.length); // 期望 2，实际 1
```

### 根本原因
`Element::CloneNode(bool deep)` 的递归克隆逻辑有问题:
- 可能只克隆了第一个子节点
- 或者递归终止条件不正确

### 影响范围
- **DOM 操作测试**: 1/19 失败 (94.7% 通过率)
- **影响功能**: 
  - 深克隆功能不完整
  - 可能影响依赖深克隆的框架功能

### 解决方案
1. 检查 `Element::CloneNode()` 的实现
2. 确保正确遍历所有子节点
3. 确保递归克隆所有后代节点

### 相关代码
- `core/dom/element.cpp` - `Element::CloneNode(bool deep)`
- `core/dom/node.cpp` - `Node::CloneNode(bool deep)`

---

## 📈 问题优先级建议

### 立即修复 (影响核心功能)
1. **outerHTML setter 崩溃** - 导致测试中断
   - 修复时间: 0.5 小时
   - 影响: 解除测试阻塞

### 短期修复 (1-2 天)
2. **innerHTML setter/getter 问题** - 影响 HTML 操作
   - 修复时间: 0.5-1 天
   - 影响: HTML 内容测试通过率从 41% → 90%+

3. **cloneNode 深克隆 bug** - 影响 DOM 操作
   - 修复时间: 0.5 小时
   - 影响: DOM 操作测试通过率从 94.7% → 100%

### 中期修复 (3-5 天)
4. **querySelector 架构问题** - 需要架构调整
   - 修复时间: 3-5 天
   - 影响: 查询选择器测试通过率从 50% → 90%+
   - 总体通过率从 90.5% → 95%+

---

## 🎯 修复后预期通过率

| 修复阶段 | 通过率 | 提升 |
|---------|--------|------|
| **当前** | 90.5% | - |
| 修复 outerHTML setter | 91.4% | +0.9% |
| 修复 innerHTML | 96.6% | +5.2% |
| 修复 cloneNode | 97.4% | +0.8% |
| 修复 querySelector | **99.1%** | +1.7% |

---

## 📝 测试命令

### 运行完整测试 (支持中文)
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001
.\build\bin\Release\comprehensive_test_app.exe
```

### 只查看失败的测试
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
.\build\bin\Release\comprehensive_test_app.exe 2>&1 | Select-String -Pattern "❌" -Context 2,0
```

### 统计测试结果
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$output = .\build\bin\Release\comprehensive_test_app.exe 2>&1 | Out-String
$passed = ([regex]::Matches($output, '✅')).Count
$failed = ([regex]::Matches($output, '❌')).Count
Write-Host "通过: $passed"
Write-Host "失败: $failed"
Write-Host "总计: $($passed + $failed)"
Write-Host "通过率: $([math]::Round(($passed / ($passed + $failed)) * 100, 2))%"
```

---

**文档维护**: 请在修复问题后更新此文档，标记问题状态为"已修复"并记录修复日期。

