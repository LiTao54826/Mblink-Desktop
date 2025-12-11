console.log('测试 setTimeout 调试版...');

var count = 0;

function tick() {
    count++;
    console.log('Tick #' + count);
    if (count < 5) {
        setTimeout(tick, 500);
    }
}

console.log('调用 setTimeout...');
var timerId = setTimeout(tick, 100);
console.log('setTimeout 返回的 timer ID:', timerId);

console.log('脚本执行完毕，等待定时器回调...');
