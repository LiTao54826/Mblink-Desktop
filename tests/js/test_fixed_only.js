// tests/js/test_fixed_only.js
// 测试 position: fixed 元素的布局和点击事件

console.log('[TEST_START] Fixed Element Only Tests');

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log('[TEST_PASS] ' + name);
    } else {
        console.log('[TEST_FAIL] ' + name);
    }
}

console.log('[DEBUG] Creating fixed button...');

// 创建一个简单的 fixed 按钮
var btn = document.createElement('button');
btn.id = 'fixed-btn';
btn.textContent = 'Fixed Button';

// 设置样式
btn.style.position = 'fixed';
btn.style.top = '50px';
btn.style.left = '50px';
btn.style.width = '120px';
btn.style.height = '40px';
btn.style.backgroundColor = '#4CAF50';
btn.style.color = 'white';
btn.style.border = 'none';
btn.style.zIndex = '1000';

// 检查样式是否正确设置
console.log('[DEBUG] Style after setting:');
console.log('[DEBUG]   position: ' + btn.style.position);
console.log('[DEBUG]   top: ' + btn.style.top);
console.log('[DEBUG]   left: ' + btn.style.left);

document.body.appendChild(btn);
console.log('[DEBUG] Button appended to body');

// 强制布局
var h = document.body.offsetHeight;

// 检查 getBoundingClientRect 是否可用
console.log('[DEBUG] Testing layout...');
console.log('[DEBUG] btn type: ' + typeof btn);
console.log('[DEBUG] btn.getBoundingClientRect type: ' + typeof btn.getBoundingClientRect);

if (typeof btn.getBoundingClientRect !== 'function') {
    console.log('[ERROR] getBoundingClientRect is not a function!');
    logTest('getBoundingClientRect available', false);
} else {
    console.log('[DEBUG] Calling getBoundingClientRect...');
    var rect = btn.getBoundingClientRect();
    console.log('[DEBUG] Fixed button rect: top=' + rect.top + ', left=' + rect.left + ', width=' + rect.width + ', height=' + rect.height);

    logTest('Fixed element top position (expect 50)', Math.abs(rect.top - 50) < 1);
    logTest('Fixed element left position (expect 50)', Math.abs(rect.left - 50) < 1);
    logTest('Fixed element width (expect 120)', Math.abs(rect.width - 120) < 1);
    logTest('Fixed element height (expect 40)', Math.abs(rect.height - 40) < 1);

    // 测试 elementFromPoint
    console.log('[DEBUG] Testing elementFromPoint...');
    var clickX = 110;  // 50 + 60 (center of button)
    var clickY = 70;   // 50 + 20 (center of button)
    console.log('[DEBUG] Testing point: (' + clickX + ', ' + clickY + ')');

    var elementAtPoint = document.elementFromPoint(clickX, clickY);
    console.log('[DEBUG] Element at point: ' + (elementAtPoint ? (elementAtPoint.id || elementAtPoint.tagName) : 'null'));

    logTest('elementFromPoint finds fixed button', elementAtPoint && elementAtPoint.id === 'fixed-btn');
}

console.log('[TEST_END]');
