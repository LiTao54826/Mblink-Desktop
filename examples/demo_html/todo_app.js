/**
 * @file todo_app.js
 * @brief 待办事项管理器 - 展示完整的 DOM 绑定功能
 */

console.log('📝 启动待办事项管理器...');

// 应用状态
let todos = [];
let nextId = 1;

// 创建应用容器
const app = document.createElement('div');
app.setAttribute('id', 'todo-app');
app.style.setProperty('max-width', '800px');
app.style.setProperty('margin', '0 auto');
app.style.setProperty('padding', '20px');
app.style.setProperty('font-family', 'Arial, sans-serif');

// ========== 标题 ==========
const header = document.createElement('div');
header.style.setProperty('text-align', 'center');
header.style.setProperty('margin-bottom', '30px');

const title = document.createElement('h1');
title.textContent = '📝 待办事项管理器';
title.style.setProperty('color', '#2c3e50');
title.style.setProperty('margin', '0');
title.style.setProperty('font-size', '36px');

const subtitle = document.createElement('p');
subtitle.textContent = '使用 LightUI DOM 绑定构建';
subtitle.style.setProperty('color', '#7f8c8d');
subtitle.style.setProperty('margin-top', '10px');

header.appendChild(title);
header.appendChild(subtitle);

// ========== 输入区域 ==========
const inputContainer = document.createElement('div');
inputContainer.style.setProperty('display', 'flex');
inputContainer.style.setProperty('gap', '10px');
inputContainer.style.setProperty('margin-bottom', '20px');

const input = document.createElement('input');
input.setAttribute('type', 'text');
input.setAttribute('placeholder', '输入新的待办事项...');
input.style.setProperty('flex', '1');
input.style.setProperty('padding', '12px');
input.style.setProperty('font-size', '16px');
input.style.setProperty('border', '2px solid #ddd');
input.style.setProperty('border-radius', '5px');

const addButton = document.createElement('button');
addButton.textContent = '➕ 添加';
addButton.style.setProperty('padding', '12px 24px');
addButton.style.setProperty('font-size', '16px');
addButton.style.setProperty('background', '#3498db');
addButton.style.setProperty('color', 'white');
addButton.style.setProperty('border', 'none');
addButton.style.setProperty('border-radius', '5px');
addButton.style.setProperty('cursor', 'pointer');

inputContainer.appendChild(input);
inputContainer.appendChild(addButton);

// ========== 统计信息 ==========
const statsContainer = document.createElement('div');
statsContainer.style.setProperty('background', '#ecf0f1');
statsContainer.style.setProperty('padding', '15px');
statsContainer.style.setProperty('border-radius', '5px');
statsContainer.style.setProperty('margin-bottom', '20px');
statsContainer.style.setProperty('display', 'flex');
statsContainer.style.setProperty('justify-content', 'space-around');

const totalStat = document.createElement('div');
totalStat.setAttribute('id', 'total-stat');
totalStat.style.setProperty('text-align', 'center');

const completedStat = document.createElement('div');
completedStat.setAttribute('id', 'completed-stat');
completedStat.style.setProperty('text-align', 'center');

const pendingStat = document.createElement('div');
pendingStat.setAttribute('id', 'pending-stat');
pendingStat.style.setProperty('text-align', 'center');

statsContainer.appendChild(totalStat);
statsContainer.appendChild(completedStat);
statsContainer.appendChild(pendingStat);

// ========== 待办列表 ==========
const todoList = document.createElement('div');
todoList.setAttribute('id', 'todo-list');

// ========== 组装应用 ==========
app.appendChild(header);
app.appendChild(inputContainer);
app.appendChild(statsContainer);
app.appendChild(todoList);
document.body.appendChild(app);

// ========== 功能函数 ==========

function updateStats() {
    const total = todos.length;
    const completed = todos.filter(t => t.completed).length;
    const pending = total - completed;

    totalStat.textContent = '';
    const totalNum = document.createElement('div');
    totalNum.textContent = total.toString();
    totalNum.style.setProperty('font-size', '32px');
    totalNum.style.setProperty('font-weight', 'bold');
    totalNum.style.setProperty('color', '#3498db');
    const totalLabel = document.createElement('div');
    totalLabel.textContent = '总计';
    totalLabel.style.setProperty('color', '#7f8c8d');
    totalStat.appendChild(totalNum);
    totalStat.appendChild(totalLabel);

    completedStat.textContent = '';
    const completedNum = document.createElement('div');
    completedNum.textContent = completed.toString();
    completedNum.style.setProperty('font-size', '32px');
    completedNum.style.setProperty('font-weight', 'bold');
    completedNum.style.setProperty('color', '#27ae60');
    const completedLabel = document.createElement('div');
    completedLabel.textContent = '已完成';
    completedLabel.style.setProperty('color', '#7f8c8d');
    completedStat.appendChild(completedNum);
    completedStat.appendChild(completedLabel);

    pendingStat.textContent = '';
    const pendingNum = document.createElement('div');
    pendingNum.textContent = pending.toString();
    pendingNum.style.setProperty('font-size', '32px');
    pendingNum.style.setProperty('font-weight', 'bold');
    pendingNum.style.setProperty('color', '#e74c3c');
    const pendingLabel = document.createElement('div');
    pendingLabel.textContent = '待完成';
    pendingLabel.style.setProperty('color', '#7f8c8d');
    pendingStat.appendChild(pendingNum);
    pendingStat.appendChild(pendingLabel);
}

