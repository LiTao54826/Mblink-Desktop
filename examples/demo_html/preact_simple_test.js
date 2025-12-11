/**
 * @file preact_simple_test.js
 * @brief 简单的 Preact 测试 - 验证基本功能
 */

console.log('🧪 简单 Preact 测试开始...');

// 测试1: 渲染简单文本
function Test1() {
    return preact.h('div', null, 'Hello Preact!');
}

console.log('测试1: 渲染简单组件');
preact.render(preact.h(Test1), document.body);

console.log('✅ 测试完成！');
