/**
 * @file test_click_event.js
 * @brief 测试按钮点击事件
 */

console.log('🧪 测试按钮点击事件...');

// 测试1: 原生 DOM 点击事件
console.log('');
console.log('测试1: 原生 DOM 点击事件');
var nativeButton = document.createElement('button');
nativeButton.textContent = '原生按钮 (点击我)';
nativeButton.style.setProperty('padding', '10px 20px');
nativeButton.style.setProperty('margin', '10px');
nativeButton.style.setProperty('font-size', '16px');
nativeButton.addEventListener('click', function () {
    console.log('✅ 原生按钮被点击了！');
});
document.body.appendChild(nativeButton);

// 测试2: Preact 点击事件 (使用 onclick)
console.log('');
console.log('测试2: Preact 点击事件 (onclick)');

var clickCount = 0;

function PreactButton() {
    var state = preactHooks.useState(0);
    var count = state[0];
    var setCount = state[1];

    console.log('PreactButton 渲染, count =', count);

    return preact.h('div', { style: 'margin: 10px;' },
        preact.h('button', {
            onclick: function () {
                console.log('🎯 Preact 按钮点击事件触发！当前 count =', count);
                setCount(count + 1);
            },
            style: 'padding: 10px 20px; font-size: 16px; background: #3498db; color: white; border: none;'
        }, 'Preact 按钮 (点击计数: ' + count + ')')
    );
}

preact.render(preact.h(PreactButton), document.body);

console.log('');
console.log('✅ 测试页面加载完成');
console.log('请点击按钮测试事件处理...');
