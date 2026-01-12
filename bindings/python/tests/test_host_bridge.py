"""
LightUI Python Binding Tests - HostBridge

测试 Python ↔ JS 双向通信功能。
"""

import sys
import os

# 添加构建目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../build/python/Release'))

import lightui_core as lui


def test_host_bridge_creation():
    """测试 HostBridge 创建"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        assert bridge is not None
        assert bridge.is_valid()
        print("[TEST_PASS] host_bridge_creation")
    except Exception as e:
        print(f"[TEST_FAIL] host_bridge_creation: {e}")


def test_bind_simple_function():
    """测试绑定简单函数"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        # 绑定一个简单函数
        def add_numbers(args):
            if args is None:
                return 0
            if isinstance(args, dict):
                return args.get('a', 0) + args.get('b', 0)
            return 0
        
        bridge.bind("addNumbers", add_numbers)
        
        # 通过 JS 调用
        result = runtime.eval('host.call("addNumbers", {a: 10, b: 20})')
        assert result == 30, f"Expected 30, got {result}"
        
        print("[TEST_PASS] bind_simple_function")
    except Exception as e:
        print(f"[TEST_FAIL] bind_simple_function: {e}")


def test_bind_return_object():
    """测试绑定返回对象的函数"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        def get_user(args):
            return {
                "name": "Alice",
                "age": 30,
                "active": True
            }
        
        bridge.bind("getUser", get_user)
        
        # 通过 JS 调用
        result = runtime.eval('host.call("getUser", null)')
        assert result["name"] == "Alice"
        assert result["age"] == 30
        assert result["active"] == True
        
        print("[TEST_PASS] bind_return_object")
    except Exception as e:
        print(f"[TEST_FAIL] bind_return_object: {e}")


def test_bind_return_array():
    """测试绑定返回数组的函数"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        def get_items(args):
            return [1, 2, 3, "four", {"five": 5}]
        
        bridge.bind("getItems", get_items)
        
        result = runtime.eval('host.call("getItems", null)')
        assert result == [1, 2, 3, "four", {"five": 5}]
        
        print("[TEST_PASS] bind_return_array")
    except Exception as e:
        print(f"[TEST_FAIL] bind_return_array: {e}")


def test_unbind_function():
    """测试解绑函数"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        def my_func(args):
            return "hello"
        
        bridge.bind("myFunc", my_func)
        
        # 调用成功
        result = runtime.eval('host.call("myFunc", null)')
        assert result == "hello"
        
        # 解绑
        bridge.unbind("myFunc")
        
        # 调用应该返回错误
        result = runtime.eval('host.call("myFunc", null)')
        assert isinstance(result, dict) and "error" in result
        
        print("[TEST_PASS] unbind_function")
    except Exception as e:
        print(f"[TEST_FAIL] unbind_function: {e}")


def test_python_exception_handling():
    """测试 Python 异常处理"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        def raise_error(args):
            raise ValueError("Test error from Python")
        
        bridge.bind("raiseError", raise_error)
        
        # 调用应该返回错误而不是崩溃
        result = runtime.eval('host.call("raiseError", null)')
        assert isinstance(result, dict)
        assert "error" in result
        
        print("[TEST_PASS] python_exception_handling")
    except Exception as e:
        print(f"[TEST_FAIL] python_exception_handling: {e}")


def test_state_integration():
    """测试 HostBridge 与 State 集成"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        # 创建状态
        counter = app.state("counter", 0)
        
        # 绑定函数来修改状态
        def increment_counter(args):
            current = counter.get()
            counter.set(current + 1)
            return counter.get()
        
        bridge.bind("incrementCounter", increment_counter)
        
        # 通过 JS 调用
        result = runtime.eval('host.call("incrementCounter", null)')
        assert result == 1
        
        result = runtime.eval('host.call("incrementCounter", null)')
        assert result == 2
        
        # 验证 Python 端也能看到变化
        assert counter.get() == 2
        
        print("[TEST_PASS] state_integration")
    except Exception as e:
        print(f"[TEST_FAIL] state_integration: {e}")


def test_js_state_access():
    """测试 JS 通过 host.state 访问状态"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        # 创建状态
        name = app.state("userName", "Alice")
        
        # JS 读取状态
        result = runtime.eval('host.state.get("userName")')
        assert result == "Alice", f"Expected 'Alice', got {result}"
        
        # JS 设置状态
        runtime.eval('host.state.set("userName", "Bob")')
        
        # Python 验证
        assert name.get() == "Bob", f"Expected 'Bob', got {name.get()}"
        
        print("[TEST_PASS] js_state_access")
    except Exception as e:
        print(f"[TEST_FAIL] js_state_access: {e}")


def test_complex_data_roundtrip():
    """测试复杂数据往返"""
    try:
        app = lui.App()
        runtime = lui.Runtime()
        bridge = lui.HostBridge(runtime, app)
        
        def process_data(args):
            # 接收复杂数据，处理后返回
            if args is None:
                return None
            
            result = {
                "received": args,
                "processed": True,
                "count": len(args) if isinstance(args, (list, dict)) else 1
            }
            return result
        
        bridge.bind("processData", process_data)
        
        # 发送复杂数据
        result = runtime.eval('''
            host.call("processData", {
                users: [
                    {name: "Alice", age: 30},
                    {name: "Bob", age: 25}
                ],
                metadata: {
                    version: "1.0",
                    timestamp: 12345
                }
            })
        ''')
        
        assert result["processed"] == True
        assert result["count"] == 2  # dict has 2 keys
        assert "received" in result
        
        print("[TEST_PASS] complex_data_roundtrip")
    except Exception as e:
        print(f"[TEST_FAIL] complex_data_roundtrip: {e}")


def main():
    print("[TEST_START] LightUI Python Binding Tests - HostBridge")
    
    test_host_bridge_creation()
    test_bind_simple_function()
    test_bind_return_object()
    test_bind_return_array()
    test_unbind_function()
    test_python_exception_handling()
    test_state_integration()
    test_js_state_access()
    test_complex_data_roundtrip()
    
    print("[TEST_END]")


if __name__ == "__main__":
    main()
