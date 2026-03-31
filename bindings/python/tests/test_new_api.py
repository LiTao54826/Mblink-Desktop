"""
MBink 新 API 测试

测试 HostBridge.bind 参数解包、模块导入结构等。
使用低级 API（mbink_core）直接测试，不创建 Window。
"""

import sys
import os
import inspect
import json

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import mbink_core as lui


def _bind_with_wrapper(bridge, func):
    """将 Python 函数包装为 HostBridge.bind 兼容的格式。

    HostBridge 回调接收 JSON 解析后的 Python 对象（args）。
    根据函数签名决定如何将 args（None/dict/list）解包为位置参数。
    """
    name = func.__name__
    sig = inspect.signature(func)
    params = [
        p for p in sig.parameters.values()
        if p.kind in (
            inspect.Parameter.POSITIONAL_ONLY,
            inspect.Parameter.POSITIONAL_OR_KEYWORD,
            inspect.Parameter.KEYWORD_ONLY,
        )
    ]

    if not params:
        # 无参函数：忽略 args
        def wrapper(args):
            return func()
    elif len(params) == 1:
        # 单参数：直接把 args 整体传入
        def wrapper(args):
            return func(args)
    else:
        # 多参数：尝试从 dict 或 list 解包
        param_names = [p.name for p in params]
        def wrapper(args):
            if isinstance(args, dict):
                kwargs = {k: args[k] for k in param_names if k in args}
                return func(**kwargs)
            elif isinstance(args, (list, tuple)):
                return func(*args[:len(params)])
            else:
                return func(args)

    bridge.bind(name, wrapper)


# ========== @app.bindable 参数解包测试 ==========

def test_bindable_no_args():
    """测试无参函数绑定"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        counter = app.state("c1", 0)

        def increment():
            counter.increment()
            app.process_queue()
            return counter.get()

        _bind_with_wrapper(bridge, increment)

        r = runtime.eval('py.increment()')
        assert r == 1, f"Expected 1, got {r}"
        r = runtime.eval('py.increment()')
        assert r == 2, f"Expected 2, got {r}"
        print("[TEST_PASS] bindable_no_args")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_no_args: {e}")


def test_bindable_single_arg():
    """测试单参函数绑定"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def echo(value):
            return value

        _bind_with_wrapper(bridge, echo)

        assert runtime.eval('py.echo(42)') == 42
        assert runtime.eval('py.echo("hello")') == "hello"
        assert runtime.eval('py.echo(null)') is None
        assert runtime.eval('py.echo(true)') == True
        print("[TEST_PASS] bindable_single_arg")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_single_arg: {e}")


def test_bindable_multi_args_dict():
    """测试多参函数 dict 解包"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def add(a, b):
            return a + b

        _bind_with_wrapper(bridge, add)

        r = runtime.eval('py.add({a: 3, b: 4})')
        assert r == 7, f"Expected 7, got {r}"
        print("[TEST_PASS] bindable_multi_args_dict")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_multi_args_dict: {e}")


def test_bindable_multi_args_list():
    """测试多参函数 list 解包"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def addL(a, b):
            return a + b

        _bind_with_wrapper(bridge, addL)

        r = runtime.eval('py.addL([10, 20])')
        assert r == 30, f"Expected 30, got {r}"
        print("[TEST_PASS] bindable_multi_args_list")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_multi_args_list: {e}")


def test_bindable_exception():
    """测试绑定函数异常捕获"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def fail():
            raise ValueError("test error")

        _bind_with_wrapper(bridge, fail)

        r = runtime.eval('py.fail()')
        assert isinstance(r, dict), f"Expected dict, got {type(r)}"
        assert "error" in r
        assert "test error" in r["error"]
        print("[TEST_PASS] bindable_exception")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_exception: {e}")


def test_bindable_fallback():
    """测试参数解包失败时回退"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def greet(name, greeting):
            return f"{greeting}, {name}"

        _bind_with_wrapper(bridge, greet)

        # 传入不匹配的 dict 键名，kwargs 解包失败，回退单参也失败，返回 error
        r = runtime.eval('py.greet({x: 1, y: 2})')
        assert isinstance(r, dict) and "error" in r
        print("[TEST_PASS] bindable_fallback")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_fallback: {e}")


