"""
LightUI Python Binding - Hello World 示例

最简单的窗口示例，展示基本的窗口创建和 HTML 加载。
"""

import sys
import os

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI Hello World 示例")
    print("=" * 50)
    
    # 创建应用
    with LightUIApp("Hello World", 400, 300) as app:
        # 加载简单的 HTML
        app.load_html('''
            <!DOCTYPE html>
            <html>
            <head>
                <style>
                    body {
                        font-family: Arial, sans-serif;
                        display: flex;
                        justify-content: center;
                        align-items: center;
                        height: 100vh;
                        margin: 0;
                        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                        color: white;
                    }
                    h1 {
                        font-size: 48px;
                        text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
                    }
                </style>
            </head>
            <body>
                <h1>Hello, LightUI!</h1>
            </body>
            </html>
        ''')
        
        print("窗口已创建，按 Ctrl+C 退出")
        
        # 运行事件循环
        # app.run()  # 取消注释以运行窗口
        
        # 演示模式：只显示信息
        print("（演示模式：窗口未实际运行）")


if __name__ == "__main__":
    main()
