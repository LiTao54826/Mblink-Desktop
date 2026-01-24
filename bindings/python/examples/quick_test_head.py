"""
快速测试 document.head 是否可用
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from lightui import LightUIApp
    
    print("=" * 60)
    print("快速测试 document.head")
    print("=" * 60)
    
    app = LightUIApp("Test", 400, 300, headless=True)
    
    # 加载 HTML
    app.load_html("""<!DOCTYPE html>
<html>
<head><title>Test</title></head>
<body><h1>Test</h1></body>
</html>""")
    
    # 测试
    result = app.load_js("""
        (function() {
            const info = {
                hasHead: !!document.head,
                hasBody: !!document.body,
                headType: typeof document.head,
                bodyType: typeof document.body
            };
            
            console.log('document.head:', document.head);
            console.log('document.body:', document.body);
            
            if (!document.head) {
                throw new Error('document.head is ' + (document.head === null ? 'null' : 'undefined'));
            }
            
            return info;
        })();
    """)
    
    print(f"\n✅ 测试成功!")
    print(f"   - document.head 存在: {result.get('hasHead')}")
    print(f"   - document.body 存在: {result.get('hasBody')}")
    print(f"   - document.head 类型: {result.get('headType')}")
    print(f"   - document.body 类型: {result.get('bodyType')}")
    print("\n🎉 document.head 工作正常！Python 绑定已修复！")
    print("=" * 60)
    
except Exception as e:
    print(f"\n❌ 测试失败: {e}")
    print("\n💡 这说明 Python 绑定的 DLL 还没有更新")
    print("   请运行: cmake --build build --config Release --target lightui_core")
    print("=" * 60)
    import traceback
    traceback.print_exc()

