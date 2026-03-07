"""
LightUI C API ctypes 绑定层
自动加载 lightui.dll / liblightui.so，声明所有 C 函数签名
"""

import ctypes
import ctypes.util
import os
import sys
import platform

# ========== 类型别名 ==========
c_int = ctypes.c_int
c_int64 = ctypes.c_int64
c_double = ctypes.c_double
c_bool = ctypes.c_bool
c_char_p = ctypes.c_char_p
c_void_p = ctypes.c_void_p
c_float = ctypes.c_float
c_size_t = ctypes.c_size_t
POINTER = ctypes.POINTER

# ========== 回调类型 ==========
# char* (*LightUICallback)(const char* args_json, void* user_data)
# 注意：回调返回值必须来自 lightui_copy_string()，由 LightUI 在 C 侧释放
LightUICallback = ctypes.CFUNCTYPE(c_void_p, c_char_p, c_void_p)
# void (*LightUIStateCallback)(const char* name, const char* value_json, void* user_data)
LightUIStateCallback = ctypes.CFUNCTYPE(None, c_char_p, c_char_p, c_void_p)
# void (*LightUIResizeCallback)(int width, int height, void* user_data)
LightUIResizeCallback = ctypes.CFUNCTYPE(None, c_int, c_int, c_void_p)
# void (*LightUIVoidCallback)(void* user_data)
LightUIVoidCallback = ctypes.CFUNCTYPE(None, c_void_p)
# void (*LightUIUpdateCallback)(float delta_time, void* user_data)
LightUIUpdateCallback = ctypes.CFUNCTYPE(None, c_float, c_void_p)


# ========== LightUIConfig 结构体 ==========
class LightUIConfig(ctypes.Structure):
    _fields_ = [
        ("title", c_char_p),
        ("width", c_int),
        ("height", c_int),
        ("headless", c_bool),
        ("borderless", c_bool),
        ("transparent", c_bool),
        ("always_on_top", c_bool),
        ("resizable", c_bool),
        ("gpu", c_bool),
        ("fullscreen", c_bool),
        ("resize_border_width", c_int),
        ("min_width", c_int), ("min_height", c_int),
        ("max_width", c_int), ("max_height", c_int),
    ]


# ========== DLL 加载 ==========
def _find_dll():
    """查找 lightui 动态库"""
    # 可能的文件名
    if platform.system() == "Windows":
        names = ["lightui.dll"]
    elif platform.system() == "Darwin":
        names = ["liblightui.dylib"]
    else:
        names = ["liblightui.so"]

    # 搜索路径
    pkg_dir = os.path.dirname(os.path.abspath(__file__))
    proj_root = os.path.normpath(os.path.join(pkg_dir, "..", "..", ".."))
    search_dirs = [
        pkg_dir,  # 当前包目录
        os.path.join(pkg_dir, "bin"),  # bin 子目录
        os.getcwd(),  # 当前工作目录
        os.path.join(proj_root, "build", "bin", "Release"),  # CMake Release 输出
        os.path.join(proj_root, "build", "bin", "Debug"),    # CMake Debug 输出
    ]

    # 从环境变量获取额外搜索路径
    env_path = os.environ.get("LIGHTUI_DLL_PATH", "")
    if env_path:
        search_dirs.insert(0, env_path)

    for d in search_dirs:
        for name in names:
            path = os.path.join(d, name)
            if os.path.exists(path):
                return path

    # 最后尝试系统路径
    for name in names:
        try:
            return ctypes.util.find_library(name.replace(".dll", "").replace("lib", "").replace(".so", "").replace(".dylib", ""))
        except Exception:
            pass

    return None


def load_dll(path=None):
    """加载 lightui 动态库并绑定函数签名"""
    if path is None:
        path = _find_dll()
    if path is None:
        raise FileNotFoundError(
            "找不到 lightui 动态库。请设置 LIGHTUI_DLL_PATH 环境变量，"
            "或将 lightui.dll 放在当前目录下。"
        )

    lib = ctypes.CDLL(path)
    _bind_functions(lib)
    return lib