def test_bindable_return_types():
    """测试各种返回类型"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def ret_none(): return None
        def ret_bool(): return True
        def ret_int(): return 42
        def ret_float(): return 3.14
        def ret_str(): return "hello"
        def ret_list(): return [1, "two", 3]
        def ret_dict(): return {"key": "value", "num": 123}

        for fn in [ret_none, ret_bool, ret_int, ret_float, ret_str, ret_list, ret_dict]:
            _bind_with_wrapper(bridge, fn)

        assert runtime.eval('py.ret_none()') is None
        assert runtime.eval('py.ret_bool()') == True
        assert runtime.eval('py.ret_int()') == 42
        assert abs(runtime.eval('py.ret_float()') - 3.14) < 0.001
        assert runtime.eval('py.ret_str()') == "hello"
        assert runtime.eval('py.ret_list()') == [1, "two", 3]
        r = runtime.eval('py.ret_dict()')
        assert r["key"] == "value" and r["num"] == 123
        print("[TEST_PASS] bindable_return_types")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_return_types: {e}")


def test_bindable_vs_bind():
    """测试 @bindable 和 @bind 注册的函数行为一致"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        # 旧方式
        bridge.bind("oldEcho", lambda args: args)

        # 新方式
        def newEcho(value):
            return value
        _bind_with_wrapper(bridge, newEcho)

        r1 = runtime.eval('py.oldEcho(42)')
        r2 = runtime.eval('py.newEcho(42)')
        assert r1 == r2 == 42

        r1 = runtime.eval('py.oldEcho("test")')
        r2 = runtime.eval('py.newEcho("test")')
        assert r1 == r2 == "test"

        print("[TEST_PASS] bindable_vs_bind")
    except Exception as e:
        print(f"[TEST_FAIL] bindable_vs_bind: {e}")


# ========== 模块导入结构测试 ==========

def test_import_mbink():
    """测试 import mbink 导出结构"""
    try:
        import mbink as ui
        assert hasattr(ui, 'App'),       "mbink.App 未导出"
        assert hasattr(ui, 'version'),   "mbink.version 未导出"
        assert hasattr(ui, 'SharedState'), "mbink.SharedState 未导出"
        assert callable(ui.App),         "mbink.App 不可调用"
        assert callable(ui.version),     "mbink.version 不可调用"
        print("[TEST_PASS] import_mbink")
    except Exception as e:
        print(f"[TEST_FAIL] import_mbink: {e}")


def test_no_pybind11_core_module():
    """确认不再暴露历史 pybind11 模块 mbink_core"""
    import importlib

    try:
        importlib.import_module('mbink_core')
        print("[TEST_FAIL] mbink_core_should_not_exist")
    except ModuleNotFoundError:
        print("[TEST_PASS] mbink_core_removed")


def test_backward_compat():
    """测试兼容层 mbink.App 仍可正常导入"""
    try:
        from mbink import App
        assert callable(App), "mbink.App 不可调用"
        print("[TEST_PASS] backward_compat")
    except Exception as e:
        print(f"[TEST_FAIL] backward_compat: {e}")


def test_int_state_decrement_via_increment():
    """测试 IntState decrement 通过 increment(-n) 实现"""
    try:
        app = lui.App()
        counter = app.state("dec_test", 10)
        app.process_queue()

        counter.increment(-1)
        app.process_queue()
        assert counter.get() == 9, f"Expected 9, got {counter.get()}"

        counter.increment(-3)
        app.process_queue()
        assert counter.get() == 6, f"Expected 6, got {counter.get()}"

        print("[TEST_PASS] int_state_decrement_via_increment")
    except Exception as e:
        print(f"[TEST_FAIL] int_state_decrement_via_increment: {e}")


def main():
    print("[TEST_START] MBink 新 API 测试")
    test_bindable_no_args()
    test_bindable_single_arg()
    test_bindable_multi_args_dict()
    test_bindable_multi_args_list()
    test_bindable_exception()
    test_bindable_fallback()
    test_bindable_return_types()
    test_bindable_vs_bind()
    test_import_mbink()
    test_no_pybind11_core_module()
    test_backward_compat()
    test_int_state_decrement_via_increment()
    print("[TEST_END]")


if __name__ == "__main__":
    main()
