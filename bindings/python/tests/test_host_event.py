"""
LightUI Host Event System Python 测试

测试 bridge.emit() / app.emit() API。
使用低级 API 测试，不创建 Window。
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'lightui', 'bin'))

import lightui_core as lui


print("[TEST_START] Host Event System Python Tests")

# 使用单个 Runtime/App/Bridge 实例避免资源冲突
app = lui.App()
runtime = lui.Runtime()
bridge = lui.HostBridge(runtime, app)


# --- 1. Python emit → JS callback 收到数据 ---
def test_bridge_emit_basic():
    try:
        lid = runtime.eval('host.on("pyEvt", function(d) { _received = d; });')
        runtime.eval('var _received = null;')
        bridge.emit("pyEvt", 42)
        bridge.flush_events()
        result = runtime.eval('_received')
        # 清理 listener
        runtime.eval('host.off(' + str(int(lid)) + ')')
        assert result == 42, f"Expected 42, got {result}"
        print("[TEST_PASS] bridge_emit_basic")
    except Exception as e:
        print(f"[TEST_FAIL] bridge_emit_basic: {e}")

test_bridge_emit_basic()


# --- 2. 各种数据类型序列化 ---
def test_emit_data_types():
    try:
        lid = runtime.eval('host.on("typed", function(d) { _val = d; });')
        runtime.eval('var _val = null;')

        cases = [
            (None, "null"),
            (True, "bool_true"),
            (False, "bool_false"),
            (123, "int"),
            (3.14, "float"),
            ("hello", "str"),
            ([1, 2, 3], "list"),
            ({"a": 1}, "dict"),
        ]

        all_pass = True
        for data, label in cases:
            bridge.emit("typed", data)
            bridge.flush_events()
            result = runtime.eval('_val')

            if label == "float":
                ok = isinstance(result, float) and abs(result - 3.14) < 0.001
            elif label == "null":
                ok = result is None
            else:
                ok = result == data

            if not ok:
                print(f"[TEST_FAIL] emit_type_{label}: expected {data}, got {result}")
                all_pass = False

        # 清理 listener
        runtime.eval('host.off(' + str(int(lid)) + ')')

        if all_pass:
            print("[TEST_PASS] emit_data_types")
    except Exception as e:
        print(f"[TEST_FAIL] emit_data_types: {e}")

test_emit_data_types()


# --- 3. 空事件名抛出 ValueError ---
def test_emit_empty_name():
    try:
        try:
            bridge.emit("", 1)
            print("[TEST_FAIL] emit_empty_name: no exception raised")
        except ValueError:
            print("[TEST_PASS] emit_empty_name")
    except Exception as e:
        print(f"[TEST_FAIL] emit_empty_name: {e}")

test_emit_empty_name()


# --- 4. 无 listener 时 emit 不报错 ---
def test_emit_no_listener():
    try:
        bridge.emit("nobody_listens", {"x": 1})
        bridge.flush_events()
        print("[TEST_PASS] emit_no_listener")
    except Exception as e:
        print(f"[TEST_FAIL] emit_no_listener: {e}")

test_emit_no_listener()


# --- 5. 多次 emit 按顺序到达 ---
def test_emit_order():
    try:
        lid = runtime.eval('host.on("seq", function(d) { _order.push(d); });')
        runtime.eval('var _order = [];')
        bridge.emit("seq", 1)
        bridge.emit("seq", 2)
        bridge.emit("seq", 3)
        bridge.flush_events()
        result = runtime.eval('_order')
        # 清理 listener
        runtime.eval('host.off(' + str(int(lid)) + ')')
        assert result == [1, 2, 3], f"Expected [1,2,3], got {result}"
        print("[TEST_PASS] emit_order")
    except Exception as e:
        print(f"[TEST_FAIL] emit_order: {e}")

test_emit_order()


print("[TEST_END]")

# 确保销毁顺序：bridge(释放JS回调) → runtime → app
del bridge
del runtime
del app
