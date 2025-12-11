console.log('测试文本节点更新重绘...');

// 创建一个显示计数的 div
var div = document.createElement('div');
div.style.setProperty('font-size', '48px');
div.style.setProperty('padding', '20px');
div.style.setProperty('text-align', 'center');
document.body.appendChild(div);

// 直接添加文本
var textNode = document.createTextNode('计数: 0');
div.appendChild(textNode);

var count = 0;

// 每秒更新计数
setInterval(function () {
    count++;
    console.log('更新计数:', count);
    textNode.textContent = '计数: ' + count;
}, 1000);

console.log('✅ 测试启动，文本应该每秒更新');
