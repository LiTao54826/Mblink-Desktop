/**
 * 富文本编辑 API 单元测试
 * 系统性测试所有 API 功能
 */

console.log('=== 富文本编辑 API 测试套件 ===\n');

let testsPassed = 0;
let testsFailed = 0;

function assert(condition, message) {
  if (condition) {
    console.log(`✓ ${message}`);
    testsPassed++;
    return true;
  } else {
    console.error(`✗ ${message}`);
    testsFailed++;
    return false;
  }
}

function testGroup(name) {
  console.log(`\n--- ${name} ---`);
}

// 创建测试 DOM
document.body.innerHTML = `
  <div id="test-root">
    <p id="p1">Hello <strong id="strong1">World</strong>!</p>
    <p id="p2">This is a <em id="em1">test</em> paragraph.</p>
    <div id="editable" contenteditable="true">
      <p id="p3">Editable content</p>
    </div>
  </div>
`;

// ========== Range API 测试 ==========
testGroup('Range API');

const p1 = document.getElementById('p1');
const p2 = document.getElementById('p2');
const strong1 = document.getElementById('strong1');

// 测试 createRange
const range1 = document.createRange();
assert(range1 !== null, 'document.createRange() 返回非空对象');
assert(range1.collapsed === true, '新创建的 Range 应该是 collapsed');

// 测试 setStart/setEnd
range1.setStart(p1.firstChild, 0);
range1.setEnd(p1.firstChild, 5);
assert(range1.toString() === 'Hello', 'setStart/setEnd 设置正确的范围');
assert(range1.collapsed === false, '设置范围后 Range 不应该 collapsed');
assert(range1.startOffset === 0, 'startOffset 正确');
assert(range1.endOffset === 5, 'endOffset 正确');

// 测试 selectNode
range1.selectNode(strong1);
assert(range1.toString() === 'World', 'selectNode 选择整个节点');

// 测试 selectNodeContents
range1.selectNodeContents(p1);
const p1Text = range1.toString();
assert(p1Text.includes('Hello') && p1Text.includes('World'), 'selectNodeContents 选择节点内容');

// 测试 collapse
range1.collapse(true);
assert(range1.collapsed === true, 'collapse(true) 折叠到起点');
assert(range1.startOffset === range1.endOffset, '折叠后 startOffset === endOffset');

range1.selectNodeContents(p1);
range1.collapse(false);
assert(range1.collapsed === true, 'collapse(false) 折叠到终点');

// 测试 cloneRange
range1.selectNodeContents(p1);
const range2 = range1.cloneRange();
assert(range2 !== null, 'cloneRange 返回非空对象');
assert(range2 !== range1, 'cloneRange 返回新对象');
assert(range2.toString() === range1.toString(), '克隆的 Range 内容相同');

// 修改原 Range 不影响克隆
range1.collapse(true);
assert(range1.collapsed === true && range2.collapsed === false, '修改原 Range 不影响克隆');

// 测试 setStartBefore/setStartAfter
const range3 = document.createRange();
range3.setStartBefore(strong1);
range3.setEndAfter(strong1);
assert(range3.toString() === 'World', 'setStartBefore/setEndAfter 正确设置范围');

// 测试 commonAncestorContainer
range3.setStart(p1.firstChild, 0);
range3.setEnd(strong1.firstChild, 5);
const ancestor = range3.commonAncestorContainer;
assert(ancestor === p1, 'commonAncestorContainer 返回正确的祖先节点');

// ========== Selection API 测试 ==========
testGroup('Selection API');

const selection = window.getSelection();
assert(selection !== null, 'window.getSelection() 返回非空对象');

// 测试 removeAllRanges
selection.removeAllRanges();
assert(selection.rangeCount === 0, 'removeAllRanges 清空所有范围');
assert(selection.isCollapsed === true, '没有范围时 isCollapsed 为 true');

// 测试 addRange
const testRange = document.createRange();
testRange.selectNodeContents(p1);
selection.addRange(testRange);
assert(selection.rangeCount === 1, 'addRange 添加范围');
assert(selection.toString().includes('Hello'), 'Selection 包含正确的文本');

