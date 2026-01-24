"""
测试 Python 绑定是否编译成功
"""

import sys
import os

# 添加 lightui 模块路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'lightui'))

try:
    import lightui_core
    print("✅ lightui_core 模块加载成功")
    print(f"   版本: {lightui_core.version()}")
    print()
    
    # 测试 EventLoop 是否有 set_window 方法
    print("检查 EventLoop 类...")
    el = lightui_core.EventLoop(None)
    if hasattr(el, 'set_window'):
        print("✅ EventLoop.set_window() 方法存在")
    else:
        print("❌ EventLoop.set_window() 方法不存在")
    
    if hasattr(el, 'set_quickjs_runtime'):
        print("✅ EventLoop.set_quickjs_runtime() 方法存在")
    else:
        print("❌ EventLoop.set_quickjs_runtime() 方法不存在")
    print()
    
    # 测试 Window 是否有新方法
    print("检查 Window 类...")
    win = lightui_core.Window("Test", 800, 600, True)
    if hasattr(win, 'needs_repaint'):
        print("✅ Window.needs_repaint() 方法存在")
    else:
        print("❌ Window.needs_repaint() 方法不存在")
    
    if hasattr(win, 'mark_dirty'):
        print("✅ Window.mark_dirty() 方法存在")
    else:
        print("❌ Window.mark_dirty() 方法不存在")
    
    if hasattr(win, 'get_document_ptr'):
        print("✅ Window.get_document_ptr() 方法存在")
    else:
        print("❌ Window.get_document_ptr() 方法不存在")
    print()
    
    # 测试 Runtime 是否有新方法
    print("检查 Runtime 类...")
    rt = lightui_core.Runtime()
    if hasattr(rt, 'register_module'):
        print("✅ Runtime.register_module() 方法存在")
    else:
        print("❌ Runtime.register_module() 方法不存在")
    
    if hasattr(rt, 'set_base_module_path'):
        print("✅ Runtime.set_base_module_path() 方法存在")
    else:
        print("❌ Runtime.set_base_module_path() 方法不存在")
    
    if hasattr(rt, 'init_fetch_bindings'):
        print("✅ Runtime.init_fetch_bindings() 方法存在")
    else:
        print("❌ Runtime.init_fetch_bindings() 方法不存在")
    print()
    
    # 测试 DevToolsManager 是否存在
    print("检查 DevToolsManager 类...")
    if hasattr(lightui_core, 'DevToolsManager'):
        print("✅ DevToolsManager 类存在")
        dtm = lightui_core.DevToolsManager.get_instance()
        print(f"✅ DevToolsManager.get_instance() 成功: {dtm}")
    else:
        print("❌ DevToolsManager 类不存在")
    print()
    
    # 测试全局函数
    print("检查全局函数...")
    if hasattr(lightui_core, 'set_image_base_path'):
        print("✅ set_image_base_path() 函数存在")
    else:
        print("❌ set_image_base_path() 函数不存在")
    
    if hasattr(lightui_core, 'clear_font_cache'):
        print("✅ clear_font_cache() 函数存在")
    else:
        print("❌ clear_font_cache() 函数不存在")
    
    if hasattr(lightui_core, 'cleanup_dom_bindings'):
        print("✅ cleanup_dom_bindings() 函数存在")
    else:
        print("❌ cleanup_dom_bindings() 函数不存在")
    print()
    
    print("=" * 50)
    print("编译测试完成！")
    print("=" * 50)
    
except ImportError as e:
    print(f"❌ 无法加载 lightui_core 模块: {e}")
    print()
    print("可能的原因:")
    print("1. Python 绑定尚未编译")
    print("2. 编译失败")
    print("3. lightui_core.pyd 文件不在正确的位置")
    print()
    print("请运行以下命令编译:")
    print("  cmake --build build --config Release --target lightui_core")
    sys.exit(1)
except Exception as e:
    print(f"❌ 测试过程中出错: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

