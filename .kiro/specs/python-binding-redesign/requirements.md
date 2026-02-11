# 需求文档

## 简介

LightUI Python 绑定当前 API 设计存在多处不够 Pythonic 的问题：JS 调用约定混乱（`host.call()` vs `py.funcName()`）、函数绑定装饰器要求不自然的签名、状态管理缺乏声明式风格、模块导出命名空间混乱、资源管理不够自动化。本需求定义了对 Python 高级 API 层（`bindings/python/lightui/`）的重新设计，使其更优雅、更 Pythonic，适合用 Python 进行桌面应用开发。所有修改限定在 `bindings/python/` 目录内，不修改 C++ 代码。

## 术语表

- **LightUIApp**：高级 Python API 类，整合 Window、Document、Runtime、EventLoop、HostBridge，对外统一为 `ui.App`
- **Binding**：C++ 扩展模块 `lightui_core`（通过 pybind11 构建），提供底层能力
- **HostBridge**：Python ↔ JS 双向通信桥接，C++ 层已支持 `host.call()` 和 `py.funcName()` 两种 JS 调用方式
- **State**：响应式状态对象基类，子类包括 IntState、StringState、ListState、DictState
- **Bindable_Decorator**：新的 `@app.bindable` 装饰器，用于将 Python 函数注册为 JS 可调用函数
- **High_Level_API**：面向用户的简化 API 层（`lightui/` 包）
- **Low_Level_API**：直接暴露 C++ 绑定的底层 API（`lightui_core` 模块）
- **py_Namespace**：JS 全局对象 `py`，C++ HostBridge 在 `bind()` 时自动将函数注册到此命名空间，支持 `py.funcName(args)` 直接调用

## 需求

### 需求 1：简化模块导入与命名空间

**用户故事：** 作为 Python 开发者，我希望通过 `import lightui as ui` 即可获得所有常用 API，以便快速开始开发。

#### 验收标准

1. WHEN 用户执行 `import lightui as ui` 时，THE High_Level_API SHALL 导出 `App` 类作为主入口
2. WHEN 用户执行 `import lightui as ui` 时，THE High_Level_API SHALL 导出 `version` 函数用于获取版本号
3. THE `__init__.py` SHALL 将高级 API（App、version）作为顶层导出，将低级 API 移入 `lightui.core` 子模块
4. WHEN 用户需要低级 API 时，THE High_Level_API SHALL 支持通过 `from lightui.core import Window, Runtime` 方式访问
5. THE `__init__.py` SHALL 不再同时导出 `LightUIApp`、`App`、`CoreApp` 三个名称，仅导出 `App` 作为唯一的应用类名称
6. THE High_Level_API SHALL 保持向后兼容，`LightUIApp` 作为 `App` 的别名仍可导入

### 需求 2：Pythonic 函数绑定

**用户故事：** 作为 Python 开发者，我希望用更自然的方式将 Python 函数暴露给 JS 调用，以便减少样板代码。

#### 验收标准

1. THE App SHALL 提供 `@app.bindable` 装饰器，WHEN 使用该装饰器时，SHALL 自动使用函数名作为 JS 调用名称
2. WHEN 使用 `@app.bindable` 装饰器绑定函数时，THE Bindable_Decorator SHALL 支持函数接收多个独立参数（而非单个 JSON 参数），参数从 JS 传入的 JSON 对象或数组中自动解包
3. WHEN 使用 `@app.bindable` 装饰器绑定无参数函数时，THE Bindable_Decorator SHALL 允许函数签名为 `def func()` 而无需声明未使用的 `args` 参数
4. WHEN 使用 `@app.bindable` 装饰器绑定函数时，THE App SHALL 同时将函数注册到 HostBridge，使 JS 端可通过 `py.funcName(args)` 直接调用
5. THE App SHALL 保留 `@app.bind("name")` 装饰器作为显式命名的备选方式，保持向后兼容
6. WHEN `@app.bindable` 装饰器绑定的函数抛出异常时，THE App SHALL 捕获异常并返回包含错误信息的 JSON 对象给 JS 端

### 需求 3：统一 JS 调用约定为 `py.funcName()`

**用户故事：** 作为开发者，我希望 HTML 中的 JS 代码使用唯一且直观的方式调用 Python 函数，以便消除混乱。

#### 验收标准

1. THE 示例代码和文档 SHALL 统一使用 `py.funcName(args)` 作为唯一的 JS 调用方式
2. THE 文档和注释 SHALL 不再提及 `host.call()` 方式，仅展示 `py.funcName()` 用法
3. WHEN 使用 `@app.bindable` 或 `@app.bind("name")` 绑定函数时，THE App SHALL 确保 JS 端可通过 `py.funcName()` 调用

### 需求 4：改进状态管理 API

**用户故事：** 作为 Python 开发者，我希望状态管理更加声明式和直观，以便更自然地管理应用数据。

#### 验收标准

