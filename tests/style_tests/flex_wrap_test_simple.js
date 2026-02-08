/**
 * 简单的 flex-wrap 测试
 * 用于验证 flex-wrap: wrap 是否正确生效
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'flex-wrap: wrap 测试'),
        
        // 测试1: 有明确宽度的容器
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试1: 容器有明确宽度 (width: 300px)'),
        h('div', { 
            id: 'test1',
            style: 'display: flex; flex-wrap: wrap; width: 300px; gap: 10px; background: #e0e0e0; padding: 10px; border: 2px solid blue;' 
        },
            h('div', { style: 'width: 100px; height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'width: 100px; height: 50px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'width: 100px; height: 50px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'width: 100px; height: 50px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 测试2: 嵌套在有宽度的容器中
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试2: 嵌套在有宽度的容器中'),
        h('div', { 
            style: 'width: 300px; background: #fff; padding: 10px; border: 2px solid green;' 
        },
            h('div', { 
                id: 'test2',
                style: 'display: flex; flex-wrap: wrap; gap: 10px; background: #e0e0e0; padding: 10px;' 
            },
                h('div', { style: 'width: 100px; height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
                h('div', { style: 'width: 100px; height: 50px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
                h('div', { style: 'width: 100px; height: 50px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
                h('div', { style: 'width: 100px; height: 50px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
            )
        ),
        
        // 测试3: Card Grid Layout (来自 05_nested_layouts.js)
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试3: Card Grid Layout (min-width)'),
        h('div', {
            style: 'width: 600px; background: white; padding: 20px; border: 2px solid purple;'
        },
            h('div', {
                id: 'test3',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;'
            },
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 150px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '1'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 150px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '2'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 150px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '3'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 150px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '4')
            )
        ),
        
        // 状态显示
        h('div', { 
            id: 'status',
            style: 'margin-top: 20px; padding: 15px; background: white; border-radius: 8px;' 
        },
            h('h3', { style: 'margin: 0 0 10px 0;' }, '布局信息'),
            h('pre', { id: 'layout-info', style: 'margin: 0; font-size: 12px; white-space: pre-wrap;' }, '加载中...')
        )
    );
}

render(h(App), document.body);

// 延迟获取布局信息
setTimeout(() => {
    const layoutInfo = document.getElementById('layout-info');
    if (!layoutInfo) return;
    
    let info = '';
    
    ['test1', 'test2', 'test3'].forEach((testId, idx) => {
        const container = document.getElementById(testId);
        if (!container) return;
        
        const rect = container.getBoundingClientRect();
        const children = container.children;
        
        info += `=== 测试${idx + 1} ===\n`;
        info += `容器尺寸: ${rect.width.toFixed(0)}x${rect.height.toFixed(0)}\n`;
        
        let firstY = null;
        let hasWrapped = false;
        
        for (let i = 0; i < children.length; i++) {
            const childRect = children[i].getBoundingClientRect();
            const relX = childRect.left - rect.left;
            const relY = childRect.top - rect.top;
            info += `  [${i+1}] x=${relX.toFixed(0)}, y=${relY.toFixed(0)}\n`;
            
            if (firstY === null) {
                firstY = relY;
            } else if (relY > firstY + 5) {
                hasWrapped = true;
            }
        }
        
        info += `换行状态: ${hasWrapped ? '✓ 已换行' : '✗ 未换行'}\n\n`;
    });
    
    layoutInfo.textContent = info;
    console.log(info);
}, 500);

