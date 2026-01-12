"""
LightUI Python Binding Property Tests

使用 hypothesis 进行属性测试，验证设计文档中定义的正确性属性。
"""

import sys
import os

# 添加构建目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../build/python/Release'))

import lightui_core as lui
from hypothesis import given, settings, assume
from hypothesis import strategies as st

# ============================================================
# 策略定义
# ============================================================

# 限制整数范围在 int64 范围内
safe_integers = st.integers(min_value=-2**62, max_value=2**62)

# JSON 兼容的基本类型
json_primitives = st.one_of(
    st.none(),
    st.booleans(),
    safe_integers,
    st.floats(allow_nan=False, allow_infinity=False, min_value=-1e10, max_value=1e10),
    st.text(max_size=100, alphabet=st.characters(whitelist_categories=('L', 'N', 'P', 'S')))
)

# 简单 JSON 值（不递归）
simple_json = st.one_of(
    json_primitives,
    st.lists(json_primitives, max_size=10),
    st.dictionaries(st.text(min_size=1, max_size=20, alphabet=st.characters(whitelist_categories=('L', 'N'))), json_primitives, max_size=10)
)

# 状态名称
state_names = st.text(
    alphabet=st.characters(whitelist_categories=('L', 'N'), whitelist_characters='_'),
    min_size=1, max_size=20
)


# ============================================================
# Property 2: State Round-Trip Consistency
# ============================================================

@given(value=simple_json)
@settings(max_examples=100)
def test_property_state_round_trip(value):
    """
    Property Test: State Round-Trip Consistency
    
    Feature: python_binding, Property 2: State Round-Trip Consistency
    对于任意 JSON 兼容值 v，state.set(v) 后 state.get() == v
    Validates: Requirements 4.1, 4.2
    """
    app = lui.App()
    state = app.state("test_state", value)
    
    result = state.get()
    
    # 比较值（处理浮点数精度问题）
    if isinstance(value, float):
        assert abs(result - value) < 1e-10, f"Float mismatch: {result} != {value}"
    else:
        assert result == value, f"Round-trip failed: {result} != {value}"


# ============================================================
# Property 5: IntState Atomic Operations
# ============================================================

@given(
    initial=st.integers(min_value=-1000000, max_value=1000000),
    delta=st.integers(min_value=-1000, max_value=1000)
)
@settings(max_examples=100)
def test_property_int_increment(initial, delta):
    """
    Property Test: IntState Increment
    
    Feature: python_binding, Property 5: IntState Atomic Operations
    increment(delta) 后值等于 initial + delta
    Validates: Requirements 5.1, 5.2
    """
    app = lui.App()
    counter = app.state("counter", initial)
    
    counter.increment(delta)
    
    assert counter.get() == initial + delta, \
        f"Increment failed: {initial} + {delta} != {counter.get()}"


@given(
    initial=st.integers(min_value=1, max_value=1000),
    factor=st.floats(min_value=0.1, max_value=10.0, allow_nan=False)
)
@settings(max_examples=100)
def test_property_int_multiply(initial, factor):
    """
    Property Test: IntState Multiply
    
    Feature: python_binding, Property 5: IntState Atomic Operations
    multiply(factor) 后值等于 int(initial * factor)
    Validates: Requirements 5.3
    """
    app = lui.App()
    counter = app.state("counter", initial)
    
    counter.multiply(factor)
    
    expected = int(initial * factor)
    assert counter.get() == expected, \
        f"Multiply failed: int({initial} * {factor}) = {expected} != {counter.get()}"


# ============================================================
# Property 6: ListState Operations Correctness
# ============================================================

@given(
    initial=st.lists(safe_integers, max_size=10),
    item=safe_integers
)
@settings(max_examples=100)
def test_property_list_append(initial, item):
    """
    Property Test: ListState Append
    
    Feature: python_binding, Property 6: ListState Operations Correctness
    append(item) 后列表末尾是 item
    Validates: Requirements 6.1
    """
    app = lui.App()
    items = app.state("items", initial.copy())
    
    items.append(item)
    
    result = items.get()
    assert result[-1] == item, f"Append failed: last item is {result[-1]}, expected {item}"
    assert len(result) == len(initial) + 1, f"Length mismatch after append"


@given(initial=st.lists(safe_integers, min_size=1, max_size=10))
@settings(max_examples=100)
def test_property_list_pop(initial):
    """
    Property Test: ListState Pop
    
    Feature: python_binding, Property 6: ListState Operations Correctness
    pop() 后列表长度减 1
    Validates: Requirements 6.2
    """
    app = lui.App()
    items = app.state("items", initial.copy())
    
    items.pop()
    
    result = items.get()
    assert len(result) == len(initial) - 1, f"Pop failed: length is {len(result)}, expected {len(initial) - 1}"


