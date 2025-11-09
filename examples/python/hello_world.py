"""
LightUI Python示例 - Hello World

功能：
- 创建简单窗口
- 显示Hello World
- 演示基本用法

运行方法：
    python examples/python/hello_world.py
"""

import sys
import os

# 添加bindings/python到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'bindings', 'python'))

import lightui

def main():
    # 创建窗口
    app = lightui.Window("Hello World", 400, 300)
    
    # 加载UI
    app.load_ui("""
        // 导入Preact
        import { render } from 'preact';
        import { useState } from 'preact/hooks';
        
        // 定义组件
        function App() {
            const [count, setCount] = useState(0);
            
            return (
                <div style={{
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    justifyContent: 'center',
                    height: '100vh',
                    fontFamily: 'Arial, sans-serif'
                }}>
                    <h1>Hello World!</h1>
                    <p>Welcome to LightUI</p>
                    <p>Count: {count}</p>
                    <button onClick={() => setCount(count + 1)}>
                        Click Me
                    </button>
                </div>
            );
        }
        
        // 渲染到document.body
        render(<App />, document.body);
    """)
    
    # 运行
    app.run()

if __name__ == "__main__":
    main()

