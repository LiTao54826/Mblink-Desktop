"""
LightUI Python Binding - 定时器测试

测试 setTimeout, setInterval, requestAnimationFrame 是否正常工作
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI 定时器测试")
    print("=" * 50)
    
    with LightUIApp("定时器测试", 600, 400) as app:
        # 绑定 Python 函数
        @app.bind("logMessage")
        def log_message(msg):
            print(f"[Python] {msg}")
            return True
        
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
        .log {
            background: #16213e;
            padding: 15px;
            border-radius: 8px;
            margin: 10px 0;
            font-family: monospace;
            max-height: 200px;
            overflow-y: auto;
        }
        button {
            background: #e94560;
            color: white;
            border: none;
            padding: 10px 20px;
            margin: 5px;
            border-radius: 5px;
            cursor: pointer;
        }
        button:hover {
            background: #d63850;
        }
        h1 { color: #e94560; }
        .status { color: #4ade80; }
    </style>
</head>
<body>
    <h1>🕐 定时器测试</h1>
    
    <div>
        <button onclick="testSetTimeout()">测试 setTimeout</button>
        <button onclick="testSetInterval()">测试 setInterval</button>
        <button onclick="testRAF()">测试 requestAnimationFrame</button>
        <button onclick="clearLog()">清空日志</button>
    </div>
    
    <div class="log" id="log"></div>
    
    <script>
        var intervalId = null;
        var rafId = null;
        var rafCount = 0;
        
        function log(msg) {
            var logEl = document.getElementById('log');
            var time = new Date().toLocaleTimeString();
            logEl.innerHTML += '[' + time + '] ' + msg + '<br>';
            logEl.scrollTop = logEl.scrollHeight;
            console.log(msg);
            host.call('logMessage', msg);
        }
        
        function clearLog() {
            document.getElementById('log').innerHTML = '';
            log('日志已清空');
        }
        
        function testSetTimeout() {
            log('开始 setTimeout 测试...');
            
            setTimeout(function() {
                log('✅ setTimeout 500ms 触发!');
            }, 500);
            
            setTimeout(function() {
                log('✅ setTimeout 1000ms 触发!');
            }, 1000);
            
            setTimeout(function() {
                log('✅ setTimeout 1500ms 触发!');
                log('setTimeout 测试完成!');
            }, 1500);
        }
        
        function testSetInterval() {
            if (intervalId) {
                clearInterval(intervalId);
                intervalId = null;
                log('⏹️ setInterval 已停止');
                return;
            }
            
            log('开始 setInterval 测试 (每500ms)...');
            var count = 0;
            intervalId = setInterval(function() {
                count++;
                log('⏱️ setInterval 触发 #' + count);
                if (count >= 5) {
                    clearInterval(intervalId);
                    intervalId = null;
                    log('✅ setInterval 测试完成 (5次后自动停止)');
                }
            }, 500);
        }
        
        function testRAF() {
            if (rafId) {
                cancelAnimationFrame(rafId);
                rafId = null;
                log('⏹️ requestAnimationFrame 已停止');
                return;
            }
            
            log('开始 requestAnimationFrame 测试...');
            rafCount = 0;
            
            function animate() {
                rafCount++;
                if (rafCount <= 10) {
                    log('🎬 RAF 帧 #' + rafCount);
                    rafId = requestAnimationFrame(animate);
                } else {
                    log('✅ requestAnimationFrame 测试完成 (10帧)');
                    rafId = null;
                }
            }
            
            rafId = requestAnimationFrame(animate);
        }
        
        // 初始化
        log('定时器测试页面已加载');
        log('点击按钮测试各种定时器 API');
    </script>
</body>
</html>
        ''')
        
        print("\n✅ 定时器测试页面已加载")
        print("📝 点击按钮测试 setTimeout, setInterval, requestAnimationFrame")
        
        app.run()


if __name__ == "__main__":
    main()
