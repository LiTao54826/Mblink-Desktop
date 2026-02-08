/**
 * 测试: 嵌套容器中的 flex-wrap
 * 复现 Card Grid Layout 的结构
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h2', { style: 'margin: 0 0 20px 0;' }, '嵌套容器 flex-wrap 测试'),
        
        // 测试1: 直接在 body 下的 flex 容器 (无嵌套)
        h('h3', { style: 'margin: 10px 0;' }, '测试1: 直接 flex 容器 (无嵌套)'),
        h('div', { 
            id: 'test1',
            style: 'display: flex; flex-wrap: wrap; gap: 15px; background: white; padding: 20px;' 
        },
            h('div', { style: 'min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #9c27b0; color: white; display: flex; align-items: center; justify-content: center;' }, '5'),
            h('div', { style: 'min-width: 200px; height: 60px; background: #00bcd4; color: white; display: flex; align-items: center; justify-content: center;' }, '6')
        ),
        
        // 测试2: 嵌套在 div 中 (模拟 Card Grid 结构)
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试2: 嵌套在 div 中 (Card Grid 结构)'),
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('div', { 
                id: 'test2',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            },
                h('div', { style: 'min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
                h('div', { style: 'min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
                h('div', { style: 'min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
                h('div', { style: 'min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4'),
                h('div', { style: 'min-width: 200px; height: 60px; background: #9c27b0; color: white; display: flex; align-items: center; justify-content: center;' }, '5'),
                h('div', { style: 'min-width: 200px; height: 60px; background: #00bcd4; color: white; display: flex; align-items: center; justify-content: center;' }, '6')
            )
        ),
        
        // 测试3: 嵌套 + flex: 1 1 calc(...)
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试3: 嵌套 + flex: 1 1 calc(33.333% - 10px)'),
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('div', { 
                id: 'test3',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            },
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center;' }, '1'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center;' }, '2'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center;' }, '3'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #e91e63; color: white; display: flex; align-items: center; justify-content: center;' }, '4'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #9c27b0; color: white; display: flex; align-items: center; justify-content: center;' }, '5'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 60px; background: #00bcd4; color: white; display: flex; align-items: center; justify-content: center;' }, '6')
            )
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
    var tests = ['test1', 'test2', 'test3'];
    
    for (var t = 0; t < tests.length; t++) {
        var testId = tests[t];
        var container = document.getElementById(testId);
        if (!container) continue;
        
        var rect = container.getBoundingClientRect();
        var children = container.children;
        
        info += '=== ' + testId + ' ===\n';
        info += '容器: ' + rect.width.toFixed(0) + 'x' + rect.height.toFixed(0) + '\n';
        
        var rows = {};
        for (var i = 0; i < children.length; i++) {
            var childRect = children[i].getBoundingClientRect();
            var relX = childRect.left - rect.left;
            var relY = Math.round(childRect.top - rect.top);
            if (!rows[relY]) rows[relY] = [];
            rows[relY].push((i + 1) + '(x:' + relX.toFixed(0) + ')');
        }
        
        var rowCount = 0;
        for (var key in rows) {
            rowCount++;
            info += '  行' + rowCount + ' (y=' + key + '): ' + rows[key].join(', ') + '\n';
        }
        info += '  换行: ' + (rowCount > 1 ? '✓ 是 (' + rowCount + '行)' : '✗ 否 (BUG!)') + '\n\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
}, 500);

