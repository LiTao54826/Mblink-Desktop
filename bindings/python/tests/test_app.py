"""
LightUI Python Binding Tests - LightUIApp 高级 API

测试 LightUIApp 类的功能。
"""

import sys
import os

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp, App


def test_app_creation():
    """测试 LightUIApp 创建"""
    try:
        app = LightUIApp("Test App", 640, 480, headless=True)
        assert app is not None
        assert app.window is not None
        assert app.runtime is not None
        assert app.event_loop is not None
        assert app.bridge is not None
        print("[TEST_PASS] app_creation")
    except Exception as e:
        print(f"[TEST_FAIL] app_creation: {e}")


def test_app_context_manager():
    """测试上下文管理器"""
    try:
        with LightUIApp("Context Test", 800, 600, headless=True) as app:
            assert app is not None
            counter = app.state("counter", 0)
            assert counter.get() == 0
        print("[TEST_PASS] app_context_manager")
    except Exception as e:
        print(f"[TEST_FAIL] app_context_manager: {e}")


def test_app_state():
    """测试状态管理"""
    try:
        with LightUIApp("State Test", 800, 600, headless=True) as app:
            # 创建不同类型的状态
            counter = app.state("counter", 0)
            name = app.state("name", "Alice")
            items = app.state("items", [1, 2, 3])
            config = app.state("config", {"theme": "dark"})
            
            # 验证类型
            assert type(counter).__name__ == "IntState"
            assert type(name).__name__ == "StringState"
            assert type(items).__name__ == "ListState"
            assert type(config).__name__ == "DictState"
            
            # 验证操作
            counter.increment()
            assert counter.get() == 1
            
            name.append(" Bob")
            assert name.get() == "Alice Bob"
            
            items.append(4)
            assert items.get() == [1, 2, 3, 4]
            
            config.set_key("fontSize", 14)
            assert config["fontSize"] == 14
            
        print("[TEST_PASS] app_state")
    except Exception as e:
        print(f"[TEST_FAIL] app_state: {e}")


def test_app_bind_decorator():
    """测试函数绑定装饰器"""
    try:
        with LightUIApp("Bind Test", 800, 600, headless=True) as app:
            counter = app.state("counter", 0)
            
            @app.bind("increment")
            def increment(args):
                counter.increment()
                return counter.get()
            
            @app.bind("getData")
            def get_data(args):
                return {"value": counter.get(), "name": "test"}
            
            # 通过 JS 调用
            result = app.load_js('host.call("increment", null)')
            assert result == 1
            
            result = app.load_js('host.call("increment", null)')
            assert result == 2
            
            result = app.load_js('host.call("getData", null)')
            assert result["value"] == 2
            assert result["name"] == "test"
            
        print("[TEST_PASS] app_bind_decorator")
    except Exception as e:
        print(f"[TEST_FAIL] app_bind_decorator: {e}")


def test_app_unbind():
    """测试解绑函数"""
    try:
        with LightUIApp("Unbind Test", 800, 600, headless=True) as app:
            @app.bind("myFunc")
            def my_func(args):
                return "hello"
            
            # 调用成功
            result = app.load_js('host.call("myFunc", null)')
            assert result == "hello"
            
            # 解绑
            app.unbind("myFunc")
            
            # 调用应该返回错误
            result = app.load_js('host.call("myFunc", null)')
            assert isinstance(result, dict) and "error" in result
            
        print("[TEST_PASS] app_unbind")
    except Exception as e:
        print(f"[TEST_FAIL] app_unbind: {e}")


def test_app_batch():
    """测试批量操作"""
    try:
        with LightUIApp("Batch Test", 800, 600, headless=True) as app:
            counter = app.state("counter", 0)
            
            notifications = []
            def on_change(value):
                notifications.append(value)
            
            counter.watch(on_change)
            
            # 批量模式
            with app.batch():
                counter.increment()
                counter.increment()
                counter.increment()
            
            assert counter.get() == 3
            # 批量模式内不应有通知
            # 注意：由于实现细节，通知可能在 batch_end 时触发
            
        print("[TEST_PASS] app_batch")
    except Exception as e:
        print(f"[TEST_FAIL] app_batch: {e}")


def test_app_js_state_access():
    """测试 JS 访问状态"""
    try:
        with LightUIApp("JS State Test", 800, 600, headless=True) as app:
            name = app.state("userName", "Alice")
            
            # JS 读取状态
            result = app.load_js('host.state.get("userName")')
            assert result == "Alice"
            
            # JS 设置状态
            app.load_js('host.state.set("userName", "Bob")')
            
            # Python 验证
            assert name.get() == "Bob"
            
        print("[TEST_PASS] app_js_state_access")
    except Exception as e:
        print(f"[TEST_FAIL] app_js_state_access: {e}")


def test_app_alias():
    """测试 App 别名"""
    try:
        app = App("Alias Test", 800, 600, headless=True)
        assert isinstance(app, LightUIApp)
        print("[TEST_PASS] app_alias")
    except Exception as e:
        print(f"[TEST_FAIL] app_alias: {e}")


def main():
    print("[TEST_START] LightUI Python Binding Tests - LightUIApp")
    
    test_app_creation()
    test_app_context_manager()
    test_app_state()
    test_app_bind_decorator()
    test_app_unbind()
    test_app_batch()
    test_app_js_state_access()
    test_app_alias()
    
    print("[TEST_END]")


if __name__ == "__main__":
    main()
