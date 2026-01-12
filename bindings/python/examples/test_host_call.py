"""
测试 host.call 是否正常工作
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("测试 host.call 和 document")
    print("=" * 50)
    
    with LightUIApp("Test", 400, 300, headless=True) as app:
        call_count = [0]  # 使用列表来模拟可变状态
        
        # 绑定测试函数 - 返回整数
        @app.bind("incrementCount")
        def increment_count(arg):
            call_count[0] += 1
            print(f"[Python] incrementCount called, returning: {call_count[0]}")
            return call_count[0]
        
        # 绑定测试函数 - 返回字符串
        @app.bind("getGreeting")
        def get_greeting(name):
            result = f"Hello, {name}!"
            print(f"[Python] getGreeting called with: {name}, returning: {result}")
            return result
        
        # 加载简单的测试 HTML
        app.load_html('''
<!DOCTYPE html>
<html>
<body>
    <div id="count">0</div>
    <div id="greeting"></div>
    <script>
        console.log("[JS] Script starting...");
        
        // 测试返回整数
        var count = host.call('incrementCount', null);
        console.log("[JS] incrementCount returned:", count, "type:", typeof count);
        
        // 测试返回字符串
        var greeting = host.call('getGreeting', 'World');
        console.log("[JS] getGreeting returned:", greeting, "type:", typeof greeting);
        
        // 更新 DOM
        if (typeof document !== 'undefined') {
            var countEl = document.getElementById('count');
            if (countEl) {
                countEl.textContent = count;
                console.log("[JS] Updated count element");
            }
            var greetingEl = document.getElementById('greeting');
            if (greetingEl) {
                greetingEl.textContent = greeting;
                console.log("[JS] Updated greeting element");
            }
        }
        
        console.log("[JS] Script done");
    </script>
</body>
</html>
        ''')
        
        print("\n运行几次事件循环...")
        for i in range(5):
            app.run_once()
        
        print("\n测试完成")
        
    # 使用 os._exit 强制退出，避免清理时卡住
    import os
    os._exit(0)


if __name__ == "__main__":
    main()
