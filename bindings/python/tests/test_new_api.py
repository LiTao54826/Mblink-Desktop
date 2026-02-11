"""
LightUI 新 API 测试

测试 @app.bindable 装饰器、参数解包、模块导入结构、向后兼容等。
使用低级 API + _make_wrapper 避免创建 Window。
"""

import sys
import os
import inspect

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui.app import _make_wrapper
import lightui_core as lui


def _bind_with_wrapper(bridge, func):
    """模拟 @app.bindable 的注册逻辑"""
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
    wrapper = _make_wrapper(func, params)
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

def test_import_lightui():
    """测试 import lightui 导出结构"""
    try:
        import lightui as ui
        assert hasattr(ui, 'App')
        assert hasattr(ui, 'version')
        assert hasattr(ui, 'LightUIApp')
        assert ui.LightUIApp is ui.App
        print("[TEST_PASS] import_lightui")
    except Exception as e:
        print(f"[TEST_FAIL] import_lightui: {e}")


def test_import_core():
    """测试 from lightui.core import 低级 API"""
    try:
        from lightui.core import Window, Runtime, HostBridge, CoreApp
        from lightui.core import State, IntState, StringState, ListState, DictState
        from lightui.core import version, BatchContext
        assert callable(version)
        print("[TEST_PASS] import_core")
    except Exception as e:
        print(f"[TEST_FAIL] import_core: {e}")


def test_backward_compat():
    """测试向后兼容"""
    try:
        from lightui import LightUIApp, App
        assert LightUIApp is App
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
    print("[TEST_START] LightUI 新 API 测试")
    test_bindable_no_args()
    test_bindable_single_arg()
    test_bindable_multi_args_dict()
    test_bindable_multi_args_list()
    test_bindable_exception()
    test_bindable_fallback()
    test_bindable_return_types()
    test_bindable_vs_bind()
    test_import_lightui()
    test_import_core()
    test_backward_compat()
    test_int_state_decrement_via_increment()
    print("[TEST_END]")


if __name__ == "__main__":
    main()
