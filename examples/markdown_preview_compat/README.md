# Markdown Preview Compatibility

This example is the first verified app-level Markdown preview workflow for
MBlink. It focuses on a complete preview surface instead of a parser-only demo:

- source editing and live DOM/CSS preview
- headings, paragraphs, emphasis, inline code, links, images, lists, task lists,
  blockquotes, tables, code fences, and raw HTML displayed as text
- an independent renderer adapter shape so a mature Markdown core can replace
  the local fixture without making Preact the compatibility contract
- LLM chat pressure with message-level virtualization and streaming append
  controls
- a long-document block window that renders visible Markdown source blocks first
  and expands the mounted block range incrementally for very long documents

The included renderer is intentionally compact and repo-local so the example can
be built and verified without network package installation. It is compatibility
evidence for the MBlink preview workflow, not a claim that MBlink has a built-in
Markdown runtime parser.

## Verify

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File examples\markdown_preview_compat\verify.ps1
```

The script builds the project, opens it with `mblink-ui-dev`, captures a
file-mode snapshot plus screenshot, queries the important preview selectors,
edits the source textarea, exercises the chat/streaming controls, and checks
logs/errors.

Stable selectors include:

- `#markdown-preview-root`
- `#markdown-source`
- `#markdown-preview`
- `#markdown-table-fixture`
- `#markdown-code-fixture`
- `#markdown-safe-html-fixture`
- `#markdown-message-list`
- `#markdown-stream-append`
- `#markdown-load-long-doc`
- `#markdown-block-window-state`
