/**
 * 滚动测试 - 用于调试滚动后文字消失的问题
 */

var h = Preact.h;
var render = Preact.render;

// 创建多个带背景色的div来测试滚动
function ScrollTest() {
    var boxes = [];
    for (var i = 0; i < 20; i++) {
        var bgColor = i % 2 === 0 ? '#4CAF50' : '#2196F3';
        boxes.push(
            h('div', { 
                key: i,
                style: 'background: ' + bgColor + '; color: white; padding: 20px; margin: 10px 0; text-align: center;' 
            }, 'Box ' + (i + 1) + ' - 这是第 ' + (i + 1) + ' 个测试框')
        );
    }
    
    return h('div', { style: 'padding: 20px;' },
        h('h1', { style: 'margin: 0 0 20px 0;' }, '滚动测试'),
        h('p', { style: 'margin: 0 0 20px 0;' }, '请滚动页面，观察文字是否消失'),
        boxes
    );
}

// 渲染应用
render(h(ScrollTest), document.body);
console.log('Scroll test app rendered!');

