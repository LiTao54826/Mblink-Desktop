/**
 * CSS 动画测试 - 使用内联样式
 * 
 * 使用方法: esm_loader.exe test_animation_inline.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    console.log('=== CSS 动画测试 (内联样式) ===');

    // 基础盒子样式
    var boxBase = {
        width: '100px',
        height: '100px',
        margin: '20px',
        display: 'inline-block',
        borderRadius: '10px',
        textAlign: 'center',
        lineHeight: '100px',
        color: 'white',
        fontWeight: 'bold',
        fontSize: '14px'
    };

    // 淡入动画盒子
    function FadeBox() {
        var state = useState(0);
        var opacity = state[0];
        var setOpacity = state[1];
        var direction = useState(1);
        var dir = direction[0];
        var setDir = direction[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setOpacity(function(o) {
                    var newO = o + dir * 0.05;
                    if (newO >= 1) {
                        setDir(-1);
                        return 1;
                    } else if (newO <= 0) {
                        setDir(1);
                        return 0;
                    }
                    return newO;
                });
            }, 50);
            return function() { clearInterval(timer); };
        }, [dir]);

        var style = Object.assign({}, boxBase, {
            background: '#3498db',
            opacity: opacity
        });

        return h('div', { style: style }, 'Fade');
    }

    // 滑动动画盒子
    function SlideBox() {
        var state = useState(0);
        var x = state[0];
        var setX = state[1];
        var direction = useState(1);
        var dir = direction[0];
        var setDir = direction[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setX(function(pos) {
                    var newPos = pos + dir * 5;
                    if (newPos >= 150) {
                        setDir(-1);
                        return 150;
                    } else if (newPos <= 0) {
                        setDir(1);
                        return 0;
                    }
                    return newPos;
                });
            }, 30);
            return function() { clearInterval(timer); };
        }, [dir]);

        var style = Object.assign({}, boxBase, {
            background: '#e74c3c',
            transform: 'translateX(' + x + 'px)'
        });

        return h('div', { style: style }, 'Slide');
    }

    // 旋转动画盒子
    function SpinBox() {
        var state = useState(0);
        var angle = state[0];
        var setAngle = state[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setAngle(function(a) { return (a + 5) % 360; });
            }, 30);
            return function() { clearInterval(timer); };
        }, []);

        var style = Object.assign({}, boxBase, {
            background: '#2ecc71',
            transform: 'rotate(' + angle + 'deg)'
        });

        return h('div', { style: style }, 'Spin');
    }

    // 缩放动画盒子
    function PulseBox() {
        var state = useState(1);
        var scale = state[0];
        var setScale = state[1];
        var direction = useState(1);
        var dir = direction[0];
        var setDir = direction[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setScale(function(s) {
                    var newS = s + dir * 0.02;
                    if (newS >= 1.3) {
                        setDir(-1);
                        return 1.3;
                    } else if (newS <= 1) {
                        setDir(1);
                        return 1;
                    }
                    return newS;
                });
            }, 30);
            return function() { clearInterval(timer); };
        }, [dir]);

        var style = Object.assign({}, boxBase, {
            background: '#9b59b6',
            transform: 'scale(' + scale + ')'
        });

        return h('div', { style: style }, 'Pulse');
    }

    // 颜色变化盒子
    function ColorBox() {
        var state = useState(0);
        var hue = state[0];
        var setHue = state[1];

        useEffect(function() {
            var timer = setInterval(function() {
                setHue(function(h) { return (h + 2) % 360; });
            }, 30);
            return function() { clearInterval(timer); };
        }, []);

        // 简单的 HSL 到 RGB 转换
        var r, g, b;
        var h2 = hue / 60;
        var x = 1 - Math.abs(h2 % 2 - 1);
        if (h2 < 1) { r = 1; g = x; b = 0; }
        else if (h2 < 2) { r = x; g = 1; b = 0; }
        else if (h2 < 3) { r = 0; g = 1; b = x; }
        else if (h2 < 4) { r = 0; g = x; b = 1; }
        else if (h2 < 5) { r = x; g = 0; b = 1; }
        else { r = 1; g = 0; b = x; }

        var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';

        var style = Object.assign({}, boxBase, {
            background: color
        });

        return h('div', { style: style }, 'Color');
    }

    // 主应用
    function App() {
        return h('div', { style: { padding: '20px' } },
            h('h1', { style: { color: '#2c3e50', marginBottom: '20px' } }, 'Animation Demo (JS)'),
            h('p', { style: { color: '#7f8c8d', marginBottom: '20px' } }, '使用 JavaScript 实现的动画效果'),
            h('div', null,
                h(FadeBox),
                h(SlideBox),
                h(SpinBox),
                h(PulseBox),
                h(ColorBox)
            )
        );
    }

    console.log('渲染中...');
    render(h(App), document.body);
    console.log('完成');
})();
