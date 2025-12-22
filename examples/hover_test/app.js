/**
 * @file app.js
 * @brief 简单的 hover 测试
 * 
 * 测试 :hover 伪类效果
 * 按钮悬浮时背景色会自动变暗
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;

    var styles = {
        app: {
            fontFamily: 'Arial, sans-serif',
            padding: '20px',
            maxWidth: '600px',
            margin: '0 auto'
        },
        button: {
            backgroundColor: '#007bff',
            color: 'white',
            border: 'none',
            padding: '10px 20px',
            borderRadius: '4px',
            cursor: 'pointer',
            marginRight: '8px',
            marginBottom: '8px'
        },
        buttonDanger: {
            backgroundColor: '#dc3545',
            color: 'white',
            border: 'none',
            padding: '10px 20px',
            borderRadius: '4px',
            cursor: 'pointer',
            marginRight: '8px'
        },
        counter: {
            fontSize: '48px',
            textAlign: 'center',
            color: '#007bff',
            margin: '20px 0'
        }
    };

    function Counter() {
        var countState = useState(0);
        var count = countState[0];
        var setCount = countState[1];

        return h(
            'div',
            { style: { padding: '16px', backgroundColor: '#f5f5f5', borderRadius: '8px' } },
            h('h3', null, '计数器'),
            h('div', { style: styles.counter }, count),
            h(
                'div',
                { style: { textAlign: 'center' } },
                h(
                    'button',
                    {
                        style: styles.button,
                        onClick: function () {
                            setCount(count - 1);
                        }
                    },
                    '减少'
                ),
                h(
                    'button',
                    {
                        style: styles.button,
                        onClick: function () {
                            setCount(count + 1);
                        }
                    },
                    '增加'
                ),
                h(
                    'button',
                    {
                        style: styles.buttonDanger,
                        onClick: function () {
                            setCount(0);
                        }
                    },
                    '重置'
                )
            )
        );
    }

    function App() {
        return h(
            'div',
            { style: styles.app },
            h('h1', { style: { textAlign: 'center' } }, 'Hover 测试'),
            h('p', { style: { textAlign: 'center', color: '#666' } }, 
                '鼠标悬浮在按钮上，背景色会自动变暗'),
            h(Counter, null)
        );
    }

    console.log('Starting Hover Test App...');
    render(h(App, null), document.body);
    console.log('Hover Test App rendered!');
})();
