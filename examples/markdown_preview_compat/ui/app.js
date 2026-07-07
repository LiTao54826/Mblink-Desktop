import { h, render } from 'preact';
import { useEffect, useMemo, useRef, useState } from 'preact/hooks';
import { createMarkdownRenderer, splitMarkdownBlocks } from './markdown_renderer.js';
import { MAINSTREAM_MARKDOWN, STREAM_CHUNKS, buildChatMessages, buildLongMarkdownDocument } from './markdown_samples.js';

const markdownRenderer = createMarkdownRenderer({ h });
const PREVIEW_COMMIT_DELAY_MS = 180;

const css = `
* { box-sizing: border-box; }
body {
  margin: 0;
  overflow: hidden;
  font-family: "Segoe UI", Arial, sans-serif;
  background: #f4f5f7;
  color: #20242b;
}
button, textarea { font: inherit; }
.app-shell {
  height: 100vh;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.topbar {
  min-height: 62px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 20px;
  padding: 0 22px;
  background: #20242b;
  color: #f7f8fa;
}
.title-block { display: flex; flex-direction: column; gap: 2px; min-width: 280px; }
.title-block h1 { margin: 0; font-size: 19px; font-weight: 700; }
.title-block span { font-size: 12px; color: #bdc6d4; }
.toolbar { display: flex; align-items: center; gap: 8px; flex-wrap: wrap; justify-content: flex-end; }
.btn {
  min-height: 34px;
  padding: 7px 12px;
  border: 1px solid #ccd3df;
  border-radius: 6px;
  background: #ffffff;
  color: #20242b;
  cursor: pointer;
}
.btn.active {
  background: #2f6f61;
  border-color: #2f6f61;
  color: #ffffff;
}
.btn.primary {
  background: #b84b38;
  border-color: #b84b38;
  color: #ffffff;
}
.btn.small {
  min-height: 28px;
  padding: 5px 9px;
  font-size: 12px;
}
.workspace {
  flex: 1;
  height: calc(100vh - 62px);
  max-height: calc(100vh - 62px);
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(360px, 0.92fr) minmax(520px, 1.25fr);
  min-width: 920px;
  overflow: hidden;
}
.source-pane, .preview-pane {
  height: calc(100vh - 62px);
  max-height: calc(100vh - 62px);
  min-height: 0;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.source-pane {
  border-right: 1px solid #d6dce5;
  background: #ffffff;
}
.pane-head {
  min-height: 52px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 13px 16px;
  border-bottom: 1px solid #e2e6ee;
}
.pane-head h2 { margin: 0; font-size: 15px; }
.pane-head small { color: #667085; }
.pane-title { display: flex; flex-direction: column; gap: 2px; min-width: 0; }
.pane-actions { display: flex; align-items: center; gap: 6px; flex-wrap: wrap; justify-content: flex-end; }
.source-editor {
  display: block;
  height: 568px;
  min-height: 568px;
  max-height: 568px;
  width: 100%;
  padding: 16px;
  border: 0;
  outline: none;
  resize: none;
  overflow: auto;
  line-height: 1.45;
  color: #20242b;
  background: #fbfcfe;
  border-bottom: 1px solid #e2e6ee;
}
.stats {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 8px;
  padding: 12px 16px 16px;
}
.stat {
  min-width: 0;
  padding: 8px;
  border: 1px solid #e2e6ee;
  border-radius: 6px;
  background: #f8fafc;
}
.stat strong { display: block; font-size: 17px; color: #20242b; }
.stat span { display: block; font-size: 11px; color: #667085; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.preview-scroll {
  height: calc(100vh - 114px);
  max-height: calc(100vh - 114px);
  min-height: 0;
  overflow: auto;
  padding: 22px;
}
.document-surface {
  max-width: 820px;
  margin: 0 auto;
  padding: 26px 28px;
  background: #ffffff;
  border: 1px solid #e0e5ed;
  border-radius: 8px;
}
.md-heading { color: #20242b; line-height: 1.18; margin: 22px 0 10px; }
.md-h1 { margin-top: 0; font-size: 30px; }
.md-h2 { font-size: 22px; }
.md-h3 { font-size: 18px; }
.md-paragraph, .md-list, .md-blockquote { line-height: 1.58; font-size: 15px; }
.md-paragraph { margin: 10px 0; }
.md-link { color: #246b8f; text-decoration: underline; }
.md-inline-code {
  padding: 1px 5px;
  border-radius: 5px;
  background: #eef2f6;
  color: #7d3b24;
}
.md-image {
  display: block;
  width: 64px;
  height: 64px;
  margin: 12px 0;
  border-radius: 6px;
  border: 1px solid #d6dce5;
  background: linear-gradient(135deg, #2f6f61 0 50%, #d19a45 50% 100%);
  image-rendering: pixelated;
}
.md-list { padding-left: 24px; margin: 10px 0; }
.md-task {
  display: flex;
  align-items: center;
  gap: 8px;
  list-style: none;
  margin-left: -20px;
}
.md-task-check { width: 16px; height: 16px; flex: 0 0 auto; }
.md-task.is-done span { color: #4f635e; }
.md-blockquote {
  margin: 14px 0;
  padding: 10px 14px;
  border-left: 4px solid #d19a45;
  background: #fff8ec;
  color: #3e4753;
}
.md-code-block, .md-html-escaped {
  overflow: auto;
  margin: 14px 0;
  padding: 14px;
  border-radius: 7px;
  line-height: 1.45;
  background: #19202a;
  color: #e7edf7;
}
.md-html-escaped {
  border: 1px solid #d19a45;
  background: #2d2418;
}
.md-table {
  width: 100%;
  border-collapse: collapse;
  margin: 16px 0;
  font-size: 14px;
}
.md-table th, .md-table td {
  padding: 9px 10px;
  border: 1px solid #d8dee8;
  text-align: left;
  vertical-align: top;
}
.md-table th { background: #eef2f6; }
.md-rule { border: 0; border-top: 1px solid #d8dee8; margin: 18px 0; }
.chat-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
  max-width: 880px;
  margin: 0 auto;
}
.message {
  padding: 14px 16px;
  border: 1px solid #dfe5ee;
  border-radius: 8px;
  background: #ffffff;
}
.message.user { border-left: 4px solid #246b8f; }
.message.assistant { border-left: 4px solid #2f6f61; }
.message-head {
  display: flex;
  justify-content: space-between;
  color: #667085;
  font-size: 12px;
  margin-bottom: 8px;
}
.virtual-note {
  padding: 9px 12px;
  border: 1px dashed #a7b3c5;
  border-radius: 6px;
  color: #4b5565;
  background: #f8fafc;
}
`;

