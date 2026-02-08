# Preact 样式和布局系统测试套件

这是一套专门用于测试 MBink 样式系统和布局引擎的 Preact 应用示例。

## 测试场景

### 01_basic_styles.js - 基础样式渲染
**测试目标**: 验证内联样式、CSS 属性解析、颜色处理
**测试内容**:
- 颜色测试 (Hex, RGB, RGBA)
- 尺寸和单位 (px)
- 边框和圆角
- 内外边距
- 文本样式 (字体大小、粗细、对齐)
- 阴影效果

**潜在 BUG**: 样式属性不生效、颜色解析错误、单位处理问题

### 02_flexbox_layout.js - Flexbox 布局系统
**测试目标**: 验证 flex 容器、flex 项目、对齐方式、方向、换行
**测试内容**:
- flex-direction (row, column, row-reverse, column-reverse)
- justify-content (flex-start, center, flex-end, space-between, space-around)
- align-items (flex-start, center, flex-end, stretch)
- flex-grow 和 flex-shrink
- flex-wrap (nowrap, wrap)
- gap 属性

**潜在 BUG**: flex 属性不生效、对齐错误、尺寸计算问题、换行异常

### 03_box_model.js - 盒模型和溢出处理
**测试目标**: 验证 box-sizing、overflow、min/max 尺寸
**测试内容**:
- box-sizing (content-box, border-box)
- overflow (visible, hidden, scroll, auto)
- overflow-x 和 overflow-y
- min/max width 和 height
- text-overflow (ellipsis)
- word-wrap (break-word)

**潜在 BUG**: 盒模型计算错误、溢出处理异常、尺寸约束失效

### 04_positioning.js - 定位系统
**测试目标**: 验证 static、relative、absolute、fixed 定位
**测试内容**:
- position: static (默认)
- position: relative (相对定位 + top/left 偏移)
- position: absolute (绝对定位 + 四角定位 + 居中)
- position: fixed (固定定位)
- z-index (层叠顺序)
- 复杂定位组合

**潜在 BUG**: 定位计算错误、z-index 层叠问题、偏移量不生效

### 05_nested_layouts.js - 复杂嵌套布局
**测试目标**: 验证多层嵌套、混合布局模式、复杂组合
**测试内容**:
- 嵌套 Flexbox
- 卡片网格布局
- 侧边栏布局
- 圣杯布局 (Holy Grail Layout)

**潜在 BUG**: 嵌套计算错误、布局冲突、性能问题

### 06_dynamic_styles.js - 动态样式更新
**测试目标**: 验证样式动态变化、重新布局、重绘性能
**测试内容**:
- 动态尺寸变化
- 动态颜色变化
- 旋转和透明度
- 动态圆角
- Flexbox 动态变化
- 自动动画

**潜在 BUG**: 样式更新不触发重绘、布局抖动、内存泄漏

### 07_text_fonts.js - 文本和字体样式
**测试目标**: 验证字体渲染、文本对齐、行高、字间距
**测试内容**:
- 字体大小 (10px - 48px)
- 字体粗细 (100 - 900)
- 文本对齐 (left, center, right, justify)
- 行高 (line-height)
- 字间距和词间距
- 文本装饰 (underline, overline, line-through)
- 文本转换 (uppercase, lowercase, capitalize)
- 文本阴影
- 长文本处理 (ellipsis, word-wrap)

**潜在 BUG**: 字体加载失败、文本溢出、行高计算错误

### 08_stress_test.js - 边界情况和压力测试
**测试目标**: 验证大量元素渲染、深层嵌套、极端值处理
**测试内容**:
- 大量元素渲染 (10-1000 个元素)
- 深层嵌套 (1-20 层)
- 极端尺寸 (1x1px, 2000x100px, 100x2000px)
- 极端文本 (超长单词、10000 字符、特殊字符)
- 极端颜色值 (透明、半透明)
- 复杂变换 (rotate, scale, skew)

