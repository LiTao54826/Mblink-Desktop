/**
 * 最简单的 Preact 测试
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    console.log('=== Preact 简单测试 ===');

    // 测试1: 静态文本子节点
    function Test1() {
        console.log('[Test1] 渲染静态文本');
        return h('div', { style: { padding: '20px', background: '#f0f0f0', margin: '10px' } },
            '静态文本测试'
        );
    }

    // 测试2: 使用 textContent 属性
    function Test2() {
        console.log('[Test2] 使用 textContent');
        return h('div', { 
            style: { padding: '20px', background: '#e0e0e0', margin: '10px' },
            textContent: 'textContent 测试'
        });
    }

    // 测试3: 嵌套 span
    function Test3() {
        console.log('[Test3] 嵌套 span');
        return h('div', { style: { padding: '20px', background: '#d0d0d0', margin: '10px' } },
            h('span', null, 'span 中的文本')
        );
    }

    // 测试4: 动态状态
    function Test4() {
        var state = useState(0);
        var count = state[0];
        var setCount = state[1];

        console.log('[Test4] count = ' + count);

        useEffect(function() {
            var timer = setInterval(function() {
                setCount(function(c) { return c + 1; });
            }, 1000);
            return function() { clearInterval(timer); };
        }, []);

        return h('div', { 
            style: { padding: '20px', background: '#c0c0c0', margin: '10px', fontSize: '24px' },
            textContent: '计数: ' + count
        });
    }

    // 主应用
    function App() {
        return h('div', null,
            h('h1', null, 'Preact 测试'),
            h(Test1, null),
            h(Test2, null),
            h(Test3, null),
            h(Test4, null)
        );
    }

    console.log('开始渲染...');
    render(h(App, null), document.body);
    console.log('渲染完成');
})();