def _bind_functions(lib):
    """声明所有 C 函数签名"""
    H = c_void_p  # LightUIHandle

    # 生命周期
    lib.lightui_init.restype = c_int
    lib.lightui_init.argtypes = []
    lib.lightui_cleanup.restype = None
    lib.lightui_cleanup.argtypes = []
    lib.lightui_version.restype = c_char_p
    lib.lightui_version.argtypes = []

    # 窗口管理
    lib.lightui_create.restype = H
    lib.lightui_create.argtypes = [c_char_p, c_int, c_int]
    lib.lightui_create_ex.restype = H
    lib.lightui_create_ex.argtypes = [POINTER(LightUIConfig)]
    lib.lightui_default_config.restype = LightUIConfig
    lib.lightui_default_config.argtypes = []
    lib.lightui_destroy.restype = None
    lib.lightui_destroy.argtypes = [H]
    lib.lightui_run.restype = None
    lib.lightui_run.argtypes = [H]
    lib.lightui_stop.restype = None
    lib.lightui_stop.argtypes = [H]
    lib.lightui_poll_events.restype = c_bool
    lib.lightui_poll_events.argtypes = [H]

    # 窗口属性
    lib.lightui_set_title.restype = c_int
    lib.lightui_set_title.argtypes = [H, c_char_p]
    lib.lightui_set_size.restype = c_int
    lib.lightui_set_size.argtypes = [H, c_int, c_int]
    lib.lightui_get_size.restype = c_int
    lib.lightui_get_size.argtypes = [H, POINTER(c_int), POINTER(c_int)]
    lib.lightui_set_position.restype = c_int
    lib.lightui_set_position.argtypes = [H, c_int, c_int]
    lib.lightui_get_position.restype = c_int
    lib.lightui_get_position.argtypes = [H, POINTER(c_int), POINTER(c_int)]
    lib.lightui_set_min_size.restype = c_int
    lib.lightui_set_min_size.argtypes = [H, c_int, c_int]
    lib.lightui_set_max_size.restype = c_int
    lib.lightui_set_max_size.argtypes = [H, c_int, c_int]
    lib.lightui_minimize.restype = c_int
    lib.lightui_minimize.argtypes = [H]
    lib.lightui_maximize.restype = c_int
    lib.lightui_maximize.argtypes = [H]
    lib.lightui_restore.restype = c_int
    lib.lightui_restore.argtypes = [H]
    lib.lightui_show.restype = c_int
    lib.lightui_show.argtypes = [H]
    lib.lightui_hide.restype = c_int
    lib.lightui_hide.argtypes = [H]
    lib.lightui_set_fullscreen.restype = c_int
    lib.lightui_set_fullscreen.argtypes = [H, c_bool]
    lib.lightui_set_resizable.restype = c_int
    lib.lightui_set_resizable.argtypes = [H, c_bool]
    lib.lightui_set_borderless.restype = c_int
    lib.lightui_set_borderless.argtypes = [H, c_bool]
    lib.lightui_set_always_on_top.restype = c_int
    lib.lightui_set_always_on_top.argtypes = [H, c_bool]

    # UI 加载
    lib.lightui_load_html.restype = c_int
    lib.lightui_load_html.argtypes = [H, c_char_p]
    lib.lightui_load_html_file.restype = c_int
    lib.lightui_load_html_file.argtypes = [H, c_char_p]
    lib.lightui_eval_js.restype = c_int
    lib.lightui_eval_js.argtypes = [H, c_char_p]
    lib.lightui_eval_module.restype = c_int
    lib.lightui_eval_module.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_load_js_file.restype = c_int
    lib.lightui_load_js_file.argtypes = [H, c_char_p]
    lib.lightui_load_bytecode.restype = c_int
    lib.lightui_load_bytecode.argtypes = [H, c_void_p, c_size_t]

    # 函数绑定
    lib.lightui_bind.restype = c_int
    lib.lightui_bind.argtypes = [H, c_char_p, LightUICallback, c_void_p]
    lib.lightui_unbind.restype = None
    lib.lightui_unbind.argtypes = [H, c_char_p]

    # 事件回调
    lib.lightui_on_resize.restype = c_int
    lib.lightui_on_resize.argtypes = [H, LightUIResizeCallback, c_void_p]
    lib.lightui_on_close.restype = c_int
    lib.lightui_on_close.argtypes = [H, LightUIVoidCallback, c_void_p]
    lib.lightui_on_focus.restype = c_int
    lib.lightui_on_focus.argtypes = [H, LightUIVoidCallback, c_void_p]
    lib.lightui_on_blur.restype = c_int
    lib.lightui_on_blur.argtypes = [H, LightUIVoidCallback, c_void_p]
    lib.lightui_on_update.restype = c_int
    lib.lightui_on_update.argtypes = [H, LightUIUpdateCallback, c_void_p]

    # 事件发送
    lib.lightui_emit.restype = c_int
    lib.lightui_emit.argtypes = [H, c_char_p, c_char_p]

    # DevTools
    lib.lightui_devtools_open.restype = c_int
    lib.lightui_devtools_open.argtypes = [H]
    lib.lightui_devtools_close.restype = c_int
    lib.lightui_devtools_close.argtypes = [H]

    # 状态创建
    lib.lightui_state_create_null.restype = c_int
    lib.lightui_state_create_null.argtypes = [H, c_char_p]
    lib.lightui_state_create_bool.restype = c_int
    lib.lightui_state_create_bool.argtypes = [H, c_char_p, c_bool]
    lib.lightui_state_create_int.restype = c_int
    lib.lightui_state_create_int.argtypes = [H, c_char_p, c_int64]
    lib.lightui_state_create_double.restype = c_int
    lib.lightui_state_create_double.argtypes = [H, c_char_p, c_double]
    lib.lightui_state_create_string.restype = c_int
    lib.lightui_state_create_string.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_create_array.restype = c_int
    lib.lightui_state_create_array.argtypes = [H, c_char_p]
    lib.lightui_state_create_object.restype = c_int
    lib.lightui_state_create_object.argtypes = [H, c_char_p]
    lib.lightui_state_create_json.restype = c_int
    lib.lightui_state_create_json.argtypes = [H, c_char_p, c_char_p]

    # 状态查询
    lib.lightui_state_exists.restype = c_bool
    lib.lightui_state_exists.argtypes = [H, c_char_p]
    lib.lightui_state_type.restype = c_int
    lib.lightui_state_type.argtypes = [H, c_char_p]
    lib.lightui_state_delete.restype = None
    lib.lightui_state_delete.argtypes = [H, c_char_p]

    # 状态读取
    lib.lightui_state_get_bool.restype = c_bool
    lib.lightui_state_get_bool.argtypes = [H, c_char_p]
    lib.lightui_state_get_int.restype = c_int64
    lib.lightui_state_get_int.argtypes = [H, c_char_p]
    lib.lightui_state_get_double.restype = c_double
    lib.lightui_state_get_double.argtypes = [H, c_char_p]
    lib.lightui_state_get_string.restype = c_char_p
    lib.lightui_state_get_string.argtypes = [H, c_char_p]
    lib.lightui_state_get_length.restype = c_int
    lib.lightui_state_get_length.argtypes = [H, c_char_p]
    lib.lightui_state_get_json.restype = c_char_p
    lib.lightui_state_get_json.argtypes = [H, c_char_p]
    lib.lightui_state_get_at.restype = c_char_p
    lib.lightui_state_get_at.argtypes = [H, c_char_p, c_int]
    lib.lightui_state_get_key.restype = c_char_p
    lib.lightui_state_get_key.argtypes = [H, c_char_p, c_char_p]

    # 状态写入
    lib.lightui_state_set_null.restype = c_int
    lib.lightui_state_set_null.argtypes = [H, c_char_p]
    lib.lightui_state_set_bool.restype = c_int
    lib.lightui_state_set_bool.argtypes = [H, c_char_p, c_bool]
    lib.lightui_state_set_int.restype = c_int
    lib.lightui_state_set_int.argtypes = [H, c_char_p, c_int64]
    lib.lightui_state_set_double.restype = c_int
    lib.lightui_state_set_double.argtypes = [H, c_char_p, c_double]
    lib.lightui_state_set_string.restype = c_int
    lib.lightui_state_set_string.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_set_json.restype = c_int
    lib.lightui_state_set_json.argtypes = [H, c_char_p, c_char_p]


    # 数组操作
    lib.lightui_state_array_push.restype = c_int
    lib.lightui_state_array_push.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_array_push_int.restype = c_int
    lib.lightui_state_array_push_int.argtypes = [H, c_char_p, c_int64]
    lib.lightui_state_array_push_double.restype = c_int
    lib.lightui_state_array_push_double.argtypes = [H, c_char_p, c_double]
    lib.lightui_state_array_push_string.restype = c_int
    lib.lightui_state_array_push_string.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_array_push_bool.restype = c_int
    lib.lightui_state_array_push_bool.argtypes = [H, c_char_p, c_bool]
    lib.lightui_state_array_pop.restype = c_int
    lib.lightui_state_array_pop.argtypes = [H, c_char_p]
    lib.lightui_state_array_shift.restype = c_int
    lib.lightui_state_array_shift.argtypes = [H, c_char_p]
    lib.lightui_state_array_unshift.restype = c_int
    lib.lightui_state_array_unshift.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_array_remove.restype = c_int
    lib.lightui_state_array_remove.argtypes = [H, c_char_p, c_int]
    lib.lightui_state_array_clear.restype = c_int
    lib.lightui_state_array_clear.argtypes = [H, c_char_p]
    lib.lightui_state_array_set.restype = c_int
    lib.lightui_state_array_set.argtypes = [H, c_char_p, c_int, c_char_p]
    lib.lightui_state_array_set_int.restype = c_int
    lib.lightui_state_array_set_int.argtypes = [H, c_char_p, c_int, c_int64]
    lib.lightui_state_array_set_double.restype = c_int
    lib.lightui_state_array_set_double.argtypes = [H, c_char_p, c_int, c_double]
    lib.lightui_state_array_set_string.restype = c_int
    lib.lightui_state_array_set_string.argtypes = [H, c_char_p, c_int, c_char_p]

    # 对象操作
    lib.lightui_state_object_set.restype = c_int
    lib.lightui_state_object_set.argtypes = [H, c_char_p, c_char_p, c_char_p]
    lib.lightui_state_object_set_int.restype = c_int
    lib.lightui_state_object_set_int.argtypes = [H, c_char_p, c_char_p, c_int64]
    lib.lightui_state_object_set_double.restype = c_int
    lib.lightui_state_object_set_double.argtypes = [H, c_char_p, c_char_p, c_double]
    lib.lightui_state_object_set_string.restype = c_int
    lib.lightui_state_object_set_string.argtypes = [H, c_char_p, c_char_p, c_char_p]
    lib.lightui_state_object_set_bool.restype = c_int
    lib.lightui_state_object_set_bool.argtypes = [H, c_char_p, c_char_p, c_bool]
    lib.lightui_state_object_remove.restype = c_int
    lib.lightui_state_object_remove.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_object_clear.restype = c_int
    lib.lightui_state_object_clear.argtypes = [H, c_char_p]

    # 数值/字符串操作
    lib.lightui_state_increment.restype = c_int
    lib.lightui_state_increment.argtypes = [H, c_char_p, c_double]
    lib.lightui_state_multiply.restype = c_int
    lib.lightui_state_multiply.argtypes = [H, c_char_p, c_double]
    lib.lightui_state_string_append.restype = c_int
    lib.lightui_state_string_append.argtypes = [H, c_char_p, c_char_p]
    lib.lightui_state_string_prepend.restype = c_int
    lib.lightui_state_string_prepend.argtypes = [H, c_char_p, c_char_p]

    # 监听
    lib.lightui_state_watch.restype = c_int
    lib.lightui_state_watch.argtypes = [H, c_char_p, LightUIStateCallback, c_void_p]
    lib.lightui_state_unwatch.restype = None
    lib.lightui_state_unwatch.argtypes = [H, c_int]

    # 批量/队列
    lib.lightui_state_batch_begin.restype = None
    lib.lightui_state_batch_begin.argtypes = [H]
    lib.lightui_state_batch_end.restype = None
    lib.lightui_state_batch_end.argtypes = [H]
    lib.lightui_state_set_merge_mode.restype = None
    lib.lightui_state_set_merge_mode.argtypes = [H, c_bool]
    lib.lightui_process_queue.restype = c_int
    lib.lightui_process_queue.argtypes = [H]
    lib.lightui_queue_size.restype = c_int
    lib.lightui_queue_size.argtypes = [H]

    # 工具
    lib.lightui_free.restype = None
    lib.lightui_free.argtypes = [c_void_p]
    lib.lightui_copy_string.restype = c_void_p
    lib.lightui_copy_string.argtypes = [c_char_p]

    lib.lightui_last_error.restype = c_char_p
    lib.lightui_last_error.argtypes = []

    # ========== 共享 C 对象 (SharedObject) ==========
    SH = c_void_p  # LightUISharedHandle

    lib.lightui_shared_create.restype = SH
    lib.lightui_shared_create.argtypes = [H, c_char_p]
    lib.lightui_shared_destroy.restype = None
    lib.lightui_shared_destroy.argtypes = [SH]

    # setter
    lib.lightui_shared_set_int.restype = c_int
    lib.lightui_shared_set_int.argtypes = [SH, c_char_p, c_int64]
    lib.lightui_shared_set_double.restype = c_int
    lib.lightui_shared_set_double.argtypes = [SH, c_char_p, c_double]
    lib.lightui_shared_set_string.restype = c_int
    lib.lightui_shared_set_string.argtypes = [SH, c_char_p, c_char_p]
    lib.lightui_shared_set_bool.restype = c_int
    lib.lightui_shared_set_bool.argtypes = [SH, c_char_p, c_bool]
    lib.lightui_shared_set_null.restype = c_int
    lib.lightui_shared_set_null.argtypes = [SH, c_char_p]
    lib.lightui_shared_set_json.restype = c_int
    lib.lightui_shared_set_json.argtypes = [SH, c_char_p, c_char_p]

    # getter
    lib.lightui_shared_get_int.restype = c_int64
    lib.lightui_shared_get_int.argtypes = [SH, c_char_p]
    lib.lightui_shared_get_double.restype = c_double
    lib.lightui_shared_get_double.argtypes = [SH, c_char_p]
    lib.lightui_shared_get_string.restype = c_void_p
    lib.lightui_shared_get_string.argtypes = [SH, c_char_p]
    lib.lightui_shared_get_bool.restype = c_bool
    lib.lightui_shared_get_bool.argtypes = [SH, c_char_p]
    lib.lightui_shared_get_json.restype = c_void_p
    lib.lightui_shared_get_json.argtypes = [SH, c_char_p]

    # 属性查询
    lib.lightui_shared_get_type.restype = c_int
    lib.lightui_shared_get_type.argtypes = [SH, c_char_p]
    lib.lightui_shared_delete.restype = c_int
    lib.lightui_shared_delete.argtypes = [SH, c_char_p]
    lib.lightui_shared_has.restype = c_bool
    lib.lightui_shared_has.argtypes = [SH, c_char_p]

    # 批量更新
    lib.lightui_shared_batch_begin.restype = None
    lib.lightui_shared_batch_begin.argtypes = [SH]
    lib.lightui_shared_batch_end.restype = None
    lib.lightui_shared_batch_end.argtypes = [SH]