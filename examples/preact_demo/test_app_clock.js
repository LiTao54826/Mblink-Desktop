/**
 * @file test_app_clock.js
 * @brief 测试 app.js 中的时钟组件
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    // 和 app.js 完全一样的样式
    var styles = {
        card: {
            backgroundColor: '#f5f5f5',
            borderRadius: '8px',
            padding: '16px',
            marginBottom: '16px',
            boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
        }
    };

    function formatTime(date) {
        var hours = date.getHours();
        var minutes = date.getMinutes();
        var seconds = date.getSeconds();
        var hh = hours < 10 ? '0' + hours : '' + hours;
        var mm = minutes < 10 ? '0' + minutes : '' + minutes;
        var ss = seconds < 10 ? '0' + seconds : '' + seconds;
        return hh + ':' + mm + ':' + ss;
    }

    // 和 app.js 完全一样的 Clock 组件
    function Clock() {
        var timeState = useState(formatTime(new Date()));
        var time = timeState[0];
        var setTime = timeState[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
            }, 1000);
            return function () {
                clearInterval(timer);
            };
        }, []);

        console.log('[Clock] time=' + time);

        return h(
            'div',
            { style: styles.card },
            h('h3', null, '实时时钟'),
            h('div', {
                style: {
                    fontSize: '32px',
                    textAlign: 'center',
                    fontFamily: 'monospace',
                    color: '#28a745'
                }
            }, time)
        );
    }

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, 'Clock Test (app.js style)'),
            h(Clock, null)
        );
    }

    console.log('Starting clock test...');
    render(h(App, null), document.body);
    console.log('Clock test rendered!');
})();
