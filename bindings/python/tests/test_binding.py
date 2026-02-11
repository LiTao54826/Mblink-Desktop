"""
LightUI Python 绑定基础测试

测试低级 API（lightui_core）的基本功能。
注意：状态操作后需调用 app.process_queue() 才能生效。
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui.core import version as lui_version
import lightui_core as lui


def test_version():
    """测试版本号"""
    try:
        v = lui_version()
        assert isinstance(v, str) and len(v) > 0
        print(f"[TEST_PASS] version ({v})")
    except Exception as e:
        print(f"[TEST_FAIL] version: {e}")


def test_app_creation():
    """测试 App 创建"""
    try:
        app = lui.App("Test", 640, 480)
        assert app is not None
        print("[TEST_PASS] app_creation")
    except Exception as e:
        print(f"[TEST_FAIL] app_creation: {e}")


def test_int_state():
    """测试 IntState"""
    try:
        app = lui.App()
        counter = app.state("counter", 0)
        assert type(counter).__name__ == "IntState"
        assert counter.get() == 0

        counter.set(10)
        app.process_queue()
        assert counter.get() == 10, f"Expected 10, got {counter.get()}"

        counter.increment()
        app.process_queue()
        assert counter.get() == 11

        counter.increment(5)
        app.process_queue()
        assert counter.get() == 16

        counter.multiply(2)
        app.process_queue()
        assert counter.get() == 32

        print("[TEST_PASS] int_state")
    except Exception as e:
        print(f"[TEST_FAIL] int_state: {e}")


def test_string_state():
    """测试 StringState"""
    try:
        app = lui.App()
        name = app.state("name", "Hello")
        app.process_queue()
        assert type(name).__name__ == "StringState"
        assert name.get() == "Hello"

        name.append(" World")
        app.process_queue()
        assert name.get() == "Hello World"

        name.prepend(">>> ")
        app.process_queue()
        assert name.get() == ">>> Hello World"

        assert len(name) == 15
        print("[TEST_PASS] string_state")
    except Exception as e:
        print(f"[TEST_FAIL] string_state: {e}")


def test_list_state():
    """测试 ListState"""
    try:
        app = lui.App()
        items = app.state("items", [1, 2, 3])
        app.process_queue()
        assert type(items).__name__ == "ListState"
        assert items.get() == [1, 2, 3]

        items.append(4)
        app.process_queue()
        assert items.get() == [1, 2, 3, 4]
        assert items[0] == 1
        assert len(items) == 4

        items.pop()
        app.process_queue()
        assert items.get() == [1, 2, 3]

        items.clear()
        app.process_queue()
        assert items.get() == []

        print("[TEST_PASS] list_state")
    except Exception as e:
        print(f"[TEST_FAIL] list_state: {e}")


def test_dict_state():
    """测试 DictState"""
    try:
        app = lui.App()
        config = app.state("config", {"a": 1, "b": 2})
        app.process_queue()
        assert type(config).__name__ == "DictState"
        assert config.get() == {"a": 1, "b": 2}
        assert config["a"] == 1
        assert "a" in config
        assert "z" not in config

        config.set_key("c", 3)
        app.process_queue()
        assert config["c"] == 3

        config.remove_key("a")
        app.process_queue()
        assert "a" not in config

        config.clear()
        app.process_queue()
        assert config.get() == {}

        print("[TEST_PASS] dict_state")
    except Exception as e:
        print(f"[TEST_FAIL] dict_state: {e}")


def test_state_proxy_identity():
    """测试同名状态返回同一对象"""
    try:
        app = lui.App()
        c1 = app.state("counter", 0)
        c2 = app.state("counter", 100)
        assert c1 is c2
        assert c1.get() == 0
        print("[TEST_PASS] state_proxy_identity")
    except Exception as e:
        print(f"[TEST_FAIL] state_proxy_identity: {e}")


def test_batch_operations():
    """测试批量操作"""
    try:
        app = lui.App()
        counter = app.state("counter", 0)
        with lui.BatchContext(app):
            counter.increment()
            counter.increment()
            counter.increment()
        app.process_queue()
        assert counter.get() == 3, f"Expected 3, got {counter.get()}"
        print("[TEST_PASS] batch_operations")
    except Exception as e:
        print(f"[TEST_FAIL] batch_operations: {e}")


def test_watch():
    """测试状态监听"""
    try:
        app = lui.App()
        counter = app.state("counter", 0)
        changes = []

        def on_change(value):
            changes.append(value)

        watch_id = counter.watch(on_change)

        counter.increment()
        app.process_queue()
        counter.increment()
        app.process_queue()

        assert len(changes) == 2, f"Expected 2, got {len(changes)}"
        assert changes == [1, 2], f"Expected [1, 2], got {changes}"

        counter.unwatch(watch_id)
        counter.increment()
        app.process_queue()
        assert len(changes) == 2
        print("[TEST_PASS] watch")
    except Exception as e:
        print(f"[TEST_FAIL] watch: {e}")


def test_type_inference():
    """测试类型推断"""
    try:
        app = lui.App()
        assert type(app.state("n", None)).__name__ == "State"
        assert type(app.state("b", True)).__name__ == "State"
        assert type(app.state("i", 42)).__name__ == "IntState"
        assert type(app.state("f", 3.14)).__name__ == "State"
        assert type(app.state("s", "hi")).__name__ == "StringState"
        assert type(app.state("l", [1])).__name__ == "ListState"
        assert type(app.state("d", {"a": 1})).__name__ == "DictState"
        print("[TEST_PASS] type_inference")
    except Exception as e:
        print(f"[TEST_FAIL] type_inference: {e}")


def test_host_bridge_bind():
    """测试 HostBridge 绑定和 py.funcName() 调用"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        def add(args):
            if isinstance(args, dict):
                return args.get('a', 0) + args.get('b', 0)
            return 0

        bridge.bind("add", add)
        result = runtime.eval('py.add({a: 10, b: 20})')
        assert result == 30, f"Expected 30, got {result}"
        print("[TEST_PASS] host_bridge_bind")
    except Exception as e:
        print(f"[TEST_FAIL] host_bridge_bind: {e}")


