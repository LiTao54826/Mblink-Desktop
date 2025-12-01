/**
 * Table布局测试
 * 测试HTML表格标签的渲染
 */

var h = Preact.h;
var render = Preact.render;

function TableTest() {
    return h('div', { style: 'padding: 20px; font-family: Arial, sans-serif;' },
        h('h1', { style: 'color: #333; margin-bottom: 20px;' }, 'Table布局测试'),

        // 基础表格
        h('h2', { style: 'color: #666; margin-top: 30px;' }, '1. 基础表格'),
        h('table', { style: 'border: 2px solid #4CAF50; border-collapse: collapse; margin: 10px 0;' },
            h('thead', null,
                h('tr', null,
                    h('th', { style: 'border: 1px solid #ccc; padding: 10px; background: #4CAF50; color: white;' }, '姓名'),
                    h('th', { style: 'border: 1px solid #ccc; padding: 10px; background: #4CAF50; color: white;' }, '年龄'),
                    h('th', { style: 'border: 1px solid #ccc; padding: 10px; background: #4CAF50; color: white;' }, '城市')
                )
            ),
            h('tbody', null,
                h('tr', null,
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '张三'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '25'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '北京')
                ),
                h('tr', { style: 'background: #f9f9f9;' },
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '李四'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '30'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '上海')
                ),
                h('tr', null,
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '王五'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '28'),
                    h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '广州')
                )
            )
        ),
        
        // 简单表格（无thead/tbody）
        h('h2', { style: 'color: #666; margin-top: 30px;' }, '2. 简单表格（无thead/tbody）'),
        h('table', { style: 'border: 2px solid #2196F3; margin: 10px 0;' },
            h('tr', null,
                h('td', { style: 'border: 1px solid #ccc; padding: 8px; background: #e3f2fd;' }, '产品'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px; background: #e3f2fd;' }, '价格'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px; background: #e3f2fd;' }, '数量')
            ),
            h('tr', null,
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, 'Apple'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '$10'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '100')
            ),
            h('tr', null,
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, 'Orange'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '$8'),
                h('td', { style: 'border: 1px solid #ccc; padding: 8px;' }, '150')
            )
        ),
        
        // 带背景色的表格
        h('h2', { style: 'color: #666; margin-top: 30px;' }, '3. 带背景色的表格'),
        h('table', { style: 'border: 2px solid #FF9800; background: #fff3e0; margin: 10px 0;' },
            h('tr', null,
                h('th', { style: 'border: 1px solid #FF9800; padding: 10px; background: #FF9800; color: white;' }, '周一'),
                h('th', { style: 'border: 1px solid #FF9800; padding: 10px; background: #FF9800; color: white;' }, '周二'),
                h('th', { style: 'border: 1px solid #FF9800; padding: 10px; background: #FF9800; color: white;' }, '周三')
            ),
            h('tr', null,
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '数学'),
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '英语'),
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '物理')
            ),
            h('tr', null,
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '语文'),
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '化学'),
                h('td', { style: 'border: 1px solid #FF9800; padding: 8px;' }, '生物')
            )
        ),
        
        // 说明文字
        h('div', { style: 'margin-top: 30px; padding: 15px; background: #e8f5e9; border-radius: 8px;' },
            h('h3', { style: 'color: #2e7d32; margin-bottom: 10px;' }, '测试说明'),
            h('p', { style: 'color: #666; line-height: 1.6;' }, 
                '本示例测试HTML表格标签的渲染，包括：table, thead, tbody, tr, td, th 等元素。'
            ),
            h('p', { style: 'color: #666; line-height: 1.6;' }, 
                '表格应该正确显示边框、背景色和内容对齐。'
            )
        )
    );
}

// 渲染应用
render(h(TableTest), document.body);