function renderTodos() {
    // 清空列表
    todoList.textContent = '';

    if (todos.length === 0) {
        const empty = document.createElement('div');
        empty.textContent = '暂无待办事项，添加一个开始吧！';
        empty.style.setProperty('text-align', 'center');
        empty.style.setProperty('padding', '40px');
        empty.style.setProperty('color', '#95a5a6');
        empty.style.setProperty('font-size', '18px');
        todoList.appendChild(empty);
        return;
    }

    todos.forEach(todo => {
        const item = document.createElement('div');
        item.style.setProperty('background', 'white');
        item.style.setProperty('padding', '15px');
        item.style.setProperty('margin-bottom', '10px');
        item.style.setProperty('border-radius', '5px');
        item.style.setProperty('border', '1px solid #ddd');
        item.style.setProperty('display', 'flex');
        item.style.setProperty('align-items', 'center');
        item.style.setProperty('gap', '10px');

        // 复选框
        const checkbox = document.createElement('input');
        checkbox.setAttribute('type', 'checkbox');
        if (todo.completed) {
            checkbox.setAttribute('checked', 'checked');
        }
        checkbox.style.setProperty('width', '20px');
        checkbox.style.setProperty('height', '20px');
        checkbox.style.setProperty('cursor', 'pointer');

        // 文本
        const text = document.createElement('span');
        text.textContent = todo.text;
        text.style.setProperty('flex', '1');
        text.style.setProperty('font-size', '16px');
        if (todo.completed) {
            text.style.setProperty('text-decoration', 'line-through');
            text.style.setProperty('color', '#95a5a6');
        }

        // 删除按钮
        const deleteBtn = document.createElement('button');
        deleteBtn.textContent = '🗑️';
        deleteBtn.style.setProperty('background', '#e74c3c');
        deleteBtn.style.setProperty('color', 'white');
        deleteBtn.style.setProperty('border', 'none');
        deleteBtn.style.setProperty('padding', '8px 12px');
        deleteBtn.style.setProperty('border-radius', '3px');
        deleteBtn.style.setProperty('cursor', 'pointer');

        // 事件监听
        checkbox.addEventListener('click', function () {
            todo.completed = !todo.completed;
            renderTodos();
            updateStats();
            console.log('✅ 切换完成状态:', todo.text);
        });

        deleteBtn.addEventListener('click', function () {
            todos = todos.filter(t => t.id !== todo.id);
            renderTodos();
            updateStats();
            console.log('🗑️ 删除:', todo.text);
        });

        item.appendChild(checkbox);
        item.appendChild(text);
        item.appendChild(deleteBtn);
        todoList.appendChild(item);
    });
}

function addTodo() {
    const text = input.value.trim();
    if (!text) {
        console.log('⚠️ 请输入待办内容');
        return;
    }

    todos.push({
        id: nextId++,
        text: text,
        completed: false
    });

    input.value = '';
    renderTodos();
    updateStats();
    console.log('➕ 添加待办:', text);
}

// ========== 事件绑定 ==========
addButton.addEventListener('click', addTodo);

input.addEventListener('keydown', function (e) {
    if (e.key === 'Enter') {
        addTodo();
    }
});

// ========== 初始化 ==========
updateStats();
renderTodos();

// 添加一些示例数据
setTimeout(() => {
    console.log('📌 添加示例待办事项...');
    todos = [
        { id: nextId++, text: '学习 LightUI DOM 绑定', completed: true },
        { id: nextId++, text: '创建待办事项应用', completed: true },
        { id: nextId++, text: '测试事件监听功能', completed: false },
        { id: nextId++, text: '优化应用样式', completed: false }
    ];
    renderTodos();
    updateStats();
}, 100);

console.log('✅ 待办事项管理器启动完成！');
