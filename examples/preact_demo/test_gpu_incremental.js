/**
 * GPU 增量渲染测试
 * 
 * 这个测试用例使用 will-change 属性来触发层提升，
 * 验证 GPU 增量渲染是否正常工作。
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    console.log('=== GPU 增量渲染测试 ===');

    // 静态背景层 - 不会被重绘
    function StaticBackground() {
        return h('div', { 
            style: { 
                position: 'absolute',
                top: '0',
                left: '0',
                right: '0',
                bottom: '0',
                background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
                zIndex: '-1'
            }
        });
    }

    // 静态内容 - 不会被重绘
    function StaticContent() {
        return h('div', { 
            style: { 
                padding: '20px',
                color: 'white',
                fontSize: '18px'
            }
        },
            h('h1', null, 'GPU 增量渲染测试'),
            h('p', null, '下面的计数器使用 will-change: transform 触发层提升'),
            h('p', null, '只有计数器的层会被重新光栅化，背景和这段文字不会被重绘')
        );
    }

    // 动态计数器 - 使用 will-change 触发层提升
    function Counter() {
        var state = useState(0);
        var count = state[0];
        var setCount = state[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setCount(function(c) { return c + 1; });
            }, 100);  // 每 100ms 更新一次，测试高频更新
            return function() { clearInterval(timer); };
        }, []);

        // 使用 will-change: transform 触发层提升
        return h('div', { 
            style: { 
                willChange: 'transform',  // 关键：触发层提升
                padding: '40px',
                margin: '20px',
                background: 'rgba(255, 255, 255, 0.9)',
                borderRadius: '16px',
                boxShadow: '0 8px 32px rgba(0, 0, 0, 0.2)',
                fontSize: '48px',
                fontWeight: 'bold',
                textAlign: 'center',
                color: '#333'
            }
        }, '计数: ' + count);
    }

    // 另一个动态元素 - 使用 CSS 动画
    function AnimatedBox() {
        return h('div', { 
            style: { 
                willChange: 'opacity',  // 触发层提升
                width: '100px',
                height: '100px',
                margin: '20px auto',
                background: '#ff6b6b',
                borderRadius: '8px',
                animation: 'pulse 2s infinite'
            }
        });
    }

    // 主应用
    function App() {
        return h('div', { 
            style: { 
                position: 'relative',
                minHeight: '100vh',
                fontFamily: 'system-ui, sans-serif'
            }
        },
            h(StaticBackground, null),
            h(StaticContent, null),
            h(Counter, null),
            h(AnimatedBox, null),
            h('div', { 
                style: { 
                    padding: '20px',
                    color: 'white',
                    fontSize: '14px',
                    opacity: '0.8'
                }
            }, '如果 GPU 增量渲染正常工作，CPU 占用应该很低')
        );
    }

    // 添加 CSS 动画
    var style = document.createElement('style');
    style.textContent = '@keyframes pulse { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.5; transform: scale(0.95); } }';
    document.head.appendChild(style);

    console.log('开始渲染...');
    render(h(App, null), document.body);
    console.log('渲染完成');
})();
