/**
 * @file test_clock_minimal.js
 * @brief 最小化时钟测试
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    function formatTime(date) {
        var hours = date.getHours();
        var minutes = date.getMinutes();
        var seconds = date.getSeconds();
        var hh = hours < 10 ? '0' + hours : '' + hours;
        var mm = minutes < 10 ? '0' + minutes : '' + minutes;
        var ss = seconds < 10 ? '0' + seconds : '' + seconds;
        return hh + ':' + mm + ':' + ss;
    }

    // 简单的静态文本测试
    function StaticText() {
        return h('div', { style: { fontSize: '32px', color: 'blue' } }, '静态文本');
    }

    // 使用 useState 的文本测试
    function DynamicText() {
        var state = useState('初始值');
        var text = state[0];
        console.log('[DynamicText] text:', text);
        return h('div', { style: { fontSize: '32px', color: 'green' } }, text);
    }

    // 时钟组件
    function Clock() {
        var state = useState(formatTime(new Date()));
        var time = state[0];
        var setTime = state[1];
        
        console.log('[Clock] time:', time);

        useEffect(function () {
            var timer = setInterval(function () {
                var newTime = formatTime(new Date());
                console.log('[Clock] setTime:', newTime);
                setTime(newTime);
            }, 1000);
            return function () {
                clearInterval(timer);
            };
        }, []);

        return h('div', { style: { fontSize: '32px', color: 'red' } }, time);
    }

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, '时钟测试'),
            h('h2', null, '1. 静态文本:'),
            h(StaticText, null),
            h('h2', null, '2. 动态文本 (useState):'),
            h(DynamicText, null),
            h('h2', null, '3. 时钟 (useState + useEffect):'),
            h(Clock, null)
        );
    }

    console.log('Starting minimal clock test...');
    render(h(App, null), document.body);
    console.log('Minimal clock test rendered!');
})();
