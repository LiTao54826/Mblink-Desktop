/**
 * 测试 flex-wrap 在 overflow: visible 容器中的行为
 * 用于调试 flex-wrap 换行 bug
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'flex-wrap + overflow: visible 测试'),
        
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '外层容器: width: 150px, overflow: visible'),
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '内层 flex 容器: display: flex, flex-wrap: wrap, gap: 10px'),
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '子元素: 每个 width: 200px (超过容器宽度，应该换行)'),
        
        // 外层容器 - overflow: visible
        h('div', { 
            id: 'outer-container',
            style: 'width: 150px; height: 100px; background: #e8f5e9; border: 2px solid #4caf50; overflow: visible; position: relative;' 
        },
            // 内层 flex 容器 - flex-wrap: wrap
            h('div', { 
                id: 'flex-container',
                style: 'display: flex; flex-wrap: wrap; gap: 10px; background: rgba(255,0,0,0.1);' 
            },
                h('div', { 
                    style: 'width: 200px; height: 40px; background: #4caf50; color: white; padding: 5px; display: flex; align-items: center; justify-content: center;' 
                }, 'Item 1 (200px)'),
                h('div', { 
                    style: 'width: 200px; height: 40px; background: #2196f3; color: white; padding: 5px; display: flex; align-items: center; justify-content: center;' 
                }, 'Item 2 (200px)'),
                h('div', { 
                    style: 'width: 200px; height: 40px; background: #ff9800; color: white; padding: 5px; display: flex; align-items: center; justify-content: center;' 
                }, 'Item 3 (200px)')
            )
        ),
        
        // 状态显示
        h('div', { 
            id: 'status',
            style: 'margin-top: 150px; padding: 15px; background: white; border-radius: 8px;' 
        },
            h('h3', { style: 'margin: 0 0 10px 0;' }, '布局信息'),
            h('pre', { id: 'layout-info', style: 'margin: 0; font-size: 12px; white-space: pre-wrap;' }, '加载中...')
        )
    );
}

render(h(App), document.body);

// 延迟获取布局信息
setTimeout(() => {
    const outerContainer = document.getElementById('outer-container');
    const flexContainer = document.getElementById('flex-container');
    const layoutInfo = document.getElementById('layout-info');
    
    if (outerContainer && flexContainer && layoutInfo) {
        const outerRect = outerContainer.getBoundingClientRect();
        const flexRect = flexContainer.getBoundingClientRect();
        
        // 获取子元素位置
        const children = flexContainer.children;
        
        let info = `=== 外层容器 (overflow: visible) ===\n`;
        info += `尺寸: ${outerRect.width}x${outerRect.height}\n`;
        
        info += `\n=== Flex 容器 ===\n`;
        info += `尺寸: ${flexRect.width}x${flexRect.height}\n`;
        
        info += `\n=== 子元素位置 ===\n`;
        let firstY = null;
        let hasWrapped = false;
        for (let i = 0; i < children.length; i++) {
            const rect = children[i].getBoundingClientRect();
            const relX = rect.left - flexRect.left;
            const relY = rect.top - flexRect.top;
            info += `[${i+1}] x=${relX.toFixed(0)}, y=${relY.toFixed(0)}, size=${rect.width.toFixed(0)}x${rect.height.toFixed(0)}\n`;
            
            if (firstY === null) {
                firstY = relY;
            } else if (relY > firstY + 5) {
                hasWrapped = true;
            }
        }
        
        info += `\n=== 结论 ===\n`;
        info += `flex-wrap 是否生效: ${hasWrapped ? '✓ 是 (已换行)' : '✗ 否 (BUG! 未换行)'}\n`;
        
        if (!hasWrapped) {
            info += `\n⚠️ BUG: flex-wrap: wrap 未生效!\n`;
            info += `预期: 子元素应该换行到多行\n`;
            info += `实际: 所有子元素在同一行\n`;
            info += `\n可能原因: 当外层容器 overflow: visible 时,\n`;
            info += `available_main 可能为 INFINITY, 导致换行条件永远不满足\n`;
        }
        
        layoutInfo.textContent = info;
    }
}, 500);

