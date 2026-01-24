"""诊断初始化渲染问题"""
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp

def main():
    print("=" * 50)
    print("诊断初始化渲染问题")
    print("=" * 50)
    
    with LightUIApp("渲染诊断", 600, 400) as app:
        # 简单的 HTML + CSS 测试
        html = """
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body {
                    background: #667eea;
                    margin: 0;
                    padding: 20px;
                }
                .box {
                    background: white;
                    padding: 20px;
                    border-radius: 8px;
                    text-align: center;
                }
                h1 {
                    color: #333;
                    margin: 0;
                }
            </style>
        </head>
        <body>
            <div class="box">
                <h1>Hello World</h1>
                <p>如果看到白色盒子和紫色背景，说明 CSS 正常工作</p>
            </div>
        </body>
        </html>
        """
        
        print("\n[1] 加载 HTML...")
        success = app.load_html(html)
        print(f"    load_html 返回: {success}")
        
        # 检查 document 状态
        doc = app.document
        print(f"\n[2] Document 状态:")
        print(f"    doc.is_valid() = {doc.is_valid()}")
        
        body = doc.body
        print(f"    body = {body}")
        if body:
            print(f"    body.tag_name = {body.tag_name}")
            print(f"    body.children count = {len(body.children)}")
        
        # 检查 head 和 style
        head = doc.head
        print(f"\n[3] Head 状态:")
        print(f"    head = {head}")
        if head:
            print(f"    head.children count = {len(head.children)}")
            for child in head.children:
                print(f"      - {child.tag_name}")
        
        # 强制触发一次渲染
        print("\n[4] 强制渲染...")
        app.window.render()
        
        print("\n[5] 运行事件循环...")
        print("    观察窗口是否正确显示紫色背景和白色盒子")
        print("    如果初始显示不正确，移动鼠标或调整窗口大小后是否恢复")
        
        app.run()

if __name__ == "__main__":
    main()

