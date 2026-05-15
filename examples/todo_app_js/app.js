/**
 * 纯 JS 版 Todo App - 用于对比测试输入延迟
 * 使用 Preact + useState，不依赖 Python SharedObject
 */
import { h, render } from 'preact';
import { useState } from 'preact/hooks';

// ── 样式（与 Python 版完全相同）─────────────────────────────
const S = {
  app: { fontFamily: '"Segoe UI", sans-serif', background: '#f5f5f5', minHeight: '100vh', display: 'flex', flexDirection: 'column' },
  header: { background: '#4a90d9', color: '#fff', padding: '16px 20px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' },
  title: { fontSize: '20px', fontWeight: 700 },
  clock: { fontSize: '13px', opacity: 0.85 },
  inputRow: { display: 'flex', gap: '8px', padding: '16px 20px', background: '#fff', borderBottom: '1px solid #e0e0e0' },
  input: { flex: 1, padding: '10px 14px', fontSize: '15px', border: '1px solid #ddd', borderRadius: '6px', outline: 'none' },
  addBtn: { padding: '10px 18px', background: '#4a90d9', color: '#fff', border: 'none', borderRadius: '6px', cursor: 'pointer', fontSize: '15px', fontWeight: 600 },
  filterRow: { display: 'flex', gap: '6px', padding: '10px 20px', background: '#fff', borderBottom: '1px solid #e0e0e0' },
  list: { flex: 1, padding: '12px 20px', display: 'flex', flexDirection: 'column', gap: '8px' },
  check: { width: '18px', height: '18px', cursor: 'pointer', accentColor: '#4a90d9' },
  footer: { padding: '10px 20px', background: '#fff', borderTop: '1px solid #e0e0e0', display: 'flex', justifyContent: 'space-between', alignItems: 'center', fontSize: '13px', color: '#888' },
  clearBtn: { background: 'none', border: 'none', cursor: 'pointer', color: '#e57373', fontSize: '13px' },
  empty: { textAlign: 'center', color: '#bbb', padding: '40px 0', fontSize: '15px' },
};

function filterBtn(active) {
  return { padding: '5px 14px', border: '1px solid #ddd', borderRadius: '20px', cursor: 'pointer', fontSize: '13px', background: active ? '#4a90d9' : '#fff', color: active ? '#fff' : '#555' };
}
function itemStyle(done) {
  return { display: 'flex', alignItems: 'center', gap: '10px', padding: '12px 14px', background: '#fff', borderRadius: '8px', boxShadow: '0 1px 3px rgba(0,0,0,0.08)', opacity: done ? 0.6 : 1 };
}
function itemTextStyle(done) {
  return { flex: 1, fontSize: '15px', textDecoration: done ? 'line-through' : 'none', color: done ? '#999' : '#333' };
}
const delBtnStyle = { background: 'none', border: 'none', cursor: 'pointer', color: '#ccc', fontSize: '18px', lineHeight: 1 };

// ── 时钟组件 ─────────────────────────────────────────────────
function Clock() {
  const [time, setTime] = useState(new Date());
  
  // 使用 setInterval 更新时钟
  if (!globalThis.__clockInterval) {
    globalThis.__clockInterval = setInterval(() => setTime(new Date()), 1000);
  }
  
  const pad = (n) => (n < 10 ? '0' : '') + n;
  const s = pad(time.getHours()) + ':' + pad(time.getMinutes()) + ':' + pad(time.getSeconds());
  
  return h('span', { style: S.clock }, s);
}

// ── 主应用组件（使用 useState）─────────────────────────────────
function App() {
  const [todos, setTodos] = useState([]);
  const [filter, setFilter] = useState('all');
  const [nextId, setNextId] = useState(1);

  const visible = todos.filter(t => 
    filter === 'all' ? true : filter === 'active' ? !t.done : t.done
  );
  const activeCount = todos.filter(t => !t.done).length;
  const doneCount = todos.filter(t => t.done).length;

  function handleAdd() {
    const inp = document.getElementById('todo-input');
    if (!inp) return;
    const text = inp.value.trim();
    if (!text) return;
    
    setTodos([...todos, { id: nextId, text, done: false }]);
    setNextId(nextId + 1);
    inp.value = '';
  }

  function handleKey(e) {
    if (e.key === 'Enter') handleAdd();
  }

  function toggleTodo(id) {
    setTodos(todos.map(t => t.id === id ? { ...t, done: !t.done } : t));
  }

  function deleteTodo(id) {
    setTodos(todos.filter(t => t.id !== id));
  }

  function clearDone() {
    setTodos(todos.filter(t => !t.done));
  }

  return h('div', { style: S.app },
    h('div', { style: S.header },
      h('span', { style: S.title }, 'Todo List (Pure JS)'),
      // h(Clock)
    ),
    h('div', { style: S.inputRow },
      h('input', { id: 'todo-input', style: S.input, placeholder: 'add task', onKeyDown: handleKey }),
      h('button', { style: S.addBtn, onClick: handleAdd }, '+ add')
    ),
    h('div', { style: S.filterRow },
      h('button', { style: filterBtn(filter === 'all'), onClick: () => setFilter('all') }, 'All'),
      h('button', { style: filterBtn(filter === 'active'), onClick: () => setFilter('active') }, 'Active'),
      h('button', { style: filterBtn(filter === 'done'), onClick: () => setFilter('done') }, 'Done')
    ),
    h('div', { style: S.list },
      visible.length === 0
        ? h('div', { style: S.empty }, todos.length === 0 ? 'No tasks yet' : 'No matching tasks')
        : visible.map(t =>
            h('div', { key: t.id, style: itemStyle(t.done) },
              h('input', { type: 'checkbox', style: S.check, checked: t.done, onChange: () => toggleTodo(t.id) }),
              h('span', { style: itemTextStyle(t.done) }, t.text),
              h('button', { style: delBtnStyle, onClick: () => deleteTodo(t.id) }, 'x')
            )
          )
    ),
    h('div', { style: S.footer },
      h('span', null, activeCount + ' active / ' + doneCount + ' done'),
      doneCount > 0 ? h('button', { style: S.clearBtn, onClick: clearDone }, 'Clear done') : null
    )
  );
}

// ── 渲染 ─────────────────────────────────────────────────────
render(h(App), document.body);
console.log('[Pure JS Todo App] Loaded - 用于对比测试输入延迟');
