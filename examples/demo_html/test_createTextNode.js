/**
 * @file test_createTextNode.js
 * @brief 测试 createTextNode 和 String 函数
 */

console.log('测试开始...');

console.log('测试1: typeof String');
console.log('  typeof String:', typeof String);

console.log('测试2: typeof document.createTextNode');
console.log('  typeof document.createTextNode:', typeof document.createTextNode);

console.log('测试3: String("hello")');
try {
    var result = String("hello");
    console.log('  结果:', result);
} catch (e) {
    console.log('  错误:', e);
}

console.log('测试4: document.createTextNode("hello")');
try {
    var node = document.createTextNode("hello");
    console.log('  结果:', node);
} catch (e) {
    console.log('  错误:', e);
}

console.log('测试5: document.createTextNode(String("hello"))');
try {
    var node2 = document.createTextNode(String("hello"));
    console.log('  结果:', node2);
} catch (e) {
    console.log('  错误:', e);
}

console.log('测试完成！');
