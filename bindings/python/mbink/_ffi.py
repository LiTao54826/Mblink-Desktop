"""
MBink C API ctypes 绑定层
自动加载 mbink.dll / libmbink.so，声明所有 C 函数签名
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
c_uint32 = ctypes.c_uint32
c_ushort = ctypes.c_ushort
POINTER = ctypes.POINTER

# ========== 回调类型 ==========
# char* (*MBinkCallback)(const char* args_json, void* user_data)
# 注意：回调返回值必须来自 mbink_copy_string()，由 MBink 在 C 侧释放
MBinkCallback = ctypes.CFUNCTYPE(c_void_p, c_char_p, c_void_p)
MBinkAsyncCallback = ctypes.CFUNCTYPE(c_void_p, c_char_p, c_void_p)
# void (*MBinkStateCallback)(const char* name, const char* value_json, void* user_data)
MBinkStateCallback = ctypes.CFUNCTYPE(None, c_char_p, c_char_p, c_void_p)
# void (*MBinkResizeCallback)(int width, int height, void* user_data)
MBinkResizeCallback = ctypes.CFUNCTYPE(None, c_int, c_int, c_void_p)
# void (*MBinkVoidCallback)(void* user_data)
MBinkVoidCallback = ctypes.CFUNCTYPE(None, c_void_p)
# bool (*MBinkBoolCallback)(void* user_data)
MBinkBoolCallback = ctypes.CFUNCTYPE(c_bool, c_void_p)
# void (*MBinkUpdateCallback)(float delta_time, void* user_data)
MBinkUpdateCallback = ctypes.CFUNCTYPE(None, c_float, c_void_p)
MBinkObserveCallback = ctypes.CFUNCTYPE(None, c_int, c_char_p, c_void_p)


# ========== MBinkConfig 结构体 ==========
class MBinkConfig(ctypes.Structure):
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


class MBinkRuntimeOptions(ctypes.Structure):
    _fields_ = [
        ("runtime_epoch", c_char_p),
        ("load_embedded_runtime", c_bool),
        ("load_official_preact", c_bool),
    ]


class MBinkUiDevSnapshotOptions(ctypes.Structure):
    _fields_ = [
        ("runtime_epoch", c_char_p),
        ("max_nodes", c_size_t),
        ("max_depth", c_int),
        ("root_selector", c_char_p),
        ("include_screenshot", c_bool),
        ("inline_screenshot", c_bool),
        ("screenshot_file", c_char_p),
    ]


class MBinkDevToolsHttpOptions(ctypes.Structure):
    _fields_ = [
        ("bind_host", c_char_p),
        ("port", c_ushort),
        ("auth_token", c_char_p),
        ("require_auth", c_bool),
    ]


class MBinkDevToolsHttpInfo(ctypes.Structure):
    _fields_ = [
        ("port", c_ushort),
        ("url", c_char_p),
        ("auth_token", c_char_p),
    ]


# ========== DLL 加载 ==========
def _find_dll():
    """查找 MBink 动态库"""
    # 可能的文件名
    if platform.system() == "Windows":
        names = ["mbink.dll"]
    elif platform.system() == "Darwin":
        names = ["libmbink.dylib"]
    else:
        names = ["libmbink.so"]

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
    env_path = os.environ.get("MBINK_DLL_PATH", "")
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
    """加载 MBink 动态库并绑定函数签名"""
    if path is None:
        path = _find_dll()
    if path is None:
        raise FileNotFoundError(
            "找不到 MBink 动态库。请设置 MBINK_DLL_PATH 环境变量，"
            "或将 mbink.dll 放在当前目录下。"
        )
    lib = ctypes.CDLL(path)
    lib._mbink_dll_path = path
    _bind_functions(lib)
    return lib


def _devtools_library_names():
    if platform.system() == "Windows":
        return ["mbink_devtools.dll"]
    if platform.system() == "Darwin":
        return ["libmbink_devtools.dylib"]
    return ["libmbink_devtools.so"]


def _find_devtools_dll(mbink_lib=None, path=None):
    if path:
        return os.path.abspath(path)

    names = _devtools_library_names()
    candidates = []
    env_path = os.environ.get("MBINK_DEVTOOLS_PATH", "")
    if env_path:
        if os.path.isdir(env_path):
            candidates.extend(os.path.abspath(os.path.join(env_path, name)) for name in names)
        else:
            candidates.append(os.path.abspath(env_path))

    mbink_path = getattr(mbink_lib, "_mbink_dll_path", None)
    if mbink_path:
        base_dir = os.path.dirname(os.path.abspath(mbink_path))
        candidates.extend(os.path.join(base_dir, name) for name in names)

    pkg_dir = os.path.dirname(os.path.abspath(__file__))
    for directory in (
        os.path.join(pkg_dir, "bin"),
        pkg_dir,
    ):
        candidates.extend(os.path.abspath(os.path.join(directory, name)) for name in names)

    for candidate in candidates:
        if candidate and os.path.exists(candidate):
            return candidate

    return None


def load_devtools_dll(mbink_lib=None, path=None):
    resolved = _find_devtools_dll(mbink_lib, path)
    if resolved is None:
        raise FileNotFoundError(
            "mbink_devtools dynamic library not found. Set MBINK_DEVTOOLS_PATH "
            "or place mbink_devtools.dll next to mbink.dll."
        )
    lib = ctypes.CDLL(resolved)
    lib._mbink_devtools_dll_path = resolved
    _bind_devtools_functions(lib)
    return lib


def _bind_devtools_functions(lib):
    H = c_void_p
    lib.mbink_devtools_open.restype = c_int
    lib.mbink_devtools_open.argtypes = [H]
    lib.mbink_devtools_close.restype = c_int
    lib.mbink_devtools_close.argtypes = [H]
    lib.mbink_devtools_default_http_options.restype = MBinkDevToolsHttpOptions
    lib.mbink_devtools_default_http_options.argtypes = []
    lib.mbink_devtools_http_start.restype = c_int
    lib.mbink_devtools_http_start.argtypes = [H, POINTER(MBinkDevToolsHttpOptions), POINTER(MBinkDevToolsHttpInfo)]
    lib.mbink_devtools_http_stop.restype = c_int
    lib.mbink_devtools_http_stop.argtypes = [H]
    lib.mbink_devtools_http_info_free.restype = None
    lib.mbink_devtools_http_info_free.argtypes = [POINTER(MBinkDevToolsHttpInfo)]
    lib.mbink_ui_dev_default_snapshot_options.restype = MBinkUiDevSnapshotOptions
    lib.mbink_ui_dev_default_snapshot_options.argtypes = []
    lib.mbink_ui_dev_snapshot_json.restype = c_int
    lib.mbink_ui_dev_snapshot_json.argtypes = [H, POINTER(MBinkUiDevSnapshotOptions), POINTER(c_void_p)]
    lib.mbink_ui_dev_snapshot_file.restype = c_int
    lib.mbink_ui_dev_snapshot_file.argtypes = [H, c_char_p, POINTER(MBinkUiDevSnapshotOptions)]
    lib.mbink_ui_dev_command_json.restype = c_int
    lib.mbink_ui_dev_command_json.argtypes = [H, c_char_p, POINTER(c_void_p)]


def _bind_functions(lib):
    """声明所有 C 函数签名"""
    H = c_void_p  # MBinkHandle

    # 生命周期
    lib.mbink_init.restype = c_int
    lib.mbink_init.argtypes = []
    lib.mbink_cleanup.restype = None
    lib.mbink_cleanup.argtypes = []
    lib.mbink_version.restype = c_char_p
    lib.mbink_version.argtypes = []
    lib.mbink_last_error.restype = c_char_p
    lib.mbink_last_error.argtypes = []

    # 窗口管理
    lib.mbink_create.restype = H
    lib.mbink_create.argtypes = [c_char_p, c_int, c_int]
    lib.mbink_create_ex.restype = H
    lib.mbink_create_ex.argtypes = [POINTER(MBinkConfig)]
    lib.mbink_default_config.restype = MBinkConfig
    lib.mbink_default_config.argtypes = []
    lib.mbink_destroy.restype = None
    lib.mbink_destroy.argtypes = [H]
    lib.mbink_run.restype = None
    lib.mbink_run.argtypes = [H]
    lib.mbink_stop.restype = None
    lib.mbink_stop.argtypes = [H]
    lib.mbink_poll_events.restype = c_bool
    lib.mbink_poll_events.argtypes = [H]
    lib.mbink_default_runtime_options.restype = MBinkRuntimeOptions
    lib.mbink_default_runtime_options.argtypes = []
    lib.mbink_configure_runtime.restype = c_int
    lib.mbink_configure_runtime.argtypes = [H, POINTER(MBinkRuntimeOptions)]
    lib.mbink_load_embedded_runtime.restype = c_int
    lib.mbink_load_embedded_runtime.argtypes = [H, c_bool]
    lib.mbink_load_entry_file.restype = c_int
    lib.mbink_load_entry_file.argtypes = [H, c_char_p, c_bool]
    lib.mbink_load_module_file.restype = c_int
    lib.mbink_load_module_file.argtypes = [H, c_char_p]
    lib.mbink_render_frame.restype = c_int
    lib.mbink_render_frame.argtypes = [H, c_int]
    lib.mbink_runtime_epoch.restype = c_int
    lib.mbink_runtime_epoch.argtypes = [H, POINTER(c_void_p)]
    lib.mbink_lifecycle_state.restype = c_int
    lib.mbink_lifecycle_state.argtypes = [H]
    lib.mbink_lifecycle_reason.restype = c_int
    lib.mbink_lifecycle_reason.argtypes = [H, POINTER(c_void_p)]

    # 函数绑定
    lib.mbink_bind.restype = c_int
    lib.mbink_bind.argtypes = [H, c_char_p, MBinkCallback, c_void_p]
    lib.mbink_bind_async.restype = c_int
    lib.mbink_bind_async.argtypes = [H, c_char_p, MBinkAsyncCallback, c_void_p]
    lib.mbink_unbind.restype = None
    lib.mbink_unbind.argtypes = [H, c_char_p]

    # 窗口属性
    lib.mbink_set_title.restype = c_int
    lib.mbink_set_title.argtypes = [H, c_char_p]
    lib.mbink_tray_create.restype = c_int
    lib.mbink_tray_create.argtypes = [H, c_char_p]
    lib.mbink_tray_destroy.restype = c_int
    lib.mbink_tray_destroy.argtypes = [H]
    lib.mbink_tray_set_tooltip.restype = c_int
    lib.mbink_tray_set_tooltip.argtypes = [H, c_char_p]
    lib.mbink_tray_set_menu.restype = c_int
    lib.mbink_tray_set_menu.argtypes = [H, c_char_p]
    lib.mbink_tray_set_left_click_callback.restype = c_int
    lib.mbink_tray_set_left_click_callback.argtypes = [H, MBinkVoidCallback, c_void_p]
    lib.mbink_tray_set_menu_callback.restype = c_int
    lib.mbink_tray_set_menu_callback.argtypes = [H, MBinkCallback, c_void_p]
    lib.mbink_set_size.restype = c_int
    lib.mbink_set_size.argtypes = [H, c_int, c_int]
    lib.mbink_get_size.restype = c_int
    lib.mbink_get_size.argtypes = [H, POINTER(c_int), POINTER(c_int)]
    lib.mbink_set_position.restype = c_int
    lib.mbink_set_position.argtypes = [H, c_int, c_int]
    lib.mbink_get_position.restype = c_int
    lib.mbink_get_position.argtypes = [H, POINTER(c_int), POINTER(c_int)]
    lib.mbink_set_min_size.restype = c_int
    lib.mbink_set_min_size.argtypes = [H, c_int, c_int]
    lib.mbink_set_max_size.restype = c_int
    lib.mbink_set_max_size.argtypes = [H, c_int, c_int]
    lib.mbink_minimize.restype = c_int
    lib.mbink_minimize.argtypes = [H]
    lib.mbink_maximize.restype = c_int
    lib.mbink_maximize.argtypes = [H]
    lib.mbink_restore.restype = c_int
    lib.mbink_restore.argtypes = [H]
    lib.mbink_show.restype = c_int
    lib.mbink_show.argtypes = [H]
    lib.mbink_hide.restype = c_int
    lib.mbink_hide.argtypes = [H]
    lib.mbink_set_fullscreen.restype = c_int
    lib.mbink_set_fullscreen.argtypes = [H, c_bool]
    lib.mbink_set_resizable.restype = c_int
    lib.mbink_set_resizable.argtypes = [H, c_bool]
    lib.mbink_set_borderless.restype = c_int
    lib.mbink_set_borderless.argtypes = [H, c_bool]
    lib.mbink_set_always_on_top.restype = c_int
    lib.mbink_set_always_on_top.argtypes = [H, c_bool]

    # UI 加载
    lib.mbink_load_html.restype = c_int
    lib.mbink_load_html.argtypes = [H, c_char_p]
    lib.mbink_load_html_file.restype = c_int
    lib.mbink_load_html_file.argtypes = [H, c_char_p]
    lib.mbink_eval_js.restype = c_int
    lib.mbink_eval_js.argtypes = [H, c_char_p]
    lib.mbink_eval_module.restype = c_int
    lib.mbink_eval_module.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_load_js_file.restype = c_int
    lib.mbink_load_js_file.argtypes = [H, c_char_p]
    lib.mbink_load_bytecode.restype = c_int
    lib.mbink_load_bytecode.argtypes = [H, c_void_p, c_size_t]
    lib.mbink_compile_resources.restype = c_int
    lib.mbink_compile_resources.argtypes = [c_char_p, c_char_p, c_char_p]
    lib.mbink_load_resource_file.restype = c_int
    lib.mbink_load_resource_file.argtypes = [
        c_char_p, c_char_p, c_char_p,
        POINTER(c_void_p), POINTER(c_size_t), POINTER(c_uint32)
    ]
    lib.mbink_mount_resource_package.restype = c_int
    lib.mbink_mount_resource_package.argtypes = [H, c_char_p, c_char_p, c_char_p]

    # 函数绑定
    lib.mbink_bind.restype = c_int
    lib.mbink_bind.argtypes = [H, c_char_p, MBinkCallback, c_void_p]
    lib.mbink_unbind.restype = None
    lib.mbink_unbind.argtypes = [H, c_char_p]

    # 事件回调
    lib.mbink_on_resize.restype = c_int
    lib.mbink_on_resize.argtypes = [H, MBinkResizeCallback, c_void_p]
    lib.mbink_on_close_request.restype = c_int
    lib.mbink_on_close_request.argtypes = [H, MBinkBoolCallback, c_void_p]
    lib.mbink_on_close.restype = c_int
    lib.mbink_on_close.argtypes = [H, MBinkVoidCallback, c_void_p]
    lib.mbink_on_focus.restype = c_int
    lib.mbink_on_focus.argtypes = [H, MBinkVoidCallback, c_void_p]
    lib.mbink_on_blur.restype = c_int
    lib.mbink_on_blur.argtypes = [H, MBinkVoidCallback, c_void_p]
    lib.mbink_on_update.restype = c_int
    lib.mbink_on_update.argtypes = [H, MBinkUpdateCallback, c_void_p]

    # 事件发送
    lib.mbink_emit.restype = c_int
    lib.mbink_emit.argtypes = [H, c_char_p, c_char_p]

    lib.mbink_observe_set_callback.restype = c_int
    lib.mbink_observe_set_callback.argtypes = [H, MBinkObserveCallback, c_void_p]
    lib.mbink_observe_console_json.restype = c_int
    lib.mbink_observe_console_json.argtypes = [H, POINTER(c_void_p)]
    lib.mbink_observe_errors_json.restype = c_int
    lib.mbink_observe_errors_json.argtypes = [H, POINTER(c_void_p)]
    lib.mbink_observe_lifecycle_json.restype = c_int
    lib.mbink_observe_lifecycle_json.argtypes = [H, POINTER(c_void_p)]
    lib.mbink_observe_clear.restype = c_int
    lib.mbink_observe_clear.argtypes = [H, c_int]

    # 状态创建
    lib.mbink_state_create_null.restype = c_int
    lib.mbink_state_create_null.argtypes = [H, c_char_p]
    lib.mbink_state_create_bool.restype = c_int
    lib.mbink_state_create_bool.argtypes = [H, c_char_p, c_bool]
    lib.mbink_state_create_int.restype = c_int
    lib.mbink_state_create_int.argtypes = [H, c_char_p, c_int64]
    lib.mbink_state_create_double.restype = c_int
    lib.mbink_state_create_double.argtypes = [H, c_char_p, c_double]
    lib.mbink_state_create_string.restype = c_int
    lib.mbink_state_create_string.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_create_array.restype = c_int
    lib.mbink_state_create_array.argtypes = [H, c_char_p]
    lib.mbink_state_create_object.restype = c_int
    lib.mbink_state_create_object.argtypes = [H, c_char_p]
    lib.mbink_state_create_json.restype = c_int
    lib.mbink_state_create_json.argtypes = [H, c_char_p, c_char_p]

    # 状态查询
    lib.mbink_state_exists.restype = c_bool
    lib.mbink_state_exists.argtypes = [H, c_char_p]
    lib.mbink_state_type.restype = c_int
    lib.mbink_state_type.argtypes = [H, c_char_p]
    lib.mbink_state_delete.restype = None
    lib.mbink_state_delete.argtypes = [H, c_char_p]

    # 状态读取
    lib.mbink_state_get_bool.restype = c_bool
    lib.mbink_state_get_bool.argtypes = [H, c_char_p]
    lib.mbink_state_get_int.restype = c_int64
    lib.mbink_state_get_int.argtypes = [H, c_char_p]
    lib.mbink_state_get_double.restype = c_double
    lib.mbink_state_get_double.argtypes = [H, c_char_p]
    lib.mbink_state_get_string.restype = c_char_p
    lib.mbink_state_get_string.argtypes = [H, c_char_p]
    lib.mbink_state_get_length.restype = c_int
    lib.mbink_state_get_length.argtypes = [H, c_char_p]
    lib.mbink_state_get_json.restype = c_char_p
    lib.mbink_state_get_json.argtypes = [H, c_char_p]
    lib.mbink_state_get_at.restype = c_char_p
    lib.mbink_state_get_at.argtypes = [H, c_char_p, c_int]
    lib.mbink_state_get_key.restype = c_char_p
    lib.mbink_state_get_key.argtypes = [H, c_char_p, c_char_p]

    # 状态写入
    lib.mbink_state_set_null.restype = c_int
    lib.mbink_state_set_null.argtypes = [H, c_char_p]
    lib.mbink_state_set_bool.restype = c_int
    lib.mbink_state_set_bool.argtypes = [H, c_char_p, c_bool]
    lib.mbink_state_set_int.restype = c_int
    lib.mbink_state_set_int.argtypes = [H, c_char_p, c_int64]
    lib.mbink_state_set_double.restype = c_int
    lib.mbink_state_set_double.argtypes = [H, c_char_p, c_double]
    lib.mbink_state_set_string.restype = c_int
    lib.mbink_state_set_string.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_set_json.restype = c_int
    lib.mbink_state_set_json.argtypes = [H, c_char_p, c_char_p]


    # 数组操作
    lib.mbink_state_array_push.restype = c_int
    lib.mbink_state_array_push.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_array_push_int.restype = c_int
    lib.mbink_state_array_push_int.argtypes = [H, c_char_p, c_int64]
    lib.mbink_state_array_push_double.restype = c_int
    lib.mbink_state_array_push_double.argtypes = [H, c_char_p, c_double]
    lib.mbink_state_array_push_string.restype = c_int
    lib.mbink_state_array_push_string.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_array_push_bool.restype = c_int
    lib.mbink_state_array_push_bool.argtypes = [H, c_char_p, c_bool]
    lib.mbink_state_array_pop.restype = c_int
    lib.mbink_state_array_pop.argtypes = [H, c_char_p]
    lib.mbink_state_array_shift.restype = c_int
    lib.mbink_state_array_shift.argtypes = [H, c_char_p]
    lib.mbink_state_array_unshift.restype = c_int
    lib.mbink_state_array_unshift.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_array_remove.restype = c_int
    lib.mbink_state_array_remove.argtypes = [H, c_char_p, c_int]
    lib.mbink_state_array_clear.restype = c_int
    lib.mbink_state_array_clear.argtypes = [H, c_char_p]
    lib.mbink_state_array_set.restype = c_int
    lib.mbink_state_array_set.argtypes = [H, c_char_p, c_int, c_char_p]
    lib.mbink_state_array_set_int.restype = c_int
    lib.mbink_state_array_set_int.argtypes = [H, c_char_p, c_int, c_int64]
    lib.mbink_state_array_set_double.restype = c_int
    lib.mbink_state_array_set_double.argtypes = [H, c_char_p, c_int, c_double]
    lib.mbink_state_array_set_string.restype = c_int
    lib.mbink_state_array_set_string.argtypes = [H, c_char_p, c_int, c_char_p]

    # 对象操作
    lib.mbink_state_object_set.restype = c_int
    lib.mbink_state_object_set.argtypes = [H, c_char_p, c_char_p, c_char_p]
    lib.mbink_state_object_set_int.restype = c_int
    lib.mbink_state_object_set_int.argtypes = [H, c_char_p, c_char_p, c_int64]
    lib.mbink_state_object_set_double.restype = c_int
    lib.mbink_state_object_set_double.argtypes = [H, c_char_p, c_char_p, c_double]
    lib.mbink_state_object_set_string.restype = c_int
    lib.mbink_state_object_set_string.argtypes = [H, c_char_p, c_char_p, c_char_p]
    lib.mbink_state_object_set_bool.restype = c_int
    lib.mbink_state_object_set_bool.argtypes = [H, c_char_p, c_char_p, c_bool]
    lib.mbink_state_object_remove.restype = c_int
    lib.mbink_state_object_remove.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_object_clear.restype = c_int
    lib.mbink_state_object_clear.argtypes = [H, c_char_p]

    # 数值/字符串操作
    lib.mbink_state_increment.restype = c_int
    lib.mbink_state_increment.argtypes = [H, c_char_p, c_double]
    lib.mbink_state_multiply.restype = c_int
    lib.mbink_state_multiply.argtypes = [H, c_char_p, c_double]
    lib.mbink_state_string_append.restype = c_int
    lib.mbink_state_string_append.argtypes = [H, c_char_p, c_char_p]
    lib.mbink_state_string_prepend.restype = c_int
    lib.mbink_state_string_prepend.argtypes = [H, c_char_p, c_char_p]

    # 监听
    lib.mbink_state_watch.restype = c_int
    lib.mbink_state_watch.argtypes = [H, c_char_p, MBinkStateCallback, c_void_p]
    lib.mbink_state_unwatch.restype = None
    lib.mbink_state_unwatch.argtypes = [H, c_int]

    # 批量/队列
    lib.mbink_state_batch_begin.restype = None
    lib.mbink_state_batch_begin.argtypes = [H]
    lib.mbink_state_batch_end.restype = None
    lib.mbink_state_batch_end.argtypes = [H]
    lib.mbink_state_set_merge_mode.restype = None
    lib.mbink_state_set_merge_mode.argtypes = [H, c_bool]
    lib.mbink_process_queue.restype = c_int
    lib.mbink_process_queue.argtypes = [H]
    lib.mbink_queue_size.restype = c_int
    lib.mbink_queue_size.argtypes = [H]

    # 工具
    lib.mbink_free.restype = None
    lib.mbink_free.argtypes = [c_void_p]
    lib.mbink_copy_string.restype = c_void_p
    lib.mbink_copy_string.argtypes = [c_char_p]

    lib.mbink_last_error.restype = c_char_p
    lib.mbink_last_error.argtypes = []

    # ========== 共享 C 对象 (SharedObject) ==========
    SH = c_void_p  # MBinkSharedHandle

    lib.mbink_shared_create.restype = SH
    lib.mbink_shared_create.argtypes = [H, c_char_p]
    lib.mbink_shared_destroy.restype = None
    lib.mbink_shared_destroy.argtypes = [SH]

    # setter
    lib.mbink_shared_set_int.restype = c_int
    lib.mbink_shared_set_int.argtypes = [SH, c_char_p, c_int64]
    lib.mbink_shared_set_double.restype = c_int
    lib.mbink_shared_set_double.argtypes = [SH, c_char_p, c_double]
    lib.mbink_shared_set_string.restype = c_int
    lib.mbink_shared_set_string.argtypes = [SH, c_char_p, c_char_p]
    lib.mbink_shared_set_bool.restype = c_int
    lib.mbink_shared_set_bool.argtypes = [SH, c_char_p, c_bool]
    lib.mbink_shared_set_null.restype = c_int
    lib.mbink_shared_set_null.argtypes = [SH, c_char_p]
    lib.mbink_shared_set_json.restype = c_int
    lib.mbink_shared_set_json.argtypes = [SH, c_char_p, c_char_p]

    # getter
    lib.mbink_shared_get_int.restype = c_int64
    lib.mbink_shared_get_int.argtypes = [SH, c_char_p]
    lib.mbink_shared_get_double.restype = c_double
    lib.mbink_shared_get_double.argtypes = [SH, c_char_p]
    lib.mbink_shared_get_string.restype = c_void_p
    lib.mbink_shared_get_string.argtypes = [SH, c_char_p]
    lib.mbink_shared_get_bool.restype = c_bool
    lib.mbink_shared_get_bool.argtypes = [SH, c_char_p]
    lib.mbink_shared_get_json.restype = c_void_p
    lib.mbink_shared_get_json.argtypes = [SH, c_char_p]

    # 属性查询
    lib.mbink_shared_get_type.restype = c_int
    lib.mbink_shared_get_type.argtypes = [SH, c_char_p]
    lib.mbink_shared_delete.restype = c_int
    lib.mbink_shared_delete.argtypes = [SH, c_char_p]
    lib.mbink_shared_has.restype = c_bool
    lib.mbink_shared_has.argtypes = [SH, c_char_p]

    # 批量更新
    lib.mbink_shared_batch_begin.restype = None
    lib.mbink_shared_batch_begin.argtypes = [SH]
    lib.mbink_shared_batch_end.restype = None
    lib.mbink_shared_batch_end.argtypes = [SH]

    # 原生 UI 对象句柄
    LV = c_void_p
    TM = c_void_p

    lib.mbink_logview_get.restype = LV
    lib.mbink_logview_get.argtypes = [H, c_char_p]
    lib.mbink_logview_destroy.restype = None
    lib.mbink_logview_destroy.argtypes = [LV]
    lib.mbink_logview_append.restype = c_int
    lib.mbink_logview_append.argtypes = [LV, c_char_p, c_char_p, c_char_p]
    lib.mbink_logview_clear.restype = None
    lib.mbink_logview_clear.argtypes = [LV]
    lib.mbink_logview_export.restype = c_void_p
    lib.mbink_logview_export.argtypes = [LV, c_char_p]

    lib.mbink_terminal_get.restype = TM
    lib.mbink_terminal_get.argtypes = [H, c_char_p]
    lib.mbink_terminal_destroy.restype = None
    lib.mbink_terminal_destroy.argtypes = [TM]
    lib.mbink_terminal_write.restype = c_int
    lib.mbink_terminal_write.argtypes = [TM, c_char_p]
    lib.mbink_terminal_clear.restype = None
    lib.mbink_terminal_clear.argtypes = [TM]
    lib.mbink_terminal_execute.restype = c_int
    lib.mbink_terminal_execute.argtypes = [TM, c_char_p]
    lib.mbink_terminal_start_shell.restype = c_int
    lib.mbink_terminal_start_shell.argtypes = [TM, c_char_p]
    lib.mbink_terminal_send_input.restype = c_int
    lib.mbink_terminal_send_input.argtypes = [TM, c_char_p]
    lib.mbink_terminal_resize.restype = None
    lib.mbink_terminal_resize.argtypes = [TM, c_int, c_int]
    lib.mbink_terminal_serialize.restype = c_void_p
    lib.mbink_terminal_serialize.argtypes = [TM]
