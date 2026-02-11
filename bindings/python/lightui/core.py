"""
LightUI 低级 API — 直接访问 C++ 绑定

提供对 lightui_core C++ 扩展模块的直接访问。
适用于需要精细控制底层组件的高级用户。

常规开发请使用高级 API：
    import lightui as ui
    app = ui.App("My App", 800, 600)

低级 API 使用示例：
    from lightui.core import Window, Runtime, HostBridge
    window = Window("Test", 800, 600)
    runtime = Runtime()
"""

import sys
import os

# 确保 pyd 所在的 bin 目录在搜索路径中
_bin_path = os.path.join(os.path.dirname(__file__), 'bin')
if _bin_path not in sys.path:
    sys.path.insert(0, _bin_path)

try:
    import lightui_core
except ImportError as e:
    raise ImportError(
        f"无法导入 lightui_core。请确保已构建 pyd 并放置在 lightui/bin/ 目录下。\n"
        f"构建方法：cmake --build build --config Release --target lightui_core\n"
        f"原始错误：{e}"
    )

# 重导出所有低级类
Window = lightui_core.Window
Document = lightui_core.Document
Element = lightui_core.Element
Runtime = lightui_core.Runtime
EventLoop = lightui_core.EventLoop
HostBridge = lightui_core.HostBridge
DevToolsManager = lightui_core.DevToolsManager
TaskScheduler = lightui_core.TaskScheduler

# C++ App（用 CoreApp 区分，避免与高级 API 的 App 混淆）
CoreApp = lightui_core.App

# 状态类
State = lightui_core.State
IntState = lightui_core.IntState
StringState = lightui_core.StringState
ListState = lightui_core.ListState
DictState = lightui_core.DictState

# 辅助类
BatchContext = lightui_core.BatchContext

# 函数
version = lightui_core.version
cleanup_dom_bindings = lightui_core.cleanup_dom_bindings
clear_font_cache = lightui_core.clear_font_cache
set_image_base_path = lightui_core.set_image_base_path

__all__ = [
    "Window", "Document", "Element", "Runtime", "EventLoop",
    "HostBridge", "DevToolsManager", "TaskScheduler",
    "CoreApp", "State", "IntState", "StringState", "ListState", "DictState",
    "BatchContext", "version", "cleanup_dom_bindings", "clear_font_cache",
    "set_image_base_path",
]