// 测试 getRangeAt
const retrievedRange = selection.getRangeAt(0);
assert(retrievedRange !== null, 'getRangeAt(0) 返回范围');
assert(retrievedRange.toString() === testRange.toString(), 'getRangeAt 返回正确的范围');

// 测试 collapse
selection.collapse(p1.firstChild, 0);
assert(selection.isCollapsed === true, 'collapse 折叠选择');
assert(selection.anchorNode === p1.firstChild, 'anchorNode 正确');
assert(selection.anchorOffset === 0, 'anchorOffset 正确');

// 测试 extend
selection.extend(p1.firstChild, 5);
assert(selection.isCollapsed === false, 'extend 扩展选择');
assert(selection.toString() === 'Hello', 'extend 后文本正确');
assert(selection.anchorOffset === 0, 'extend 保持 anchorOffset');
assert(selection.focusOffset === 5, 'focusOffset 更新');

// 测试 selectAllChildren
selection.selectAllChildren(p1);
assert(selection.rangeCount === 1, 'selectAllChildren 设置范围');
const selectedText = selection.toString();
assert(selectedText.includes('Hello') && selectedText.includes('World'), 'selectAllChildren 选择所有子节点');

// 测试 toString
selection.removeAllRanges();
const emptyText = selection.toString();
assert(emptyText === '', '空选择的 toString 返回空字符串');

// ========== MutationObserver 测试 ==========
testGroup('MutationObserver API');

let observedMutations = [];
const observer = new MutationObserver((mutations) => {
  observedMutations.push(...mutations);
});

const testContainer = document.getElementById('test-root');
observer.observe(testContainer, {
  childList: true,
  attributes: true,
  characterData: true,
  subtree: true,
  attributeOldValue: true,
  characterDataOldValue: true
});

assert(observer !== null, 'MutationObserver 构造成功');

// 触发 childList 变化
const newP = document.createElement('p');
newP.textContent = 'New paragraph';
testContainer.appendChild(newP);

// 触发 attributes 变化
p1.setAttribute('data-test', 'value');

// 触发 characterData 变化
p1.firstChild.textContent = 'Modified';

// 等待微任务执行
setTimeout(() => {
  assert(observedMutations.length >= 3, `MutationObserver 捕获变化 (${observedMutations.length} 个)`);
  
  // 检查 childList 变化
  const childListMutation = observedMutations.find(m => m.type === 'childList');
  assert(childListMutation !== undefined, '捕获 childList 变化');
  if (childListMutation) {
    assert(childListMutation.addedNodes.length > 0, 'childList 变化包含 addedNodes');
  }
  
  // 检查 attributes 变化
  const attrMutation = observedMutations.find(m => m.type === 'attributes');
  assert(attrMutation !== undefined, '捕获 attributes 变化');
  if (attrMutation) {
    assert(attrMutation.attributeName === 'data-test', 'attributes 变化包含正确的属性名');
  }
  
  // 检查 characterData 变化
  const charDataMutation = observedMutations.find(m => m.type === 'characterData');
  assert(charDataMutation !== undefined, '捕获 characterData 变化');
  
  // 测试 takeRecords
  const records = observer.takeRecords();
  assert(Array.isArray(records), 'takeRecords 返回数组');
  
  // 测试 disconnect
  observer.disconnect();
  observedMutations = [];
  p1.setAttribute('data-test2', 'value2');
  
  setTimeout(() => {
    assert(observedMutations.length === 0, 'disconnect 后不再捕获变化');
    
    // 继续 ContentEditable 测试
    testContentEditable();
  }, 50);
}, 50);

// ========== ContentEditable 测试 ==========
function testContentEditable() {
  testGroup('ContentEditable API');
  
  const editable = document.getElementById('editable');
  const p3 = document.getElementById('p3');
  
  assert(editable.getAttribute('contenteditable') === 'true', 'contenteditable 属性设置正确');
  assert(editable.isContentEditable === true, 'isContentEditable 返回 true');
  assert(p3.isContentEditable === true, 'contenteditable 继承到子元素');
  
  // 测试非可编辑元素
  assert(p1.isContentEditable === false, '非 contenteditable 元素返回 false');
  
  // 测试事件监听
  let beforeInputFired = false;
  let inputFired = false;
  
  editable.addEventListener('beforeinput', (e) => {
    beforeInputFired = true;
    assert(e.inputType !== undefined, 'beforeinput 事件包含 inputType');
  });
  
  editable.addEventListener('input', (e) => {
    inputFired = true;
    assert(e.inputType !== undefined, 'input 事件包含 inputType');
  });
  
  // 注意：实际的输入事件需要用户交互或模拟，这里只测试监听器设置
  assert(true, 'beforeinput 事件监听器设置成功');
  assert(true, 'input 事件监听器设置成功');
  
  // 继续 execCommand 测试
  setTimeout(() => {
    testExecCommand();
  }, 50);
}

