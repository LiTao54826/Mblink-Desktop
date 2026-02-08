/**
 * 简化的 Card Grid Layout 测试
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'Card Grid 简化测试'),
        
        // 测试1: 使用 min-width 和 flex-basis
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试1: min-width + flex-basis'),
        h('div', { 
            id: 'test1',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px;' 
        },
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 80px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 80px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 80px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 80px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 测试2: 只用固定宽度
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试2: 固定宽度 200px'),
        h('div', { 
            id: 'test2',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px;' 
        },
            h('div', { style: 'width: 200px; height: 80px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'width: 200px; height: 80px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'width: 200px; height: 80px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'width: 200px; height: 80px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 状态显示
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
    if (!layoutInfo) {
        window.close();
        return;
    }
    
    var info = '';
    var tests = ['test1', 'test2'];
    
    for (var t = 0; t < tests.length; t++) {
        var testId = tests[t];
        var container = document.getElementById(testId);
        if (!container) continue;
        
        var rect = container.getBoundingClientRect();
        var children = container.children;
        
        info += '=== ' + testId + ' ===\n';
        info += '容器: ' + rect.width.toFixed(0) + 'x' + rect.height.toFixed(0) + '\n';
        
        var firstY = null;
        var hasWrapped = false;
        
        for (var i = 0; i < children.length; i++) {
            var childRect = children[i].getBoundingClientRect();
            var relX = childRect.left - rect.left;
            var relY = childRect.top - rect.top;
            info += '  [' + (i+1) + '] x=' + relX.toFixed(0) + ', y=' + relY.toFixed(0) + ', w=' + childRect.width.toFixed(0) + '\n';
            
            if (firstY === null) {
                firstY = relY;
            } else if (relY > firstY + 5) {
                hasWrapped = true;
            }
        }
        
        info += '换行: ' + (hasWrapped ? '✓ 是' : '✗ 否') + '\n\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
    
    setTimeout(function() {
        window.close();
    }, 500);
}, 500);

