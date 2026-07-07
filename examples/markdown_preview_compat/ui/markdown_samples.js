export const SAMPLE_IMAGE = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII=';

export const MAINSTREAM_MARKDOWN = `# Mainstream Markdown Preview

This document exercises **bold text**, *emphasis*, \`inline code\`, and a [safe external link](https://example.com/mblink-markdown).

![Tiny generated preview swatch](${SAMPLE_IMAGE})

## Lists

- [x] GFM task item rendered as a disabled checkbox
- [ ] unchecked task item
- unordered item with **inline formatting**

1. Ordered item one
2. Ordered item two

> Blockquotes should remain readable and visibly separate from body text.

## Table

| Feature | Status | Notes |
| --- | --- | --- |
| Headings | Ready | H1-H6 map to DOM headings |
| Lists | Ready | Ordered, unordered, and task list shapes |
| Code fences | Ready | Preserves whitespace without executing text |
| Safe HTML | Escaped | Raw HTML is displayed as text |

## Code

\`\`\`js
export function renderMarkdown(input, adapter) {
  return adapter.render(input);
}
\`\`\`

## Safe HTML

<script>alert("not executed")</script>
<div onclick="bad()">Raw HTML is displayed as text.</div>
`;

export const STREAM_CHUNKS = [
  '\n\n### Streaming chunk 1\n\nThe assistant can append Markdown while the preview remains responsive.',
  '\n\n- streamed list item\n- another streamed list item with `inline code`',
  '\n\n| Chunk | Value |\n| --- | --- |\n| streamed table | rendered |'
];

export function buildChatMessages() {
  const messages = [
    {
      id: 'm001',
      role: 'user',
      body: 'Please summarize the Markdown compatibility plan.'
    },
    {
      id: 'm002',
      role: 'assistant',
      body: 'The P0 path should render **mainstream Markdown** with safe HTML handling and visual styling before editor-heavy features.'
    },
    {
      id: 'm003',
      role: 'user',
      body: 'Can this support LLM chat pages with long answers?'
    },
    {
      id: 'm004',
      role: 'assistant',
      body: 'Yes. Message-level virtualization is separate from the Markdown parser. The renderer handles each message body, while the chat list decides which messages are mounted.'
    }
  ];

  for (let i = 5; i <= 32; i += 1) {
    messages.push({
      id: `m${String(i).padStart(3, '0')}`,
      role: i % 3 === 0 ? 'user' : 'assistant',
      body: `Synthetic long-thread message ${i}. It includes **formatting**, a small list, and enough text to make virtualization visible.\n\n- message index ${i}\n- parser work stays per-message`
    });
  }

  return messages;
}

export function buildLongMarkdownDocument() {
  const sections = [MAINSTREAM_MARKDOWN, '\n\n## Long Document Window\n'];
  for (let i = 1; i <= 48; i += 1) {
    sections.push(`\n\n### Generated section ${i}\n\nThis block exists to prove that a very long Markdown document can be split into source block windows before rendering.\n\n- block index ${i}\n- windowed rendering keeps parser work and mounted DOM smaller\n`);
  }
  return sections.join('');
}
