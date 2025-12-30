// 对比 Preact 渲染和原生 DOM 渲染的差异
import { h, render } from 'preact';
import { Button } from '../../js/fluent/Button.js';

// 创建容器
const container = document.createElement('div');
container.style.padding = '20px';
container.style.backgroundColor = '#f0f0f0';
container.style.display = 'flex';
container.style.flexDirection = 'column';
container.style.gap = '20px';
document.body.appendChild(container);

// ========== 原生 DOM 按钮 ==========
const nativeSection = document.createElement('div');
nativeSection.innerHTML = '<h3>原生 DOM 按钮</h3>';
container.appendChild(nativeSection);

const nativeRow = document.createElement('div');
nativeRow.style.display = 'flex';
nativeRow.style.gap = '10px';
nativeRow.style.alignItems = 'flex-start';
nativeSection.appendChild(nativeRow);

// 原生按钮 - 完全模拟 Fluent Button 的样式
function createNativeButton(text, size) {
    const sizeConfig = {
        small: { height: '24px', minWidth: '64px', fontSize: '12px', padding: '4px 8px' },
        medium: { height: '32px', minWidth: '96px', fontSize: '14px', padding: '6px 12px' },
        large: { height: '40px', minWidth: '96px', fontSize: '16px', padding: '8px 16px' }
    }[size];
    
    const btn = document.createElement('button');
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.gap = '6px';
    btn.style.height = sizeConfig.height;
    btn.style.minWidth = sizeConfig.minWidth;
    btn.style.padding = sizeConfig.padding;
    btn.style.fontSize = sizeConfig.fontSize;
    btn.style.fontFamily = "'Segoe UI', sans-serif";
    btn.style.fontWeight = '600';
    btn.style.lineHeight = 'normal';
    btn.style.cursor = 'pointer';
    btn.style.outline = 'none';
    btn.style.whiteSpace = 'nowrap';
    btn.style.userSelect = 'none';
    btn.style.boxSizing = 'border-box';
    btn.style.verticalAlign = 'middle';
    btn.style.borderRadius = '4px';
    btn.style.backgroundColor = '#ffffff';
    btn.style.color = '#050505';
    btn.style.border = '1px solid #c4c4c4';
    
    const span = document.createElement('span');
    span.textContent = text;
    btn.appendChild(span);
    
    return btn;
}

nativeRow.appendChild(createNativeButton('Native Small', 'small'));
nativeRow.appendChild(createNativeButton('Native Medium', 'medium'));
nativeRow.appendChild(createNativeButton('Native Large', 'large'));

// ========== Preact Fluent 按钮 ==========
const preactSection = document.createElement('div');
preactSection.innerHTML = '<h3>Preact Fluent 按钮</h3>';
container.appendChild(preactSection);

const preactContainer = document.createElement('div');
preactSection.appendChild(preactContainer);

function PreactButtons() {
    return h('div', { 
        style: { 
            display: 'flex', 
            gap: '10px',
            alignItems: 'flex-start'
        } 
    }, [
        h(Button, { key: 'small', size: 'small' }, 'Preact Small'),
        h(Button, { key: 'medium', size: 'medium' }, 'Preact Medium'),
        h(Button, { key: 'large', size: 'large' }, 'Preact Large')
    ]);
}

render(h(PreactButtons), preactContainer);

// ========== 调试信息 ==========
setTimeout(() => {
    console.log('=== 调试信息 ===');
    
    // 检查原生按钮
    const nativeButtons = nativeRow.querySelectorAll('button');
    nativeButtons.forEach((btn, i) => {
        const span = btn.querySelector('span');
        console.log(`原生按钮 ${i}:`, {
            btnHeight: btn.offsetHeight,
            spanHeight: span?.offsetHeight,
            btnStyle: btn.style.cssText
        });
    });
    
    // 检查 Preact 按钮
    const preactButtons = preactContainer.querySelectorAll('button');
    preactButtons.forEach((btn, i) => {
        const span = btn.querySelector('span');
        console.log(`Preact按钮 ${i}:`, {
            btnHeight: btn.offsetHeight,
            spanHeight: span?.offsetHeight,
            btnStyle: btn.style.cssText
        });
    });
}, 100);
