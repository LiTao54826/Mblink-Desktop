# Implementation Plan: Python Binding Polish

## Overview

按增量方式完善 LightUI Python 绑定：先修复 Bug，再更新类型存根和安装脚本，然后补全示例代码，最后补充测试覆盖。每步都确保不破坏现有功能。

## Tasks

- [x] 1. 修复已知 Bug
  - [x] 1.1 统一版本号为 "0.5.0"
    - 修改 `bindings/python/lightui/__init__.py` 中 `__version__` 为 `"0.5.0"`
    - 修改 `bindings/python/setup.py` 中 `VERSION` 为 `"0.5.0"`
    - 修改 `bindings/python/tests/test_binding.py` 中版本号断言为 `"0.5.0"`
    - 修改 `bindings/python/tests/test_window.py` 中版本号断言为 `"0.5.0"`
    - _Requirements: 1.1, 1.2, 1.3, 1.4_

  - [x] 1.2 修复 LightUIApp._states 未初始化
    - 在 `bindings/python/lightui/app.py` 的 `LightUIApp.__init__` 中，在 `self._bound_functions` 之前添加 `self._states: Dict[str, Any] = {}`
    - _Requirements: 2.1, 2.2, 2.3_

- [x] 2. Checkpoint - 确保 Bug 修复无误
  - 确保所有测试通过，ask the user if questions arise.

- [x] 3. 更新类型存根和安装脚本
  - [x] 3.1 更新 lightui_core.pyi 类型存根
    - 添加 DevToolsManager 类定义（get_instance、initialize、open、close、toggle、is_open、shutdown）
    - 添加 TaskScheduler 类定义
    - 为 Window 类补充 set_js_runtime、get_task_scheduler、needs_repaint、mark_dirty
    - 为 Document 类补充 set_base_path、load_external_stylesheets、execute_scripts
    - 为 Runtime 类补充 register_module、set_base_module_path、init_fetch_bindings
    - 为 EventLoop 类补充 TaskScheduler 构造函数、set_quickjs_runtime、set_window、set_state_manager
    - 为 ListState 类补充 shift、unshift
    - 添加全局函数 cleanup_dom_bindings、clear_font_cache、set_image_base_path
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8_

  - [x] 3.2 更新 setup.py
    - 版本号改为 "0.5.0"
    - 添加 package_data 包含 bin/*.pyd 和 bin/*.so
    - 更新 python_requires 为 '>=3.8'
    - 在 extras_require['dev'] 中添加 hypothesis>=6.0.0
    - 移除已注释的 C 扩展配置代码
    - _Requirements: 5.1, 5.2, 5.3, 5.4_

- [x] 4. 补全示例代码
  - [x] 4.1 创建 counter.py 示例
    - 在 `bindings/python/examples/counter.py` 创建计数器示例
    - 演示 IntState、bind 装饰器、HTML 加载
    - _Requirements: 4.1_

  - [x] 4.2 创建 todo_app.py 示例
    - 在 `bindings/python/examples/todo_app.py` 创建待办事项示例
    - 演示 ListState、多函数绑定、DOM 操作
    - _Requirements: 4.2_

  - [x] 4.3 创建 state_demo.py 示例
    - 在 `bindings/python/examples/state_demo.py` 创建状态类型演示
    - 展示所有 State 子类型的 API 用法、watch、batch
    - _Requirements: 4.3_

- [ ] 5. 补充测试覆盖
  - [x] 5.1 创建 test_phase3.py 测试文件
    - 在 `bindings/python/tests/test_phase3.py` 创建 Phase 3 API 测试
    - 包含 DevToolsManager 单例测试、ES 模块注册与导入测试、资源清理函数测试
    - _Requirements: 6.3, 6.4, 6.5_

  - [ ]* 5.2 新增 HostBridge JSON 往返属性测试
    - 在 `bindings/python/tests/test_property.py` 中添加属性测试
    - **Property 2: HostBridge JSON 往返一致性**
    - **Validates: Requirements 6.6**

  - [ ]* 5.3 新增 LightUIApp state 代理属性测试
    - 在 `bindings/python/tests/test_property.py` 中添加属性测试
    - **Property 1: State 代理身份与缓存一致性**
    - **Validates: Requirements 2.2, 2.3**

- [ ] 6. Final checkpoint - 确保所有测试通过
  - 确保所有测试通过，ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Python 测试使用 `[TEST_PASS]/[TEST_FAIL]` 标记格式（手动测试）和 hypothesis（属性测试）
- C++ 绑定层（bindings.cpp）不需要修改，version() 已返回 "0.5.0"
- 所有修改限定在 `bindings/python/` 目录内
