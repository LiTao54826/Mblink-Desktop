// 测试 Preact 渲染 display: contents 的行为
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f0f0f0';

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

console.log("[TEST_START] Preact Display Contents in Flex Container");

// 创建 flex 容器
const container = document.createElement('div');
container.id = 'flex-container';
Object.assign(container.style, {
    position: 'fixed',
    top: '20px',
    left: '50%',
    transform: 'translateX(-50%)',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '8px',
    border: '2px dashed red',
    padding: '10px',
});
document.body.appendChild(container);

// 使用 Preact 渲染
function Item({ content, id }) {
    return h('div', {
        id: id,
        style: {
            padding: '10px 16px',
            backgroundColor: '#ffffff',
            borderRadius: '6px',
            boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
        }
    }, content);
}

// 渲染两个不同宽度的元素
render(
    h('div', { style: { display: 'contents' } },
        h(Item, { id: 'long-item', content: '这是一个很长很长很长很长很长的内容' }),
        h(Item, { id: 'short-item', content: '短内容' })
    ),
    container
);

// 检查 DOM 结构
setTimeout(() => {
    console.log('Container children count:', container.childNodes.length);
    for (let i = 0; i < container.childNodes.length; i++) {
        const child = container.childNodes[i];
        console.log(`Child ${i}: tagName=${child.tagName}, id=${child.id}, display=${child.style?.display}`);
    }
    
    const longItem = document.getElementById('long-item');
    const shortItem = document.getElementById('short-item');
    
    if (!longItem || !shortItem) {
        console.log('[TEST_FAIL] Items not found');
        console.log("[TEST_END]");
        return;
    }
    
    const longRect = longItem.getBoundingClientRect();
    const shortRect = shortItem.getBoundingClientRect();
    
    console.log(`Long item: width=${longRect.width}, left=${longRect.left}`);
    console.log(`Short item: width=${shortRect.width}, left=${shortRect.left}`);
    
    // 短元素的宽度应该小于长元素
    const widthsDifferent = shortRect.width < longRect.width;
    logTest('Short item should be narrower than long item', widthsDifferent);
    
    // 检查是否居中
    const longCenter = longRect.left + longRect.width / 2;
    const shortCenter = shortRect.left + shortRect.width / 2;
    const centered = Math.abs(longCenter - shortCenter) < 2;
    logTest('Both items should be centered', centered);
    
    console.log(`Long item center: ${longCenter}`);
    console.log(`Short item center: ${shortCenter}`);
    
    console.log("[TEST_END]");
}, 200);
