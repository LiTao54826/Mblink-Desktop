/**
 * 交互式富文本编辑器
 */

import { h, render } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';

console.log('=== 交互式富文本编辑器启动 ===');

function RichTextEditor() {
  const [logs, setLogs] = useState([{ time: new Date().toLocaleTimeString(), msg: '日志面板已就绪...', type: 'info' }]);
  const [mutationCount, setMutationCount] = useState(0);
  const [selectionInfo, setSelectionInfo] = useState('选择: 无');
  const [rangeInfo, setRangeInfo] = useState('Range: 无');
  const [statusText, setStatusText] = useState('就绪');
  const editorRef = useRef(null);
  const observerRef = useRef(null);

  const addLog = (msg, type = 'info') => {
    const time = new Date().toLocaleTimeString();
    setLogs(prev => [...prev, { time, msg, type }]);
    console.log(`[${type}] ${msg}`);
  };

  const updateSelectionInfo = () => {
    try {
      const selection = window.getSelection();
      if (selection.rangeCount > 0) {
        const range = selection.getRangeAt(0);
        const text = selection.toString();
        
        if (text.length > 0) {
          setSelectionInfo(`选择: "${text.substring(0, 30)}${text.length > 30 ? '...' : ''}" (${text.length} 字符)`);
          setRangeInfo(`Range: collapsed=${range.collapsed}, start=${range.startOffset}, end=${range.endOffset}`);
        } else {
          setSelectionInfo('选择: 光标位置');
          setRangeInfo(`Range: collapsed=${range.collapsed}, offset=${range.startOffset}`);
        }
      } else {
        setSelectionInfo('选择: 无');
        setRangeInfo('Range: 无');
      }
    } catch (e) {
      console.error('更新选择信息失败:', e);
    }
  };

  useEffect(() => {
    if (!editorRef.current) return;

    addLog('✓ 富文本编辑器初始化完成', 'info');

    // 设置 MutationObserver
    let count = 0;
    const observer = new MutationObserver((mutations) => {
      mutations.forEach((mutation) => {
        count++;
        setMutationCount(count);
        
        let msg = `变化 #${count}: ${mutation.type}`;
        if (mutation.type === 'childList') {
          if (mutation.addedNodes.length > 0) msg += ` (添加 ${mutation.addedNodes.length} 节点)`;
          if (mutation.removedNodes.length > 0) msg += ` (移除 ${mutation.removedNodes.length} 节点)`;
        } else if (mutation.type === 'characterData') {
          msg += ` (文本变化)`;
        } else if (mutation.type === 'attributes') {
          msg += ` (属性: ${mutation.attributeName})`;
        }
        
        addLog(msg, 'mutation');
      });
    });

    observer.observe(editorRef.current, {
      childList: true,
      attributes: true,
      characterData: true,
      subtree: true,
      attributeOldValue: true,
      characterDataOldValue: true
    });

    observerRef.current = observer;
    addLog('✓ MutationObserver 正在监控变化', 'mutation');

    // 事件监听
    const editor = editorRef.current;

    const handleBeforeInput = (e) => {
      addLog(`beforeinput: ${e.inputType} "${e.data || ''}"`, 'event');
    };

    const handleInput = (e) => {
      addLog(`input: ${e.inputType} "${e.data || ''}"`, 'event');
      setStatusText('内容已修改');
    };

    const handleCopy = () => {
      addLog('copy 事件触发', 'event');
      setStatusText('已复制到剪贴板');
    };

    const handleCut = () => {
      addLog('cut 事件触发', 'event');
      setStatusText('已剪切到剪贴板');
    };

    const handlePaste = () => {
      addLog('paste 事件触发', 'event');
      setStatusText('已从剪贴板粘贴');
    };

    const handleFocus = () => {
      addLog('编辑器获得焦点', 'event');
      setStatusText('编辑中...');
    };

    const handleBlur = () => {
      addLog('编辑器失去焦点', 'event');
      setStatusText('就绪');
    };

    editor.addEventListener('beforeinput', handleBeforeInput);
    editor.addEventListener('input', handleInput);
    editor.addEventListener('copy', handleCopy);
    editor.addEventListener('cut', handleCut);
    editor.addEventListener('paste', handlePaste);
    editor.addEventListener('focus', handleFocus);
    editor.addEventListener('blur', handleBlur);

    const handleSelectionChange = () => {
      updateSelectionInfo();
      addLog('选择已变化', 'selection');
    };
    document.addEventListener('selectionchange', handleSelectionChange);

    return () => {
      if (observerRef.current) observerRef.current.disconnect();
      editor.removeEventListener('beforeinput', handleBeforeInput);
      editor.removeEventListener('input', handleInput);
      editor.removeEventListener('copy', handleCopy);
      editor.removeEventListener('cut', handleCut);
      editor.removeEventListener('paste', handlePaste);
      editor.removeEventListener('focus', handleFocus);
      editor.removeEventListener('blur', handleBlur);
      document.removeEventListener('selectionchange', handleSelectionChange);
    };
  }, []);

  const execCmd = (cmd) => {
    try {
      document.execCommand(cmd);
      addLog(`执行命令: ${cmd}`, 'event');
    } catch (e) {
      addLog(`命令失败: ${cmd} - ${e.message}`, 'event');
    }
  };

  const handleClear = () => {
    if (editorRef.current) {
      editorRef.current.innerHTML = '<p><br></p>';
      addLog('内容已清空', 'event');
      setStatusText('内容已清空');
    }
  };

  const handleKeyDown = (e) => {
    const ctrl = e.ctrlKey || e.metaKey;
    if (ctrl && e.key === 'b') {
      e.preventDefault();
      execCmd('bold');
    } else if (ctrl && e.key === 'i') {
      e.preventDefault();
      execCmd('italic');
    } else if (ctrl && e.key === 'u') {
      e.preventDefault();
      execCmd('underline');
    }
  };

  const btnStyle = {
    padding: '8px 12px',
    background: 'white',
    border: '1px solid #bdc3c7',
    borderRadius: '4px',
    cursor: 'pointer',
    fontSize: '14px'
  };

  return h('div', {
    style: {
      fontFamily: 'Arial, sans-serif',
      background: '#f5f5f5',
      padding: '20px',
      minHeight: '100vh',
      margin: 0
    }
  }, [
    h('div', {
      style: {
        maxWidth: '800px',
        margin: '0 auto',
        background: 'white',
        borderRadius: '8px',
        boxShadow: '0 2px 8px rgba(0,0,0,0.1)',
        overflow: 'hidden'
      }
    }, [
      // Header
      h('div', {
        style: {
          background: '#2c3e50',
          color: 'white',
          padding: '20px',
          textAlign: 'center'
        }
      }, [
        h('h1', { style: { fontSize: '24px', margin: '0 0 5px 0' } }, '🎨 富文本编辑器测试'),
        h('p', { style: { fontSize: '14px', opacity: 0.8, margin: 0 } }, '测试 Range、Selection、MutationObserver、ContentEditable 和 execCommand API')
      ]),

      // Toolbar
      h('div', {
        style: {
          background: '#ecf0f1',
          padding: '10px',
          borderBottom: '1px solid #bdc3c7',
          display: 'flex',
          gap: '5px',
          flexWrap: 'wrap'
        }
      }, [
        h('button', { onClick: () => execCmd('bold'), style: btnStyle }, h('strong', {}, 'B')),
        h('button', { onClick: () => execCmd('italic'), style: btnStyle }, h('em', {}, 'I')),
        h('button', { onClick: () => execCmd('underline'), style: btnStyle }, h('u', {}, 'U')),
        h('div', { style: { width: '1px', background: '#bdc3c7' } }),
        h('button', { onClick: () => execCmd('copy'), style: btnStyle }, '📋 复制'),
        h('button', { onClick: () => execCmd('cut'), style: btnStyle }, '✂️ 剪切'),
        h('button', { onClick: () => execCmd('paste'), style: btnStyle }, '📄 粘贴'),
        h('div', { style: { width: '1px', background: '#bdc3c7' } }),
        h('button', { onClick: () => execCmd('selectAll'), style: btnStyle }, '全选'),
        h('button', { onClick: handleClear, style: btnStyle }, '🗑️ 清空')
      ]),

      // Editor
      h('div', {
        ref: editorRef,
        contentEditable: true,
        onKeyDown: handleKeyDown,
        style: {
          padding: '20px',
          minHeight: '300px',
          fontSize: '16px',
          lineHeight: '1.6',
          outline: 'none'
        },
        dangerouslySetInnerHTML: {
          __html: '<p>欢迎使用富文本编辑器！</p><p>你可以在这里输入文本，使用工具栏按钮或键盘快捷键来格式化文本。</p><p>试试选择一些文本，然后点击 <strong>粗体</strong>、<em>斜体</em> 或 <u>下划线</u> 按钮。</p><p>所有的编辑操作都会被 MutationObserver 监控并记录在下方的日志中。</p>'
        }
      }),

      // Status Bar
      h('div', {
        style: {
          background: '#ecf0f1',
          padding: '10px 20px',
          borderTop: '1px solid #bdc3c7',
          fontSize: '12px',
          color: '#7f8c8d'
        }
      }, statusText),

      // Info Panel
      h('div', {
        style: {
          padding: '20px',
          background: '#f8f9fa',
          borderTop: '1px solid #e9ecef'
        }
      }, [
        h('h3', { style: { fontSize: '14px', margin: '0 0 10px 0', color: '#2c3e50' } }, '📊 实时信息'),
        h('div', { style: { fontSize: '12px', padding: '5px 0', color: '#555', fontFamily: 'monospace' } }, selectionInfo),
        h('div', { style: { fontSize: '12px', padding: '5px 0', color: '#555', fontFamily: 'monospace' } }, rangeInfo),
        h('div', { style: { fontSize: '12px', padding: '5px 0', color: '#555', fontFamily: 'monospace' } }, `变化次数: ${mutationCount}`)
      ]),

      // Log Panel
      h('div', {
        style: {
          maxHeight: '200px',
          overflowY: 'auto',
          background: '#2c3e50',
          color: '#ecf0f1',
          padding: '10px',
          fontFamily: 'monospace',
          fontSize: '11px'
        }
      }, logs.map((log, i) => 
        h('div', {
          key: i,
          style: {
            padding: '2px 0',
            borderBottom: '1px solid rgba(255,255,255,0.1)',
            color: log.type === 'event' ? '#3498db' : 
                   log.type === 'mutation' ? '#e74c3c' : 
                   log.type === 'selection' ? '#2ecc71' : '#ecf0f1'
          }
        }, `[${log.time}] ${log.msg}`)
      ))
    ])
  ]);
}

render(h(RichTextEditor), document.body);
console.log('=== 编辑器已就绪 ===');
