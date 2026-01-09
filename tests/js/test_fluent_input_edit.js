/**
 * 测试 Fluent Input 组件的编辑功能
 */

import { h, render } from 'preact';
import { Input } from '../../js/fluent/index.js';

console.log('[TEST_START] Fluent Input Edit Test');

function App() {
    return h('div', { style: { padding: '20px' } }, [
        h('div', { key: 'label', style: { marginBottom: '8px' } }, 'Fluent Input:'),
        h(Input, { 
            key: 'input',
            placeholder: 'Type here...',
            id: 'test-input'
        }),
        h('div', { key: 'native-label', style: { marginTop: '20px', marginBottom: '8px' } }, 'Native Input:'),
        h('input', {
            key: 'native',
            type: 'text',
            placeholder: 'Native input',
            id: 'native-input',
            style: { height: '32px', padding: '0 10px' }
        })
    ]);
}

render(h(App), document.body);

setTimeout(() => {
    // 查找所有 input 元素
    const allInputs = document.querySelectorAll('input');
    console.log('[DEBUG] Total input elements: ' + allInputs.length);
    
    for (let i = 0; i < allInputs.length; i++) {
        console.log('[DEBUG] Input ' + i + ': id=' + (allInputs[i].id || 'none'));
    }
    
    const nativeInput = document.getElementById('native-input');
    const fluentInput = document.getElementById('test-input');
    
    console.log('[DEBUG] Fluent input element: ' + (fluentInput ? fluentInput.tagName : 'null'));
    console.log('[DEBUG] Native input element: ' + (nativeInput ? nativeInput.tagName : 'null'));
    
    // 测试原生 input
    if (nativeInput) {
        nativeInput.focus();
        setTimeout(() => {
            const focused = document.activeElement === nativeInput;
            console.log('[DEBUG] Native input focused: ' + focused);
            if (focused) {
                console.log('[TEST_PASS] Native input focus');
            } else {
                console.log('[TEST_FAIL] Native input focus');
            }
            
            // 测试 fluent input
            if (fluentInput) {
                fluentInput.focus();
                setTimeout(() => {
                    const fluentFocused = document.activeElement === fluentInput;
                    console.log('[DEBUG] Fluent input focused: ' + fluentFocused);
                    console.log('[DEBUG] Active element tag: ' + (document.activeElement ? document.activeElement.tagName : 'null'));
                    
                    if (fluentFocused) {
                        console.log('[TEST_PASS] Fluent input focus');
                    } else {
                        console.log('[TEST_FAIL] Fluent input focus');
                    }
                    
                    console.log('[TEST_END]');
                }, 100);
            } else {
                console.log('[TEST_FAIL] Fluent input not found');
                console.log('[TEST_END]');
            }
        }, 100);
    } else {
        console.log('[TEST_FAIL] Native input not found');
        console.log('[TEST_END]');
    }
}, 500);
