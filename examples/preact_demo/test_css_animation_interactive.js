/**
 * CSS 动画交互测试
 * 测试鼠标 hover、click 触发的动画
 * 
 * 使用方法: esm_loader.exe test_css_animation_interactive.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;

    console.log('=== CSS 动画交互测试 ===');

    // 添加 CSS 样式
    var style = document.createElement('style');
    style.textContent = `
        /* Hover 放大动画 */
        @keyframes scaleUp {
            from { transform: scale(1); }
            to { transform: scale(1.1); }
        }

        /* 点击波纹动画 */
        @keyframes ripple {
            0% { transform: scale(0); opacity: 1; }
            100% { transform: scale(2); opacity: 0; }
        }

        /* 摇晃动画 */
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            25% { transform: translateX(-10px); }
            75% { transform: translateX(10px); }
        }

        /* 心跳动画 */
        @keyframes heartbeat {
            0%, 100% { transform: scale(1); }
            25% { transform: scale(1.1); }
            50% { transform: scale(1); }
            75% { transform: scale(1.1); }
        }

        /* 旋转动画 */
        @keyframes rotate360 {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        /* 颜色闪烁 */
        @keyframes flash {
            0%, 100% { background-color: #3498db; }
            50% { background-color: #e74c3c; }
        }

        .container {
            padding: 20px;
            font-family: sans-serif;
        }

        .box {
            width: 120px;
            height: 120px;
            margin: 15px;
            display: inline-block;
            border-radius: 8px;
            text-align: center;
            line-height: 120px;
            color: white;
            font-weight: bold;
            cursor: pointer;
            transition: box-shadow 0.3s ease;
        }

        .box:hover {
            box-shadow: 0 8px 16px rgba(0,0,0,0.3);
        }

        .hover-scale {
            background: #3498db;
        }
        .hover-scale:hover {
            animation: scaleUp 0.3s ease forwards;
        }

        .hover-shake {
            background: #e74c3c;
        }
        .hover-shake:hover {
            animation: shake 0.5s ease;
        }

        .hover-rotate {
            background: #2ecc71;
        }
        .hover-rotate:hover {
            animation: rotate360 0.5s ease;
        }

        .hover-flash {
            background: #3498db;
        }
        .hover-flash:hover {
            animation: flash 0.5s ease infinite;
        }

        .click-heartbeat {
            background: #9b59b6;
        }

        .click-ripple {
            background: #f39c12;
            position: relative;
            overflow: hidden;
        }

        .btn {
            padding: 12px 24px;
            margin: 10px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: bold;
            color: white;
            transition: transform 0.2s ease;
        }

        .btn:hover {
            transform: translateY(-2px);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }

        .btn-success {
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
        }

        .btn-danger {
            background: linear-gradient(135deg, #eb3349 0%, #f45c43 100%);
        }

        .section {
            margin: 20px 0;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 8px;
        }

        .section h3 {
            margin: 0 0 15px 0;
            color: #2c3e50;
        }
    `;
    document.head.appendChild(style);

    // Hover 放大效果
    function HoverScaleBox() {
        return h('div', { className: 'box hover-scale' }, 'Hover Me');
    }

    // Hover 摇晃效果
    function HoverShakeBox() {
        return h('div', { className: 'box hover-shake' }, 'Shake');
    }

    // Hover 旋转效果
    function HoverRotateBox() {
        return h('div', { className: 'box hover-rotate' }, 'Rotate');
    }

    // Hover 闪烁效果
    function HoverFlashBox() {
        return h('div', { className: 'box hover-flash' }, 'Flash');
    }

    // 点击心跳效果
    function ClickHeartbeatBox() {
        var state = useState(false);
        var isAnimating = state[0];
        var setIsAnimating = state[1];

        function handleClick() {
            setIsAnimating(true);
            setTimeout(function() {
                setIsAnimating(false);
            }, 800);
        }

        var style = {
            animation: isAnimating ? 'heartbeat 0.8s ease' : 'none'
        };

        return h('div', { 
            className: 'box click-heartbeat',
            style: style,
            onClick: handleClick
        }, 'Click Me');
    }

    // 点击计数器
    function ClickCounter() {
        var state = useState(0);
        var count = state[0];
        var setCount = state[1];

        var animState = useState(false);
        var isAnimating = animState[0];
        var setIsAnimating = animState[1];

        function handleClick() {
            setCount(count + 1);
            setIsAnimating(true);
            setTimeout(function() {
                setIsAnimating(false);
            }, 300);
        }

        var boxStyle = {
            background: '#1abc9c',
            animation: isAnimating ? 'scaleUp 0.3s ease' : 'none'
        };

        return h('div', { 
            className: 'box',
            style: boxStyle,
            onClick: handleClick
        }, 'Count: ' + count);
    }

    // 切换动画按钮
    function ToggleAnimationButton() {
        var state = useState(false);
        var isSpinning = state[0];
        var setIsSpinning = state[1];

        function handleClick() {
            setIsSpinning(!isSpinning);
        }

        // 使用一个半圆弧作为旋转图标，旋转时更明显
        var iconStyle = {
            display: 'inline-block',
            width: '20px',
            height: '20px',
            marginRight: '8px',
            border: '3px solid transparent',
            borderTopColor: 'white',
            borderRightColor: 'white',
            borderRadius: '50%',
            animation: isSpinning ? 'rotate360 0.8s linear infinite' : 'none'
        };

        return h('button', { 
            className: 'btn btn-primary',
            onClick: handleClick
        }, 
            h('span', { style: iconStyle }),
            isSpinning ? 'Stop' : 'Start'
        );
    }

    // 动画状态切换
    function AnimationStateDemo() {
        var state = useState('none');
        var animType = state[0];
        var setAnimType = state[1];

        var boxStyle = {
            width: '150px',
            height: '150px',
            background: '#34495e',
            borderRadius: '8px',
            margin: '20px auto',
            animation: animType !== 'none' ? animType + ' 1s ease infinite' : 'none'
        };

        return h('div', null,
            h('div', { style: boxStyle }),
            h('div', { style: { textAlign: 'center' } },
                h('button', { 
                    className: 'btn btn-primary',
                    onClick: function() { setAnimType('shake'); }
                }, 'Shake'),
                h('button', { 
                    className: 'btn btn-success',
                    onClick: function() { setAnimType('heartbeat'); }
                }, 'Heartbeat'),
                h('button', { 
                    className: 'btn btn-danger',
                    onClick: function() { setAnimType('none'); }
                }, 'Stop')
            )
        );
    }

    // 主应用
    function App() {
        return h('div', { className: 'container' },
            h('h1', { style: { color: '#2c3e50' } }, 'CSS 动画交互测试'),
            
            h('div', { className: 'section' },
                h('h3', null, 'Hover 动画'),
                h('p', { style: { color: '#7f8c8d', marginBottom: '10px' } }, '将鼠标悬停在方块上查看效果'),
                h(HoverScaleBox),
                h(HoverShakeBox),
                h(HoverRotateBox),
                h(HoverFlashBox)
            ),

            h('div', { className: 'section' },
                h('h3', null, 'Click 动画'),
                h('p', { style: { color: '#7f8c8d', marginBottom: '10px' } }, '点击方块触发动画'),
                h(ClickHeartbeatBox),
                h(ClickCounter)
            ),

            h('div', { className: 'section' },
                h('h3', null, '按钮动画'),
                h(ToggleAnimationButton)
            ),

            h('div', { className: 'section' },
                h('h3', null, '动画状态切换'),
                h(AnimationStateDemo)
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
