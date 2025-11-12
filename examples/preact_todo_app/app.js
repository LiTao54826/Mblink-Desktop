/**
 * @file app.js
 * @brief Preact Todo App - 更复杂的示例，展示组件组合、列表渲染等
 */

// ========== 子组件 ==========

// Header组件
function Header(props) {
    return Preact.h('div', { 
        style: 'background-color: #2196F3; color: white; padding: 20px; margin-bottom: 20px;' 
    },
        Preact.h('h1', { 
            style: 'margin: 0; font-size: 28px;' 
        }, props.title),
        Preact.h('p', { 
            style: 'margin: 5px 0 0 0; font-size: 14px; opacity: 0.9;' 
        }, props.subtitle)
    );
}

// TodoItem组件
function TodoItem(props) {
    var item = props.item;
    var index = props.index;
    
    var itemStyle = 'padding: 12px; margin-bottom: 8px; background-color: white; border-left: 4px solid ' + 
                    (item.completed ? '#4CAF50' : '#FF9800') + ';';
    
    var textStyle = 'margin: 0; font-size: 16px; ' + 
                    (item.completed ? 'text-decoration: line-through; color: #999;' : 'color: #333;');
    
    var statusStyle = 'display: inline-block; padding: 2px 8px; margin-left: 10px; font-size: 12px; ' +
                      'background-color: ' + (item.completed ? '#4CAF50' : '#FF9800') + '; ' +
                      'color: white; border-radius: 3px;';
    
    return Preact.h('div', { style: itemStyle },
        Preact.h('p', { style: textStyle },
            '#' + (index + 1) + ' - ' + item.text,
            Preact.h('span', { style: statusStyle }, 
                item.completed ? '✓ Done' : '⏳ Pending'
            )
        )
    );
}

// TodoList组件
function TodoList(props) {
    var items = props.items;
    
    var containerStyle = 'background-color: #f5f5f5; padding: 15px; border-radius: 8px;';
    
    var titleStyle = 'margin: 0 0 15px 0; font-size: 20px; color: #333; font-weight: bold;';
    
    // 创建TodoItem数组
    var todoItems = [];
    for (var i = 0; i < items.length; i++) {
        todoItems.push(
            Preact.h(TodoItem, { 
                item: items[i], 
                index: i,
                key: 'todo-' + i 
            })
        );
    }
    
    return Preact.h('div', { style: containerStyle },
        Preact.h('h2', { style: titleStyle }, 
            '📝 Todo List (' + items.length + ' items)'
        ),
        Preact.h('div', null, todoItems)
    );
}

// Stats组件 - 显示统计信息
function Stats(props) {
    var items = props.items;
    
    var total = items.length;
    var completed = 0;
    var pending = 0;
    
    for (var i = 0; i < items.length; i++) {
        if (items[i].completed) {
            completed++;
        } else {
            pending++;
        }
    }
    
    var containerStyle = 'display: flex; margin-top: 20px;';
    
    var cardStyle = 'flex: 1; padding: 15px; margin-right: 10px; border-radius: 8px; text-align: center;';
    
    var totalCardStyle = cardStyle + ' background-color: #2196F3; color: white;';
    var completedCardStyle = cardStyle + ' background-color: #4CAF50; color: white;';
    var pendingCardStyle = cardStyle + ' background-color: #FF9800; color: white; margin-right: 0;';
    
    var numberStyle = 'font-size: 32px; font-weight: bold; margin: 0;';
    var labelStyle = 'font-size: 14px; margin: 5px 0 0 0; opacity: 0.9;';
    
    return Preact.h('div', { style: containerStyle },
        Preact.h('div', { style: totalCardStyle },
            Preact.h('p', { style: numberStyle }, total.toString()),
            Preact.h('p', { style: labelStyle }, 'Total Tasks')
        ),
        Preact.h('div', { style: completedCardStyle },
            Preact.h('p', { style: numberStyle }, completed.toString()),
            Preact.h('p', { style: labelStyle }, 'Completed')
        ),
        Preact.h('div', { style: pendingCardStyle },
            Preact.h('p', { style: numberStyle }, pending.toString()),
            Preact.h('p', { style: labelStyle }, 'Pending')
        )
    );
}

// Footer组件
function Footer() {
    var footerStyle = 'margin-top: 30px; padding: 15px; background-color: #f5f5f5; ' +
                      'border-top: 2px solid #ddd; text-align: center;';
    
    var textStyle = 'margin: 0; font-size: 14px; color: #666;';
    
    return Preact.h('div', { style: footerStyle },
        Preact.h('p', { style: textStyle }, 
            '🚀 Powered by MBink + Preact + QuickJS + Skia'
        ),
        Preact.h('p', { style: textStyle }, 
            '✨ Lightweight Desktop Application Framework'
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    // 模拟数据
    var todoItems = [
        { text: 'Implement Preact integration', completed: true },
        { text: 'Create Virtual DOM renderer', completed: true },
        { text: 'Build component system', completed: true },
        { text: 'Add event handling', completed: false },
        { text: 'Implement useState hook', completed: false },
        { text: 'Add useEffect hook', completed: false },
        { text: 'Create more examples', completed: false }
    ];
    
    var appStyle = 'padding: 0; margin: 0; background-color: #e0e0e0; min-height: 100vh;';
    
    var containerStyle = 'max-width: 800px; margin: 0 auto; padding: 20px;';
    
    return Preact.h('div', { style: appStyle },
        Preact.h('div', { style: containerStyle },
            Preact.h(Header, { 
                title: 'MBink Todo App',
                subtitle: 'A Preact-powered desktop application'
            }),
            Preact.h(Stats, { items: todoItems }),
            Preact.h('div', { style: 'height: 20px;' }), // Spacer
            Preact.h(TodoList, { items: todoItems }),
            Preact.h(Footer)
        )
    );
}

// ========== 渲染应用 ==========

var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);