**潜在 BUG**: 性能问题、栈溢出、内存泄漏、渲染卡顿

## 运行方法

### 🌐 浏览器对比测试（推荐）

**第一步：启动本地服务器**
```bash
cd tests\style_tests
start_server.bat
```

**第二步：打开浏览器**
- 访问 http://localhost:8000/index.html 查看测试索引
- 点击任意测试卡片的 "🌐 浏览器" 按钮查看标准渲染效果
- 或直接访问：
  - http://localhost:8000/01_basic_styles.html
  - http://localhost:8000/02_flexbox_layout.html
  - http://localhost:8000/03_box_model.html
  - 等等...

**第三步：运行 MBink 测试对比**
```bash
# 在另一个终端窗口运行
build\bin\Release\esm_loader.exe tests\style_tests\01_basic_styles.js
```

**对比要点**：
- ✅ 浏览器和 MBink 使用**完全相同的 .js 文件**
- ✅ 浏览器使用 CDN 的 Preact，MBink 使用内嵌的 Preact
- ✅ 可以并排对比渲染效果，找出差异

### 🚀 单独运行 MBink 测试
```bash
build\bin\Release\esm_loader.exe tests\style_tests\01_basic_styles.js
build\bin\Release\esm_loader.exe tests\style_tests\02_flexbox_layout.js
build\bin\Release\esm_loader.exe tests\style_tests\03_box_model.js
build\bin\Release\esm_loader.exe tests\style_tests\04_positioning.js
build\bin\Release\esm_loader.exe tests\style_tests\05_nested_layouts.js
build\bin\Release\esm_loader.exe tests\style_tests\06_dynamic_styles.js
build\bin\Release\esm_loader.exe tests\style_tests\07_text_fonts.js
build\bin\Release\esm_loader.exe tests\style_tests\08_stress_test.js
```

### 📦 批量运行所有测试
```bash
# Windows
tests\style_tests\run_all_tests.bat

# 或者手动逐个运行
```

## 测试检查清单

### 视觉检查
- [ ] 颜色是否正确显示
- [ ] 布局是否符合预期
- [ ] 文本是否清晰可读
- [ ] 边框和圆角是否正确
- [ ] 阴影效果是否显示
- [ ] 动画是否流畅

### 交互检查
- [ ] 滑块控制是否工作
- [ ] 按钮点击是否响应
- [ ] 滚动是否正常
- [ ] 动态更新是否触发重绘

### 性能检查
- [ ] 大量元素渲染是否卡顿
- [ ] 深层嵌套是否导致崩溃
- [ ] 内存使用是否正常
- [ ] CPU 占用是否合理

### 控制台检查
- [ ] 是否有 JavaScript 错误
- [ ] 是否有样式解析警告
- [ ] 是否有布局计算错误
- [ ] 性能日志是否正常

## 已知问题

记录在测试过程中发现的问题：

1. **问题描述**: 
   - 测试场景: 
   - 复现步骤: 
   - 预期行为: 
   - 实际行为: 
   - 截图/日志: 

## 开发工具

使用 DevTools 进行调试：
```bash
build\bin\Release\esm_loader.exe tests\style_tests\01_basic_styles.js --devtools
```

## 注意事项

1. 确保已经编译了 `esm_loader.exe`
2. 测试文件使用 ES6 模块语法 (import/export)
3. Preact 和 Hooks 已经内嵌在 esm_loader 中
4. 所有测试都是纯 JavaScript，不依赖 Python 绑定
5. 测试主要关注样式系统和布局引擎，不测试 DOM API

## 贡献

添加新测试场景：
1. 在 `tests/style_tests/` 目录下创建新的 `.js` 文件
2. 使用 `import { h, render } from 'preact'` 导入 Preact
3. 创建测试组件并渲染到 DOM
4. 更新此 README 文件

## 许可证

与 MBink 项目相同

