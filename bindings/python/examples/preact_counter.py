"""
LightUI Python Binding - 简单的 Preact 计数器示例

这是一个最简单的 Preact 示例，展示基本的组件和状态管理。
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI Preact 计数器示例")
    print("=" * 50)
    
    with LightUIApp("Preact Counter", 400, 300) as app:
        # 创建计数器状态
        counter = app.state("counter", 0)
        
        # 绑定：增加计数
        @app.bind("increment")
        def increment():
            """增加计数"""
            counter.increment()
            print(f"计数器: {counter.get()}")
            return counter.get()
        
        # 绑定：减少计数
        @app.bind("decrement")
        def decrement():
            """减少计数"""
            counter.decrement()
            print(f"计数器: {counter.get()}")
            return counter.get()
        
        # 绑定：重置计数
        @app.bind("reset")
        def reset():
            """重置计数"""
            counter.set(0)
            print("计数器已重置")
            return 0
        
        # 绑定：获取当前值
        @app.bind("getValue")
        def get_value():
            """获取当前计数值"""
            return counter.get()
        
        # 加载 Preact 库
        preact_path = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'js', 'preact')
        preact_js = os.path.join(preact_path, 'preact.js')
        hooks_js = os.path.join(preact_path, 'hooks.js')
        
        print(f"📦 加载 Preact 库...")
        app.load_js_file(preact_js)
        app.load_js_file(hooks_js)
        
        # 加载应用代码
        js_code = """
// Preact 计数器应用
const { h, render } = preact;
const { useState, useEffect } = preactHooks;

// 样式
const styles = `
    * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
    }
    
    body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        display: flex;
        align-items: center;
        justify-content: center;
        height: 100vh;
    }
    
    .container {
        background: white;
        border-radius: 16px;
        padding: 40px;
        box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
        text-align: center;
    }
    
    .title {
        font-size: 24px;
        color: #333;
        margin-bottom: 20px;
    }
    
    .counter {
        font-size: 72px;
        font-weight: 700;
        color: #667eea;
        margin: 30px 0;
    }
    
    .buttons {
        display: flex;
        gap: 12px;
        justify-content: center;
    }
    
    .btn {
        padding: 12px 24px;
        border: none;
        border-radius: 8px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.2s;
    }
    
    .btn-primary {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
    }
    
    .btn-primary:hover {
        transform: translateY(-2px);
        box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
    }
    
    .btn-secondary {
        background: #f0f0f0;
        color: #333;
    }
    
    .btn-secondary:hover {
        background: #e0e0e0;
    }
`;

// 计数器组件
function Counter() {
    const [count, setCount] = useState(0);
    
    // 加载初始值
    useEffect(() => {
        const value = host.call('getValue');
        setCount(value);
    }, []);
    
    // 增加
    const handleIncrement = () => {
        const newValue = host.call('increment');
        setCount(newValue);
    };
    
    // 减少
    const handleDecrement = () => {
        const newValue = host.call('decrement');
        setCount(newValue);
    };
    
    // 重置
    const handleReset = () => {
        const newValue = host.call('reset');
        setCount(newValue);
    };
    
    return h('div', { className: 'container' },
        h('h1', { className: 'title' }, '🎯 Preact 计数器'),
        h('div', { className: 'counter' }, count),
        h('div', { className: 'buttons' },
            h('button', {
                className: 'btn btn-secondary',
                onClick: handleDecrement
            }, '➖ 减少'),
            h('button', {
                className: 'btn btn-primary',
                onClick: handleIncrement
            }, '➕ 增加'),
            h('button', {
                className: 'btn btn-secondary',
                onClick: handleReset
            }, '🔄 重置')
        )
    );
}

// 初始化
function init() {
    console.log('Initializing Preact Counter...');
    
    // 注入样式
    const styleEl = document.createElement('style');
    styleEl.textContent = styles;
    document.head.appendChild(styleEl);
    
    // 渲染应用
    render(h(Counter), document.body);
    
    console.log('✅ Preact Counter initialized');
}

// 启动
init();
        """
        
        print("📦 加载应用代码...")
        app.load_js(js_code, "counter.js")
        
        print("\n✅ Preact 计数器应用已启动")
        print("🎯 点击按钮来增加、减少或重置计数器")
        
        app.run()


if __name__ == "__main__":
    main()

