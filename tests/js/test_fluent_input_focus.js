/**
 * 测试 Fluent Input 组件的焦点获取
 */

console.log('[TEST_START] Fluent Input Focus Test');

// 创建一个简单的原生 input 测试
const container = document.createElement('div');
container.style.padding = '20px';

// 测试1: 原生 input
const nativeInput = document.createElement('input');
nativeInput.type = 'text';
nativeInput.placeholder = 'Native Input';
nativeInput.style.height = '32px';
nativeInput.style.padding = '0 10px';
nativeInput.style.marginBottom = '10px';
nativeInput.style.display = 'block';
container.appendChild(nativeInput);

// 测试2: 模拟 Fluent Input 结构
const fluentContainer = document.createElement('div');
fluentContainer.style.display = 'inline-flex';
fluentContainer.style.alignItems = 'center';
fluentContainer.style.height = '32px';
fluentContainer.style.border = '1px solid #ccc';
fluentContainer.style.borderRadius = '4px';
fluentContainer.style.marginBottom = '10px';

const fluentInput = document.createElement('input');
fluentInput.type = 'text';
fluentInput.placeholder = 'Fluent-style Input';
fluentInput.style.flex = '1';
fluentInput.style.height = '100%';
fluentInput.style.border = 'none';
fluentInput.style.outline = 'none';
fluentInput.style.backgroundColor = 'transparent';
fluentInput.style.padding = '0 10px';

fluentContainer.appendChild(fluentInput);
container.appendChild(fluentContainer);

document.body.appendChild(container);

// 测试焦点
setTimeout(() => {
    console.log('[DEBUG] Testing native input focus...');
    nativeInput.focus();
    
    setTimeout(() => {
        const nativeFocused = document.activeElement === nativeInput;
        console.log('[DEBUG] Native input focused: ' + nativeFocused);
        
        if (nativeFocused) {
            console.log('[TEST_PASS] Native input can receive focus');
        } else {
            console.log('[TEST_FAIL] Native input cannot receive focus');
        }
        
        // 测试 fluent 风格 input
        console.log('[DEBUG] Testing fluent-style input focus...');
        fluentInput.focus();
        
        setTimeout(() => {
            const fluentFocused = document.activeElement === fluentInput;
            console.log('[DEBUG] Fluent-style input focused: ' + fluentFocused);
            console.log('[DEBUG] Active element: ' + (document.activeElement ? document.activeElement.tagName : 'null'));
            
            if (fluentFocused) {
                console.log('[TEST_PASS] Fluent-style input can receive focus');
            } else {
                console.log('[TEST_FAIL] Fluent-style input cannot receive focus');
            }
            
            console.log('[TEST_END]');
        }, 100);
    }, 100);
}, 200);
