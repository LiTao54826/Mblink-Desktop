/**
 * 隔离测试: flex-wrap 与 min-width
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 20px 0;' }, 'flex-wrap + min-width 测试'),
        
        // 测试1: 固定宽度 (正常工作)
        h('h3', { style: 'margin: 10px 0;' }, '测试1: width: 200px (应该换行)'),
        h('div', { 
            id: 'test1',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px; width: 700px;' 
        },
            h('div', { style: 'width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 测试2: min-width (可能有问题)
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试2: min-width: 200px (可能不换行)'),
        h('div', { 
            id: 'test2',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px; width: 700px;' 
        },
            h('div', { style: 'min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 测试3: flex-basis + min-width
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试3: flex: 1 1 200px; min-width: 200px'),
        h('div', { 
            id: 'test3',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px; width: 700px;' 
        },
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'flex: 1 1 200px; min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 测试4: 无容器宽度
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试4: 容器无宽度 + min-width: 200px'),
        h('div', { 
            id: 'test4',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px;' 
        },
            h('div', { style: 'min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4')
        ),
        
        // 布局信息
        h('div', { style: 'margin-top: 20px; padding: 15px; background: white; border-radius: 8px;' },
            h('pre', { id: 'layout-info', style: 'margin: 0; font-size: 11px;' }, '加载中...')
        )
    );
}

render(h(App), document.body);

setTimeout(function() {
    var layoutInfo = document.getElementById('layout-info');
    if (!layoutInfo) return;
    
    var info = '';
    var tests = ['test1', 'test2', 'test3', 'test4'];
    
    for (var t = 0; t < tests.length; t++) {
        var testId = tests[t];
        var container = document.getElementById(testId);
        if (!container) continue;
        
        var rect = container.getBoundingClientRect();
        var children = container.children;
        
        info += '=== ' + testId + ' (容器宽:' + rect.width.toFixed(0) + ') ===\n';
        
        var rows = {};
        for (var i = 0; i < children.length; i++) {
            var childRect = children[i].getBoundingClientRect();
            var relY = Math.round(childRect.top - rect.top);
            if (!rows[relY]) rows[relY] = [];
            rows[relY].push(i + 1);
        }
        
        var rowCount = 0;
        for (var key in rows) {
            rowCount++;
            info += '  行' + rowCount + ': Card ' + rows[key].join(',') + '\n';
        }
        info += '  换行: ' + (rowCount > 1 ? '✓' : '✗') + '\n\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
}, 500);

