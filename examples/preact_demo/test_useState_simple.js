/**
 * @file test_useState_simple.js
 * @brief 测试 useState 是否影响文本显示
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;

    // 测试1: 不使用 useState 的静态文本
    function StaticText() {
        console.log('[StaticText] rendering');
        return h('div', { 
            style: { fontSize: '32px', color: 'blue', padding: '10px', backgroundColor: '#e0e0e0' } 
        }, '静态文本 (无 useState)');
    }

    // 测试2: 使用 useState 但不更新
    function UseStateText() {
        var state = useState('useState 文本');
        var text = state[0];
        console.log('[UseStateText] text=' + text);
        return h('div', { 
            style: { fontSize: '32px', color: 'green', padding: '10px', backgroundColor: '#d0d0d0' } 
        }, text);
    }

    // 测试3: 使用 useState 初始化为时间字符串
    function UseStateTime() {
        var now = new Date();
        var timeStr = now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds();
        var state = useState(timeStr);
        var text = state[0];
        console.log('[UseStateTime] text=' + text);
        return h('div', { 
            style: { fontSize: '32px', color: 'red', padding: '10px', backgroundColor: '#c0c0c0' } 
        }, text);
    }

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, 'useState 测试'),
            h('h2', null, '1. 静态文本:'),
            h(StaticText, null),
            h('h2', null, '2. useState 文本:'),
            h(UseStateText, null),
            h('h2', null, '3. useState 时间:'),
            h(UseStateTime, null)
        );
    }

    console.log('Starting useState test...');
    render(h(App, null), document.body);
    console.log('useState test rendered!');
})();
