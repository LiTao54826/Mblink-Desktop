/**
 * CSS 动画测试
 * 
 * 使用方法: esm_loader.exe test_css_animation.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    console.log('=== CSS 动画测试 ===');

    // 添加 CSS 样式
    var style = document.createElement('style');
    style.textContent = `
        /* 淡入动画 */
        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        /* 滑入动画 */
        @keyframes slideIn {
            0% { transform: translateX(-100px); opacity: 0; }
            100% { transform: translateX(0px); opacity: 1; }
        }

        /* 弹跳动画 */
        @keyframes bounce {
            0%, 100% { transform: translateY(0px); }
            50% { transform: translateY(-30px); }
        }

        /* 旋转动画 */
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        /* 缩放动画 */
        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.2); }
        }

        /* 颜色渐变动画 */
        @keyframes colorChange {
            0% { background-color: rgb(255, 100, 100); }
            33% { background-color: rgb(100, 255, 100); }
            66% { background-color: rgb(100, 100, 255); }
            100% { background-color: rgb(255, 100, 100); }
        }

        .container {
            padding: 20px;
            font-family: sans-serif;
        }

        .demo-box {
            width: 100px;
            height: 100px;
            margin: 20px;
            display: inline-block;
            border-radius: 8px;
            text-align: center;
            line-height: 100px;
            color: white;
            font-weight: bold;
        }

        .fade-in {
            background: #3498db;
            animation: fadeIn 2s ease-in-out;
        }

        .slide-in {
            background: #e74c3c;
            animation: slideIn 1s ease-out;
        }

        .bounce {
            background: #2ecc71;
            animation: bounce 1s ease-in-out infinite;
        }

        .spin {
            background: #9b59b6;
            animation: spin 2s linear infinite;
        }

        .pulse {
            background: #f39c12;
            animation: pulse 1s ease-in-out infinite;
        }

        .color-change {
            animation: colorChange 3s linear infinite;
        }

        .controls {
            margin: 20px;
            padding: 15px;
            background: #ecf0f1;
            border-radius: 8px;
        }

        .btn {
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        }

        .btn-primary {
            background: #3498db;
            color: white;
        }

        .btn-danger {
            background: #e74c3c;
            color: white;
        }
    `;
    document.head.appendChild(style);

    // 淡入动画演示
    function FadeInDemo() {
        return h('div', { className: 'demo-box fade-in' }, 'Fade In');
    }

    // 滑入动画演示
    function SlideInDemo() {
        return h('div', { className: 'demo-box slide-in' }, 'Slide In');
    }

    // 弹跳动画演示
    function BounceDemo() {
        return h('div', { className: 'demo-box bounce' }, 'Bounce');
    }

    // 旋转动画演示
    function SpinDemo() {
        return h('div', { className: 'demo-box spin' }, 'Spin');
    }

    // 脉冲动画演示
    function PulseDemo() {
        return h('div', { className: 'demo-box pulse' }, 'Pulse');
    }

    // 颜色变化动画演示
    function ColorChangeDemo() {
        return h('div', { className: 'demo-box color-change' }, 'Color');
    }

    // 可控制的动画演示
    function ControllableAnimation() {
        var state = useState(true);
        var isPlaying = state[0];
        var setIsPlaying = state[1];

        var keyState = useState(0);
        var key = keyState[0];
        var setKey = keyState[1];

        var animationStyle = {
            width: '150px',
            height: '150px',
            background: '#1abc9c',
            borderRadius: '8px',
            margin: '20px auto',
            animation: isPlaying ? 'bounce 1s ease-in-out infinite' : 'none',
            animationPlayState: isPlaying ? 'running' : 'paused'
        };

        function toggleAnimation() {
            setIsPlaying(!isPlaying);
        }

        function restartAnimation() {
            setKey(key + 1);
            setIsPlaying(true);
        }

        return h('div', { className: 'controls' },
            h('h3', null, '动画控制'),
            h('div', { key: key, style: animationStyle }),
            h('button', { 
                className: 'btn btn-primary',
                onClick: toggleAnimation 
            }, isPlaying ? '暂停' : '播放'),
            h('button', { 
                className: 'btn btn-danger',
                onClick: restartAnimation 
            }, '重新开始')
        );
    }

    // Fill-mode 演示
    function FillModeDemo() {
        var state = useState(0);
        var key = state[0];
        var setKey = state[1];

        function restart() {
            setKey(key + 1);
        }

        var forwardsStyle = {
            width: '80px',
            height: '80px',
            background: '#e74c3c',
            margin: '10px',
            display: 'inline-block',
            animation: 'slideIn 1s ease-out forwards'
        };

        var backwardsStyle = {
            width: '80px',
            height: '80px',
            background: '#3498db',
            margin: '10px',
            display: 'inline-block',
            animation: 'fadeIn 1s ease-out 1s backwards'
        };

        var bothStyle = {
            width: '80px',
            height: '80px',
            background: '#2ecc71',
            margin: '10px',
            display: 'inline-block',
            animation: 'slideIn 1s ease-out 0.5s both'
        };

        return h('div', { className: 'controls' },
            h('h3', null, 'Fill-mode 演示'),
            h('div', { key: 'forwards-' + key, style: forwardsStyle },
                h('span', { style: { fontSize: '12px', color: 'white' } }, 'forwards')
            ),
            h('div', { key: 'backwards-' + key, style: backwardsStyle },
                h('span', { style: { fontSize: '12px', color: 'white' } }, 'backwards')
            ),
            h('div', { key: 'both-' + key, style: bothStyle },
                h('span', { style: { fontSize: '12px', color: 'white' } }, 'both')
            ),
            h('br'),
            h('button', { 
                className: 'btn btn-primary',
                onClick: restart 
            }, '重新播放')
        );
    }

    // 多动画演示
    function MultiAnimationDemo() {
        var style = {
            width: '120px',
            height: '120px',
            background: '#9b59b6',
            margin: '20px auto',
            borderRadius: '8px',
            animation: 'fadeIn 1s ease-out, bounce 2s ease-in-out 1s infinite'
        };

        return h('div', { className: 'controls' },
            h('h3', null, '多动画组合'),
            h('div', { style: style },
                h('span', { style: { color: 'white', lineHeight: '120px', display: 'block', textAlign: 'center' } }, 'Multi')
            )
        );
    }

    // 主应用
    function App() {
        return h('div', { className: 'container' },
            h('h1', { style: { color: '#2c3e50' } }, 'CSS 动画测试'),
            
            h('h2', { style: { color: '#34495e' } }, '基础动画'),
            h('div', null,
                h(FadeInDemo),
                h(SlideInDemo),
                h(BounceDemo),
                h(SpinDemo),
                h(PulseDemo),
                h(ColorChangeDemo)
            ),

            h(ControllableAnimation),
            h(FillModeDemo),
            h(MultiAnimationDemo)
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
