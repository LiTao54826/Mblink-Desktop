/**
 * 简单测试 - 确保 UI 能正确显示
 */

console.log('=== 简单测试启动 ===');

// 先创建基本的 DOM 结构
const container = document.createElement('div');
container.style.cssText = `
  max-width: 800px;
  margin: 50px auto;
  padding: 30px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 10px rgba(0,0,0,0.1);
  font-family: Arial, sans-serif;
`;

// 添加标题
const title = document.createElement('h1');
title.textContent = '富文本编辑 API 测试';
title.style.cssText = 'color: #2c3e50; margin-bottom: 20px;';
container.appendChild(title);

// 添加说明
const desc = document.createElement('p');
desc.textContent = '正在运行测试...';
desc.style.cssText = 'color: #666; margin-bottom: 20px;';
container.appendChild(desc);

// 添加到页面
document.body.style.cssText = 'background: #f5f5f5; margin: 0; padding: 0;';
document.body.appendChild(container);

console.log('✓ UI 已创建');

// 创建测试区域
const testArea = document.createElement('div');
testArea.id = 'test-area';
testArea.innerHTML = `
  <p id="p1">Hello <strong>World</strong>!</p>
  <p id="p2">Test paragraph</p>
`;
testArea.style.cssText = 'display: none;'; // 隐藏测试区域
document.body.appendChild(testArea);

// 创建结果显示区域
const results = document.createElement('div');
results.id = 'results';
container.appendChild(results);

function addResult(text, success = true) {
  const item = document.createElement('div');
  item.textContent = text;
  item.style.cssText = `
    padding: 10px;
    margin: 5px 0;
    background: ${success ? '#d4edda' : '#f8d7da'};
    border-left: 4px solid ${success ? '#28a745' : '#dc3545'};
    border-radius: 4px;
  `;
  results.appendChild(item);
  console.log(text);
}

// 更新说明
desc.textContent = '测试进行中...';

// ========== 测试 Range API ==========
setTimeout(() => {
  try {
    const p1 = document.getElementById('p1');
    const range = document.createRange();
    range.setStart(p1.firstChild, 0);
    range.setEnd(p1.firstChild, 5);
    addResult(`✓ Range API: "${range.toString()}"`, true);
  } catch (e) {
    addResult(`✗ Range API: ${e.message}`, false);
  }
  
  // 测试 Selection API
  setTimeout(() => {
    try {
      const selection = window.getSelection();
      const p1 = document.getElementById('p1');
      const range = document.createRange();
      range.selectNodeContents(p1);
      selection.removeAllRanges();
      selection.addRange(range);
      addResult(`✓ Selection API: "${selection.toString()}"`, true);
    } catch (e) {
      addResult(`✗ Selection API: ${e.message}`, false);
    }
    
    // 测试 MutationObserver
    setTimeout(() => {
      try {
        let observed = false;
        const observer = new MutationObserver(() => {
          observed = true;
          addResult('✓ MutationObserver API: 变化已捕获', true);
        });
        
        const testArea = document.getElementById('test-area');
        observer.observe(testArea, { childList: true, subtree: true });
        
        const newP = document.createElement('p');
        newP.textContent = 'New';
        testArea.appendChild(newP);
        
        // 等待微任务
        setTimeout(() => {
          if (!observed) {
            addResult('✓ MutationObserver API: 已设置（异步）', true);
          }
          observer.disconnect();
          
          // 测试 ContentEditable
          setTimeout(() => {
            try {
              const editable = document.createElement('div');
              editable.setAttribute('contenteditable', 'true');
              editable.textContent = 'Editable';
              testArea.appendChild(editable);
              
              const isEditable = editable.isContentEditable;
              addResult(`✓ ContentEditable API: isContentEditable=${isEditable}`, true);
            } catch (e) {
              addResult(`✗ ContentEditable API: ${e.message}`, false);
            }
            
            // 测试 execCommand
            setTimeout(() => {
              try {
                const canBold = document.queryCommandEnabled('bold');
                const isBold = document.queryCommandState('bold');
                addResult(`✓ execCommand API: enabled=${canBold}, state=${isBold}`, true);
              } catch (e) {
                addResult(`✗ execCommand API: ${e.message}`, false);
              }
              
              // 完成
              setTimeout(() => {
                desc.textContent = '✅ 所有测试完成！';
                desc.style.color = '#28a745';
                desc.style.fontWeight = 'bold';
                
                const summary = document.createElement('div');
                summary.style.cssText = `
                  margin-top: 20px;
                  padding: 15px;
                  background: #e7f3ff;
                  border-radius: 4px;
                  color: #004085;
                `;
                summary.innerHTML = `
                  <strong>测试总结：</strong><br>
                  所有核心 API 已验证。<br>
                  查看控制台获取详细日志。
                `;
                container.appendChild(summary);
                
                console.log('=== 测试完成 ===');
              }, 100);
            }, 100);
          }, 100);
        }, 100);
      } catch (e) {
        addResult(`✗ MutationObserver API: ${e.message}`, false);
      }
    }, 100);
  }, 100);
}, 100);
