"""
SharedState — Python/JS 共享 C 对象代理类

用法：
    data = app.shared("data")
    data.count = 0        # → C: JS_SetPropertyStr → JS: data.count = 0
    data.count += 1       # → C: get + set → JS: 自动触发 __onSharedUpdate
    print(data.count)     # → C: JS_GetPropertyStr → 1

    # 批量更新（只触发一次 __onSharedUpdate）
    with data.batch():
        data.x = 1
        data.y = 2
        data.z = 3
"""

import json as _json
from contextlib import contextmanager

# LightUIType 枚举值
_TYPE_NULL = 0
_TYPE_BOOL = 1
_TYPE_INT = 2
_TYPE_DOUBLE = 3
_TYPE_STRING = 4
_TYPE_ARRAY = 5
_TYPE_OBJECT = 6


class SharedState:
    """Python 代理类，通过 __setattr__/__getattr__ 映射到 C 共享对象"""

    def __init__(self, lib, handle, shared_handle, name):
        # 使用 object.__setattr__ 避免触发自定义 __setattr__
        object.__setattr__(self, '_lib', lib)
        object.__setattr__(self, '_handle', handle)
        object.__setattr__(self, '_shared', shared_handle)
        object.__setattr__(self, '_name', name)

    def __setattr__(self, key, value):
        if key.startswith('_'):
            object.__setattr__(self, key, value)
            return

        lib = object.__getattribute__(self, '_lib')
        sh = object.__getattribute__(self, '_shared')
        k = key.encode('utf-8')

        if value is None:
            lib.lightui_shared_set_null(sh, k)
        elif isinstance(value, bool):
            lib.lightui_shared_set_bool(sh, k, value)
        elif isinstance(value, int):
            lib.lightui_shared_set_int(sh, k, value)
        elif isinstance(value, float):
            lib.lightui_shared_set_double(sh, k, value)
        elif isinstance(value, str):
            lib.lightui_shared_set_string(sh, k, value.encode('utf-8'))
        else:
            # list, dict 等复杂类型 → JSON
            lib.lightui_shared_set_json(sh, k,
                                        _json.dumps(value, ensure_ascii=False).encode('utf-8'))

    def __getattr__(self, key):
        if key.startswith('_'):
            return object.__getattribute__(self, key)

        lib = object.__getattribute__(self, '_lib')
        sh = object.__getattribute__(self, '_shared')
        k = key.encode('utf-8')

        t = lib.lightui_shared_get_type(sh, k)

        if t == _TYPE_NULL:
            return None
        elif t == _TYPE_BOOL:
            return lib.lightui_shared_get_bool(sh, k)
        elif t == _TYPE_INT:
            return lib.lightui_shared_get_int(sh, k)
        elif t == _TYPE_DOUBLE:
            return lib.lightui_shared_get_double(sh, k)
        elif t == _TYPE_STRING:
            raw = lib.lightui_shared_get_string(sh, k)
            if raw:
                result = raw.decode('utf-8')
                lib.lightui_free(raw)
                return result
            return None
        else:
            # ARRAY / OBJECT → JSON 反序列化
            raw = lib.lightui_shared_get_json(sh, k)
            if raw:
                result = _json.loads(raw.decode('utf-8'))
                lib.lightui_free(raw)
                return result
            return None

    def __delattr__(self, key):
        lib = object.__getattribute__(self, '_lib')
        sh = object.__getattribute__(self, '_shared')
        lib.lightui_shared_delete(sh, key.encode('utf-8'))

    def __contains__(self, key):
        lib = object.__getattribute__(self, '_lib')
        sh = object.__getattribute__(self, '_shared')
        return lib.lightui_shared_has(sh, key.encode('utf-8'))

    @contextmanager
    def batch(self):
        """批量更新上下文管理器 — 抑制中间 __onSharedUpdate 调用"""
        lib = object.__getattribute__(self, '_lib')
        sh = object.__getattribute__(self, '_shared')
        lib.lightui_shared_batch_begin(sh)
        try:
            yield self
        finally:
            lib.lightui_shared_batch_end(sh)

    def __repr__(self):
        name = object.__getattribute__(self, '_name')
        return f"<SharedState '{name}'>"

