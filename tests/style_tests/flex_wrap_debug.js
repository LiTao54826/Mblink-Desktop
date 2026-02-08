/**
 * flex-wrap 调试测试
 * 用于验证 flex-wrap: wrap 是否正确生效
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        // 测试1: flex-wrap: wrap 基本测试
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'flex-wrap: wrap 测试'),
        
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '容器宽度: 300px, 每个子元素: 80px, 共5个 = 400px (应该换行)'),
        
        h('div', { 
            id: 'flex-wrap-container',
            style: 'display: flex; flex-wrap: wrap; width: 300px; gap: 10px; background: #e0e0e0; padding: 10px; border: 2px solid blue;' 
        },
            h('div', { style: 'width: 80px; height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'width: 80px; height: 50px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'width: 80px; height: 50px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'width: 80px; height: 50px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4'),
            h('div', { style: 'width: 80px; height: 50px; background: #9c27b0; color: white; display: flex; align-items: center; justify-content: center;' }, '5')
        ),
        
        // 测试2: flex-wrap: nowrap 对比
        h('h2', { style: 'margin: 20px 0 10px 0;' }, 'flex-wrap: nowrap 对比'),
        
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '相同设置但 flex-wrap: nowrap (应该溢出)'),
        
        h('div', { 
            id: 'flex-nowrap-container',
            style: 'display: flex; flex-wrap: nowrap; width: 300px; gap: 10px; background: #e0e0e0; padding: 10px; border: 2px solid red; overflow: visible;' 
        },
            h('div', { style: 'width: 80px; height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center; flex-shrink: 0;' }, '1'),
            h('div', { style: 'width: 80px; height: 50px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center; flex-shrink: 0;' }, '2'),
            h('div', { style: 'width: 80px; height: 50px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center; flex-shrink: 0;' }, '3'),
            h('div', { style: 'width: 80px; height: 50px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center; flex-shrink: 0;' }, '4'),
            h('div', { style: 'width: 80px; height: 50px; background: #9c27b0; color: white; display: flex; align-items: center; justify-content: center; flex-shrink: 0;' }, '5')
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
    const wrapContainer = document.getElementById('flex-wrap-container');
    const nowrapContainer = document.getElementById('flex-nowrap-container');
    const layoutInfo = document.getElementById('layout-info');
    
    if (wrapContainer && nowrapContainer && layoutInfo) {
        const wrapRect = wrapContainer.getBoundingClientRect();
        const nowrapRect = nowrapContainer.getBoundingClientRect();
        
        // 获取子元素位置
        const wrapChildren = wrapContainer.children;
        const nowrapChildren = nowrapContainer.children;
        
        let info = `=== flex-wrap: wrap 容器 ===\n`;
        info += `容器尺寸: ${wrapRect.width}x${wrapRect.height}\n`;
        info += `子元素位置:\n`;
        for (let i = 0; i < wrapChildren.length; i++) {
            const rect = wrapChildren[i].getBoundingClientRect();
            info += `  [${i+1}] x=${rect.left - wrapRect.left}, y=${rect.top - wrapRect.top}\n`;
        }
        
        info += `\n=== flex-wrap: nowrap 容器 ===\n`;
        info += `容器尺寸: ${nowrapRect.width}x${nowrapRect.height}\n`;
        info += `子元素位置:\n`;
        for (let i = 0; i < nowrapChildren.length; i++) {
            const rect = nowrapChildren[i].getBoundingClientRect();
            info += `  [${i+1}] x=${rect.left - nowrapRect.left}, y=${rect.top - nowrapRect.top}\n`;
        }
        
        // 判断是否换行
        const wrapFirstY = wrapChildren[0].getBoundingClientRect().top;
        const wrapLastY = wrapChildren[wrapChildren.length - 1].getBoundingClientRect().top;
        const hasWrapped = wrapLastY > wrapFirstY;
        
        info += `\n=== 结论 ===\n`;
        info += `flex-wrap: wrap 是否换行: ${hasWrapped ? '✓ 是' : '✗ 否 (BUG!)'}\n`;
        info += `wrap容器高度: ${wrapRect.height}px\n`;
        info += `nowrap容器高度: ${nowrapRect.height}px\n`;
        
        if (!hasWrapped) {
            info += `\n⚠️ BUG: flex-wrap: wrap 未生效!\n`;
            info += `预期: 子元素应该换行到多行\n`;
            info += `实际: 所有子元素在同一行\n`;
        }
        
        layoutInfo.textContent = info;
    }
}, 500);

