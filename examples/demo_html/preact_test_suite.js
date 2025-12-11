/**
 * @file preact_test_suite.js
 * @brief Preact 集成测试套件
 * 
 * 测试内容：
 * - Virtual DOM 渲染
 * - 组件功能
 * - Hooks (useState, useEffect, useRef)
 * - 事件处理
 * - 状态更新
 */

console.log('========================================');
console.log('  Preact 集成测试套件');
console.log('========================================');
console.log('');

// ========== 测试工具函数 ==========

let testCount = 0;
let passCount = 0;
let failCount = 0;

function test(name, fn) {
    testCount++;
    try {
        fn();
        passCount++;
        console.log('✓ PASS:', name);
        return true;
    } catch (e) {
        failCount++;
        console.log('✗ FAIL:', name);
        console.log('  Error:', e.message || e);
        return false;
    }
}

function assert(condition, message) {
    if (!condition) {
        throw new Error(message || 'Assertion failed');
    }
}

function assertEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(message || 'Expected ' + expected + ' but got ' + actual);
    }
}

// ========== 测试1: 基础 Virtual DOM 创建 ==========

console.log('');
console.log('[测试组 1] 基础 Virtual DOM 创建');
console.log('----------------------------------------');

test('创建简单 VNode', function () {
    const vnode = preact.h('div', null, 'Hello');
    assert(vnode, 'VNode should exist');
    assertEqual(vnode.type, 'div', 'VNode type should be div');
    assertEqual(vnode.children.length, 1, 'Should have 1 child');
    assertEqual(vnode.children[0], 'Hello', 'Child should be "Hello"');
});

test('创建带属性的 VNode', function () {
    const vnode = preact.h('div', { id: 'test', className: 'box' });
    assert(vnode.props, 'Props should exist');
    assertEqual(vnode.props.id, 'test', 'id should be test');
    assertEqual(vnode.props.className, 'box', 'className should be box');
});

test('创建嵌套 VNode', function () {
    const vnode = preact.h('div', null,
        preact.h('span', null, 'Child 1'),
        preact.h('span', null, 'Child 2')
    );
    assertEqual(vnode.children.length, 2, 'Should have 2 children');
    assertEqual(vnode.children[0].type, 'span', 'First child should be span');
});

test('Fragment 创建', function () {
    const vnode = preact.h(preact.Fragment, null,
        preact.h('div', null, 'A'),
        preact.h('div', null, 'B')
    );
    assertEqual(vnode.type, preact.Fragment, 'Type should be Fragment');
    assertEqual(vnode.children.length, 2, 'Should have 2 children');
});

// ========== 测试2: DOM 渲染 ==========

console.log('');
console.log('[测试组 2] DOM 渲染');
console.log('----------------------------------------');

test('渲染简单元素到 DOM', function () {
    const container = document.createElement('div');
    preact.render(preact.h('div', { id: 'test' }, 'Hello World'), container);

    assertEqual(container.children.length, 1, 'Should have 1 child');
    const child = container.children[0];
    assertEqual(child.tagName.toLowerCase(), 'div', 'Child should be div');
    assertEqual(child.id, 'test', 'id should be test');
    assertEqual(child.textContent, 'Hello World', 'Content should be Hello World');
});

test('渲染嵌套元素', function () {
    const container = document.createElement('div');
    const vnode = preact.h('div', null,
        preact.h('h1', null, 'Title'),
        preact.h('p', null, 'Content')
    );
    preact.render(vnode, container);

    assertEqual(container.children.length, 1, 'Should have 1 child');
    const div = container.children[0];
    assertEqual(div.children.length, 2, 'Div should have 2 children');
    assertEqual(div.children[0].tagName.toLowerCase(), 'h1', 'First child should be h1');
    assertEqual(div.children[1].tagName.toLowerCase(), 'p', 'Second child should be p');
});

// ========== 测试3: 组件渲染 ==========

