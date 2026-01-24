"""
诊断 document.head 问题的根源
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

print("=" * 70)
print("诊断 document.head 问题")
print("=" * 70)

try:
    import lightui_core as core
    
    # 创建组件
    runtime = core.Runtime()
    window = core.Window("Test", 400, 300, True)
    window.set_js_runtime(runtime)
    doc = window.document
    
    # 加载 HTML
    html = """<!DOCTYPE html>
<html>
<head><title>Test</title></head>
<body><h1>Test</h1></body>
</html>"""
    
    doc.load_html(html)
    print("✓ HTML 加载成功")
    
    # 从 C++ 层检查
    print("\n[C++ 层检查]")
    heads = doc.get_elements_by_tag_name("head")
    bodies = doc.get_elements_by_tag_name("body")
    print(f"  getElementsByTagName('head'): {len(heads)} 个元素")
    print(f"  getElementsByTagName('body'): {len(bodies)} 个元素")
    
    if heads:
        print(f"  ✓ <head> 元素存在: {heads[0].tag_name}")
    else:
        print(f"  ❌ <head> 元素不存在！")
    
    if bodies:
        print(f"  ✓ <body> 元素存在: {bodies[0].tag_name}")
    else:
        print(f"  ❌ <body> 元素不存在！")
    
    # 从 JS 层检查
    print("\n[JS 层检查]")
    
    # 测试 1: 检查 'head' 属性是否存在
    has_head_prop = runtime.eval("'head' in document")
    has_body_prop = runtime.eval("'body' in document")
    print(f"  'head' in document: {has_head_prop}")
    print(f"  'body' in document: {has_body_prop}")
    
    if not has_head_prop:
        print("\n❌ 问题诊断: DOM 绑定中没有 'head' 属性！")
        print("   原因: Python 绑定的 DLL 没有更新")
        print("   解决方案: 重新编译 Python 绑定")
        print("   命令: cmake --build build --config Release --target lightui_core")
    else:
        # 测试 2: 检查 document.head 的值
        head_value = runtime.eval("document.head")
        body_value = runtime.eval("document.body")
        print(f"  document.head: {head_value}")
        print(f"  document.body: {body_value}")
        
        if head_value is None:
            print("\n❌ 问题诊断: document.head 是 null！")
            print("   原因: Document::GetHead() 返回 nullptr")
            print("   可能的原因:")
            print("   1. HTML 解析器没有创建 <head> 元素")
            print("   2. Document::SetHead() 没有被调用")
        else:
            print("\n✅ document.head 工作正常！")
    
    print("\n" + "=" * 70)
    
except Exception as e:
    print(f"\n❌ 测试失败: {e}")
    import traceback
    traceback.print_exc()

