# 富文本编辑器测试应用

这个目录包含了用于测试 LightUI 富文本编辑 API 的示例应用。

## 功能测试

本测试套件涵盖以下 API：

- ✅ **Range API** - DOM 范围选择和操作
- ✅ **Selection API** - 用户选择管理
- ✅ **MutationObserver API** - DOM 变化监控
- ✅ **ContentEditable** - 可编辑内容支持
- ✅ **Input Events** - beforeinput 和 input 事件
- ✅ **Clipboard API** - 复制、剪切、粘贴操作
- ✅ **execCommand API** - 文本格式化命令

## 快速开始

### 推荐测试顺序

1. **最小化测试**（验证基本功能）
   ```bash
   build/bin/Release/esm_loader.exe examples/rich_text_editor/minimal_test.js
   ```
   ✅ 已验证正常工作

2. **快速 API 测试**（完整的 API 验证）
   ```bash
   build/bin/Release/esm_loader.exe examples/rich_text_editor/quick_test.js
   ```
   测试所有核心 API 并显示结果

3. **交互式编辑器**（手动测试）
   ```bash
   build/bin/Release/esm_loader.exe examples/rich_text_editor/interactive_editor.js
   ```
   完整的编辑器界面，可以手动测试所有功能

### Windows 批处理脚本

```bash
examples\rich_text_editor\run_tests.bat
```

## 测试应用

### 1. 快速测试 (`quick_test.js`) ⚡ 推荐

快速验证所有 API 是否正常工作，立即显示结果。

**运行方式：**
```bash
build/bin/Release/esm_loader.exe examples/rich_text_editor/quick_test.js
```

**特点：**
- ⚡ 快速执行，无需等待
- ✅ 测试所有核心 API
- 📊 清晰的控制台输出
- 🎨 可视化结果页面

### 2. API 单元测试 (`api_test.js`)

系统性测试所有 API 的基本功能，自动运行并显示测试结果。

**运行方式：**
```bash
build/bin/Release/esm_loader.exe examples/rich_text_editor/api_test.js
```

**测试内容：**
- Range API 的所有方法和属性
- Selection API 的选择操作
- MutationObserver 的变化监控
- ContentEditable 的可编辑性检测
- execCommand 的命令执行

### 3. 基础功能测试 (`app.js`)

按顺序测试各个 API 的核心功能，输出详细的测试日志。

**运行方式：**
```bash
build/bin/Release/esm_loader.exe examples/rich_text_editor/app.js
```

**测试流程：**
1. Range API 测试
2. Selection API 测试
3. MutationObserver 测试
4. ContentEditable 测试
5. Clipboard 测试
6. execCommand 测试

### 4. 交互式编辑器 (`interactive_editor.js`)

完整的富文本编辑器界面，可以手动测试所有编辑功能。

**运行方式：**
```bash
build/bin/Release/esm_loader.exe examples/rich_text_editor/interactive_editor.js
```

**功能特性：**
- 📝 实时文本编辑
- 🎨 格式化工具栏（粗体、斜体、下划线）
- 📋 剪贴板操作（复制、剪切、粘贴）
- ⌨️ 键盘快捷键支持
- 📊 实时信息面板（显示选择状态、Range 信息）
- 📜 事件日志面板（记录所有事件和变化）
- 🔍 MutationObserver 实时监控

**键盘快捷键：**
- `Ctrl+B` / `Cmd+B` - 粗体
- `Ctrl+I` / `Cmd+I` - 斜体
- `Ctrl+U` / `Cmd+U` - 下划线
- `Ctrl+C` / `Cmd+C` - 复制
- `Ctrl+X` / `Cmd+X` - 剪切
- `Ctrl+V` / `Cmd+V` - 粘贴
- `Ctrl+A` / `Cmd+A` - 全选

## 预期结果

### API 单元测试
- 所有测试应该通过（绿色 ✓）
- 最终显示测试统计信息
- 失败的测试会显示红色 ✗

### 基础功能测试
- 控制台输出详细的测试步骤
- 每个 API 测试完成后显示 ✓ 标记
- 最后显示 "所有测试完成"

### 交互式编辑器
- 显示完整的编辑器界面
- 可以输入和编辑文本
- 工具栏按钮响应点击
- 日志面板显示所有事件
- 信息面板实时更新

## 故障排查

### 如果测试失败

1. **检查编译**：确保 `esm_loader` 已成功编译
   ```bash
   cmake --build build --config Release --target esm_loader
   ```

2. **查看日志**：运行测试时查看控制台输出，了解具体错误

3. **检查实现**：参考 `.kiro/specs/rich-text-editing-api/` 中的设计文档

### 常见问题

**Q: MutationObserver 没有触发回调**
A: 确保微任务队列正常工作，检查 `TaskScheduler::ProcessMicrotasks()` 是否被调用

**Q: Selection 操作无效**
A: 确保 DOM 元素已正确创建，并且 SelectionManager 已初始化

**Q: execCommand 不工作**
A: 确保有活动的选择，并且目标元素是 contenteditable

## 开发说明

### 添加新测试

在 `api_test.js` 中添加新的测试用例：

```javascript
testGroup('新功能测试');

// 测试代码
assert(condition, '测试描述');
```

### 修改交互式编辑器

编辑 `interactive_editor.js`，可以：
- 添加新的工具栏按钮
- 修改样式
- 添加新的事件监听器
- 扩展日志功能

## 相关文档

- [富文本编辑 API 需求文档](../../.kiro/specs/rich-text-editing-api/requirements.md)
- [富文本编辑 API 设计文档](../../.kiro/specs/rich-text-editing-api/design.md)
- [富文本编辑 API 任务列表](../../.kiro/specs/rich-text-editing-api/tasks.md)

## 许可证

与 LightUI 项目相同的许可证。