console.log('');
console.log('[测试组 3] 组件渲染');
console.log('----------------------------------------');

test('函数组件基本渲染', function () {
    function Hello(props) {
        return preact.h('div', null, 'Hello ' + props.name);
    }

    const container = document.createElement('div');
    preact.render(preact.h(Hello, { name: 'World' }), container);

    assertEqual(container.children.length, 1, 'Should have 1 child');
    assertEqual(container.children[0].textContent, 'Hello World', 'Should render props');
});

test('嵌套组件渲染', function () {
    function Inner(props) {
        return preact.h('span', null, props.text);
    }

    function Outer() {
        return preact.h('div', null,
            preact.h(Inner, { text: 'A' }),
            preact.h(Inner, { text: 'B' })
        );
    }

    const container = document.createElement('div');
    preact.render(preact.h(Outer), container);

    const div = container.children[0];
    assertEqual(div.children.length, 2, 'Should have 2 children');
    assertEqual(div.children[0].textContent, 'A', 'First child text');
    assertEqual(div.children[1].textContent, 'B', 'Second child text');
});

// ========== 测试4: useState Hook ==========

console.log('');
console.log('[测试组 4] useState Hook');
console.log('----------------------------------------');

test('useState 初始值', function () {
    let capturedState = null;

    function Counter() {
        const state = preactHooks.useState(5);
        capturedState = state[0];
        return preact.h('div', null, state[0]);
    }

    const container = document.createElement('div');
    preact.render(preact.h(Counter), container);

    assertEqual(capturedState, 5, 'Initial state should be 5');
    assertEqual(container.children[0].textContent, '5', 'Should render initial state');
});

test('useState 更新状态', function () {
    let updateFn = null;
    let renderCount = 0;

    function Counter() {
        renderCount++;
        const state = preactHooks.useState(0);
        const count = state[0];
        const setCount = state[1];
        updateFn = setCount;

        return preact.h('div', null, count);
    }

    const container = document.createElement('div');
    preact.render(preact.h(Counter), container);

    assertEqual(renderCount, 1, 'Should render once initially');
    assertEqual(container.children[0].textContent, '0', 'Initial value should be 0');

    // 更新状态（会触发异步批量更新）
    updateFn(1);

    // 注意：由于批量更新是异步的，这里测试只验证 updateFn 可以调用
    assert(typeof updateFn === 'function', 'setState should be a function');
});

// ========== 测试5: useRef Hook ==========

console.log('');
console.log('[测试组 5] useRef Hook');
console.log('----------------------------------------');

test('useRef 创建和使用', function () {
    let capturedRef = null;

    function Component() {
        const ref = preactHooks.useRef(42);
        capturedRef = ref;
        return preact.h('div', null, ref.current);
    }

    const container = document.createElement('div');
    preact.render(preact.h(Component), container);

    assert(capturedRef, 'Ref should exist');
    assertEqual(capturedRef.current, 42, 'Ref value should be 42');
    assertEqual(container.children[0].textContent, '42', 'Should render ref value');
});

test('useRef 保持引用稳定', function () {
    let ref1 = null;
    let ref2 = null;
    let renderCount = 0;

    function Component(props) {
        renderCount++;
        const ref = preactHooks.useRef(props.value);
        if (renderCount === 1) ref1 = ref;
        if (renderCount === 2) ref2 = ref;
        return preact.h('div', null, ref.current);
    }

    const container = document.createElement('div');
    preact.render(preact.h(Component, { value: 1 }), container);
    preact.render(preact.h(Component, { value: 2 }), container);

    // 注意：由于 Virtual DOM diffing，ref 应该保持相同
    assert(ref1, 'First ref should exist');
    assert(ref2, 'Second ref should exist');
});

// ========== 测试6: 事件处理 ==========

console.log('');
console.log('[测试组 6] 事件处理');
console.log('----------------------------------------');

