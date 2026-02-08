/**
 * Card Grid Layout 测试 - 复现 05_nested_layouts.js 中的问题
 */
import { h, render } from 'preact';

function App() {
    return h('div', { 
        style: 'padding: 20px; background: #f0f0f0;' 
    },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'Card Grid Layout 测试'),
        
        // 测试1: 原始 Card Grid Layout (来自 05_nested_layouts.js)
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试1: 原始 Card Grid Layout'),
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('div', { 
                id: 'test1',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            },
                h('div', { 
                    style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
                },
                    h('div', { 
                        style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                    }, '1'),
                    h('div', { style: 'padding: 15px;' },
                        h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, 'Card 1'),
                        h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card.')
                    )
                ),
                h('div', { 
                    style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
                },
                    h('div', { 
                        style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                    }, '2'),
                    h('div', { style: 'padding: 15px;' },
                        h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, 'Card 2'),
                        h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card.')
                    )
                ),
                h('div', { 
                    style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
                },
                    h('div', { 
                        style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                    }, '3'),
                    h('div', { style: 'padding: 15px;' },
                        h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, 'Card 3'),
                        h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card.')
                    )
                ),
                h('div', { 
                    style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
                },
                    h('div', { 
                        style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                    }, '4'),
                    h('div', { style: 'padding: 15px;' },
                        h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, 'Card 4'),
                        h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card.')
                    )
                )
            )
        ),
        
        // 测试2: 简化版 - 去掉 overflow: hidden
        h('h3', { style: 'margin: 20px 0 10px 0;' }, '测试2: 去掉 overflow: hidden'),
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('div', { 
                id: 'test2',
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            },
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '1'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '2'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '3'),
                h('div', { style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; height: 80px; background: #e8f5e9; border-radius: 8px; display: flex; align-items: center; justify-content: center; font-size: 24px; font-weight: bold; color: #4caf50;' }, '4')
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
setTimeout(function() {
    var layoutInfo = document.getElementById('layout-info');
    if (!layoutInfo) return;

    var info = '';

    var tests = ['test1', 'test2'];
    for (var t = 0; t < tests.length; t++) {
        var testId = tests[t];
        var container = document.getElementById(testId);
        if (!container) continue;

        var rect = container.getBoundingClientRect();
        var children = container.children;

        info += '=== ' + testId + ' ===\n';
        info += '容器尺寸: ' + rect.width.toFixed(0) + 'x' + rect.height.toFixed(0) + '\n';

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

        info += '换行状态: ' + (hasWrapped ? '✓ 已换行' : '✗ 未换行') + '\n\n';
    }

    layoutInfo.textContent = info;
    console.log(info);

    // 自动退出
    setTimeout(function() {
        window.close();
    }, 500);
}, 500);

