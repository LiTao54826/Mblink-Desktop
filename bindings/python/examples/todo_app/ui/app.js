const { h, render } = Preact;

// ── 样式 ─────────────────────────────────────────────────
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
var delBtnStyle = { background: 'none', border: 'none', cursor: 'pointer', color: '#ccc', fontSize: '18px', lineHeight: 1 };

// ── 组件 ─────────────────────────────────────────────────
function App() {
  var todos  = (state.todos  || []).slice();
  var filter = state.filter  || 'all';
  var clock  = state.clock   || '';

  var visible = todos.filter(function(t) {
    return filter === 'all' ? true : filter === 'active' ? !t.done : t.done;
  });
  var activeCount = todos.filter(function(t) { return !t.done; }).length;
  var doneCount   = todos.filter(function(t) { return  t.done; }).length;

  function handleAdd() {
    var inp = document.getElementById('todo-input');
    if (!inp) return;
    var text = inp.value.trim();
    if (!text) return;
    py.addTodo({ text: text });
    inp.value = '';
  }
  function handleKey(e) { if (e.key === 'Enter') handleAdd(); }

  return h('div', { style: S.app },
    h('div', { style: S.header },
      h('span', { style: S.title }, 'Todo List'),
      h('span', { style: S.clock }, clock)
    ),
    h('div', { style: S.inputRow },
      h('input', { id: 'todo-input', style: S.input, placeholder: 'add task', onKeyDown: handleKey }),
      h('button', { style: S.addBtn, onClick: handleAdd }, '+ add')
    ),
    h('div', { style: S.filterRow },
      h('button', { style: filterBtn(filter === 'all'),    onClick: function() { py.setFilter({ value: 'all' }); }    }, 'All'),
      h('button', { style: filterBtn(filter === 'active'), onClick: function() { py.setFilter({ value: 'active' }); } }, 'Active'),
      h('button', { style: filterBtn(filter === 'done'),   onClick: function() { py.setFilter({ value: 'done' }); }   }, 'Done')
    ),
    h('div', { style: S.list },
      visible.length === 0
        ? h('div', { style: S.empty }, todos.length === 0 ? 'No tasks yet' : 'No matching tasks')
        : visible.map(function(t) {
            return h('div', { key: t.id, style: itemStyle(t.done) },
              h('input', { type: 'checkbox', style: S.check, checked: t.done,
                onChange: function() { py.toggleTodo({ id: t.id }); } }),
              h('span', { style: itemTextStyle(t.done) }, t.text),
              h('button', { style: delBtnStyle, onClick: function() { py.deleteTodo({ id: t.id }); } }, 'x')
            );
          })
    ),
    h('div', { style: S.footer },
      h('span', null, activeCount + ' active / ' + doneCount + ' done'),
      doneCount > 0 ? h('button', { style: S.clearBtn, onClick: function() { py.clearDone({}); } }, 'Clear done') : null
    )
  );
}
// ── 渲染 + 更新钩子 ───────────────────────────────────────
var _root = document.getElementById('root');
function rerender() {
  try {
    render(h(App), _root);
  } catch(e) {
    console.error('[rerender error]', e && e.message, e && e.stack);
  }
}
globalThis.__onSharedUpdate = function() { rerender(); };
rerender();

