import ctypes


class _NativeControl:
    def __init__(self, lib, handle, element_id):
        self._lib = lib
        self._handle = handle
        self._element_id = element_id

    @property
    def element_id(self):
        return self._element_id

    def _read_and_free_utf8(self, ptr):
        if not ptr:
            return ""
        try:
            return ctypes.cast(ptr, ctypes.c_char_p).value.decode("utf-8")
        finally:
            self._lib.mbink_free(ptr)


class LogView(_NativeControl):
    def append(self, level: str, source: str, message: str):
        ret = self._lib.mbink_logview_append(
            self._handle,
            level.encode("utf-8"),
            source.encode("utf-8"),
            message.encode("utf-8"),
        )
        if ret != 0:
            raise RuntimeError("mbink_logview_append failed")
        return self

    def clear(self):
        self._lib.mbink_logview_clear(self._handle)
        return self

    def export(self, format: str = "text"):
        ptr = self._lib.mbink_logview_export(self._handle, format.encode("utf-8"))
        return self._read_and_free_utf8(ptr)


class Terminal(_NativeControl):
    def write(self, data: str):
        ret = self._lib.mbink_terminal_write(self._handle, data.encode("utf-8"))
        if ret != 0:
            raise RuntimeError("mbink_terminal_write failed")
        return self

    def clear(self):
        self._lib.mbink_terminal_clear(self._handle)
        return self

    def execute(self, command: str):
        ret = self._lib.mbink_terminal_execute(self._handle, command.encode("utf-8"))
        if ret != 0:
            raise RuntimeError("mbink_terminal_execute failed")
        return self

    def start_shell(self, shell: str = ""):
        ret = self._lib.mbink_terminal_start_shell(self._handle, shell.encode("utf-8"))
        if ret != 0:
            raise RuntimeError("mbink_terminal_start_shell failed")
        return self

    def send_input(self, data: str):
        ret = self._lib.mbink_terminal_send_input(self._handle, data.encode("utf-8"))
        if ret != 0:
            raise RuntimeError("mbink_terminal_send_input failed")
        return self

    def resize(self, rows: int, cols: int):
        self._lib.mbink_terminal_resize(self._handle, rows, cols)
        return self

    def serialize(self):
        ptr = self._lib.mbink_terminal_serialize(self._handle)
        return self._read_and_free_utf8(ptr)
