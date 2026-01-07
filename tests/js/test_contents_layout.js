// 测试 display: contents 子元素的布局
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';

const container = document.createElement('div');
container.id = 'test-container';
container.style.border = '2px solid blue';
container.style.padding = '10px';
container.style.display = 'flex';
container.style.flexDirection = 'column';
container.style.gap = '10px';
document.body.appendChild(container);

console.log('[TEST_START] Display Contents Layout Test');

// 渲染
render(
  h('div', { style: { display: 'contents' } },
    h('div', { 
      id: 'item1',
      style: { padding: '10px', backgroundColor: '#fff', border: '1px solid red' } 
    }, 'Item 1'),
    h('div', { 
      id: 'item2',
      style: { padding: '10px', backgroundColor: '#eee', border: '1px solid green' } 
    }, 'Item 2 - longer text here'),
    h('div', { 
      id: 'item3',
      style: { padding: '10px', backgroundColor: '#ddd', border: '1px solid blue' } 
    }, 'Item 3')
  ),
  container
);

setTimeout(() => {
  // 获取每个 item 的布局信息
  const item1 = document.getElementById('item1');
  const item2 = document.getElementById('item2');
  const item3 = document.getElementById('item3');
  
  if (!item1 || !item2 || !item3) {
    console.log('[TEST_FAIL] Could not find items');
    console.log('[DEBUG] item1:', item1);
    console.log('[DEBUG] item2:', item2);
    console.log('[DEBUG] item3:', item3);
    console.log('[TEST_END]');
    return;
  }
  
  const rect1 = item1.getBoundingClientRect();
  const rect2 = item2.getBoundingClientRect();
  const rect3 = item3.getBoundingClientRect();
  
  console.log('[DEBUG] Item 1 rect: x=' + rect1.x + ' y=' + rect1.y + ' w=' + rect1.width + ' h=' + rect1.height);
  console.log('[DEBUG] Item 2 rect: x=' + rect2.x + ' y=' + rect2.y + ' w=' + rect2.width + ' h=' + rect2.height);
  console.log('[DEBUG] Item 3 rect: x=' + rect3.x + ' y=' + rect3.y + ' w=' + rect3.width + ' h=' + rect3.height);
  
  // 验证：在 flex column 布局中，每个 item 应该垂直排列
  // Item 2 应该在 Item 1 下面，Item 3 应该在 Item 2 下面
  const gap = 10; // flex gap
  
  let allPassed = true;
  
  // 检查 Item 2 是否在 Item 1 下面
  if (rect2.y >= rect1.y + rect1.height) {
    console.log('[TEST_PASS] Item 2 is below Item 1');
  } else {
    console.log('[TEST_FAIL] Item 2 is NOT below Item 1');
    allPassed = false;
  }
  
  // 检查 Item 3 是否在 Item 2 下面
  if (rect3.y >= rect2.y + rect2.height) {
    console.log('[TEST_PASS] Item 3 is below Item 2');
  } else {
    console.log('[TEST_FAIL] Item 3 is NOT below Item 2');
    allPassed = false;
  }
  
  // 检查宽度是否合理（不应该是 0）
  if (rect1.width > 0 && rect2.width > 0 && rect3.width > 0) {
    console.log('[TEST_PASS] All items have positive width');
  } else {
    console.log('[TEST_FAIL] Some items have zero width');
    allPassed = false;
  }
  
  // 检查高度是否合理
  if (rect1.height > 0 && rect2.height > 0 && rect3.height > 0) {
    console.log('[TEST_PASS] All items have positive height');
  } else {
    console.log('[TEST_FAIL] Some items have zero height');
    allPassed = false;
  }
  
  if (allPassed) {
    console.log('[TEST_PASS] All layout tests passed');
  }
  
  console.log('[TEST_END]');
}, 300);
