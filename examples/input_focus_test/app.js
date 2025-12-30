/**
 * Input Focus 测试
 * 测试点击外层 div 后调用 input.focus() 是否能正确聚焦
 */

import { h, render } from 'preact';
import { useState, useRef } from 'preact/hooks';

function App() {
    const [log, setLog] = useState([]);
    const inputRef = useRef(null);
    
    const addLog = (msg) => {
        console.log(msg);
        setLog(prev => [...prev.slice(-10), msg]);
    };
    
    // 测试1：直接点击 input
    const handleInputFocus = () => {
        addLog('Input onFocus triggered');
    };
    
    const handleInputBlur = () => {
        addLog('Input onBlur triggered');
    };
    
    // 测试2：点击外层 div，然后调用 input.focus()
    const handleContainerClick = () => {
        addLog('Container clicked, calling inputRef.current.focus()');
        if (inputRef.current) {
            addLog('inputRef.current exists: ' + inputRef.current.tagName);
            inputRef.current.focus();
            addLog('focus() called');
        } else {
            addLog('ERROR: inputRef.current is null!');
        }
    };
    
    // 测试3：按钮触发 focus
    const handleButtonClick = (e) => {
        e.preventDefault();
        e.stopPropagation();
        addLog('Button clicked, calling focus()');
        if (inputRef.current) {
            // 使用 setTimeout 确保在按钮的 mousedown 处理完成后再聚焦
            setTimeout(() => {
                inputRef.current.focus();
                addLog('focus() called via button (after timeout)');
            }, 0);
        }
    };

    return h('div', { 
        style: { 
            padding: '20px',
            fontFamily: 'sans-serif'
        } 
    }, [
        h('h3', { key: 'title' }, 'Input Focus 测试'),
        
        // 测试1：直接的 input
        h('div', { key: 'test1', style: { marginBottom: '20px' } }, [
            h('p', {}, '测试1：直接点击 input'),
            h('input', {
                type: 'text',
                placeholder: '直接点击我',
                style: {
                    padding: '8px',
                    border: '1px solid #ccc',
                    borderRadius: '4px',
                    width: '200px'
                },
                onFocus: () => addLog('Test1: Direct input focused'),
                onBlur: () => addLog('Test1: Direct input blurred')
            })
        ]),
        
        // 测试2：包装在 div 中的 input（模拟 Fluent Input）
        h('div', { key: 'test2', style: { marginBottom: '20px' } }, [
            h('p', {}, '测试2：点击外层 div（模拟 Fluent Input）'),
            h('div', {
                onClick: handleContainerClick,
                style: {
                    display: 'inline-flex',
                    alignItems: 'center',
                    padding: '8px',
                    border: '2px solid blue',
                    borderRadius: '4px',
                    cursor: 'text',
                    backgroundColor: '#f0f0f0'
                }
            }, [
                h('span', { style: { marginRight: '8px' } }, '🔍'),
                h('input', {
                    ref: inputRef,
                    type: 'text',
                    placeholder: '点击蓝色边框区域',
                    style: {
                        border: 'none',
                        outline: 'none',
                        backgroundColor: 'transparent',
                        width: '200px'
                    },
                    onFocus: handleInputFocus,
                    onBlur: handleInputBlur
                }),
                h('span', { style: { marginLeft: '8px' } }, '✕')
            ])
        ]),
        
        // 测试3：按钮触发 focus
        h('div', { key: 'test3', style: { marginBottom: '20px' } }, [
            h('p', {}, '测试3：按钮触发 focus'),
            h('button', {
                onClick: handleButtonClick,
                style: {
                    padding: '8px 16px',
                    marginRight: '10px',
                    cursor: 'pointer'
                }
            }, '点击聚焦上面的 input')
        ]),
        
        // 日志输出
        h('div', { key: 'log', style: { marginTop: '20px' } }, [
            h('h4', {}, '日志:'),
            h('div', {
                style: {
                    backgroundColor: '#1e1e1e',
                    color: '#00ff00',
                    padding: '10px',
                    borderRadius: '4px',
                    fontFamily: 'monospace',
                    fontSize: '12px',
                    minHeight: '150px',
                    whiteSpace: 'pre-wrap'
                }
            }, log.join('\n') || '(等待操作...)')
        ])
    ]);
}

// 渲染应用
render(h(App), document.body);
