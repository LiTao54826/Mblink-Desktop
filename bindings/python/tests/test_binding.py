"""
LightUI Python Binding Tests

测试 Python 绑定的基本功能。
"""

import sys
import os

# 添加构建目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../build/python/Release'))

import lightui_core as lui

def test_version():
    """测试版本号"""
    version = lui.version()
    assert version == "0.3.0", f"Expected 0.3.0, got {version}"
    print("[TEST_PASS] version")

def test_app_creation():
    """测试 App 创建"""
    app = lui.App("Test App", 640, 480)
    assert app is not None
    print("[TEST_PASS] app_creation")

def test_int_state():
    """测试 IntState"""
    app = lui.App()
    counter = app.state("counter", 0)
    
    # 类型检查
    assert type(counter).__name__ == "IntState", f"Expected IntState, got {type(counter).__name__}"
    
    # 初始值
    assert counter.get() == 0, f"Expected 0, got {counter.get()}"
    
    # set
    counter.set(10)
    assert counter.get() == 10, f"Expected 10, got {counter.get()}"
    
    # increment
    counter.increment()
    assert counter.get() == 11, f"Expected 11, got {counter.get()}"
    
    counter.increment(5)
    assert counter.get() == 16, f"Expected 16, got {counter.get()}"
    
    # multiply
    counter.multiply(2)
    assert counter.get() == 32, f"Expected 32, got {counter.get()}"
    
    print("[TEST_PASS] int_state")

def test_string_state():
    """测试 StringState"""
    app = lui.App()
    name = app.state("name", "Hello")
    
    # 类型检查
    assert type(name).__name__ == "StringState", f"Expected StringState, got {type(name).__name__}"
    
    # 初始值
    assert name.get() == "Hello", f"Expected 'Hello', got '{name.get()}'"
    
    # append
    name.append(" World")
    assert name.get() == "Hello World", f"Expected 'Hello World', got '{name.get()}'"
    
    # prepend
    name.prepend(">>> ")
    assert name.get() == ">>> Hello World", f"Expected '>>> Hello World', got '{name.get()}'"
    
    # len
    assert len(name) == 15, f"Expected 15, got {len(name)}"
    
    print("[TEST_PASS] string_state")

def test_list_state():
    """测试 ListState"""
    app = lui.App()
    items = app.state("items", [1, 2, 3])
    
    # 类型检查
    assert type(items).__name__ == "ListState", f"Expected ListState, got {type(items).__name__}"
    
    # 初始值
    assert items.get() == [1, 2, 3], f"Expected [1, 2, 3], got {items.get()}"
    
    # append
    items.append(4)
    assert items.get() == [1, 2, 3, 4], f"Expected [1, 2, 3, 4], got {items.get()}"
    
    # __getitem__
    assert items[0] == 1, f"Expected 1, got {items[0]}"
    assert items[3] == 4, f"Expected 4, got {items[3]}"
    
    # __setitem__
    items[0] = 10
    assert items[0] == 10, f"Expected 10, got {items[0]}"
    
    # __len__
    assert len(items) == 4, f"Expected 4, got {len(items)}"
    
    # pop
    items.pop()
    assert items.get() == [10, 2, 3], f"Expected [10, 2, 3], got {items.get()}"
    
    # remove
    items.remove(1)  # 移除索引 1 的元素
    assert items.get() == [10, 3], f"Expected [10, 3], got {items.get()}"
    
    # clear
    items.clear()
    assert items.get() == [], f"Expected [], got {items.get()}"
    
    print("[TEST_PASS] list_state")

