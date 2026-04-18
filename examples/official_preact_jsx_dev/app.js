import { Fragment, h, render } from 'preact';
import { useState } from 'preact/hooks';

function App() {
  const [text, setText] = useState('');
  const [items, setItems] = useState([]);

  const addItem = () => {
    const next = text.trim();
    if (!next) return;
    setItems((current) => [...current, next]);
    setText('');
  };

  return (
    <div style={{ fontFamily: 'Segoe UI, sans-serif', padding: 20, background: '#0f172a', color: '#e2e8f0', minHeight: '100vh' }}>
      <h1 id="app-title">Official Preact JSX Dev</h1>
      <p id="state-text">draft:{text || '(empty)'}</p>
      <div style={{ display: 'flex', gap: 8 }}>
        <input
          id="draft-input"
          value={text}
          onInput={(event) => setText(event.target.value)}
          placeholder="type task"
        />
        <button id="add-btn" onClick={addItem}>Add item</button>
      </div>
      <p id="summary">items:{items.length}</p>
      <ul id="items-list">
        {items.map((item, index) => <li key={index}>{item}</li>)}
      </ul>
    </div>
  );
}

render(<App />, document.body);
