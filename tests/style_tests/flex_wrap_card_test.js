/**
 * flex-wrap 测试 - 不使用 Array.from
 */
import { h, render } from 'preact';

function App() {
    var cards = [];
    for (var i = 0; i < 6; i++) {
        cards.push(
            h('div', {
                key: i,
                style: 'flex: 1 1 200px; min-width: 200px; height: 100px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center; font-size: 24px; border-radius: 8px;'
            }, String(i + 1))
        );
    }
    
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'Card Grid Layout 测试'),
        
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h3', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, '6个卡片 (min-width: 200px)'),
            h('div', { 
                id: 'test1',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            }, cards)
        ),
        
        h('div', { 
            id: 'status',
            style: 'margin-top: 20px; padding: 15px; background: white; border-radius: 8px;' 
        },
            h('pre', { id: 'layout-info', style: 'margin: 0; font-size: 12px;' }, '加载中...')
        )
    );
}

render(h(App), document.body);

setTimeout(function() {
    var layoutInfo = document.getElementById('layout-info');
    if (!layoutInfo) return;
    
    var container = document.getElementById('test1');
    if (!container) return;
    
    var rect = container.getBoundingClientRect();
    var children = container.children;
    
    var info = '=== Card Grid Layout ===\n';
    info += '容器: ' + rect.width.toFixed(0) + 'x' + rect.height.toFixed(0) + '\n';
    
    var yPositions = {};
    for (var i = 0; i < children.length; i++) {
        var childRect = children[i].getBoundingClientRect();
        var relX = childRect.left - rect.left;
        var relY = childRect.top - rect.top;
        var yKey = Math.round(relY);
        if (!yPositions[yKey]) yPositions[yKey] = [];
        yPositions[yKey].push(i + 1);
        info += '  [' + (i+1) + '] x=' + relX.toFixed(0) + ', y=' + relY.toFixed(0) + ', w=' + childRect.width.toFixed(0) + '\n';
    }
    
    var lineCount = 0;
    for (var key in yPositions) {
        lineCount++;
    }
    
    info += '\n行数: ' + lineCount + '\n';
    info += '换行状态: ' + (lineCount > 1 ? '✓ 已换行' : '✗ 未换行 (BUG!)') + '\n';
    
    if (lineCount <= 1) {
        info += '\n⚠️ BUG: flex-wrap: wrap 未生效!\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
    
    setTimeout(function() {
        window.close();
    }, 500);
}, 500);

