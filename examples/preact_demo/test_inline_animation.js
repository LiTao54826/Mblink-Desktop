/**
 * 内联样式动画测试
 * 测试通过 style 属性设置的动画
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;

    console.log('=== 内联样式动画测试 ===');

    // 添加 CSS 样式
    console.log('Creating style element...');
    var style = document.createElement('style');
    console.log('Style element created, tag:', style.tagName);
    
    var cssText = `
        @keyframes rotate360 {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.2); }
        }

        .container {
            padding: 20px;
            font-family: sans-serif;
        }

        .box {
            width: 100px;
            height: 100px;
            background: #3498db;
            margin: 20px;
            display: inline-block;
        }

        .btn {
            padding: 12px 24px;
            margin: 10px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            background: #667eea;
            color: white;
        }
    `;
    
    console.log('Setting textContent, length:', cssText.length);
    style.textContent = cssText;
    console.log('textContent set, actual length:', style.textContent.length);
    
    console.log('Appending to body...');
    // 注意：document.head 可能不存在，使用 document.body
    document.body.appendChild(style);
    console.log('Style element appended');

    function App() {
        var state = useState(false);
        var isAnimating = state[0];
        var setIsAnimating = state[1];

        console.log('Render: isAnimating =', isAnimating);

        // 方块的动画样式
        var boxStyle = {
            animation: isAnimating ? 'rotate360 1s linear infinite' : 'none'
        };

        console.log('boxStyle.animation =', boxStyle.animation);

        return h('div', { className: 'container' },
            h('h1', null, '内联样式动画测试'),
            h('p', null, '点击按钮切换动画状态'),
            h('p', null, '当前状态: ' + (isAnimating ? '动画中' : '停止')),
            
            h('div', { 
                className: 'box',
                style: boxStyle
            }),
            
            h('div', null,
                h('button', { 
                    className: 'btn',
                    onClick: function() { 
                        console.log('Button clicked, toggling animation');
                        setIsAnimating(!isAnimating); 
                    }
                }, isAnimating ? '停止动画' : '开始动画')
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