function StatGrid({ metrics }) {
  const stats = [
    ['Blocks', metrics.blocks],
    ['Headings', metrics.headings],
    ['Tables', metrics.tables],
    ['Code fences', metrics.codeBlocks],
    ['Links', metrics.links],
    ['Images', metrics.images],
    ['Safe HTML', metrics.rawHtmlBlocks],
    ['Render ms', metrics.renderMs]
  ];

  return h('div', { id: 'markdown-stats', className: 'stats' }, stats.map(([label, value]) =>
    h('div', { key: label, className: 'stat' },
      h('strong', null, String(value)),
      h('span', null, label)
    )
  ));
}

function DocumentPreview({ result, blockWindowEnabled, renderedBlocks, totalBlocks, onShowMoreBlocks }) {
  return h('article', { id: 'markdown-preview', className: 'document-surface' },
    blockWindowEnabled
      ? h('div', { id: 'markdown-block-window-state', className: 'virtual-note' },
          `Rendering ${renderedBlocks} of ${totalBlocks} Markdown blocks; block-window mode renders only the visible source block window.`
        )
      : null,
    result.nodes,
    blockWindowEnabled && renderedBlocks < totalBlocks
      ? h('button', {
          id: 'markdown-show-more-blocks',
          className: 'btn',
          onClick: onShowMoreBlocks
        }, 'Show More Blocks')
      : null
  );
}

