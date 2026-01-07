// 测试 display: contents 增量更新
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

console.log('[TEST_START] Display Contents Incremental Update');

let items = [];

function renderItems() {
  render(
    h('div', { style: { display: 'contents' } },
      items.map((item, i) => 
        h('div', { 
          key: item.id,
          id: 'item-' + item.id,
          style: { 
            padding: '10px', 
            backgroundColor: i % 2 === 0 ? '#fff' : '#eee',
            border: '1px solid #ccc'
          } 
        }, item.text)
      )
    ),
    container
  );
}

function checkLayout() {
  console.log('[DEBUG] Checking layout for ' + items.length + ' items');
  
  let prevBottom = 0;
  let allPassed = true;
  
  for (let i = 0; i < items.length; i++) {
    const elem = document.getElementById('item-' + items[i].id);
    if (!elem) {
      console.log('[TEST_FAIL] Item ' + items[i].id + ' not found in DOM');
      allPassed = false;
      continue;
    }
    
    const rect = elem.getBoundingClientRect();
    console.log('[DEBUG] Item ' + items[i].id + ': y=' + Math.round(rect.y) + 
      ' h=' + Math.round(rect.height) + ' text=' + items[i].text.substring(0, 15));
    
    if (i > 0) {
      // 检查是否在前一个元素下面
      if (rect.y < prevBottom - 1) { // 允许 1px 误差
        console.log('[TEST_FAIL] Item ' + items[i].id + ' overlaps with previous item');
        console.log('[DEBUG] Expected y >= ' + prevBottom + ', got ' + rect.y);
        allPassed = false;
      }
    }
    
    if (rect.height <= 0) {
      console.log('[TEST_FAIL] Item ' + items[i].id + ' has zero height');
      allPassed = false;
    }
    
    prevBottom = rect.y + rect.height;
  }
  
  return allPassed;
}

// 测试序列
setTimeout(() => {
  // Step 1: 添加第一个 item
  console.log('[DEBUG] Step 1: Add first item');
  items = [{ id: 1, text: 'First item' }];
  renderItems();
  
  setTimeout(() => {
    if (checkLayout()) {
      console.log('[TEST_PASS] Step 1 layout correct');
    }
    
    // Step 2: 添加第二个 item
    console.log('[DEBUG] Step 2: Add second item');
    items = [
      { id: 1, text: 'First item' },
      { id: 2, text: 'Second item - this is longer' }
    ];
    renderItems();
    
    setTimeout(() => {
      if (checkLayout()) {
        console.log('[TEST_PASS] Step 2 layout correct');
      }
      
      // Step 3: 添加第三个 item
      console.log('[DEBUG] Step 3: Add third item');
      items = [
        { id: 1, text: 'First item' },
        { id: 2, text: 'Second item - this is longer' },
        { id: 3, text: 'Third' }
      ];
      renderItems();
      
      setTimeout(() => {
        if (checkLayout()) {
          console.log('[TEST_PASS] Step 3 layout correct');
        }
        
        // Step 4: 移除中间的 item
        console.log('[DEBUG] Step 4: Remove middle item');
        items = [
          { id: 1, text: 'First item' },
          { id: 3, text: 'Third' }
        ];
        renderItems();
        
        setTimeout(() => {
          if (checkLayout()) {
            console.log('[TEST_PASS] Step 4 layout correct');
          }
          
          console.log('[TEST_END]');
        }, 200);
      }, 200);
    }, 200);
  }, 200);
}, 100);
