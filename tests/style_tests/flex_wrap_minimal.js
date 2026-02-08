/**
 * 最简单的 flex-wrap 测试
 * 用于验证 flex-wrap: wrap 是否正确生效
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'flex-wrap: wrap 最简测试'),
        
        // 测试: 有明确宽度的容器
        h('p', { style: 'margin: 5px 0; color: #666;' }, 
            '容器宽度: 300px, 每个子元素: 100px, 共4个 = 400px (应该换行)'),
        
        h('div', { 
            id: 'test1',
            style: 'display: flex; flex-wrap: wrap; width: 300px; gap: 10px; background: #e0e0e0; padding: 10px; border: 2px solid blue;' 
        },
            h('div', { style: 'width: 100px; height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'width: 100px; height: 50px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'width: 100px; height: 50px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'width: 100px; height: 50px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
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
setTimeout(function() {
    var layoutInfo = document.getElementById('layout-info');
    if (!layoutInfo) return;
    
    var container = document.getElementById('test1');
    if (!container) return;
    
    var rect = container.getBoundingClientRect();
    var children = container.children;
    
    var info = '=== 测试结果 ===\n';
    info += '容器尺寸: ' + rect.width.toFixed(0) + 'x' + rect.height.toFixed(0) + '\n';
    
    var firstY = null;
    var hasWrapped = false;
    
    for (var i = 0; i < children.length; i++) {
        var childRect = children[i].getBoundingClientRect();
        var relX = childRect.left - rect.left;
        var relY = childRect.top - rect.top;
        info += '  [' + (i+1) + '] x=' + relX.toFixed(0) + ', y=' + relY.toFixed(0) + '\n';
        
        if (firstY === null) {
            firstY = relY;
        } else if (relY > firstY + 5) {
            hasWrapped = true;
        }
    }
    
    info += '\n换行状态: ' + (hasWrapped ? '✓ 已换行' : '✗ 未换行 (BUG!)') + '\n';
    
    if (!hasWrapped) {
        info += '\n⚠️ BUG: flex-wrap: wrap 未生效!\n';
        info += '预期: 子元素应该换行到多行\n';
        info += '实际: 所有子元素在同一行\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
}, 500);