@given(data=st.data())
@settings(max_examples=100)
def test_property_list_getitem(data):
    """
    Property Test: ListState GetItem
    
    Feature: python_binding, Property 6: ListState Operations Correctness
    __getitem__(i) 返回正确的元素
    Validates: Requirements 6.6
    """
    # 先生成列表，再根据列表长度生成有效索引
    initial = data.draw(st.lists(safe_integers, min_size=1, max_size=10))
    index = data.draw(st.integers(min_value=0, max_value=len(initial)-1))
    
    app = lui.App()
    items = app.state("items", initial.copy())
    
    assert items[index] == initial[index], \
        f"GetItem failed: items[{index}] = {items[index]}, expected {initial[index]}"


# ============================================================
# Property 7: DictState Operations Correctness
# ============================================================

@given(
    initial=st.dictionaries(st.text(min_size=1, max_size=10, alphabet=st.characters(whitelist_categories=('L', 'N'))), safe_integers, max_size=5),
    key=st.text(min_size=1, max_size=10, alphabet=st.characters(whitelist_categories=('L', 'N'))),
    value=safe_integers
)
@settings(max_examples=100)
def test_property_dict_set_key(initial, key, value):
    """
    Property Test: DictState SetKey
    
    Feature: python_binding, Property 7: DictState Operations Correctness
    set_key(k, v) 后 dict[k] == v
    Validates: Requirements 7.1
    """
    app = lui.App()
    config = app.state("config", initial.copy())
    
    config.set_key(key, value)
    
    assert config[key] == value, f"SetKey failed: config[{key}] = {config[key]}, expected {value}"


@given(
    initial=st.dictionaries(st.text(min_size=1, max_size=10, alphabet=st.characters(whitelist_categories=('L', 'N'))), safe_integers, min_size=1, max_size=5)
)
@settings(max_examples=100)
def test_property_dict_remove_key(initial):
    """
    Property Test: DictState RemoveKey
    
    Feature: python_binding, Property 7: DictState Operations Correctness
    remove_key(k) 后 k not in dict
    Validates: Requirements 7.2
    """
    app = lui.App()
    config = app.state("config", initial.copy())
    
    key = list(initial.keys())[0]
    config.remove_key(key)
    
    assert key not in config, f"RemoveKey failed: {key} still in config"


@given(
    initial=st.dictionaries(st.text(min_size=1, max_size=10, alphabet=st.characters(whitelist_categories=('L', 'N'))), safe_integers, max_size=5)
)
@settings(max_examples=100)
def test_property_dict_contains(initial):
    """
    Property Test: DictState Contains
    
    Feature: python_binding, Property 7: DictState Operations Correctness
    __contains__ 正确反映键存在性
    Validates: Requirements 7.6
    """
    app = lui.App()
    config = app.state("config", initial.copy())
    
    for key in initial.keys():
        assert key in config, f"Contains failed: {key} should be in config"
    
    assert "nonexistent_key_12345" not in config, "Contains failed: nonexistent key should not be in config"


# ============================================================
# Property 8: StringState Operations Correctness
# ============================================================

# ASCII 安全的文本策略
safe_text = st.text(max_size=100, alphabet=st.characters(whitelist_categories=('L', 'N', 'P', 'S'), max_codepoint=127))

@given(
    initial=safe_text,
    suffix=st.text(max_size=20, alphabet=st.characters(whitelist_categories=('L', 'N'), max_codepoint=127))
)
@settings(max_examples=100)
def test_property_string_append(initial, suffix):
    """
    Property Test: StringState Append
    
    Feature: python_binding, Property 8: StringState Operations Correctness
    append(s) 后字符串以 s 结尾
    Validates: Requirements 8.2
    """
    app = lui.App()
    text = app.state("text", initial)
    
    text.append(suffix)
    
    result = text.get()
    assert result == initial + suffix, f"Append failed: '{result}' != '{initial + suffix}'"


@given(
    initial=safe_text,
    prefix=st.text(max_size=20, alphabet=st.characters(whitelist_categories=('L', 'N'), max_codepoint=127))
)
@settings(max_examples=100)
def test_property_string_prepend(initial, prefix):
    """
    Property Test: StringState Prepend
    
    Feature: python_binding, Property 8: StringState Operations Correctness
    prepend(s) 后字符串以 s 开头
    Validates: Requirements 8.3
    """
    app = lui.App()
    text = app.state("text", initial)
    
    text.prepend(prefix)
    
    result = text.get()
    assert result == prefix + initial, f"Prepend failed: '{result}' != '{prefix + initial}'"


