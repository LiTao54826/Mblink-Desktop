import { h, render } from 'preact';
import { useEffect, useRef } from 'preact/hooks';
import { EditorView, basicSetup, javascript, oneDark } from '../../js/codemirror6/codemirror6.js';

window.__CM6_DEBUG__ = true;

function App() {
  const editorRef = useRef(null);
  const viewRef = useRef(null);

  useEffect(() => {
    if (!editorRef.current || viewRef.current) return undefined;

    const view = new EditorView({
      doc: [
        'function greet(name) {',
        '  console.log(`hello ${name}`);',
        '',
        '',
        '',
        '',
        '',
        '',
        '',
        '',
        '',
        '',
        '',
        '}',
        '',
        'greet("CodeMirror6");'
      ].join('\n'),
      extensions: [
        basicSetup,
        javascript(),
        oneDark,
        EditorView.theme({
          '&.cm-focused > .cm-scroller > .cm-cursorLayer': {
            animation: 'none !important'
          }
        })
      ],
      parent: editorRef.current
    });

    viewRef.current = view;
    window.cm6View = view;

    return () => {
      view.destroy();
      viewRef.current = null;
      delete window.cm6View;
    };
  }, []);

  const focusEditor = () => {
    viewRef.current?.focus();
  };

  const selectAll = () => {
    const view = viewRef.current;
    if (!view) return;
    view.dispatch({ selection: { anchor: 0, head: view.state.doc.length } });
    view.focus();
  };

  const insertText = () => {
    const view = viewRef.current;
    if (!view) return;
    const pos = view.state.selection.main.head;
    view.dispatch({ changes: { from: pos, insert: '\n// inserted by demo' } });
    view.focus();
  };

  return h('div', { style: { padding: '24px', maxWidth: '960px', margin: '0 auto' } }, [
    h('h1', { key: 'title', style: { margin: '0 0 12px 0' } }, 'CodeMirror6 测试示例'),
    h('p', { key: 'hint', style: { color: '#94a3b8', margin: '0 0 16px 0' } }, '已移除调试探针，仅保留纯编辑器性能观察。'),
    h('div', { key: 'toolbar', style: { display: 'flex', gap: '8px', marginBottom: '16px', flexWrap: 'wrap' } }, [
      h('button', { onClick: focusEditor }, 'focus editor'),
      h('button', { onClick: selectAll }, 'select all'),
      h('button', { onClick: insertText }, 'insert text')
    ]),
    h('div', {
      key: 'editor',
      ref: editorRef,
      style: { border: '1px solid #334155', borderRadius: '10px', overflow: 'hidden', minHeight: '240px' }
    })
  ]);
}

render(h(App), document.body);

