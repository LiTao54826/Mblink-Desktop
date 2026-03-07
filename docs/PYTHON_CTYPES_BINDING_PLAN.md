# Python ctypes 绑定现行方案与完善计划

## 1. 当前真实架构

LightUI 当前 Python 绑定的真实生效路径为：

```text
bindings/python/lightui/__init__.py
  -> bindings/python/lightui/app.py
  -> bindings/python/lightui/_ffi.py
  -> lightui.dll / liblightui.so
  -> core/api/lightui.h + core/api/lightui.cpp
```

这意味着当前官方 Python 方案是：

- **C ABI 统一入口**
- **Python 使用 ctypes 调用动态库**
- **pybind11 仅为历史遗留参考，不再作为当前方案依据**

## 2. 设计判断

当前方案的大方向是合理的：

1. `lightui.h` 作为跨语言统一 ABI 边界
2. Python 层保持薄封装，复杂性下沉到核心库
3. SharedObject 支持 Python/JS 共享同一个 QuickJS 对象
4. 回调字符串通过 `lightui_copy_string()` / `lightui_free()` 明确所有权

但工程上仍需收敛：

- 生命周期保护不足
- 部分 Python API 语义不一致
- FFI 细节仍有边界风险
- `state()` / `shared()` 的定位需要明确说明

## 3. 现阶段推荐定位

### 3.1 App

`App` 是当前 Python 绑定的统一入口，负责：

- 动态库加载
- 窗口创建与销毁
- HTML / JS 加载
- Python 回调绑定
- SharedObject 管理

### 3.2 state 与 shared

当前体系允许两套数据通道并存：

- `state()`：底层状态 API，适合结构化状态、批量操作、历史兼容
- `shared()`：更适合 UI 数据直连与 Python/JS 共享对象场景

**当前建议：**
- 面向 UI 联动优先使用 `shared()`
- 面向底层状态控制、批处理和兼容场景保留 `state()`

## 4. 本轮完善重点

### 4.1 生命周期与资源管理

目标：让 `App` 的资源销毁过程可重复调用且安全。

需要保证：

- cleanup 幂等
- 不重复 destroy 同一个 native handle
- 销毁后不能继续误用已失效句柄
- `run()` / `atexit` / 手动清理不产生 double free

### 4.2 Python API 一致性

目标：让高层接口更符合 Python 用户预期。

重点包括：

- `title` getter 返回 `str` 而不是 `bytes`
- 对已销毁对象调用时给出明确错误
- 高层接口行为保持一致

### 4.3 ctypes FFI 边界

目标：让 ABI 调用边界更稳。

重点包括：

- 显式导入 `ctypes.util`
- 检查字符串返回值所有权
- 确保 Python 侧与 C 侧 free 契约一致

## 5. 后续执行顺序

1. 修复 `bindings/python/lightui/app.py`
2. 修复 `bindings/python/lightui/_ffi.py`
3. 必要时补充注释，说明 `state/shared` 定位
4. 编译动态库
5. 实际运行当前 Python 绑定进行验证

## 6. 约束

- 不再恢复旧 pybind11 方案作为当前实现路径
- 不新增测试脚本
- 保持当前 ctypes 绑定主线不变
- 优先做收敛与稳定性修复，而不是重写架构