@given(initial=safe_text)
@settings(max_examples=100)
def test_property_string_len(initial):
    """
    Property Test: StringState Length
    
    Feature: python_binding, Property 8: StringState Operations Correctness
    __len__ 返回正确的字符串长度
    Validates: Requirements 8.4
    """
    app = lui.App()
    text = app.state("text", initial)
    
    assert len(text) == len(initial), f"Length failed: {len(text)} != {len(initial)}"


# ============================================================
# Property 1: Type Inference Correctness
# ============================================================

@given(value=safe_integers)
@settings(max_examples=50)
def test_property_type_inference_int(value):
    """
    Property Test: Type Inference for Int
    
    Feature: python_binding, Property 1: Type Inference Correctness
    int 类型返回 IntState
    Validates: Requirements 3.2
    """
    app = lui.App()
    state = app.state("int_state", value)
    
    assert type(state).__name__ == "IntState", \
        f"Type inference failed: expected IntState, got {type(state).__name__}"


@given(value=st.text(alphabet=st.characters(whitelist_categories=('L', 'N'), max_codepoint=127)))
@settings(max_examples=50)
def test_property_type_inference_string(value):
    """
    Property Test: Type Inference for String
    
    Feature: python_binding, Property 1: Type Inference Correctness
    str 类型返回 StringState
    Validates: Requirements 3.4
    """
    app = lui.App()
    state = app.state("str_state", value)
    
    assert type(state).__name__ == "StringState", \
        f"Type inference failed: expected StringState, got {type(state).__name__}"


@given(value=st.lists(safe_integers, max_size=5))
@settings(max_examples=50)
def test_property_type_inference_list(value):
    """
    Property Test: Type Inference for List
    
    Feature: python_binding, Property 1: Type Inference Correctness
    list 类型返回 ListState
    Validates: Requirements 3.5
    """
    app = lui.App()
    state = app.state("list_state", value)
    
    assert type(state).__name__ == "ListState", \
        f"Type inference failed: expected ListState, got {type(state).__name__}"


@given(value=st.dictionaries(st.text(min_size=1, max_size=5, alphabet=st.characters(whitelist_categories=('L', 'N'))), safe_integers, max_size=3))
@settings(max_examples=50)
def test_property_type_inference_dict(value):
    """
    Property Test: Type Inference for Dict
    
    Feature: python_binding, Property 1: Type Inference Correctness
    dict 类型返回 DictState
    Validates: Requirements 3.6
    """
    app = lui.App()
    state = app.state("dict_state", value)
    
    assert type(state).__name__ == "DictState", \
        f"Type inference failed: expected DictState, got {type(state).__name__}"


# ============================================================
# Property 3: State Proxy Identity
# ============================================================

@given(name=state_names, value=safe_integers)
@settings(max_examples=50)
def test_property_state_proxy_identity(name, value):
    """
    Property Test: State Proxy Identity
    
    Feature: python_binding, Property 3: State Proxy Identity
    同名状态返回同一代理对象
    Validates: Requirements 3.8
    """
    assume(len(name.strip()) > 0)  # 确保名称非空
    
    app = lui.App()
    state1 = app.state(name, value)
    state2 = app.state(name, value + 100)  # 不同初始值
    
    assert state1 is state2, "State proxy identity failed: same name should return same object"


# ============================================================
# Property 9: Batch Mode Deferred Notification
# ============================================================

@given(
    increments=st.lists(st.integers(min_value=1, max_value=10), min_size=1, max_size=10)
)
@settings(max_examples=50)
def test_property_batch_deferred_notification(increments):
    """
    Property Test: Batch Mode Deferred Notification
    
    Feature: python_binding, Property 9: Batch Mode Deferred Notification
    批量模式下通知被延迟到 batch_end
    Validates: Requirements 10.2, 10.3, 10.4
    """
    app = lui.App()
    counter = app.state("counter", 0)
    
    notifications = []
    
    def on_change(value):
        notifications.append(value)
    
    watch_id = counter.watch(on_change)
    
    # 批量模式
    with lui.BatchContext(app):
        for inc in increments:
            counter.increment(inc)
        # 批量模式内不应有通知
        batch_notifications = len(notifications)
    
    # 批量结束后应该有通知
    counter.unwatch(watch_id)
    
    expected_total = sum(increments)
    assert counter.get() == expected_total, \
        f"Batch result wrong: {counter.get()} != {expected_total}"
    
    # 批量模式内不应有通知（通知在 batch_end 时触发）
    assert batch_notifications == 0, \
        f"Notifications during batch: {batch_notifications} (should be 0)"


# ============================================================
# 运行测试
# ============================================================

def main():
    """运行所有属性测试"""
    import pytest
    
    # 运行当前文件的测试
    exit_code = pytest.main([__file__, "-v", "--tb=short"])
    return exit_code


if __name__ == "__main__":
    exit(main())
