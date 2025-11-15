/**
 * @file app.js
 * @brief MBink Window Demo - JavaScript 应用逻辑
 */

console.log('App.js loaded');

// ========== 计数器功能 ==========
let counter = 0;

function incrementCounter() {
    counter++;
    updateCounter();
    console.log('Counter incremented:', counter);
}

function decrementCounter() {
    counter--;
    updateCounter();
    console.log('Counter decremented:', counter);
}

function resetCounter() {
    counter = 0;
    updateCounter();
    console.log('Counter reset');
}

function updateCounter() {
    const counterEl = document.getElementById('counter');
    if (counterEl) {
        counterEl.textContent = counter.toString();
    }
}

// ========== 问候功能 ==========
function greet() {
    const nameInput = document.getElementById('nameInput');
    const greetingEl = document.getElementById('greeting');

    if (nameInput && greetingEl) {
        const name = nameInput.value.trim();
        if (name) {
            greetingEl.textContent = `Hello, ${name}! Welcome to MBink!`;
            console.log('Greeted:', name);
        } else {
            greetingEl.textContent = 'Please enter your name first!';
        }
    }
}

// ========== Todo List 功能 ==========
let todos = [];

function addTodo() {
    console.log('addTodo called!');
    const todoInput = document.getElementById('todoInput');
    const todoList = document.getElementById('todoList');

    console.log('todoInput:', todoInput);
    console.log('todoList:', todoList);

    if (todoInput && todoList) {
        const text = todoInput.value.trim();
        console.log('Input text:', text);
        if (text) {
            todos.push({
                id: Date.now(),
                text: text,
                completed: false
            });

            todoInput.value = '';
            renderTodos();
            console.log('Todo added:', text);
        } else {
            console.log('Input text is empty');
        }
    } else {
        console.log('todoInput or todoList not found!');
    }
}

function toggleTodo(id) {
    const todo = todos.find(t => t.id === id);
    if (todo) {
        todo.completed = !todo.completed;
        renderTodos();
        console.log('Todo toggled:', id);
    }
}

function deleteTodo(id) {
    todos = todos.filter(t => t.id !== id);
    renderTodos();
    console.log('Todo deleted:', id);
}

function renderTodos() {
    const todoList = document.getElementById('todoList');
    if (!todoList) return;
    
    if (todos.length === 0) {
        todoList.innerHTML = '<li style="color: #6c757d; padding: 10px;">No todos yet. Add one above! 📝</li>';
        return;
    }
    
    let html = '';
    for (const todo of todos) {
        const style = todo.completed 
            ? 'text-decoration: line-through; color: #6c757d;' 
            : '';
        
        html += `
            <li style="padding: 10px; border-bottom: 1px solid #dee2e6; display: flex; align-items: center; justify-content: space-between;">
                <span style="${style}">${todo.text}</span>
                <div>
                    <button class="btn-primary" onclick="toggleTodo(${todo.id})" style="padding: 5px 10px; font-size: 12px;">
                        ${todo.completed ? '↩️ Undo' : '✓ Done'}
                    </button>
                    <button class="btn-danger" onclick="deleteTodo(${todo.id})" style="padding: 5px 10px; font-size: 12px;">
                        🗑️ Delete
                    </button>
                </div>
            </li>
        `;
    }
    
    todoList.innerHTML = html;
}

// ========== 定时器功能 ==========
let timerInterval = null;
let timerSeconds = 0;
let timerRunning = false;

function toggleTimer() {
    console.log('toggleTimer called! timerRunning =', timerRunning);
    const timerBtn = document.getElementById('timerBtn');

    if (timerRunning) {
        // 停止定时器
        if (timerInterval !== null) {
            clearInterval(timerInterval);
            timerInterval = null;
        }
        timerRunning = false;
        if (timerBtn) {
            timerBtn.textContent = 'Start';
        }
        console.log('Timer stopped');
    } else {
        // 启动定时器
        timerRunning = true;
        if (timerBtn) {
            timerBtn.textContent = 'Pause';
        }

        console.log('Calling setInterval...');
        timerInterval = setInterval(() => {
            console.log('Timer tick! timerSeconds =', timerSeconds);
            timerSeconds++;
            updateTimer();
            console.log('After update, timerSeconds =', timerSeconds);
        }, 1000);

        console.log('Timer started, interval ID =', timerInterval);
    }
}

function updateTimer() {
    console.log('updateTimer called, timerSeconds =', timerSeconds);
    const timerEl = document.getElementById('timer');
    if (timerEl) {
        const newText = `Timer: ${timerSeconds}s`;
        console.log('Setting timer text to:', newText);
        timerEl.textContent = newText;
        console.log('Timer text set successfully');
    } else {
        console.log('ERROR: timer element not found!');
    }
}

// ========== 键盘事件处理 ==========
// 注意：document.addEventListener 尚未实现，暂时禁用键盘事件
// TODO: 在 DOM 绑定中实现 document.addEventListener 后启用
/*
document.addEventListener('keydown', (e) => {
    // Enter 键提交表单
    if (e.key === 'Enter') {
        const activeElement = document.activeElement;

        if (activeElement && activeElement.id === 'nameInput') {
            greet();
        } else if (activeElement && activeElement.id === 'todoInput') {
            addTodo();
        }
    }

    // 空格键切换定时器
    if (e.key === ' ' && e.target.tagName !== 'INPUT' && e.target.tagName !== 'TEXTAREA') {
        e.preventDefault();
        toggleTimer();
    }
});
*/

// ========== 事件绑定 ==========
function bindEventListeners() {
    console.log('Binding event listeners...');

    // 计数器按钮
    const incrementBtn = document.getElementById('incrementBtn');
    const decrementBtn = document.getElementById('decrementBtn');
    const resetBtn = document.getElementById('resetBtn');

    if (incrementBtn) {
        incrementBtn.addEventListener('click', incrementCounter);
        console.log('  ✓ Increment button bound');
    }
    if (decrementBtn) {
        decrementBtn.addEventListener('click', decrementCounter);
        console.log('  ✓ Decrement button bound');
    }
    if (resetBtn) {
        resetBtn.addEventListener('click', resetCounter);
        console.log('  ✓ Reset button bound');
    }

    // 问候按钮
    const greetBtn = document.getElementById('greetBtn');
    if (greetBtn) {
        greetBtn.addEventListener('click', greet);
        console.log('  ✓ Greet button bound');
    }

    // Todo 添加按钮
    const addTodoBtn = document.getElementById('addTodoBtn');
    if (addTodoBtn) {
        addTodoBtn.addEventListener('click', addTodo);
        console.log('  ✓ Add todo button bound');
    }

    // 定时器按钮
    const timerBtn = document.getElementById('timerBtn');
    console.log('Looking for timerBtn, found:', timerBtn);
    if (timerBtn) {
        console.log('Adding click listener to timerBtn...');
        timerBtn.addEventListener('click', toggleTimer);
        console.log('  ✓ Timer button bound');

        // 测试：手动触发一次看看
        console.log('Testing timer button click...');
        toggleTimer();
    } else {
        console.log('  ✗ Timer button NOT found!');
    }

    console.log('✅ Event listeners bound');
}

// ========== 初始化 ==========
function init() {
    console.log('Initializing app...');

    // 初始化计数器
    updateCounter();

    // 初始化 Todo 列表
    renderTodos();

    // 初始化定时器
    updateTimer();

    // 绑定事件监听器（使用 addEventListener 代替 onclick）
    bindEventListeners();

    console.log('✅ App initialized');
}

// 直接初始化（因为 DOMContentLoaded 事件尚未实现）
init();

console.log('✅ App.js ready');