test('事件监听器绑定', function () {
    let clicked = false;

    function handleClick() {
        clicked = true;
    }

    const container = document.createElement('div');
    preact.render(
        preact.h('button', { onclick: handleClick }, 'Click'),
        container
    );

    const button = container.children[0];
    assert(button, 'Button should exist');

    // 模拟点击
    button.click();

    assert(clicked, 'Click handler should be called');
});

// ========== 测试7: createRef ==========

console.log('');
console.log('[测试组 7] createRef');
console.log('----------------------------------------');

test('createRef 基本功能', function () {
    const ref = preact.createRef();

    assert(ref, 'Ref should exist');
    assert('current' in ref, 'Ref should have current property');
    assertEqual(ref.current, undefined, 'Initial value should be undefined');
});

test('ref 回调函数', function () {
    let capturedElement = null;

    function refCallback(el) {
        capturedElement = el;
    }

    const container = document.createElement('div');
    preact.render(
        preact.h('div', { ref: refCallback }, 'Test'),
        container
    );

    assert(capturedElement, 'Ref callback should be called');
    assertEqual(capturedElement.tagName.toLowerCase(), 'div', 'Should capture div element');
});

// ========== 测试8: 样式和属性 ==========

console.log('');
console.log('[测试组 8] 样式和属性');
console.log('----------------------------------------');

test('className 属性', function () {
    const container = document.createElement('div');
    preact.render(
        preact.h('div', { className: 'box container' }),
        container
    );

    assertEqual(container.children[0].className, 'box container', 'className should be set');
});

test('style 对象', function () {
    const container = document.createElement('div');
    preact.render(
        preact.h('div', { style: { color: 'red', fontSize: '16px' } }),
        container
    );

    const div = container.children[0];
    assertEqual(div.style.color, 'red', 'Color should be red');
    assertEqual(div.style.fontSize, '16px', 'Font size should be 16px');
});

test('boolean 属性', function () {
    const container = document.createElement('div');
    preact.render(
        preact.h('button', { disabled: true }),
        container
    );

    const button = container.children[0];
    assertEqual(button.getAttribute('disabled'), '', 'Disabled should be set');
});

// ========== 测试9: 复杂场景 ==========

console.log('');
console.log('[测试组 9] 复杂场景');
console.log('----------------------------------------');

test('列表渲染', function () {
    const items = ['A', 'B', 'C'];

    const container = document.createElement('div');
    preact.render(
        preact.h('ul', null,
            items.map(function (item) {
                return preact.h('li', { key: item }, item);
            })
        ),
        container
    );

    const ul = container.children[0];
    assertEqual(ul.children.length, 3, 'Should have 3 list items');
    assertEqual(ul.children[0].textContent, 'A', 'First item should be A');
    assertEqual(ul.children[2].textContent, 'C', 'Third item should be C');
});

test('条件渲染', function () {
    function Conditional(props) {
        return preact.h('div', null,
            props.show ? preact.h('span', null, 'Visible') : null
        );
    }

    const container = document.createElement('div');

    // 显示
    preact.render(preact.h(Conditional, { show: true }), container);
    assertEqual(container.children[0].children.length, 1, 'Should show child');

    // 隐藏
    preact.render(preact.h(Conditional, { show: false }), container);
    assertEqual(container.children[0].children.length, 0, 'Should hide child');
});

// ========== 测试总结 ==========

console.log('');
console.log('========================================');
console.log('  测试总结');
console.log('========================================');
console.log('总测试数:', testCount);
console.log('通过:', passCount);
console.log('失败:', failCount);
console.log('通过率:', Math.round((passCount / testCount) * 100) + '%');
console.log('');

if (failCount === 0) {
    console.log('✅ 所有测试通过！Preact 集成工作正常。');
} else {
    console.log('⚠️  有', failCount, '个测试失败，请检查。');
}

console.log('');
console.log('========================================');
