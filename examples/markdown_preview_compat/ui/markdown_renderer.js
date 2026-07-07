const kAllowedHref = /^(https?:|mailto:|#)/i;
const kAllowedImageSrc = /^(https?:|data:image\/(?:png|jpe?g|gif|webp);base64,|\.\/|\.\.\/|assets\/)/i;

function blankMetrics(source) {
  return {
    sourceChars: source.length,
    blocks: 0,
    headings: 0,
    lists: 0,
    taskItems: 0,
    codeBlocks: 0,
    tables: 0,
    links: 0,
    images: 0,
    rawHtmlBlocks: 0,
    renderMs: 0
  };
}

function normalizeSource(source) {
  return String(source || '').replace(/\r\n/g, '\n').replace(/\r/g, '\n');
}

function splitTableRow(line) {
  let value = line.trim();
  if (value.startsWith('|')) value = value.slice(1);
  if (value.endsWith('|')) value = value.slice(0, -1);
  return value.split('|').map((cell) => cell.trim());
}

function isTableSeparator(line) {
  const cells = splitTableRow(line);
  return cells.length > 1 && cells.every((cell) => /^:?-{3,}:?$/.test(cell));
}

function startsBlock(line, nextLine) {
  const trimmed = line.trim();
  return trimmed.length === 0 ||
    /^#{1,6}\s+/.test(trimmed) ||
    /^(```|~~~)/.test(trimmed) ||
    /^-{3,}$/.test(trimmed) ||
    /^>\s?/.test(trimmed) ||
    /^[-*+]\s+/.test(trimmed) ||
    /^\d+\.\s+/.test(trimmed) ||
    (trimmed.indexOf('|') !== -1 && nextLine && isTableSeparator(nextLine)) ||
    (trimmed.startsWith('<') && trimmed.indexOf('>') !== -1);
}

function safeHref(value) {
  const href = String(value || '').trim();
  return kAllowedHref.test(href) ? href : '#';
}

function safeImageSrc(value) {
  const src = String(value || '').trim();
  return kAllowedImageSrc.test(src) ? src : '';
}

function findClosing(text, start, marker) {
  const index = text.indexOf(marker, start);
  return index === -1 ? null : index;
}

export function splitMarkdownBlocks(source) {
  const lines = normalizeSource(source).split('\n');
  const blocks = [];
  let i = 0;

  function pushBlock(start, end) {
    const value = lines.slice(start, end).join('\n').trim();
    if (value) blocks.push(value);
  }

  while (i < lines.length) {
    const start = i;
    const trimmed = lines[i].trim();

    if (!trimmed) {
      i += 1;
      continue;
    }

    const fence = trimmed.startsWith('```') ? '```' : trimmed.startsWith('~~~') ? '~~~' : null;
    if (fence) {
      i += 1;
      while (i < lines.length && !lines[i].trim().startsWith(fence)) i += 1;
      if (i < lines.length) i += 1;
      pushBlock(start, i);
      continue;
    }

    if (/^(#{1,6})\s+/.test(trimmed) || /^-{3,}$/.test(trimmed)) {
      i += 1;
      pushBlock(start, i);
      continue;
    }

    if (trimmed.startsWith('>')) {
      while (i < lines.length && lines[i].trim().startsWith('>')) i += 1;
      pushBlock(start, i);
      continue;
    }

    if (/^[-*+]\s+/.test(trimmed)) {
      while (i < lines.length && /^[-*+]\s+/.test(lines[i].trim())) i += 1;
      pushBlock(start, i);
      continue;
    }

    if (/^\d+\.\s+/.test(trimmed)) {
      while (i < lines.length && /^\d+\.\s+/.test(lines[i].trim())) i += 1;
      pushBlock(start, i);
      continue;
    }

    if (trimmed.indexOf('|') !== -1 && i + 1 < lines.length && isTableSeparator(lines[i + 1])) {
      i += 2;
      while (i < lines.length && lines[i].trim() && lines[i].indexOf('|') !== -1) i += 1;
      pushBlock(start, i);
      continue;
    }

    if (trimmed.startsWith('<') && trimmed.indexOf('>') !== -1) {
      while (i < lines.length && lines[i].trim().startsWith('<')) i += 1;
      pushBlock(start, i);
      continue;
    }

    i += 1;
    while (i < lines.length && !startsBlock(lines[i], lines[i + 1])) i += 1;
    pushBlock(start, i);
  }

  return blocks;
}

function makeInlineParser(h, metrics, fixtureState) {
  function parseInline(text, keyPrefix) {
    const nodes = [];
    let plain = '';
    let i = 0;

    function flushPlain() {
      if (plain) {
        nodes.push(plain);
        plain = '';
      }
    }

    while (i < text.length) {
      if (text.startsWith('![', i)) {
        const labelEnd = text.indexOf(']', i + 2);
        const targetStart = labelEnd !== -1 ? text.indexOf('(', labelEnd) : -1;
        const targetEnd = targetStart !== -1 ? text.indexOf(')', targetStart) : -1;
        if (labelEnd !== -1 && targetStart === labelEnd + 1 && targetEnd !== -1) {
          flushPlain();
          const alt = text.slice(i + 2, labelEnd);
          const src = safeImageSrc(text.slice(targetStart + 1, targetEnd));
          metrics.images += 1;
          const props = {
            key: `${keyPrefix}-img-${i}`,
            alt,
            src,
            className: 'md-image'
          };
          if (fixtureState.useIds && fixtureState.imageIdAvailable) {
            props.id = 'markdown-image-fixture';
            fixtureState.imageIdAvailable = false;
          }
          nodes.push(h('img', props));
          i = targetEnd + 1;
          continue;
        }
      }

      if (text.startsWith('[', i)) {
        const labelEnd = text.indexOf(']', i + 1);
        const targetStart = labelEnd !== -1 ? text.indexOf('(', labelEnd) : -1;
        const targetEnd = targetStart !== -1 ? text.indexOf(')', targetStart) : -1;
        if (labelEnd !== -1 && targetStart === labelEnd + 1 && targetEnd !== -1) {
          flushPlain();
          const label = text.slice(i + 1, labelEnd);
          const href = safeHref(text.slice(targetStart + 1, targetEnd));
          metrics.links += 1;
          const props = {
            key: `${keyPrefix}-link-${i}`,
            href,
            className: 'md-link'
          };
          if (fixtureState.useIds && fixtureState.linkIdAvailable) {
            props.id = 'markdown-link-fixture';
            fixtureState.linkIdAvailable = false;
          }
          nodes.push(h('a', props, parseInline(label, `${keyPrefix}-link-${i}`)));
          i = targetEnd + 1;
          continue;
        }
      }

      if (text.startsWith('**', i)) {
        const end = findClosing(text, i + 2, '**');
        if (end !== null) {
          flushPlain();
          nodes.push(h('strong', { key: `${keyPrefix}-strong-${i}` }, parseInline(text.slice(i + 2, end), `${keyPrefix}-strong-${i}`)));
          i = end + 2;
          continue;
        }
      }

      if (text[i] === '`') {
        const end = findClosing(text, i + 1, '`');
        if (end !== null) {
          flushPlain();
          nodes.push(h('code', { key: `${keyPrefix}-code-${i}`, className: 'md-inline-code' }, text.slice(i + 1, end)));
          i = end + 1;
          continue;
        }
      }

      if (text[i] === '*') {
        const end = findClosing(text, i + 1, '*');
        if (end !== null) {
          flushPlain();
          nodes.push(h('em', { key: `${keyPrefix}-em-${i}` }, parseInline(text.slice(i + 1, end), `${keyPrefix}-em-${i}`)));
          i = end + 1;
          continue;
        }
      }

      plain += text[i];
      i += 1;
    }

    flushPlain();
    return nodes;
  }

  return parseInline;
}

