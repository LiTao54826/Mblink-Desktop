# Implementation Plan: Python Binding Redesign

## Overview

重构 LightUI Python 绑定高级 API 层，使其更 Pythonic。所有修改限定在 `bindings/python/` 目录内。按模块结构重组 → 核心功能实现 → 示例更新 → 测试补充的顺序推进。

## Tasks

- [x] 1. 重组模块结构
  - [x] 1.1 创建 `lightui/core.py`，将低级 API（Window、Runtime、EventLoop 等）从 `__init__.py` 移入此模块重导出
    - 导入 `lightui_core` 并重导出所有低级类和函数
    - 包含 `version` 函数
    - _Requirements: 1.3, 1.4_
  - [x] 1.2 精简 `lightui/__init__.py`，仅导出 `App`、`version`，添加 `LightUIApp` 向后兼容别名
    - `__all__ = ["App", "version"]`
    - `LightUIApp = App` 别名保留
    - _Requirements: 1.1, 1.2, 1.5, 1.6, 9.1_

- [x] 2. 实现 `@app.bindable` 装饰器和参数解包
  - [x] 2.1 在 `lightui/app.py` 中实现 `bindable` 方法
    - 使用 `inspect.signature` 分析函数签名
    - 实现参数解包逻辑（0参/1参/多参 dict→kwargs/多参 list→args/回退）
    - 注册到 `self._bridge.bind(func.__name__, wrapper)`
    - 保留原有 `bind("name")` 方法不变
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 7.1, 7.2, 7.3, 7.4_
  - [ ]* 2.2 编写 `@app.bindable` 属性测试
    - **Property 1: Bindable 注册正确性**
    - **Property 2: 参数解包正确性**
    - **Property 3: 参数解包回退**
    - **Property 4: 异常捕获**
    - **Validates: Requirements 2.1, 2.2, 2.3, 2.4, 2.6, 7.1, 7.2, 7.3, 7.4**

- [x] 3. 实现 IntState `decrement()` 方法
  - [x] 3.1 在 `lightui/app.py` 的 `state()` 方法中，为返回的 IntState 对象动态添加 `decrement` 方法（通过 `increment(-delta)` 实现）
    - _Requirements: 4.2_
  - [ ]* 3.2 编写 IntState decrement 属性测试
    - **Property 7: IntState decrement 正确性**
    - **Validates: Requirements 4.2**

- [ ] 4. Checkpoint — 核心功能验证
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 5. 编写 Bindable 数据往返和等价性属性测试
  - [ ]* 5.1 编写 Bindable 数据往返属性测试
    - **Property 5: Bindable 数据往返一致性**
    - **Validates: Requirements 10.4**
  - [ ]* 5.2 编写 Bindable 与 Bind 等价性属性测试
    - **Property 6: Bindable 与 Bind 等价性**
    - **Validates: Requirements 10.5**
  - [ ]* 5.3 编写 Cleanup 幂等性属性测试
    - **Property 8: Cleanup 幂等性**
    - **Validates: Requirements 5.3**

- [ ] 6. 更新示例代码
  - [ ] 6.1 更新 `examples/counter.py` 使用新 API（`import lightui as ui`、`@app.bindable`、`py.funcName()`）
    - _Requirements: 6.1, 6.4, 6.5_
  - [ ] 6.2 更新 `examples/todo_app.py` 使用新 API
    - _Requirements: 6.2, 6.4, 6.5_
  - [ ] 6.3 更新 `examples/state_demo.py` 使用新 API
    - _Requirements: 6.3, 6.4_

- [ ] 7. 更新类型注解和文档
  - [ ] 7.1 为 `App` 类的 `bindable` 方法添加类型注解和中文 docstring
    - 更新 `app.py` 中所有公开方法的 docstring，标注推荐使用新 API
    - _Requirements: 8.1, 8.2, 8.3, 9.3_
  - [ ] 7.2 更新 `lightui_core.pyi` 类型存根，确保与当前 C++ 绑定 API 同步
    - _Requirements: 8.4_

- [ ] 8. 编写向后兼容性和导入结构单元测试
  - [ ]* 8.1 编写单元测试验证模块导入结构和向后兼容性
    - 测试 `import lightui as ui` 导出 `App`、`version`
    - 测试 `from lightui.core import Window, Runtime` 可用
    - 测试 `from lightui import LightUIApp` 向后兼容
    - 测试 `@app.bind("name")` 旧装饰器仍可用
    - _Requirements: 1.1, 1.2, 1.4, 9.1, 9.2, 10.3_

- [ ] 9. Final checkpoint — 全部测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- 所有修改限定在 `bindings/python/` 目录内，不修改 C++ 代码
- 属性测试使用 hypothesis 库，每个测试最少 100 次迭代
- 测试使用 `[TEST_PASS]/[TEST_FAIL]` 标记格式
