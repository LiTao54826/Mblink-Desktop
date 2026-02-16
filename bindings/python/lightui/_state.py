"""
LightUI State 统一封装
一个 State 对象对应一个 C 层状态变量，通过 .value 属性读写
"""

import json
from ._ffi import (
    c_int, c_int64, c_double, c_bool, c_char_p, c_void_p,
    LightUIStateCallback,
)

# C 层类型枚举（与 LightUIType 对应）
TYPE_NULL = 0
TYPE_BOOL = 1
TYPE_INT = 2
TYPE_DOUBLE = 3
TYPE_STRING = 4
TYPE_ARRAY = 5
TYPE_OBJECT = 6

_TYPE_NAMES = {
    TYPE_NULL: "null", TYPE_BOOL: "bool", TYPE_INT: "int",
    TYPE_DOUBLE: "double", TYPE_STRING: "string",
    TYPE_ARRAY: "array", TYPE_OBJECT: "object",
}


class State:
    """统一状态变量，支持 bool/int/float/str/list/dict/None"""

    __slots__ = ("_lib", "_handle", "_name", "_name_b", "_type", "_watchers")

    def __init__(self, lib, handle, name: str, initial=None):
        self._lib = lib
        self._handle = handle
        self._name = name
        self._name_b = name.encode("utf-8")
        self._watchers = []  # 防止回调被GC
        self._type = self._create(initial)

    # ---------- 创建 ----------
    def _create(self, value):
        lib, h, n = self._lib, self._handle, self._name_b
        if value is None:
            lib.lightui_state_create_null(h, n)
            return TYPE_NULL
        if isinstance(value, bool):
            lib.lightui_state_create_bool(h, n, value)
            return TYPE_BOOL
        if isinstance(value, int):
            lib.lightui_state_create_int(h, n, value)
            return TYPE_INT
        if isinstance(value, float):
            lib.lightui_state_create_double(h, n, value)
            return TYPE_DOUBLE
        if isinstance(value, str):
            lib.lightui_state_create_string(h, n, value.encode("utf-8"))
            return TYPE_STRING
        if isinstance(value, (list, tuple)):
            j = json.dumps(value, ensure_ascii=False)
            lib.lightui_state_create_json(h, n, j.encode("utf-8"))
            return TYPE_ARRAY
        if isinstance(value, dict):
            j = json.dumps(value, ensure_ascii=False)
            lib.lightui_state_create_json(h, n, j.encode("utf-8"))
            return TYPE_OBJECT
        # fallback: JSON 序列化
        j = json.dumps(value, ensure_ascii=False)
        lib.lightui_state_create_json(h, n, j.encode("utf-8"))
        return TYPE_OBJECT

    # ---------- 读取 ----------
    @property
    def value(self):
        lib, h, n = self._lib, self._handle, self._name_b
        t = self._type
        if t == TYPE_NULL:
            return None
        if t == TYPE_BOOL:
            return bool(lib.lightui_state_get_bool(h, n))
        if t == TYPE_INT:
            return int(lib.lightui_state_get_int(h, n))
        if t == TYPE_DOUBLE:
            return float(lib.lightui_state_get_double(h, n))
        if t == TYPE_STRING:
            raw = lib.lightui_state_get_string(h, n)
            return raw.decode("utf-8") if raw else ""
        # array / object → JSON 反序列化
        raw = lib.lightui_state_get_json(h, n)
        if raw:
            return json.loads(raw.decode("utf-8"))
        return None

    # ---------- 写入 ----------
    @value.setter
    def value(self, new_value):
        lib, h, n = self._lib, self._handle, self._name_b
        if new_value is None:
            lib.lightui_state_set_null(h, n)
            self._type = TYPE_NULL
        elif isinstance(new_value, bool):
            lib.lightui_state_set_bool(h, n, new_value)
            self._type = TYPE_BOOL
        elif isinstance(new_value, int):
            lib.lightui_state_set_int(h, n, new_value)
            self._type = TYPE_INT
        elif isinstance(new_value, float):
            lib.lightui_state_set_double(h, n, new_value)
            self._type = TYPE_DOUBLE
        elif isinstance(new_value, str):
            lib.lightui_state_set_string(h, n, new_value.encode("utf-8"))
            self._type = TYPE_STRING
        else:
            j = json.dumps(new_value, ensure_ascii=False)
            lib.lightui_state_set_json(h, n, j.encode("utf-8"))
            self._type = TYPE_ARRAY if isinstance(new_value, (list, tuple)) else TYPE_OBJECT

    # ---------- 监听 ----------
    def watch(self, callback):
        """注册状态变更监听器，callback(name, value_json)"""
        @LightUIStateCallback
        def _cb(name_ptr, val_ptr, _ud):
            name_str = name_ptr.decode("utf-8") if name_ptr else self._name
            val_str = val_ptr.decode("utf-8") if val_ptr else "null"
            callback(name_str, val_str)

        self._watchers.append(_cb)  # prevent GC
        wid = self._lib.lightui_state_watch(
            self._handle, self._name_b, _cb, None
        )
        return wid

    def unwatch(self, watch_id: int):
        self._lib.lightui_state_unwatch(self._handle, watch_id)

    # ---------- 便捷方法 ----------
    def delete(self):
        self._lib.lightui_state_delete(self._handle, self._name_b)

    @property
    def exists(self) -> bool:
        return bool(self._lib.lightui_state_exists(self._handle, self._name_b))

    @property
    def type_name(self) -> str:
        t = self._lib.lightui_state_type(self._handle, self._name_b)
        return _TYPE_NAMES.get(t, "unknown")

    def __repr__(self):
        return f"State({self._name!r}, value={self.value!r})"