1. THE App SHALL 保留 `app.state("name", initial)` 方法创建响应式状态，根据初始值类型自动返回对应的 State 子类
2. WHEN 对 IntState 调用 `decrement()` 方法时，THE IntState SHALL 将值减少指定量（默认为 1）
3. WHEN 对 State 对象调用 `watch()` 方法时，THE State SHALL 返回一个可用于取消监听的标识符
4. THE App SHALL 提供 `app.batch()` 上下文管理器用于批量状态更新，批量模式下状态变化通知延迟到退出时统一触发

### 需求 5：自动资源管理

**用户故事：** 作为 Python 开发者，我希望不需要了解内部组件的清理顺序，以便安全地使用和释放资源。

#### 验收标准

1. THE App SHALL 支持 `with` 语句作为上下文管理器，退出时自动按正确顺序清理所有资源
2. WHEN App 作为上下文管理器退出时，THE App SHALL 按以下顺序清理：停止事件循环 → 关闭 DevTools → 清理字体缓存 → 清理 DOM 绑定 → 关闭窗口
3. WHEN `cleanup()` 被多次调用时，THE App SHALL 仅执行一次清理操作，后续调用为空操作
4. THE App SHALL 提供 `app.run()` 方法启动事件循环，窗口关闭时自动退出并触发清理

### 需求 6：更新示例代码

**用户故事：** 作为新用户，我希望示例代码展示新 API 的最佳实践，以便快速学习和上手。

#### 验收标准

1. THE 示例目录 SHALL 包含使用新 API 的 `counter.py` 计数器示例，演示 `@app.bindable`、`py.funcName()` 调用方式和 IntState
2. THE 示例目录 SHALL 包含使用新 API 的 `todo_app.py` 待办事项示例，演示 ListState 和多参数绑定
3. THE 示例目录 SHALL 包含使用新 API 的 `state_demo.py` 状态演示，展示所有状态类型的用法
4. WHEN 示例代码使用 `import lightui as ui` 导入时，THE 示例 SHALL 能正常运行且无导入错误
5. THE 示例代码中的 HTML SHALL 统一使用 `py.funcName()` 方式调用 Python 函数，不使用 `host.call()`

### 需求 7：参数自动解包

**用户故事：** 作为 Python 开发者，我希望绑定函数能像普通 Python 函数一样接收参数，以便写出更自然的代码。

#### 验收标准

1. WHEN JS 端调用 `py.funcName(arg)` 传入单个值时，THE Bindable_Decorator SHALL 将该值作为第一个位置参数传递给 Python 函数
2. WHEN JS 端调用 `py.funcName({a: 1, b: 2})` 传入对象时，THE Bindable_Decorator SHALL 支持将对象字段作为关键字参数解包传递给 Python 函数（如果函数签名匹配）
3. WHEN JS 端调用 `py.funcName()` 不传参数时，THE Bindable_Decorator SHALL 正确调用无参数的 Python 函数
4. IF 参数解包失败（签名不匹配），THEN THE Bindable_Decorator SHALL 回退到将原始 JSON 值作为单个参数传递

### 需求 8：类型存根与文档

**用户故事：** 作为 Python 开发者，我希望 IDE 能提供准确的自动补全和类型提示，以便提高开发效率。

#### 验收标准

1. THE High_Level_API SHALL 为 App 类的所有公开方法提供完整的类型注解
2. THE High_Level_API SHALL 为 `@app.bindable` 装饰器提供正确的类型注解，使 IDE 能识别装饰后的函数类型
3. THE 模块 SHALL 包含中文 docstring，说明每个公开 API 的用途和用法
4. THE `lightui_core.pyi` 类型存根 SHALL 与当前 C++ 绑定 API 保持同步

### 需求 9：向后兼容

**用户故事：** 作为现有用户，我希望升级后现有代码仍能正常工作，以便平滑迁移到新 API。

#### 验收标准

1. WHEN 用户使用旧的 `from lightui import LightUIApp` 导入方式时，THE High_Level_API SHALL 仍能正常导入
2. WHEN 用户使用旧的 `@app.bind("name")` 装饰器时，THE App SHALL 仍能正常绑定函数
3. THE High_Level_API SHALL 在旧 API 的 docstring 中标注推荐使用新 API

### 需求 10：测试覆盖

**用户故事：** 作为开发者，我希望新 API 有全面的自动化测试，以便在修改代码时确保功能正确性。

#### 验收标准

1. WHEN 运行测试时，THE 测试 SHALL 验证 `@app.bindable` 装饰器能正确注册函数并通过 JS 调用
2. WHEN 运行测试时，THE 测试 SHALL 验证参数自动解包对无参、单参、多参函数均正确工作
3. WHEN 运行测试时，THE 测试 SHALL 验证新旧 API 的向后兼容性
4. WHEN 运行属性测试时，THE 测试 SHALL 验证对于任意 JSON 兼容数据，通过 `@app.bindable` 绑定的回显函数往返传输后数据保持一致
5. WHEN 运行属性测试时，THE 测试 SHALL 验证对于任意有效函数名和参数组合，`@app.bindable` 绑定和调用的行为与 `@app.bind` 一致
