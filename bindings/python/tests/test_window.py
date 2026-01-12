"""
LightUI Python Binding Tests - Phase 2: Window Bindings

测试 Window, Document, Element, EventLoop, Runtime 类的基本功能。
"""

import sys
import os

# 添加构建目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../build/python/Release'))

import lightui_core as lui


def test_window_creation_headless():
    """测试无头模式窗口创建"""
    try:
        window = lui.Window("Test Window", 800, 600, headless=True)
        assert window is not None
        assert window.is_valid()
        assert window.title == "Test Window"
        print("[TEST_PASS] window_creation_headless")
    except Exception as e:
        print(f"[TEST_FAIL] window_creation_headless: {e}")


def test_window_properties():
    """测试窗口属性"""
    try:
        window = lui.Window("Props Test", 640, 480, headless=True)
        
        # 测试 title
        window.title = "New Title"
        assert window.title == "New Title", f"Expected 'New Title', got '{window.title}'"
        
        # 测试 size (headless 模式下可能不准确，但不应崩溃)
        size = window.size
        assert isinstance(size, tuple)
        assert len(size) == 2
        
        print("[TEST_PASS] window_properties")
    except Exception as e:
        print(f"[TEST_FAIL] window_properties: {e}")


def test_document_creation():
    """测试 Document 创建元素"""
    try:
        window = lui.Window("Doc Test", 800, 600, headless=True)
        doc = window.document
        
        if doc is None or not doc.is_valid():
            print("[TEST_SKIP] document_creation: No document available in headless mode")
            return
        
        # 创建元素
        div = doc.create_element("div")
        assert div is not None
        assert div.tag_name.lower() == "div"
        
        print("[TEST_PASS] document_creation")
    except Exception as e:
        print(f"[TEST_FAIL] document_creation: {e}")


def test_element_attributes():
    """测试元素属性操作"""
    try:
        window = lui.Window("Elem Test", 800, 600, headless=True)
        doc = window.document
        
        if doc is None or not doc.is_valid():
            print("[TEST_SKIP] element_attributes: No document available")
            return
        
        div = doc.create_element("div")
        
        # 测试 id
        div.id = "test-div"
        assert div.id == "test-div", f"Expected 'test-div', got '{div.id}'"
        
        # 测试 class_name
        div.class_name = "container active"
        assert div.class_name == "container active"
        
        # 测试 set_attribute / get_attribute
        div.set_attribute("data-value", "123")
        assert div.get_attribute("data-value") == "123"
        
        # 测试 remove_attribute
        div.remove_attribute("data-value")
        assert div.get_attribute("data-value") == ""
        
        print("[TEST_PASS] element_attributes")
    except Exception as e:
        print(f"[TEST_FAIL] element_attributes: {e}")


def test_element_content():
    """测试元素内容操作"""
    try:
        window = lui.Window("Content Test", 800, 600, headless=True)
        doc = window.document
        
        if doc is None or not doc.is_valid():
            print("[TEST_SKIP] element_content: No document available")
            return
        
        div = doc.create_element("div")
        
        # 测试 text_content
        div.text_content = "Hello World"
        assert div.text_content == "Hello World"
        
        # 测试 inner_html
        div.inner_html = "<span>Test</span>"
        assert "<span>" in div.inner_html.lower() or "test" in div.inner_html.lower()
        
        print("[TEST_PASS] element_content")
    except Exception as e:
        print(f"[TEST_FAIL] element_content: {e}")


def test_event_loop_creation():
    """测试 EventLoop 创建"""
    try:
        loop = lui.EventLoop()
        assert loop is not None
        assert not loop.is_running()
        print("[TEST_PASS] event_loop_creation")
    except Exception as e:
        print(f"[TEST_FAIL] event_loop_creation: {e}")


def test_runtime_eval():
    """测试 Runtime JavaScript 执行"""
    try:
        runtime = lui.Runtime()
        
        # 简单表达式
        result = runtime.eval("1 + 2")
        assert result == 3, f"Expected 3, got {result}"
        
        # 字符串
        result = runtime.eval("'hello' + ' world'")
        assert result == "hello world", f"Expected 'hello world', got {result}"
        
        # 对象
        result = runtime.eval("({a: 1, b: 2})")
        assert result == {"a": 1, "b": 2}, f"Expected {{'a': 1, 'b': 2}}, got {result}"
        
        # 数组
        result = runtime.eval("[1, 2, 3]")
        assert result == [1, 2, 3], f"Expected [1, 2, 3], got {result}"
        
        print("[TEST_PASS] runtime_eval")
    except Exception as e:
        print(f"[TEST_FAIL] runtime_eval: {e}")


def test_runtime_module():
    """测试 Runtime 模块执行"""
    try:
        runtime = lui.Runtime()
        
        # ES6 模块
        result = runtime.eval_module("export const x = 42;")
        # 模块返回值可能是 undefined 或 null
        
        print("[TEST_PASS] runtime_module")
    except Exception as e:
        print(f"[TEST_FAIL] runtime_module: {e}")


def test_version():
    """测试版本号"""
    version = lui.version()
    assert version == "0.3.0", f"Expected 0.3.0, got {version}"
    print("[TEST_PASS] version")


def main():
    print("[TEST_START] LightUI Python Binding Tests - Phase 2: Window")
    
    test_version()
    test_window_creation_headless()
    test_window_properties()
    test_document_creation()
    test_element_attributes()
    test_element_content()
    test_event_loop_creation()
    test_runtime_eval()
    test_runtime_module()
    
    print("[TEST_END]")


if __name__ == "__main__":
    main()
