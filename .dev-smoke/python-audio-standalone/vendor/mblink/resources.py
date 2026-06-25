"""MBink 资源包 Python 封装。"""

import ctypes
from ._ffi import load_dll

RESOURCE_FLAG_BYTECODE = 1


def _as_bytes(value: str) -> bytes:
    return value.encode("utf-8") if value else b""


def _raise_last_error(lib, prefix: str):
    err = lib.mbink_last_error()
    msg = err.decode("utf-8", "ignore") if err else "unknown error"
    raise RuntimeError(f"{prefix}: {msg}")


def compile_resources(input_path: str, output_file: str, encryption_key: str = "", *, dll_path=None):
    lib = load_dll(dll_path)
    rc = lib.mbink_compile_resources(
        _as_bytes(input_path),
        _as_bytes(output_file),
        _as_bytes(encryption_key),
    )
    if rc != 0:
        _raise_last_error(lib, "mbink_compile_resources 失败")



def load_resource_file(package_file: str, resource_path: str, encryption_key: str = "", *, dll_path=None):
    lib = load_dll(dll_path)
    out_data = ctypes.c_void_p()
    out_size = ctypes.c_size_t()
    out_flags = ctypes.c_uint32()
    rc = lib.mbink_load_resource_file(
        _as_bytes(package_file),
        _as_bytes(resource_path),
        _as_bytes(encryption_key),
        ctypes.byref(out_data),
        ctypes.byref(out_size),
        ctypes.byref(out_flags),
    )
    if rc != 0:
        _raise_last_error(lib, "mbink_load_resource_file 失败")
    try:
        if not out_data.value or out_size.value == 0:
            return b"", int(out_flags.value)
        data = ctypes.string_at(out_data.value, out_size.value)
        return data, int(out_flags.value)
    finally:
        if out_data.value:
            lib.mbink_free(out_data)


def mount_resource_package(app, package_file: str, encryption_key: str = "", mount_point: str = "/"):
    return app.mount_resource_package(package_file, encryption_key, mount_point)
