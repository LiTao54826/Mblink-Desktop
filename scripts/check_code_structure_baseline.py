#!/usr/bin/env python3
"""Fail when new oversized source-file debt is introduced.

The legacy `check_code_structure.py` report is still useful, but the current
tree already has known source-size debt. This wrapper makes CI enforce the part
that is safe for public contributions today: no additional source files over the
existing 2000-line threshold outside the recorded baseline.
"""

import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = ROOT / "scripts" / "code_structure_baseline.json"


def count_lines(path: Path) -> int:
    return len(path.read_text(encoding="utf-8", errors="ignore").splitlines())


def load_baseline() -> dict:
    try:
        return json.loads(BASELINE_PATH.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"Missing baseline manifest: {BASELINE_PATH}", file=sys.stderr)
        raise


def main() -> int:
    baseline = load_baseline()
    max_source_lines = int(baseline["maxSourceLines"])
    root_dirs = [ROOT / item for item in baseline["rootDirs"]]
    source_extensions = {item.lower() for item in baseline["sourceExtensions"]}
    allowed_sources = set(baseline["oversizedSources"])

    offenders = []
    baseline_seen = []

    for root_dir in root_dirs:
        if not root_dir.exists():
            print(f"Missing scan directory: {root_dir}", file=sys.stderr)
            return 1

        for path in sorted(root_dir.rglob("*")):
            if path.suffix.lower() not in source_extensions:
                continue

            line_count = count_lines(path)
            if line_count <= max_source_lines:
                continue

            rel = path.relative_to(ROOT).as_posix()
            if rel in allowed_sources:
                baseline_seen.append((rel, line_count))
            else:
                offenders.append((rel, line_count))

    print("Code structure baseline check")
    print(f"Allowed existing oversized sources: {len(allowed_sources)}")
    print(f"Current baseline oversized sources found: {len(baseline_seen)}")

    stale_entries = allowed_sources - {rel for rel, _line_count in baseline_seen}
    if stale_entries:
        print("\nBaseline entries no longer match oversized source files:", file=sys.stderr)
        for rel in sorted(stale_entries):
            print(f"  - {rel}", file=sys.stderr)
        print(
            "\nUpdate scripts/code_structure_baseline.json with reviewer approval.",
            file=sys.stderr,
        )
        return 1

    if offenders:
        print("\nNew oversized source files detected:", file=sys.stderr)
        for rel, line_count in offenders:
            print(f"  - {rel} ({line_count} lines)", file=sys.stderr)
        print(
            "\nRefactor the file or explicitly update the baseline with reviewer approval.",
            file=sys.stderr,
        )
        return 1

    print("No new oversized source-file debt detected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
