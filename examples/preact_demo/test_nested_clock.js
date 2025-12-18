/**
 * @file test_nested_clock.js
 * @brief 测试嵌套结构中的时钟显示
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

    // 测试1: 简单结构 (这个能工作)
    function SimpleClock() {
        var state = useState(formatTime(new Date()));
        var time = state[0];
        var setTime = state[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
            }, 1000);
            return function () { clearInterval(timer); };
        }, []);

        console.log('[SimpleClock] time=' + time);
        return h('div', { style: { fontSize: '24px', color: 'green' } }, time);
    }

    // 测试2: 嵌套结构 - 和 app.js 一样
    function NestedClock() {
        var state = useState(formatTime(new Date()));
        var time = state[0];
        var setTime = state[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
            }, 1000);
            return function () { clearInterval(timer); };
        }, []);

        console.log('[NestedClock] time=' + time);
        
        // 这是 app.js 中的结构
        var result = h(
            'div',
            { style: { backgroundColor: '#f5f5f5', padding: '16px' } },
            h('h3', null, '嵌套时钟'),
            h('div', { style: { fontSize: '24px', color: 'blue' } }, time)
        );
        
        console.log('[NestedClock] result.children.length=' + result.children.length);
        console.log('[NestedClock] result.children[0]=' + JSON.stringify(result.children[0]));
        console.log('[NestedClock] result.children[1]=' + JSON.stringify(result.children[1]));
        
        return result;
    }

    // 测试3: 手动构建嵌套
    function ManualNestedClock() {
        var state = useState(formatTime(new Date()));
        var time = state[0];
        var setTime = state[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
            }, 1000);
            return function () { clearInterval(timer); };
        }, []);

        console.log('[ManualNestedClock] time=' + time);
        
        // 手动构建子元素
        var titleVNode = h('h3', null, '手动嵌套时钟');
        var timeVNode = h('div', { style: { fontSize: '24px', color: 'red' } }, time);
        
        console.log('[ManualNestedClock] titleVNode.children=' + JSON.stringify(titleVNode.children));
        console.log('[ManualNestedClock] timeVNode.children=' + JSON.stringify(timeVNode.children));
        
        return h('div', { style: { backgroundColor: '#e0e0e0', padding: '16px' } }, titleVNode, timeVNode);
    }

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, '嵌套时钟测试'),
            h('h2', null, '1. 简单时钟 (应该工作):'),
            h(SimpleClock, null),
            h('h2', null, '2. 嵌套时钟 (app.js 结构):'),
            h(NestedClock, null),
            h('h2', null, '3. 手动嵌套时钟:'),
            h(ManualNestedClock, null)
        );
    }

    console.log('Starting nested clock test...');
    render(h(App, null), document.body);
    console.log('Nested clock test rendered!');
})();