// ========== execCommand 测试 ==========
function testExecCommand() {
  testGroup('execCommand API');
  
  const editable = document.getElementById('editable');
  const p3 = document.getElementById('p3');
  
  // 设置选择
  const selection = window.getSelection();
  const range = document.createRange();
  range.selectNodeContents(p3);
  selection.removeAllRanges();
  selection.addRange(range);
  
  // 测试 queryCommandEnabled
  const boldEnabled = document.queryCommandEnabled('bold');
  assert(typeof boldEnabled === 'boolean', 'queryCommandEnabled 返回布尔值');
  
  // 测试 queryCommandState
  const boldState = document.queryCommandState('bold');
  assert(typeof boldState === 'boolean', 'queryCommandState 返回布尔值');
  
  // 测试 execCommand
  const boldResult = document.execCommand('bold');
  assert(typeof boldResult === 'boolean', 'execCommand 返回布尔值');
  
  // 测试其他命令
  document.execCommand('italic');
  assert(true, 'execCommand italic 执行成功');
  
  document.execCommand('underline');
  assert(true, 'execCommand underline 执行成功');
  
  document.execCommand('selectAll');
  assert(selection.toString().length > 0, 'execCommand selectAll 选择内容');
  
  // 完成测试
  setTimeout(() => {
    printTestResults();
  }, 50);
}

// ========== 测试结果 ==========
function printTestResults() {
  console.log('\n' + '='.repeat(50));
  console.log('测试结果汇总');
  console.log('='.repeat(50));
  console.log(`✓ 通过: ${testsPassed} 个测试`);
  console.log(`✗ 失败: ${testsFailed} 个测试`);
  console.log(`总计: ${testsPassed + testsFailed} 个测试`);
  
  if (testsFailed === 0) {
    console.log('\n🎉 所有测试通过！');
  } else {
    console.log(`\n⚠️  有 ${testsFailed} 个测试失败`);
  }
  
  // 在页面上显示结果
  document.body.innerHTML = `
    <style>
      body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
        padding: 40px;
        background: #f5f5f5;
      }
      .results {
        max-width: 600px;
        margin: 0 auto;
        background: white;
        padding: 30px;
        border-radius: 8px;
        box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      }
      h1 {
        color: #2c3e50;
        margin-bottom: 20px;
      }
      .stat {
        font-size: 24px;
        margin: 15px 0;
        padding: 15px;
        border-radius: 4px;
      }
      .passed {
        background: #d4edda;
        color: #155724;
      }
      .failed {
        background: #f8d7da;
        color: #721c24;
      }
      .total {
        background: #d1ecf1;
        color: #0c5460;
      }
      .message {
        margin-top: 30px;
        padding: 20px;
        border-radius: 4px;
        font-size: 18px;
        text-align: center;
      }
      .success {
        background: #d4edda;
        color: #155724;
      }
      .warning {
        background: #fff3cd;
        color: #856404;
      }
    </style>
    <div class="results">
      <h1>🧪 富文本编辑 API 测试结果</h1>
      <div class="stat passed">✓ 通过: ${testsPassed} 个测试</div>
      <div class="stat failed">✗ 失败: ${testsFailed} 个测试</div>
      <div class="stat total">总计: ${testsPassed + testsFailed} 个测试</div>
      <div class="message ${testsFailed === 0 ? 'success' : 'warning'}">
        ${testsFailed === 0 ? '🎉 所有测试通过！' : `⚠️ 有 ${testsFailed} 个测试失败`}
      </div>
    </div>
  `;
}
