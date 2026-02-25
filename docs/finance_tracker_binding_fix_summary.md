# Finance Tracker 按钮闪退（Python 绑定）长期修复总结

## 问题现象
- 点击“收入 / 支出 / 其他会触发 `py.xxx()` 的按钮”后应用直接闪退。
- Python 侧无可捕获异常。

## 根因
跨语言回调返回值的内存所有权不一致：

- Python 绑定原来使用：`LightUICallback = CFUNCTYPE(c_char_p, c_char_p, c_void_p)`，回调返回 `ret.encode()`。
- C++ 侧把回调返回 `char*` 当成稳定内存使用。
- 该指针在 ctypes 边界可能是临时/不可控生命周期，导致 native 崩溃。

同时，C API 注释与实现存在契约不一致：
- 头文件写“调用者 free”；
- 实现中却“不 free”。

## 长期修复方案（已落地）
统一 ABI 契约为：
1. 回调返回 `char*` 必须来自 LightUI 运行时可释放内存；
2. C 核心在消费后负责 `lightui_free()`；
3. Python 绑定通过 `lightui_copy_string()` 分配返回字符串，避免 ctypes 临时指针问题。

## 实际改动

### 1) `core/api/lightui.h`
- 更新 `LightUICallback` 注释：返回值由 LightUI 运行时释放。
- 新增 API：
  - `LIGHTUI_API char* lightui_copy_string(const char* str);`

### 2) `core/api/lightui.cpp`
- 新增 `duplicateString(const char* str)`。
- `lightui_bind(...)` 中的 host 回调改为：
  - `std::string ret(result);`
  - `lightui_free(result);`
- 新增导出实现：
  - `char* lightui_copy_string(const char* str) { return duplicateString(str); }`
- 新增原生错误统一上报函数 `reportNativeError(...)`：
  - 写入 `lightui_last_error`
  - 输出到 `stderr`
  - Windows 输出到 `OutputDebugStringA`
  - 追加写入 `lightui_native_error.log`
- 在 `lightui_bind(...)` 的回调调用路径新增异常兜底：
  - Windows 使用 `__try/__except` 捕获 SEH
  - 其他平台使用 `try/catch` 捕获 C++ 异常
  - 出错时返回 JSON error，避免静默闪退
- 注册全局未处理异常过滤器（Windows）：
  - `SetUnhandledExceptionFilter(lightuiUnhandledExceptionFilter)`
  - 尝试在进程级未捕获崩溃时落盘错误信息

### 3) `bindings/python/lightui/_ffi.py`
- 回调类型由：
  - `CFUNCTYPE(c_char_p, c_char_p, c_void_p)`
  改为：
  - `CFUNCTYPE(c_void_p, c_char_p, c_void_p)`
- 绑定 `lightui_copy_string` 签名：
  - `restype = c_void_p`
  - `argtypes = [c_char_p]`

### 4) `bindings/python/lightui/app.py`
- `@app.bind` 回调返回改为：
  - `return self._lib.lightui_copy_string(ret.encode("utf-8"))`
  - 异常分支同样使用 `lightui_copy_string(...)`


### 5) `bindings/python/lightui/shared.py`
- 修复 `SharedState.__getattr__` 的 native 字符串读取方式：
  - 以前：把 `c_char_p` 结果直接 `decode()` 后再 `lightui_free(raw)`
  - 现在：配合 `c_void_p`，改为 `ctypes.string_at(raw).decode('utf-8')` 后再 `lightui_free(raw)`
- 这样可以确保 `lightui_free` 释放的是**真实 native 指针**，避免对 ctypes 转换对象误释放。

### 6) `bindings/python/lightui/_ffi.py`（补充）
- 修复 shared getter 返回类型：
  - `lightui_shared_get_string.restype: c_char_p -> c_void_p`
  - `lightui_shared_get_json.restype: c_char_p -> c_void_p`
- 与 `shared.py` 的 `lightui_free` 形成正确配对，修复 `_push_all` 路径下高概率崩溃点。


### 7) 全量“同类风险”排查与修复
- 排查范围：`bindings/python/lightui/*.py` 全部 ctypes 绑定与字符串读取路径。
- 已确认并修复的同类崩溃风险：
  - `lightui_shared_get_string/get_json` 的 `restype` 与 `lightui_free` 释放契约不匹配（已修复为 `c_void_p + ctypes.string_at + lightui_free`）。
- 已确认无同类问题的路径：
  - `app.py` 中 `lightui_last_error()` 仅做 `decode`，不参与 `lightui_free`，契约安全。
  - 其余 `c_char_p` 返回值当前未与 `lightui_free` 组合使用，未发现同类非法释放。
- 额外稳态结论：
  - 目前 Python 侧仅在 `shared.py` 两处执行 `lightui_free`，均已改为释放真实 native 指针。
  - `_push_all()` 触发链路（`getattr -> shared_get_*`）已与 C 侧内存所有权契约一致。

## 预期效果
- `py.xxx()` 回调返回字符串的生命周期稳定。
- 消除按钮触发时的 native 指针悬空风险。
- 修复“点击即闪退且无 Python 异常”的核心问题。

## 说明
- 本次未新增测试脚本（按要求）。
- 已完成代码级长期修复实现；需编译后运行示例验证。
