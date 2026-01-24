"""
详细诊断：Python 绑定中 document.head/body 问题
"""

import sys
import os

# 添加路径（必须在导入前）
_script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_script_dir, '..'))
sys.path.insert(0, os.path.join(_script_dir, '..', 'lightui', 'bin'))

log_file = os.path.join(_script_dir, 'diagnose_doc_result.log')

def log(msg):
    print(msg)
    with open(log_file, 'a', encoding='utf-8') as f:
        f.write(msg + '\n')

def main():
    with open(log_file, 'w', encoding='utf-8') as f:
        f.write("=== 诊断开始 ===\n\n")

    try:
        import lightui_core as core
        log("✓ 导入 lightui_core")
        
        # 1. 创建 Runtime
        log("\n[1] 创建 Runtime...")
        runtime = core.Runtime()
        log(f"   runtime = {runtime}")
        
        # 2. 创建 Window（内部会创建 Document）
        log("\n[2] 创建 Window...")
        window = core.Window("Test", 800, 600, True)  # headless
        log(f"   window = {window}")
        
        # 3. 获取 Document
        log("\n[3] 获取 Document...")
        doc = window.document
        log(f"   doc = {doc}")
        log(f"   doc.is_valid() = {doc.is_valid() if doc else 'N/A'}")
        
        # 4. 设置 JS Runtime（这会调用 SetGlobalDocument）
        log("\n[4] 设置 JS Runtime...")
        window.set_js_runtime(runtime)
        log("   ✓ set_js_runtime 完成")
        
        # 5. 在 LoadHTML 之前测试 JS
        log("\n[5] LoadHTML 之前测试 JS...")
        try:
            result = runtime.eval("""
                (function() {
                    return {
                        hasDocument: typeof document !== 'undefined',
                        documentType: typeof document,
                        hasHead: document && !!document.head,
                        hasBody: document && !!document.body
                    };
                })();
            """, "<test1>")
            log(f"   结果: {result}")
        except Exception as e:
            log(f"   错误: {e}")
        
        # 6. 加载 HTML
        log("\n[6] 加载 HTML...")
        html = """<!DOCTYPE html>
<html>
<head><title>Test</title></head>
<body><h1>Hello</h1></body>
</html>"""
        success = doc.load_html(html)
        log(f"   load_html 返回: {success}")
        
        # 7. LoadHTML 之后测试 JS
        log("\n[7] LoadHTML 之后测试 JS...")
        try:
            result = runtime.eval("""
                (function() {
                    return {
                        hasDocument: typeof document !== 'undefined',
                        documentType: typeof document,
                        hasHead: document && !!document.head,
                        hasBody: document && !!document.body,
                        headTag: document && document.head ? document.head.tagName : 'N/A',
                        bodyTag: document && document.body ? document.body.tagName : 'N/A'
                    };
                })();
            """, "<test2>")
            log(f"   结果: {result}")
        except Exception as e:
            log(f"   错误: {e}")
        
        # 8. 测试 Python 端的 Document
        log("\n[8] 测试 Python 端 Document...")
        body = doc.body
        log(f"   doc.body = {body}")
        if body:
            log(f"   body.tag_name = {body.tag_name}")
        
        # 9. 重新设置全局 document？
        log("\n[9] 尝试重新获取 document...")
        doc2 = window.document
        log(f"   doc2 = {doc2}")
        log(f"   doc2 is doc: {doc2 is doc}")
        
        # 10. 检查是否需要重新调用 SetGlobalDocument
        log("\n[10] 检查 JS 全局 document 的内部状态...")
        try:
            result = runtime.eval("""
                (function() {
                    // 尝试直接访问 documentElement
                    var info = {
                        hasDocumentElement: !!document.documentElement,
                        childNodes: document.childNodes ? document.childNodes.length : 0
                    };
                    
                    if (document.documentElement) {
                        info.docElementTag = document.documentElement.tagName;
                        info.docElementChildren = document.documentElement.childNodes ? 
                            document.documentElement.childNodes.length : 0;
                    }
                    
                    // 尝试 querySelector
                    try {
                        var h1 = document.querySelector('h1');
                        info.h1Found = !!h1;
                        if (h1) info.h1Text = h1.textContent;
                    } catch(e) {
                        info.querySelectorError = e.message;
                    }
                    
                    return info;
                })();
            """, "<test3>")
            log(f"   结果: {result}")
        except Exception as e:
            log(f"   错误: {e}")
        
        log("\n=== 诊断完成 ===")
        log(f"日志: {log_file}")
        
    except Exception as e:
        import traceback
        log(f"❌ 错误: {e}")
        log(traceback.format_exc())

if __name__ == "__main__":
    main()