def test_dict_state():
    """测试 DictState"""
    app = lui.App()
    config = app.state("config", {"a": 1, "b": 2})
    
    # 类型检查
    assert type(config).__name__ == "DictState", f"Expected DictState, got {type(config).__name__}"
    
    # 初始值
    assert config.get() == {"a": 1, "b": 2}, f"Expected {{'a': 1, 'b': 2}}, got {config.get()}"
    
    # __getitem__
    assert config["a"] == 1, f"Expected 1, got {config['a']}"
    
    # __setitem__
    config["c"] = 3
    assert config["c"] == 3, f"Expected 3, got {config['c']}"
    
    # __contains__
    assert "a" in config, "Expected 'a' in config"
    assert "z" not in config, "Expected 'z' not in config"
    
    # set_key
    config.set_key("d", 4)
    assert config["d"] == 4, f"Expected 4, got {config['d']}"
    
    # keys
    keys = config.keys()
    assert set(keys) == {"a", "b", "c", "d"}, f"Expected {{'a', 'b', 'c', 'd'}}, got {set(keys)}"
    
    # remove_key
    config.remove_key("a")
    assert "a" not in config, "Expected 'a' not in config after remove"
    
    # clear
    config.clear()
    assert config.get() == {}, f"Expected {{}}, got {config.get()}"
    
    print("[TEST_PASS] dict_state")

def test_state_proxy_identity():
    """测试状态代理身份（同名状态返回同一对象）"""
    app = lui.App()
    
    counter1 = app.state("counter", 0)
    counter2 = app.state("counter", 100)  # 初始值应被忽略
    
    # 应该是同一个对象
    assert counter1 is counter2, "Expected same object for same state name"
    
    # 值应该是第一次创建时的值
    assert counter1.get() == 0, f"Expected 0, got {counter1.get()}"
    
    print("[TEST_PASS] state_proxy_identity")

def test_batch_operations():
    """测试批量操作"""
    app = lui.App()
    counter = app.state("counter", 0)
    
    # 使用 BatchContext
    with lui.BatchContext(app):
        counter.increment()
        counter.increment()
        counter.increment()
    
    assert counter.get() == 3, f"Expected 3, got {counter.get()}"
    
    # 使用 batch_begin/batch_end
    app.batch_begin()
    counter.increment()
    counter.increment()
    app.batch_end()
    
    assert counter.get() == 5, f"Expected 5, got {counter.get()}"
    
    print("[TEST_PASS] batch_operations")

def test_watch():
    """测试状态监听"""
    app = lui.App()
    counter = app.state("counter", 0)
    
    changes = []
    
    def on_change(value):
        changes.append(value)
    
    watch_id = counter.watch(on_change)
    
    counter.increment()
    counter.increment()
    
    # 应该收到两次通知
    assert len(changes) == 2, f"Expected 2 changes, got {len(changes)}"
    assert changes == [1, 2], f"Expected [1, 2], got {changes}"
    
    # 取消监听
    counter.unwatch(watch_id)
    counter.increment()
    
    # 不应该再收到通知
    assert len(changes) == 2, f"Expected 2 changes after unwatch, got {len(changes)}"
    
    print("[TEST_PASS] watch")

def test_type_inference():
    """测试类型推断"""
    app = lui.App()
    
    # None -> State
    null_state = app.state("null", None)
    assert type(null_state).__name__ == "State", f"Expected State for None, got {type(null_state).__name__}"
    
    # bool -> State (基类)
    bool_state = app.state("bool", True)
    assert type(bool_state).__name__ == "State", f"Expected State for bool, got {type(bool_state).__name__}"
    
    # int -> IntState
    int_state = app.state("int", 42)
    assert type(int_state).__name__ == "IntState", f"Expected IntState for int, got {type(int_state).__name__}"
    
    # float -> State (基类)
    float_state = app.state("float", 3.14)
    assert type(float_state).__name__ == "State", f"Expected State for float, got {type(float_state).__name__}"
    
    # str -> StringState
    str_state = app.state("str", "hello")
    assert type(str_state).__name__ == "StringState", f"Expected StringState for str, got {type(str_state).__name__}"
    
    # list -> ListState
    list_state = app.state("list", [1, 2, 3])
    assert type(list_state).__name__ == "ListState", f"Expected ListState for list, got {type(list_state).__name__}"
    
    # dict -> DictState
    dict_state = app.state("dict", {"a": 1})
    assert type(dict_state).__name__ == "DictState", f"Expected DictState for dict, got {type(dict_state).__name__}"
    
    print("[TEST_PASS] type_inference")

def main():
    print("[TEST_START] LightUI Python Binding Tests")
    
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
    
    print("[TEST_END]")

if __name__ == "__main__":
    main()
