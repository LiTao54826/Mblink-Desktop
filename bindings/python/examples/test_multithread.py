"""
LightUI Python Binding - 多线程状态更新测试

测试后台线程更新状态，主线程 UI 自动刷新
验证设计文档中的线程安全队列机制

使用 host.state.watch() 自动响应状态变化（无需轮询）
"""

import sys
import os
import threading
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI 多线程状态更新测试")
    print("=" * 50)
    
    with LightUIApp("多线程测试", 1200, 800) as app:
        # 创建共享状态
        progress = app.state("progress", 0)
        logs = app.state("logs", [])
        status = app.state("status", "idle")
        
        # 后台任务函数
        def background_task():
            """在后台线程中更新状态"""
            status.set("processing")
            logs.append("后台任务开始...")
            
            for i in range(1, 11):
                time.sleep(0.3)  # 模拟耗时操作
                progress.set(i * 10)
                logs.append(f"处理进度: {i * 10}%")
                print(f"[后台线程] 进度: {i * 10}%")
            
            status.set("completed")
            logs.append("后台任务完成!")
            print("[后台线程] 任务完成")
        
        # 绑定 Python 函数
        @app.bind("startTask")
        def start_task(args):
            """启动后台任务"""
            if status.get() == "processing":
                return {"error": "任务正在进行中"}
            
            # 重置状态
            progress.set(0)
            logs.set([])
            
            # 启动后台线程
            thread = threading.Thread(target=background_task, daemon=True)
            thread.start()
            
            return {"started": True}
        
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
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #16213e;
            border-radius: 15px;
            overflow: hidden;
            margin: 20px 0;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #e94560, #ff6b6b);
            transition: width 0.3s ease;
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
            padding: 12px 24px;
            margin: 5px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover { background: #d63850; }
        button:disabled { background: #555; cursor: not-allowed; }
        h1 { color: #e94560; }
        .status { font-size: 18px; margin: 10px 0; }
        .status.idle { color: #888; }
        .status.processing { color: #ffd93d; }
        .status.completed { color: #4ade80; }
    </style>
</head>
<body>
    <h1>🧵 多线程状态更新测试</h1>
    <p>使用 host.state.watch() 自动响应 Python 端状态变化（无需轮询）</p>
    
    <button id="startBtn" onclick="startTask()">启动后台任务</button>
    
    <div class="status" id="status">状态: 空闲</div>
    
    <div class="progress-bar">
        <div class="progress-fill" id="progressFill" style="width: 0%"></div>
    </div>
    <div id="progressText">进度: 0%</div>
    
    <h3>日志</h3>
    <div class="log" id="log"></div>
    
    <script>
        // 使用 host.state.watch() 监听状态变化（自动响应，无需轮询）
        
        // 监听进度变化
        host.state.watch('progress', function(value) {
            console.log('[JS] progress changed:', value);
            document.getElementById('progressFill').style.width = value + '%';
            document.getElementById('progressText').textContent = '进度: ' + value + '%';
        });
        
        // 监听状态变化
        host.state.watch('status', function(value) {
            console.log('[JS] status changed:', value);
            var statusEl = document.getElementById('status');
            statusEl.className = 'status ' + value;
            var statusText = { 'idle': '空闲', 'processing': '处理中...', 'completed': '已完成' };
            statusEl.textContent = '状态: ' + (statusText[value] || value);
            document.getElementById('startBtn').disabled = (value === 'processing');
        });
        
        // 监听日志变化
        host.state.watch('logs', function(value) {
            console.log('[JS] logs changed, count:', value.length);
            var logEl = document.getElementById('log');
            logEl.innerHTML = value.map(function(log) { return '<div>' + log + '</div>'; }).join('');
            logEl.scrollTop = logEl.scrollHeight;
        });
        
        function startTask() {
            var result = host.call('startTask');
            if (result && result.error) { console.log('Error:', result.error); }
        }
        
        console.log('[JS] 页面已加载，使用 host.state.watch() 监听状态变化');
    </script>
</body>
</html>
        ''')
        
        print("\n✅ 多线程测试页面已加载")
        print("📝 自动启动后台任务...")
        print("🧵 使用 host.state.watch() 自动响应状态变化")
        
        # 自动启动后台任务
        def auto_start():
            time.sleep(0.5)
            start_task(None)
            print("[TEST] 后台任务已自动启动")
        
        threading.Thread(target=auto_start, daemon=True).start()
        
        # 监控任务完成并自动退出
        def monitor_completion():
            while True:
                time.sleep(0.5)
                if status.get() == "completed":
                    print("\n[TEST_PASS] 多线程状态更新测试通过!")
                    print(f"  - 最终进度: {progress.get()}%")
                    print(f"  - 日志条数: {len(logs.get())}")
                    time.sleep(1)
                    app.stop()
                    break
        
        threading.Thread(target=monitor_completion, daemon=True).start()
        
        app.run()


if __name__ == "__main__":
    main()