export function createMarkdownRenderer({ h }) {
  return {
    render(source, options = {}) {
      const started = typeof performance !== 'undefined' && performance.now ? performance.now() : Date.now();
      const markdown = normalizeSource(source);
      const lines = markdown.split('\n');
      const metrics = blankMetrics(markdown);
      const fixtureState = {
        useIds: options.fixtureIds !== false,
        headingIdAvailable: true,
        codeIdAvailable: true,
        tableIdAvailable: true,
        htmlIdAvailable: true,
        imageIdAvailable: true,
        linkIdAvailable: true
      };
      const inline = makeInlineParser(h, metrics, fixtureState);
      const nodes = [];
      let i = 0;
      let blockIndex = 0;

      function push(node) {
        metrics.blocks += 1;
        nodes.push(node);
        blockIndex += 1;
      }

      while (i < lines.length) {
        const line = lines[i];
        const trimmed = line.trim();

        if (!trimmed) {
          i += 1;
          continue;
        }

        const fence = trimmed.startsWith('```') ? '```' : trimmed.startsWith('~~~') ? '~~~' : null;
        if (fence) {
          const language = trimmed.slice(3).trim() || 'text';
          const code = [];
          i += 1;
          while (i < lines.length && !lines[i].trim().startsWith(fence)) {
            code.push(lines[i]);
            i += 1;
          }
          if (i < lines.length) i += 1;
          metrics.codeBlocks += 1;
          const props = { key: `block-${blockIndex}`, className: 'md-code-block' };
          if (fixtureState.useIds && fixtureState.codeIdAvailable) {
            props.id = 'markdown-code-fixture';
            fixtureState.codeIdAvailable = false;
          }
          push(h('pre', props, h('code', { 'data-language': language }, code.join('\n'))));
          continue;
        }

        const heading = /^(#{1,6})\s+(.+)$/.exec(trimmed);
        if (heading) {
          const level = heading[1].length;
          metrics.headings += 1;
          const props = { key: `block-${blockIndex}`, className: `md-heading md-h${level}` };
          if (fixtureState.useIds && fixtureState.headingIdAvailable && level === 1) {
            props.id = 'markdown-heading-fixture';
            fixtureState.headingIdAvailable = false;
          }
          push(h(`h${level}`, props, inline(heading[2], `heading-${blockIndex}`)));
          i += 1;
          continue;
        }

        if (/^-{3,}$/.test(trimmed)) {
          push(h('hr', { key: `block-${blockIndex}`, className: 'md-rule' }));
          i += 1;
          continue;
        }

        if (trimmed.startsWith('>')) {
          const quoteLines = [];
          while (i < lines.length && lines[i].trim().startsWith('>')) {
            quoteLines.push(lines[i].trim().replace(/^>\s?/, ''));
            i += 1;
          }
          push(h('blockquote', { key: `block-${blockIndex}`, className: 'md-blockquote' }, quoteLines.map((value, index) =>
            h('p', { key: `quote-${index}` }, inline(value, `quote-${blockIndex}-${index}`))
          )));
          continue;
        }

        if (/^[-*+]\s+/.test(trimmed)) {
          const items = [];
          while (i < lines.length && /^[-*+]\s+/.test(lines[i].trim())) {
            const itemText = lines[i].trim().replace(/^[-*+]\s+/, '');
            const task = /^\[( |x|X)\]\s+/.exec(itemText);
            if (task) {
              metrics.taskItems += 1;
              const checked = task[1].toLowerCase() === 'x';
              const cleanText = itemText.replace(/^\[( |x|X)\]\s+/, '');
              items.push(h('li', { key: `li-${items.length}`, className: checked ? 'md-task is-done' : 'md-task' },
                h('input', { type: 'checkbox', checked, disabled: true, className: 'md-task-check' }),
                h('span', null, inline(cleanText, `ul-${blockIndex}-${items.length}`))
              ));
            } else {
              items.push(h('li', { key: `li-${items.length}` }, inline(itemText, `ul-${blockIndex}-${items.length}`)));
            }
            i += 1;
          }
          metrics.lists += 1;
          push(h('ul', { key: `block-${blockIndex}`, className: 'md-list' }, items));
          continue;
        }

        if (/^\d+\.\s+/.test(trimmed)) {
          const items = [];
          while (i < lines.length && /^\d+\.\s+/.test(lines[i].trim())) {
            const itemText = lines[i].trim().replace(/^\d+\.\s+/, '');
            items.push(h('li', { key: `li-${items.length}` }, inline(itemText, `ol-${blockIndex}-${items.length}`)));
            i += 1;
          }
          metrics.lists += 1;
          push(h('ol', { key: `block-${blockIndex}`, className: 'md-list' }, items));
          continue;
        }

        if (trimmed.indexOf('|') !== -1 && i + 1 < lines.length && isTableSeparator(lines[i + 1])) {
          const headers = splitTableRow(lines[i]);
          i += 2;
          const rows = [];
          while (i < lines.length && lines[i].trim() && lines[i].indexOf('|') !== -1) {
            rows.push(splitTableRow(lines[i]));
            i += 1;
          }
          metrics.tables += 1;
          const props = { key: `block-${blockIndex}`, className: 'md-table' };
          if (fixtureState.useIds && fixtureState.tableIdAvailable) {
            props.id = 'markdown-table-fixture';
            fixtureState.tableIdAvailable = false;
          }
          push(h('table', props,
            h('thead', null, h('tr', null, headers.map((header, index) =>
              h('th', { key: `th-${index}` }, inline(header, `th-${blockIndex}-${index}`))
            ))),
            h('tbody', null, rows.map((row, rowIndex) =>
              h('tr', { key: `tr-${rowIndex}` }, headers.map((_, cellIndex) =>
                h('td', { key: `td-${cellIndex}` }, inline(row[cellIndex] || '', `td-${blockIndex}-${rowIndex}-${cellIndex}`))
              ))
            ))
          ));
          continue;
        }

        if (trimmed.startsWith('<') && trimmed.indexOf('>') !== -1) {
          const raw = [];
          while (i < lines.length && lines[i].trim().startsWith('<')) {
            raw.push(lines[i]);
            i += 1;
          }
          metrics.rawHtmlBlocks += 1;
          const props = { key: `block-${blockIndex}`, className: 'md-html-escaped' };
          if (fixtureState.useIds && fixtureState.htmlIdAvailable) {
            props.id = 'markdown-safe-html-fixture';
            fixtureState.htmlIdAvailable = false;
          }
          push(h('pre', props, raw.join('\n')));
          continue;
        }

        const paragraph = [trimmed];
        i += 1;
        while (i < lines.length && !startsBlock(lines[i], lines[i + 1])) {
          paragraph.push(lines[i].trim());
          i += 1;
        }
        push(h('p', { key: `block-${blockIndex}`, className: 'md-paragraph' }, inline(paragraph.join(' '), `p-${blockIndex}`)));
      }

      const ended = typeof performance !== 'undefined' && performance.now ? performance.now() : Date.now();
      metrics.renderMs = Math.max(0, Math.round((ended - started) * 10) / 10);
      return { nodes, metrics };
    }
  };
}