def test_host_bridge_unbind():
    """测试 HostBridge 解绑"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        bridge.bind("myFunc", lambda args: "hello")
        result = runtime.eval('py.myFunc(null)')
        assert result == "hello"

        bridge.unbind("myFunc")
        # 解绑后 py.myFunc 不存在，JS 会抛异常，用 try/catch 捕获
        result = runtime.eval('''
            (function() {
                try { return py.myFunc(null); }
                catch(e) { return {error: e.message}; }
            })()
        ''')
        assert isinstance(result, dict) and "error" in result
        print("[TEST_PASS] host_bridge_unbind")
    except Exception as e:
        print(f"[TEST_FAIL] host_bridge_unbind: {e}")


def test_js_state_access():
    """测试 JS 通过 host.state 访问状态"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)

        name = app.state("userName", "Alice")
        app.process_queue()
        result = runtime.eval('host.state.get("userName")')
        assert result == "Alice"

        runtime.eval('host.state.set("userName", "Bob")')
        app.process_queue()
        assert name.get() == "Bob"
        print("[TEST_PASS] js_state_access")
    except Exception as e:
        print(f"[TEST_FAIL] js_state_access: {e}")


def main():
    print("[TEST_START] LightUI 基础绑定测试")
    test_version()
    test_app_creation()
    test_int_state()
    test_string_state()
    test_list_state()
    test_dict_state()
    test_state_proxy_identity()
    test_batch_operations()
    test_watch()
    test_type_inference()
    test_host_bridge_bind()
    test_host_bridge_unbind()
    test_js_state_access()
    print("[TEST_END]")


if __name__ == "__main__":
    main()
