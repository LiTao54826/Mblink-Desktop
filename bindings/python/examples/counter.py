"""
LightUI Python Binding - 计数器示例

展示 Python ↔ JS 双向通信：
- Python 管理状态
- JS 渲染 UI
- 点击按钮调用 Python 函数
"""

import sys
import os

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI 计数器示例")
    print("=" * 50)
    
    with LightUIApp("Counter", 400, 300) as app:
        # 创建计数器状态
        counter = app.state("counter", 0)
        
        # 绑定 Python 函数供 JS 调用
        @app.bind("increment")
        def increment(args):
            counter.increment()
            return counter.get()
        
        @app.bind("decrement")
        def decrement(args):
            counter.increment(-1)
            return counter.get()
        
        @app.bind("reset")
        def reset(args):
            counter.set(0)
            return counter.get()
        
        @app.bind("getCount")
        def get_count(args):
            return counter.get()
        
        # 加载 UI
        app.load_html('''
            <!DOCTYPE html>
            <html>
            <head>
                <style>
                    body {
                        font-family: 'Segoe UI', Arial, sans-serif;
                        display: flex;
                        flex-direction: column;
                        justify-content: center;
                        align-items: center;
                        height: 100vh;
                        margin: 0;
                        background: #f0f0f0;
                    }
                    .counter {
                        font-size: 72px;
                        font-weight: bold;
                        color: #333;
                        margin: 20px 0;
                    }
                    .buttons {
                        display: flex;
                        gap: 10px;
                    }
                    button {
                        font-size: 24px;
                        padding: 10px 20px;
                        border: none;
                        border-radius: 8px;
                        cursor: pointer;
                        transition: transform 0.1s;
                    }
                    button:hover {
                        transform: scale(1.05);
                    }
                    button:active {
                        transform: scale(0.95);
                    }
                    .increment {
                        background: #4CAF50;
                        color: white;
                    }
                    .decrement {
                        background: #f44336;
                        color: white;
                    }
                    .reset {
                        background: #2196F3;
                        color: white;
                    }
                </style>
            </head>
            <body>
                <h1>Counter</h1>
                <div class="counter" id="count">0</div>
                <div class="buttons">
                    <button class="decrement" onclick="decrement()">-</button>
                    <button class="reset" onclick="reset()">Reset</button>
                    <button class="increment" onclick="increment()">+</button>
                </div>
                
                <script>
                    function updateDisplay(value) {
                        document.getElementById('count').textContent = value;
                    }
                    
                    function increment() {
                        const value = host.call('increment', null);
                        updateDisplay(value);
                    }
                    
                    function decrement() {
                        const value = host.call('decrement', null);
                        updateDisplay(value);
                    }
                    
                    function reset() {
                        const value = host.call('reset', null);
                        updateDisplay(value);
                    }
                    
                    // 初始化显示
                    updateDisplay(host.call('getCount', null));
                </script>
            </body>
            </html>
        ''')
        
        print("计数器应用已创建")
        print("- 点击 + 增加计数")
        print("- 点击 - 减少计数")
        print("- 点击 Reset 重置")
        
        # 演示：通过 Python 修改状态
        print("\n演示 Python 端操作：")
        print(f"初始值: {counter.get()}")
        
        counter.increment(5)
        print(f"increment(5) 后: {counter.get()}")
        
        counter.multiply(2)
        print(f"multiply(2) 后: {counter.get()}")
        
        # 运行事件循环
        # app.run()  # 取消注释以运行窗口


if __name__ == "__main__":
    main()
