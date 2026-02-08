/**
 * 精确复现 Card Grid Layout 问题
 * 来自 05_nested_layouts.js
 */
import { h, render } from 'preact';

function App() {
    // 手动创建6个卡片，避免 Array.from
    var cards = [];
    for (var i = 0; i < 6; i++) {
        cards.push(
            h('div', { 
                key: i,
                style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
            },
                // 卡片头部
                h('div', { 
                    style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                }, String(i + 1)),
                // 卡片内容
                h('div', { style: 'padding: 15px;' },
                    h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, 'Card ' + (i + 1)),
                    h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card with nested flex layout and responsive design.')
                )
            )
        );
    }
    
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Card Grid Layout 测试'),
        
        // 卡片网格布局 - 完全复制原始代码
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'Card Grid Layout'),
            h('div', { 
                id: 'card-grid',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            }, cards)
        ),
        
        // 布局信息
        h('div', { 
            style: 'padding: 15px; background: white; border-radius: 8px;' 
        },
            h('h3', { style: 'margin: 0 0 10px 0;' }, '布局信息'),
            h('pre', { id: 'layout-info', style: 'margin: 0; font-size: 12px; background: #f5f5f5; padding: 10px; border-radius: 4px;' }, '加载中...')
        )
    );
}

render(h(App), document.body);

setTimeout(function() {
    var layoutInfo = document.getElementById('layout-info');
    var container = document.getElementById('card-grid');
    if (!layoutInfo || !container) return;
    
    var rect = container.getBoundingClientRect();
    var children = container.children;
    
    var info = '=== Card Grid Layout ===\n';
    info += '容器宽度: ' + rect.width.toFixed(0) + 'px\n';
    info += '容器高度: ' + rect.height.toFixed(0) + 'px\n\n';
    
    var rows = {};
    for (var i = 0; i < children.length; i++) {
        var childRect = children[i].getBoundingClientRect();
        var relX = childRect.left - rect.left;
        var relY = childRect.top - rect.top;
        var rowKey = Math.round(relY);
        if (!rows[rowKey]) rows[rowKey] = [];
        rows[rowKey].push(i + 1);
        info += 'Card ' + (i+1) + ': x=' + relX.toFixed(0) + ', y=' + relY.toFixed(0) + ', w=' + childRect.width.toFixed(0) + ', h=' + childRect.height.toFixed(0) + '\n';
    }
    
    var rowCount = 0;
    info += '\n行分布:\n';
    for (var key in rows) {
        rowCount++;
        info += '  第' + rowCount + '行 (y=' + key + '): Card ' + rows[key].join(', ') + '\n';
    }
    
    info += '\n总行数: ' + rowCount + '\n';
    info += '预期行数: 2 (6个卡片, 每行3个)\n';
    info += '换行状态: ' + (rowCount >= 2 ? '✓ 正常' : '✗ 未换行 (BUG!)') + '\n';
    
    if (rowCount < 2) {
        info += '\n⚠️ BUG: flex-wrap: wrap 未生效!\n';
        info += '所有卡片都在同一行，应该换行到多行\n';
    }
    
    layoutInfo.textContent = info;
    console.log(info);
}, 500);

