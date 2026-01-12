"""
LightUI Python Binding - py 命名空间测试

测试两种调用方式：
1. host.call('funcName', args) - 传统方式
2. py.funcName(args) - 新的简洁方式
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI py 命名空间测试")
    print("=" * 50)
    
    with LightUIApp("py 命名空间测试", 600, 400) as app:
        # 创建状态
        counter = app.state("counter", 0)
        
        # 绑定 Python 函数
        @app.bind("increment")
        def increment(arg=None):
            """增加计数"""
            current = counter.get()
            counter.set(current + 1)
            print(f"[Python] increment called, counter = {current + 1}")
            return current + 1
        
        @app.bind("decrement")
        def decrement(arg=None):
            """减少计数"""
            current = counter.get()
            counter.set(current - 1)
            print(f"[Python] decrement called, counter = {current - 1}")
            return current - 1
        
        @app.bind("getGreeting")
        def get_greeting(name):
            """获取问候语"""
            if name is None:
                name = "World"
            greeting = f"Hello, {name}!"
            print(f"[Python] getGreeting called with name={name}")
            return greeting
        
        @app.bind("calculate")
        def calculate(data):
            """计算"""
            if data is None:
                return 0
            a = data.get('a', 0) if isinstance(data, dict) else 0
            b = data.get('b', 0) if isinstance(data, dict) else 0
            result = a + b
            print(f"[Python] calculate({a}, {b}) = {result}")
            return result
        
        app.load_html('''
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: sans-serif;
            padding: 20px;
            background: #1a1a2e;
            color: white;
        }
        h1 { color: #e94560; }
        .section {
            background: #16213e;
            padding: 15px;
            border-radius: 8px;
            margin: 15px 0;
        }
        .section h3 { 
            color: #4ade80; 
            margin-bottom: 10px;
        }
        button {
            background: #e94560;
            color: white;
            border: none;
            padding: 8px 16px;
            margin: 5px;
            border-radius: 5px;
            cursor: pointer;
        }
        button:hover { background: #d63850; }
        .result {
            background: #0f3460;
            padding: 10px;
            border-radius: 5px;
            margin-top: 10px;
            font-family: monospace;
        }
        code {
            background: #0f3460;
            padding: 2px 6px;
            border-radius: 3px;
            color: #4ade80;
        }
    </style>
</head>
<body>
    <h1>🐍 py 命名空间测试</h1>
    
    <div class="section">
        <h3>方式对比</h3>
        <p>传统方式: <code>host.call('funcName', args)</code></p>
        <p>新方式: <code>py.funcName(args)</code></p>
    </div>
    
    <div class="section">
        <h3>计数器测试</h3>
        <button onclick="testOldWay()">host.call 方式</button>
        <button onclick="testNewWay()">py.funcName 方式</button>
        <button onclick="testDecrement()">py.decrement()</button>
        <div class="result" id="counterResult">计数: 0</div>
    </div>
    
    <div class="section">
        <h3>带参数调用</h3>
        <button onclick="testGreeting()">py.getGreeting("LightUI")</button>
        <button onclick="testCalculate()">py.calculate({a:10, b:20})</button>
        <div class="result" id="paramResult">点击按钮测试</div>
    </div>
    
    <script>
        function updateCounter(value) {
            document.getElementById('counterResult').textContent = '计数: ' + value;
        }
        
        function updateParam(text) {
            document.getElementById('paramResult').textContent = text;
        }
        
        // 传统方式
        function testOldWay() {
            console.log('使用 host.call 方式...');
            var result = host.call('increment');
            updateCounter(result);
            console.log('host.call result:', result);
        }
        
        // 新方式 - 直接调用
        function testNewWay() {
            console.log('使用 py.funcName 方式...');
            var result = py.increment();
            updateCounter(result);
            console.log('py.increment result:', result);
        }
        
        function testDecrement() {
            var result = py.decrement();
            updateCounter(result);
            console.log('py.decrement result:', result);
        }
        
        function testGreeting() {
            var result = py.getGreeting("LightUI");
            updateParam('问候语: ' + result);
            console.log('py.getGreeting result:', result);
        }
        
        function testCalculate() {
            var result = py.calculate({a: 10, b: 20});
            updateParam('计算结果: 10 + 20 = ' + result);
            console.log('py.calculate result:', result);
        }
        
        console.log('py 命名空间测试页面已加载');
        console.log('可用的 py 函数:', Object.keys(py));
    </script>
</body>
</html>
        ''')
        
        print("\n✅ 测试页面已加载")
        print("📝 点击按钮测试两种调用方式")
        
        app.run()


if __name__ == "__main__":
    main()
