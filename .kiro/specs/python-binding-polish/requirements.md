# 需求文档

## 简介

LightUI Python 绑定当前完成度约 85-90%，本需求文档定义了将其完善至生产就绪状态所需的全部工作。涵盖已知 Bug 修复、类型存根更新、示例代码补全、安装脚本更新和测试覆盖补充。

## 术语表

- **Binding**：Python 绑定模块，即 `lightui_core` C++ 扩展（通过 pybind11 构建）
- **LightUIApp**：高级 Python API 类，整合 Window、Document、Runtime、EventLoop、HostBridge
- **Type_Stub**：类型存根文件（`lightui_core.pyi`），为 IDE 提供自动补全和类型检查
- **Setup_Script**：`setup.py` 安装脚本，定义包元数据和依赖
- **State**：响应式状态对象，支持 IntState、StringState、ListState、DictState 子类型
- **DevToolsManager**：开发者工具管理器单例，提供 F12 调试面板
- **TaskScheduler**：任务调度器，支持 setTimeout/setInterval 等 JS 定时器
- **EventLoop**：事件循环，驱动窗口渲染和 JS 回调执行
- **HostBridge**：Python ↔ JS 双向通信桥接

## 需求

### 需求 1：版本号统一

**用户故事：** 作为开发者，我希望所有模块的版本号保持一致，以便准确识别当前使用的版本。

#### 验收标准

1. THE Binding SHALL 在 `version()` 函数中返回 `"0.5.0"`
2. THE `__init__.py` SHALL 声明 `__version__ = "0.5.0"`
3. THE Setup_Script SHALL 声明 `VERSION = "0.5.0"`
4. WHEN 测试文件断言版本号时，THE 测试 SHALL 使用 `"0.5.0"` 作为期望值

### 需求 2：修复 `_states` 未初始化 Bug

**用户故事：** 作为开发者，我希望 LightUIApp 的 `state()` 方法能正常工作，以便管理应用状态。

#### 验收标准

1. WHEN LightUIApp 实例被创建时，THE LightUIApp SHALL 在 `__init__` 中初始化 `self._states` 为空字典
2. WHEN `state()` 方法被调用时，THE LightUIApp SHALL 正确缓存并返回 State 对象
3. WHEN 使用相同名称重复调用 `state()` 时，THE LightUIApp SHALL 返回已缓存的同一 State 对象

### 需求 3：更新类型存根

**用户故事：** 作为开发者，我希望类型存根文件完整反映当前 C++ 绑定的全部 API，以便获得准确的 IDE 自动补全和类型检查。

#### 验收标准

1. THE Type_Stub SHALL 包含 DevToolsManager 类的完整类型定义（get_instance、initialize、open、close、toggle、is_open、shutdown）
2. THE Type_Stub SHALL 包含 TaskScheduler 类的类型定义
3. THE Type_Stub SHALL 为 Window 类补充 set_js_runtime、get_task_scheduler、needs_repaint、mark_dirty 方法签名
4. THE Type_Stub SHALL 为 Document 类补充 set_base_path、load_external_stylesheets、execute_scripts 方法签名
5. THE Type_Stub SHALL 为 Runtime 类补充 register_module、set_base_module_path、init_fetch_bindings 方法签名
6. THE Type_Stub SHALL 为 EventLoop 类补充接受 TaskScheduler 参数的构造函数、set_quickjs_runtime、set_window、set_state_manager 方法签名
7. THE Type_Stub SHALL 包含全局函数 cleanup_dom_bindings、clear_font_cache、set_image_base_path 的类型定义
8. THE Type_Stub SHALL 为 ListState 类补充 shift 和 unshift 方法签名

### 需求 4：补全示例代码

**用户故事：** 作为新用户，我希望有可运行的示例代码，以便快速学习 LightUI Python 绑定的使用方法。

#### 验收标准

1. THE 示例目录 SHALL 包含 `counter.py` 示例，演示基本的状态管理和 Python ↔ JS 交互
2. THE 示例目录 SHALL 包含 `todo_app.py` 示例，演示列表状态管理和 DOM 操作
3. THE 示例目录 SHALL 包含 `state_demo.py` 示例，演示所有状态类型（IntState、StringState、ListState、DictState）的用法
4. WHEN 示例代码在有 lightui_core.pyd 的环境中运行时，THE 示例 SHALL 正常启动且无导入错误

### 需求 5：更新安装脚本

**用户故事：** 作为开发者，我希望 setup.py 正确配置包元数据和预编译扩展分发，以便通过 pip 安装。

#### 验收标准

1. THE Setup_Script SHALL 使用 `"0.5.0"` 作为版本号
2. THE Setup_Script SHALL 配置 `package_data` 以包含 `lightui/bin/lightui_core.pyd` 预编译扩展
3. THE Setup_Script SHALL 声明 `python_requires='>=3.8'`
4. THE Setup_Script SHALL 在 `extras_require['dev']` 中包含 hypothesis 测试依赖

### 需求 6：补充测试覆盖

**用户故事：** 作为开发者，我希望有全面的自动化测试，以便在修改代码时确保功能正确性。

#### 验收标准

1. WHEN 运行版本号测试时，THE 测试 SHALL 验证 `version()` 返回 `"0.5.0"`
2. WHEN 运行 LightUIApp 测试时，THE 测试 SHALL 验证 `state()` 方法在初始化后正常工作
3. WHEN 运行 DevTools 测试时，THE 测试 SHALL 验证 DevToolsManager 单例获取、初始化和状态查询功能
4. WHEN 运行 ES 模块测试时，THE 测试 SHALL 验证 register_module 和 eval_module 的模块注册与导入功能
5. WHEN 运行资源清理测试时，THE 测试 SHALL 验证 cleanup_dom_bindings 和 clear_font_cache 调用不会引发异常
6. WHEN 运行 HostBridge 属性测试时，THE 测试 SHALL 验证对于任意 JSON 兼容数据，Python→JS→Python 往返传输后数据保持一致
