# MBlink Native Elements

Read this reference when an MBlink UI needs a terminal panel, command output, an interactive shell, or a high-volume log viewer.

MBlink has two proprietary native-rendered DOM elements:

- `<terminal>`: ANSI terminal emulation, command output, and PTY-backed shell interaction.
- `<logview>`: structured log display with efficient append, filtering, search, and export.

These are MBlink runtime elements, not standard HTML elements and not Web Components. Use only the JavaScript methods documented here unless the bindings are extended and verified.

## General Rules

- Create them with Preact `h('terminal', ...)` / `h('logview', ...)` or `document.createElement('terminal')` / `document.createElement('logview')`.
- Attach stable `id` values so host bindings can find them with `mblink_terminal_get` or `mblink_logview_get`.
- Give them explicit width and height. They are native-rendered controls inside MBlink layout and should not be left with ambiguous or zero dimensions.
- Access methods through a DOM ref after mount. Guard method calls because the ref is null before mount.
- Validate with `snapshot`, `query_element`, `inspect`, `logs`, and `errors`; rendering is native, so do not expect browser devtools behavior.
- Prefer these native elements over npm/browser widgets for terminal and log-heavy UI.

## `<terminal>`

Use `<terminal>` for ANSI output, command execution, and an interactive shell.

Basic Preact usage:

```js
import { h, render } from 'preact';
import { useEffect, useRef } from 'preact/hooks';

function App() {
  const terminalRef = useRef(null);

  useEffect(() => {
    const term = terminalRef.current;
    if (!term) return;
    term.write('MBlink terminal ready\n');
    term.write('\x1b[32mgreen ANSI text\x1b[0m\n');
  }, []);

  return h('terminal', {
    id: 'main-terminal',
    ref: terminalRef,
    rows: 24,
    cols: 80,
    style: { width: '100%', height: '100%' }
  });
}

render(h(App), document.body);
```

Supported JavaScript methods:

| Method | Use |
|---|---|
| `write(data)` | Append text or ANSI terminal output. |
| `clear()` | Clear terminal buffer and parser state. |
| `scrollTo(line)` | Scroll to a buffer line. |
| `focus()` | Mark terminal focused for input handling. |
| `execute(command)` | Run a one-shot command and stream output into the terminal. |
| `startShell(shell?)` | Start an interactive shell. Empty shell uses the backend default; on Windows, examples use `cmd.exe`. |
| `sendInput(input)` | Send input to a running shell, usually including `\n` when submitting commands. |
| `resize(rows, cols)` | Resize terminal rows and columns and notify the PTY when running. |
| `serialize()` | Return plain text buffer content. |

Common caveats:

- Initial defaults are 24 rows, 80 columns, and 10000 scrollback lines.
- Prefer `resize(rows, cols)` after mount for runtime size changes; do not assume setting attributes after creation updates native rows and columns.
- `startShell` and `execute` depend on OS shell/PTTY support. Verify on the target platform.
- `sendInput` only has effect after a shell is running.
- The terminal understands common ANSI sequences, but do not assume full xterm compatibility.
- For host-language control, use the runtime bindings around `mblink_terminal_get`, `write`, `clear`, `execute`, `start_shell`, `send_input`, `resize`, and `serialize`.

## `<logview>`

Use `<logview>` for structured logs, filtering, search, selection, and export. It is designed for large log streams and should be preferred over manually appending thousands of DOM nodes.

Basic Preact usage:

```js
import { h, render } from 'preact';
import { useEffect, useRef } from 'preact/hooks';

function App() {
  const logRef = useRef(null);

  useEffect(() => {
    const log = logRef.current;
    if (!log) return;
    log.append('INFO', 'app', 'LogView ready');
    log.append('WARN', 'network', 'Retrying request');
    log.append('ERROR', 'api', 'Request failed');
  }, []);

  return h('logview', {
    id: 'main-log',
    ref: logRef,
    style: { width: '100%', height: '100%' }
  });
}

render(h(App), document.body);
```

Supported JavaScript methods:

| Method | Use |
|---|---|
| `append(level, source, message)` | Append one log entry. Levels should be `DEBUG`, `INFO`, `WARN`, `ERROR`, or `FATAL`. |
| `clear()` | Clear all logs, search state, selection, filters, and scroll offset. |
| `scrollTo(line)` | Scroll to a displayed line. |
| `search(query, useRegex?)` | Search logs and return match count. `useRegex` defaults to `false`. |
| `clearSearch()` | Clear active search highlighting. |
| `export(format?)` | Export logs. Use `text` or `json`; omitted format defaults to text. |
| `setLevelFilter(levels)` | Show only the provided levels. Pass an array such as `['ERROR', 'WARN']`. |
| `clearFilter()` | Clear level/source filters. |
| `nextMatch()` | Move to the next search match. |
| `prevMatch()` | Move to the previous search match. |

Common caveats:

- Current JavaScript bindings do not expose `setSourceFilter`, `scrollToTop`, `scrollToBottom`, `getMatchCount`, `getCurrentMatch`, `selectAll`, or direct display-option setters. Those exist in C++ internals but are not current JS API.
- `auto-scroll`, `max-entries`, `show-timestamp`, `show-level`, and `show-source` appear in lower-level docs as element concepts, but do not rely on post-creation attribute mutation unless it is verified in the UI you are building.
- For high-volume logs, append entries directly to `<logview>` instead of creating individual DOM rows.
- For host-language control, use runtime bindings around `mblink_logview_get`, `append`, `clear`, and `export`. Host bindings expose a smaller surface than the JS element.

## Choosing Between Them

- Use `<terminal>` when output contains ANSI sequences, shell interaction, command input, or scrollback terminal semantics.
- Use `<logview>` when data has structured `level`, `source`, and `message` fields and needs search/filter/export.
- Do not use `<terminal>` as a generic log table; log filtering and export belong in `<logview>`.
- Do not use `<logview>` for interactive shell input; use `<terminal>` with `startShell` and `sendInput`.

## Verification Checklist

1. Open the project with `mblink-ui-dev open`.
2. Confirm `snapshot` contains the `<terminal>` or `<logview>` node and that its rect has nonzero width and height.
3. Use `query_element` or `inspect` on its stable selector.
4. Trigger one method call, such as `write` or `append`, from source code or temporary `eval_js`.
5. Check `logs` and `errors` for missing method or runtime binding failures.
