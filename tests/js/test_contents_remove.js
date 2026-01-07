// 测试 display: contents 移除元素
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';

const container = document.createElement('div');
container.id = 'test-container';
container.style.border = '2px solid blue';
container.style.padding = '10px';
document.body.appendChild(container);

console.log('[TEST_START] Display Contents Remove Test');

function printDOM() {
  console.log('[DEBUG] === DOM Structure ===');
  const wrapper = container.children[0];
  if (wrapper) {
    console.log('[DEBUG] Wrapper children: ' + wrapper.children.length);
    for (let i = 0; i < wrapper.children.length; i++) {
      const child = wrapper.children[i];
      console.log('[DEBUG]   Child ' + i + ': id=' + child.id + ' text=' + child.textContent);
    }
  } else {
    console.log('[DEBUG] No wrapper found');
  }
}

// Step 1: 渲染 3 个 items
console.log('[DEBUG] Step 1: Render 3 items');
render(
  h('div', { style: { display: 'contents' } },
    h('div', { key: 'a', id: 'item-a' }, 'Item A'),
    h('div', { key: 'b', id: 'item-b' }, 'Item B'),
    h('div', { key: 'c', id: 'item-c' }, 'Item C')
  ),
  container
);

setTimeout(() => {
  printDOM();
  
  // Step 2: 移除中间的 item
  console.log('[DEBUG] Step 2: Remove middle item (B)');
  render(
    h('div', { style: { display: 'contents' } },
      h('div', { key: 'a', id: 'item-a' }, 'Item A'),
      h('div', { key: 'c', id: 'item-c' }, 'Item C')
    ),
    container
  );
  
  setTimeout(() => {
    printDOM();
    
    // 验证
    const itemA = document.getElementById('item-a');
    const itemB = document.getElementById('item-b');
    const itemC = document.getElementById('item-c');
    
    if (itemA && !itemB && itemC) {
      console.log('[TEST_PASS] Correct: A exists, B removed, C exists');
    } else {
      console.log('[TEST_FAIL] Wrong state:');
      console.log('[DEBUG] itemA:', itemA ? 'exists' : 'missing');
      console.log('[DEBUG] itemB:', itemB ? 'exists (should be removed)' : 'removed');
      console.log('[DEBUG] itemC:', itemC ? 'exists' : 'missing');
    }
    
    console.log('[TEST_END]');
  }, 200);
}, 200);
