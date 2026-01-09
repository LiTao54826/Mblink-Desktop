/**
 * 调试文字基线位置
 * 检查 span 内文字的实际绘制位置
 */

console.log('[TEST_START] Text Baseline Debug');

// 创建一个简单的 flex 容器 + span + 文字结构
const container = document.createElement('div');
container.style.display = 'flex';
container.style.alignItems = 'center';
container.style.height = '40px';
container.style.backgroundColor = '#0f6cbd';
container.style.padding = '0 16px';

const span = document.createElement('span');
span.style.fontSize = '16px';
span.style.lineHeight = 'normal';
span.style.color = 'white';
span.style.backgroundColor = 'rgba(255,0,0,0.3)'; // 红色半透明背景显示 span 边界
span.textContent = 'Large';
container.appendChild(span);

document.body.style.padding = '50px';
document.body.appendChild(container);

// 对照组：line-height: 1
const container2 = document.createElement('div');
container2.style.display = 'flex';
container2.style.alignItems = 'center';
container2.style.height = '40px';
container2.style.backgroundColor = '#22c55e';
container2.style.padding = '0 16px';
container2.style.marginTop = '20px';

const span2 = document.createElement('span');
span2.style.fontSize = '16px';
span2.style.lineHeight = '1';
span2.style.color = 'white';
span2.style.backgroundColor = 'rgba(255,0,0,0.3)';
span2.textContent = 'Large';
container2.appendChild(span2);

document.body.appendChild(container2);

// 对照组：直接文字，不用 span
const container3 = document.createElement('div');
container3.style.display = 'flex';
container3.style.alignItems = 'center';
container3.style.height = '40px';
container3.style.backgroundColor = '#f59e0b';
container3.style.padding = '0 16px';
container3.style.marginTop = '20px';
container3.style.fontSize = '16px';
container3.style.lineHeight = 'normal';
container3.style.color = 'white';
container3.textContent = 'Large';

document.body.appendChild(container3);

setTimeout(() => {
    function analyze(name, cont, textEl) {
        const contRect = cont.getBoundingClientRect();
        const textRect = textEl.getBoundingClientRect();
        
        const textCenter = textRect.top + textRect.height / 2;
        const contCenter = contRect.top + contRect.height / 2;
        const centerDiff = textCenter - contCenter;
        
        console.log('[DEBUG] ' + name + ':');
        console.log('  Container: height=' + contRect.height + ', center=' + contCenter.toFixed(2));
        console.log('  Text: height=' + textRect.height + ', top=' + textRect.top.toFixed(2) + ', center=' + textCenter.toFixed(2));
        console.log('  Center diff (positive=text below center): ' + centerDiff.toFixed(2));
        
        if (Math.abs(centerDiff) < 1) {
            console.log('[TEST_PASS] ' + name + ' text is centered');
        } else {
            console.log('[TEST_FAIL] ' + name + ' text is ' + (centerDiff > 0 ? 'BELOW' : 'ABOVE') + ' center by ' + Math.abs(centerDiff).toFixed(2) + 'px');
        }
    }
    
    analyze('Span line-height:normal', container, span);
    analyze('Span line-height:1', container2, span2);
    analyze('Direct text', container3, container3);
    
    console.log('[TEST_END]');
}, 200);