function ChatPreview({ messages, limit }) {
  const visible = messages.slice(Math.max(0, messages.length - limit));
  return h('div', { id: 'markdown-message-list', className: 'chat-list' },
    h('div', { id: 'markdown-virtualization-state', className: 'virtual-note' },
      `Rendering ${visible.length} of ${messages.length} messages; message-level virtualization keeps parser work bounded.`
    ),
    visible.map((message) => {
      const rendered = markdownRenderer.render(message.body, { fixtureIds: false });
      return h('section', { key: message.id, className: `message ${message.role}` },
        h('div', { className: 'message-head' },
          h('strong', null, message.role),
          h('span', null, message.id)
        ),
        h('div', { className: 'message-body' }, rendered.nodes)
      );
    })
  );
}

function App() {
  const [source, setSource] = useState(MAINSTREAM_MARKDOWN);
  const [editorVersion, setEditorVersion] = useState(0);
  const [view, setView] = useState('document');
  const [messages, setMessages] = useState(buildChatMessages);
  const [messageLimit, setMessageLimit] = useState(8);
  const [streamIndex, setStreamIndex] = useState(0);
  const [blockWindowEnabled, setBlockWindowEnabled] = useState(false);
  const [blockLimit, setBlockLimit] = useState(16);
  const pendingSourceRef = useRef(source);
  const sourceCommitTimerRef = useRef(null);

  useEffect(() => () => {
    if (sourceCommitTimerRef.current !== null) {
      clearTimeout(sourceCommitTimerRef.current);
    }
  }, []);

  const documentBlocks = useMemo(() => blockWindowEnabled ? splitMarkdownBlocks(source) : null, [source, blockWindowEnabled]);
  const visibleDocumentSource = useMemo(() => {
    if (!blockWindowEnabled) return source;
    return documentBlocks.slice(0, blockLimit).join('\n\n');
  }, [blockLimit, blockWindowEnabled, documentBlocks, source]);
  const documentResult = useMemo(() => markdownRenderer.render(visibleDocumentSource), [visibleDocumentSource]);
  const renderedBlockCount = blockWindowEnabled ? Math.min(blockLimit, documentBlocks.length) : documentResult.metrics.blocks;
  const totalBlockCount = blockWindowEnabled ? documentBlocks.length : documentResult.metrics.blocks;

  function clearPendingSourceCommit() {
    if (sourceCommitTimerRef.current !== null) {
      clearTimeout(sourceCommitTimerRef.current);
      sourceCommitTimerRef.current = null;
    }
  }

  function updateSourceCountLabel(value) {
    const label = document.getElementById('markdown-source-count');
    if (label) {
      label.textContent = `${value.length} chars`;
    }
  }

  function replaceEditorSource(nextSource) {
    clearPendingSourceCommit();
    pendingSourceRef.current = nextSource;
    setSource(nextSource);
    setEditorVersion((version) => version + 1);
  }

  function scheduleSourceCommit(nextSource) {
    pendingSourceRef.current = nextSource;
    updateSourceCountLabel(nextSource);
    clearPendingSourceCommit();
    sourceCommitTimerRef.current = setTimeout(() => {
      sourceCommitTimerRef.current = null;
      setSource(pendingSourceRef.current);
    }, PREVIEW_COMMIT_DELAY_MS);
  }

  function resetDocument() {
    replaceEditorSource(MAINSTREAM_MARKDOWN);
    setView('document');
    setBlockWindowEnabled(false);
    setBlockLimit(16);
  }

  function loadLongDocument() {
    replaceEditorSource(buildLongMarkdownDocument());
    setView('document');
    setBlockWindowEnabled(true);
    setBlockLimit(16);
  }

  function appendStreamChunk() {
    const chunk = STREAM_CHUNKS[streamIndex % STREAM_CHUNKS.length];
    setStreamIndex(streamIndex + 1);
    setView('chat');
    setMessages((current) => {
      const next = current.slice();
      const last = next[next.length - 1];
      next[next.length - 1] = {
        ...last,
        role: 'assistant',
        body: `${last.body}${chunk}`
      };
      return next;
    });
  }

  function scrollPreview(delta) {
    const target = document.getElementById('markdown-preview-scroll');
    if (!target) return;
    target.scrollTop = Math.max(0, target.scrollTop + delta);
  }

  return h('div', { id: 'markdown-preview-root', className: 'app-shell' },
    h('style', null, css),
    h('header', { className: 'topbar' },
      h('div', { className: 'title-block' },
        h('h1', null, 'Markdown Preview Compatibility'),
        h('span', null, 'DOM/CSS preview path with safe rendering and LLM chat pressure')
      ),
      h('div', { className: 'toolbar' },
        h('button', {
          id: 'document-mode',
          className: view === 'document' ? 'btn active' : 'btn',
          onClick: () => setView('document')
        }, 'Document'),
        h('button', {
          id: 'chat-mode',
          className: view === 'chat' ? 'btn active' : 'btn',
          onClick: () => setView('chat')
        }, 'Chat'),
        h('button', {
          id: 'markdown-stream-append',
          className: 'btn primary',
          onClick: appendStreamChunk
        }, 'Append Stream Chunk'),
        h('button', {
          id: 'markdown-load-long-doc',
          className: 'btn',
          onClick: loadLongDocument
        }, 'Long Doc'),
        h('button', {
          id: 'markdown-reset',
          className: 'btn',
          onClick: resetDocument
        }, 'Reset')
      )
    ),
    h('main', { className: 'workspace' },
      h('section', { className: 'source-pane' },
        h('div', { className: 'pane-head' },
          h('h2', null, 'Source'),
          h('small', { id: 'markdown-source-count' }, `${source.length} chars`)
        ),
        h('textarea', {
          key: `source-editor-${editorVersion}`,
          id: 'markdown-source',
          className: 'source-editor',
          defaultValue: source,
          spellCheck: false,
          onInput: (event) => scheduleSourceCommit(event.target.value)
        }),
        h(StatGrid, { metrics: documentResult.metrics })
      ),
      h('section', { className: 'preview-pane' },
        h('div', { className: 'pane-head' },
          h('div', { className: 'pane-title' },
            h('h2', null, view === 'document' ? 'Preview' : 'LLM Message Preview'),
            h('small', { id: 'markdown-stream-state' },
              view === 'chat'
                ? `${streamIndex} stream chunks, ${messageLimit} visible messages`
                : blockWindowEnabled
                  ? `${renderedBlockCount} of ${totalBlockCount} source blocks`
                  : 'mainstream Markdown fixture'
            )
          ),
          h('div', { className: 'pane-actions' },
            h('button', {
              id: 'markdown-scroll-up',
              className: 'btn small',
              onClick: () => scrollPreview(-360)
            }, 'Up'),
            h('button', {
              id: 'markdown-scroll-down',
              className: 'btn small',
              onClick: () => scrollPreview(360)
            }, 'Down'),
            h('button', {
              id: 'markdown-scroll-bottom',
              className: 'btn small',
              onClick: () => scrollPreview(2400)
            }, 'Bottom')
          )
        ),
        h('div', { id: 'markdown-preview-scroll', className: 'preview-scroll' },
          view === 'document'
            ? h(DocumentPreview, {
                result: documentResult,
                blockWindowEnabled,
                renderedBlocks: renderedBlockCount,
                totalBlocks: totalBlockCount,
                onShowMoreBlocks: () => setBlockLimit(Math.min(totalBlockCount, blockLimit + 16))
              })
            : h(ChatPreview, { messages, limit: messageLimit }),
          view === 'chat'
            ? h('div', { className: 'chat-list' },
                h('button', {
                  id: 'markdown-show-more-messages',
                  className: 'btn',
                  onClick: () => setMessageLimit(Math.min(messages.length, messageLimit + 8))
                }, 'Show More Messages')
              )
            : null
        )
      )
    )
  );
}

render(h(App), document.body);
