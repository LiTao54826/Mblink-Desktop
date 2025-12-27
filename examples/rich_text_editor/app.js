/**
 * 富文本编辑器测试应用
 * 测试 Range、Selection、MutationObserver、ContentEditable、Clipboard 和 execCommand API
 */

import { h, render } from 'preact';

console.log('=== 富文本编辑器测试应用启动 ===');

// ========== 1. Range API 测试 ==========
console.log('\n--- 1. Range API 测试 ---');

// 创建测试 DOM 结构
const testContainer = document.createElement('div');
testContainer.id = 'test-container';
testContainer.innerHTML = `
  <p id="p1">Hello <strong>World</strong>!</p>
  <p id="p2">This is a <em>test</em> paragraph.</p>
`;
testContainer.style.display = 'none';
document.body.appendChild(testContainer);

const p1 = document.getElementById('p1');
const p2 = document.getElementById('p2');

// 测试 createRange
const range = document.createRange();
console.log('✓ document.createRange() 创建成功');

// 测试 setStart 和 setEnd
range.setStart(p1.firstChild, 0);
range.setEnd(p1.firstChild, 5);
console.log(`✓ Range 设置: "${range.toString()}" (应该是 "Hello")`);

// 测试 selectNode
range.selectNode(p1);
console.log(`✓ selectNode: "${range.toString()}"`);

// 测试 selectNodeContents
range.selectNodeContents(p1);
console.log(`✓ selectNodeContents: "${range.toString()}"`);

// 测试 collapse
range.collapse(true);
console.log(`✓ collapse: collapsed=${range.collapsed}`);

// 测试 cloneRange
const clonedRange = range.cloneRange();
console.log(`✓ cloneRange: 克隆成功, collapsed=${clonedRange.collapsed}`);

// ========== 2. Selection API 测试 ==========
console.log('\n--- 2. Selection API 测试 ---');

const selection = window.getSelection();
console.log('✓ window.getSelection() 获取成功');

// 测试 removeAllRanges 和 addRange
selection.removeAllRanges();
console.log(`✓ removeAllRanges: rangeCount=${selection.rangeCount}`);

const testRange = document.createRange();
testRange.selectNodeContents(p1);
selection.addRange(testRange);
console.log(`✓ addRange: rangeCount=${selection.rangeCount}, text="${selection.toString()}"`);

// 测试 collapse
selection.collapse(p1.firstChild, 0);
console.log(`✓ collapse: isCollapsed=${selection.isCollapsed}`);

// 测试 extend
selection.extend(p1.firstChild, 5);
console.log(`✓ extend: isCollapsed=${selection.isCollapsed}, text="${selection.toString()}"`);

// 测试 selectAllChildren
selection.selectAllChildren(p1);
console.log(`✓ selectAllChildren: text="${selection.toString()}"`);

// ========== 3. MutationObserver 测试 ==========
console.log('\n--- 3. MutationObserver 测试 ---');

let mutationCount = 0;
const observer = new MutationObserver((mutations) => {
  mutationCount += mutations.length;
  console.log(`✓ MutationObserver 回调触发: ${mutations.length} 个变化`);
  mutations.forEach((mutation, index) => {
    console.log(`  [${index}] type=${mutation.type}, target=${mutation.target.nodeName}`);
    if (mutation.addedNodes.length > 0) {
      console.log(`    添加节点: ${mutation.addedNodes.length} 个`);
    }
    if (mutation.removedNodes.length > 0) {
      console.log(`    移除节点: ${mutation.removedNodes.length} 个`);
    }
    if (mutation.attributeName) {
      console.log(`    属性变化: ${mutation.attributeName}`);
    }
  });
});

const observeTarget = document.getElementById('test-container');
observer.observe(observeTarget, {
  childList: true,
  attributes: true,
  characterData: true,
  subtree: true,
  attributeOldValue: true,
  characterDataOldValue: true
});
console.log('✓ MutationObserver.observe() 开始观察');

// 触发一些变化
const newP = document.createElement('p');
newP.textContent = 'New paragraph';
observeTarget.appendChild(newP);
console.log('✓ 添加新段落');

p1.setAttribute('class', 'test-class');
console.log('✓ 修改属性');

// 等待微任务执行
setTimeout(() => {
  console.log(`✓ MutationObserver 总共捕获 ${mutationCount} 个变化`);
  
  // 测试 disconnect
  observer.disconnect();
  console.log('✓ MutationObserver.disconnect() 停止观察');
  
  // 继续测试 ContentEditable
  testContentEditable();
}, 100);

