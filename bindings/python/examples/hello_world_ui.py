"""
LightUI Python Binding - Hello World UI 示例

展示完整的 UI 交互：
- 窗口创建
- HTML/CSS 渲染
- Python ↔ JS 双向通信
- 状态管理
"""

import sys
import os

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI Hello World UI 示例")
    print("=" * 50)
    
    # 创建应用
    with LightUIApp("Hello World", 1200, 800) as app:
        # 创建状态
        message = app.state("message", "Hello, World!")
        click_count = app.state("clickCount", 0)
        
        # 绑定 Python 函数供 JS 调用
        @app.bind("updateMessage")
        def update_message(new_msg):
            """更新消息"""
            print("python端update_message")

            message.set(new_msg)
            return {"success": True, "message": new_msg}
        
        @app.bind("incrementCount")
        def increment_count(arg=None):
            """增加点击计数"""
            print("python端increment_count")

            current = click_count.get()
            click_count.set(current + 1)
            return current + 1
        
        @app.bind("getGreeting")
        def get_greeting(name):
            """获取问候语"""
            print("python端get_greeting, name=", name)
            if name is None:
                name = "朋友"
            return f"你好, {name}! 欢迎使用 LightUI!"
        
        # 加载 HTML UI
        app.load_html('''
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 16px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            text-align: center;
            max-width: 400px;
            width: 100%;
        }
        
        h1 {
            color: #333;
            margin-bottom: 20px;
            font-size: 28px;
        }
        
        .message {
            font-size: 24px;
            color: #667eea;
            margin: 20px 0;
            padding: 15px;
            background: #f0f4ff;
            border-radius: 8px;
        }
        
        .counter {
            font-size: 18px;
            color: #666;
            margin: 15px 0;
        }
        
        .counter span {
            font-weight: bold;
            color: #764ba2;
        }
        
        input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            margin: 10px 0;
            transition: border-color 0.3s;
        }
        
        input:focus {
            outline: none;
            border-color: #667eea;
        }
        
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            margin: 5px;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .btn-secondary {
            background: #6c757d;
        }
        
        .greeting {
            margin-top: 20px;
            padding: 15px;
            background: #e8f5e9;
            border-radius: 8px;
            color: #2e7d32;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎉 LightUI Demo</h1>
        
        <div class="message" id="message">Hello, World!</div>
        
        <div class="counter">
            点击次数: <span id="count">0</span>
        </div>
        
        <input type="text" id="nameInput" placeholder="输入你的名字...">
        
        <div>
            <button onclick="handleSayHello()">打招呼</button>
            <button onclick="handleClickMe()">点击我!</button>
            <button class="btn-secondary" onclick="handleReset()">重置</button>
        </div>
        
        <div class="greeting" id="greeting"></div>
    </div>
    
    <script>
        // 使用 host.call 调用 Python 函数（同步调用）
        // 使用 document API 更新 UI
        
        function handleClickMe() {
            try {
                var count = host.call('incrementCount');
                console.log('Count incremented to: ' + count);
                // 更新 UI
                var countEl = document.getElementById('count');
                if (countEl) {
                    countEl.textContent = count;
                }
            } catch (e) {
                console.log('Error: ' + e);
            }
        }
        
        function handleSayHello() {
            try {
                // 获取输入框的值
                var nameInput = document.getElementById('nameInput');
                var name = (nameInput && nameInput.value) ? nameInput.value : '朋友';
                var greeting = host.call('getGreeting', name);
                console.log('Greeting: ' + greeting);
                // 更新 UI
                var greetingEl = document.getElementById('greeting');
                if (greetingEl) {
                    greetingEl.textContent = greeting;
                }
            } catch (e) {
                console.log('Error: ' + e);
            }
        }
        
        function handleReset() {
            try {
                host.call('updateMessage', 'Hello, World!');
                console.log('Message reset');
                // 更新 UI
                var messageEl = document.getElementById('message');
                if (messageEl) {
                    messageEl.textContent = 'Hello, World!';
                }
                var countEl = document.getElementById('count');
                if (countEl) {
                    countEl.textContent = '0';
                }
                var greetingEl = document.getElementById('greeting');
                if (greetingEl) {
                    greetingEl.textContent = '';
                }
            } catch (e) {
                console.log('Error: ' + e);
            }
        }
        
        console.log('Script loaded successfully!');
    </script>
</body>
</html>
        ''')
        
        print("\n✅ UI 已加载")
        print("📝 功能说明:")
        print("   - 输入名字，点击'打招呼'获取问候语")
        print("   - 点击'点击我!'增加计数")
        print("   - 点击'重置'恢复初始状态")
        print("\n🚀 运行窗口...")
        
        # 运行事件循环
        app.run()


if __name__ == "__main__":
    main()
