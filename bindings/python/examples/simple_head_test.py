"""
最简单的测试 - 不使用事件循环
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

print("开始测试...")

try:
    import lightui_core as core
    print("✓ 导入 lightui_core 成功")
    
    # 创建运行时
    runtime = core.Runtime()
    print("✓ 创建 Runtime 成功")
    
    # 创建文档
    window = core.Window("Test", 400, 300, True)  # headless
    print("✓ 创建 Window 成功")
    
    # 设置 JS 运行时
    window.set_js_runtime(runtime)
    print("✓ 设置 JS Runtime 成功")
    
    # 获取文档
    doc = window.document
    print(f"✓ 获取 Document 成功: {doc}")
    
    # 加载 HTML
    html = """<!DOCTYPE html>
<html>
<head><title>Test</title></head>
<body><h1>Test</h1></body>
</html>"""
    
    success = doc.load_html(html)
    print(f"✓ 加载 HTML: {success}")
    
    # 测试 document.head
    print("\n测试 document.head...")
    try:
        result = runtime.eval("""
            (function() {
                console.log('Testing document.head...');
                console.log('document:', document);
                console.log('document.head:', document.head);
                console.log('document.body:', document.body);
                
                return {
                    hasHead: !!document.head,
                    hasBody: !!document.body,
                    headIsNull: document.head === null,
                    headIsUndefined: document.head === undefined,
                    bodyIsNull: document.body === null,
                    bodyIsUndefined: document.body === undefined
                };
            })();
        """)
        
        print(f"\n结果: {result}")
        print(f"  - document.head 存在: {result.get('hasHead')}")
        print(f"  - document.body 存在: {result.get('hasBody')}")
        print(f"  - document.head === null: {result.get('headIsNull')}")
        print(f"  - document.head === undefined: {result.get('headIsUndefined')}")
        print(f"  - document.body === null: {result.get('bodyIsNull')}")
        print(f"  - document.body === undefined: {result.get('bodyIsUndefined')}")
        
        if result.get('hasHead'):
            print("\n✅ document.head 工作正常！")
        else:
            print("\n❌ document.head 不存在！")
            if result.get('headIsNull'):
                print("   原因: document.head 是 null (Document::GetHead() 返回 nullptr)")
            elif result.get('headIsUndefined'):
                print("   原因: document.head 是 undefined (DOM 绑定中没有 'head' 属性)")
        
    except Exception as e:
        print(f"\n❌ JavaScript 执行失败: {e}")
        import traceback
        traceback.print_exc()
    
    print("\n测试完成！")
    
except Exception as e:
    print(f"\n❌ 测试失败: {e}")
    import traceback
    traceback.print_exc()

print("\n程序退出")