// ========== 4. ContentEditable 测试 ==========
function testContentEditable() {
  console.log('\n--- 4. ContentEditable 测试 ---');
  
  // 创建可编辑区域
  const editorContainer = document.createElement('div');
  editorContainer.innerHTML = `
    <div id="editor" contenteditable="true" style="border: 1px solid #ccc; padding: 10px; min-height: 100px;">
      <p>这是一个可编辑的段落。</p>
      <p>你可以在这里输入文本。</p>
    </div>
    <div id="output" style="margin-top: 10px; padding: 10px; background: #f0f0f0;">
      <strong>事件日志：</strong>
      <div id="log"></div>
    </div>
  `;
  
  // 清空并添加新内容
  document.body.innerHTML = '';
  document.body.appendChild(editorContainer);
  
  const editor = document.getElementById('editor');
  const log = document.getElementById('log');
  
  function addLog(message) {
    const entry = document.createElement('div');
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    log.appendChild(entry);
    console.log(message);
  }
  
  console.log(`✓ contenteditable 属性: ${editor.getAttribute('contenteditable')}`);
  console.log(`✓ isContentEditable: ${editor.isContentEditable}`);
  
  // 监听 beforeinput 事件
  editor.addEventListener('beforeinput', (e) => {
    addLog(`beforeinput: inputType=${e.inputType}, data="${e.data || ''}"`);
  });
  
  // 监听 input 事件
  editor.addEventListener('input', (e) => {
    addLog(`input: inputType=${e.inputType}, data="${e.data || ''}"`);
  });
  
  console.log('✓ 已添加 beforeinput 和 input 事件监听器');
  
  // 继续测试 Clipboard
  setTimeout(() => {
    testClipboard();
  }, 100);
}

// ========== 5. Clipboard 测试 ==========
function testClipboard() {
  console.log('\n--- 5. Clipboard 测试 ---');
  
  const editor = document.getElementById('editor');
  const log = document.getElementById('log');
  
  function addLog(message) {
    const entry = document.createElement('div');
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    log.appendChild(entry);
    console.log(message);
  }
  
  // 监听剪贴板事件
  editor.addEventListener('copy', (e) => {
    addLog('copy 事件触发');
  });
  
  editor.addEventListener('cut', (e) => {
    addLog('cut 事件触发');
  });
  
  editor.addEventListener('paste', (e) => {
    addLog('paste 事件触发');
  });
  
  console.log('✓ 已添加 copy、cut、paste 事件监听器');
  
  // 选择一些文本
  const selection = window.getSelection();
  const range = document.createRange();
  const firstP = editor.querySelector('p');
  range.selectNodeContents(firstP);
  selection.removeAllRanges();
  selection.addRange(range);
  
  addLog(`已选择文本: "${selection.toString()}"`);
  
  // 继续测试 execCommand
  setTimeout(() => {
    testExecCommand();
  }, 100);
}

// ========== 6. execCommand 测试 ==========
function testExecCommand() {
  console.log('\n--- 6. execCommand 测试 ---');
  
  const editor = document.getElementById('editor');
  const log = document.getElementById('log');
  
  function addLog(message) {
    const entry = document.createElement('div');
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    log.appendChild(entry);
    console.log(message);
  }
  
  // 测试 bold 命令
  const selection = window.getSelection();
  const range = document.createRange();
  const firstP = editor.querySelector('p');
  range.setStart(firstP.firstChild, 0);
  range.setEnd(firstP.firstChild, 4);
  selection.removeAllRanges();
  selection.addRange(range);
  
  addLog(`选择文本: "${selection.toString()}"`);
  
  // 测试 queryCommandState
  const isBold = document.queryCommandState('bold');
  addLog(`queryCommandState('bold'): ${isBold}`);
  
  // 测试 queryCommandEnabled
  const canBold = document.queryCommandEnabled('bold');
  addLog(`queryCommandEnabled('bold'): ${canBold}`);
  
  // 执行 bold 命令
  const boldResult = document.execCommand('bold');
  addLog(`execCommand('bold'): ${boldResult}`);
  
  // 再次检查状态
  const isBoldAfter = document.queryCommandState('bold');
  addLog(`queryCommandState('bold') after: ${isBoldAfter}`);
  
  // 测试其他命令
  setTimeout(() => {
    // 测试 italic
    const italicResult = document.execCommand('italic');
    addLog(`execCommand('italic'): ${italicResult}`);
    
    // 测试 underline
    const underlineResult = document.execCommand('underline');
    addLog(`execCommand('underline'): ${underlineResult}`);
    
    // 测试 selectAll
    const selectAllResult = document.execCommand('selectAll');
    addLog(`execCommand('selectAll'): ${selectAllResult}`);
    addLog(`选择的文本长度: ${selection.toString().length}`);
    
    // 完成测试
    setTimeout(() => {
      console.log('\n=== 所有测试完成 ===');
      addLog('✓ 所有 API 测试完成！');
    }, 100);
  }, 100);
}
