# Contributor Automation Notes

This repository supports AI-assisted development, but the public guidance is intentionally small.

## Start here

- Read [README.md](README.md)
- Read [docs/QUICKSTART.md](docs/QUICKSTART.md)
- Read [docs/AI_WORKFLOW.md](docs/AI_WORKFLOW.md)

## Canonical AI workflow surface

For MBink-specific project work, the canonical tool/skill surface is:

- [tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md](tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md)

That document is the best source for:

- `mbink-ui-dev` CLI/MCP usage
- snapshot-first verification
- UI-first then host-integration workflow
- runtime parity expectations across `esm_loader`, Python, Rust, and Go

## GitNexus note

If you are changing runtime symbols rather than docs/support files, use the GitNexus guidance embedded by project tooling and keep impact analysis and change detection in the loop before committing.

For docs-only cleanup, that heavy symbol-level workflow is usually unnecessary.
