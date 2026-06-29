from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"Binding readiness check failed: {message}")


def first_line(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="strict").splitlines()[0]


def check_go_build_tags() -> None:
    for path in sorted((ROOT / "bindings/go").rglob("*_windows.go")):
        line = first_line(path)
        if line != "//go:build windows && cgo":
            fail(f"{path.relative_to(ROOT)} must use '//go:build windows && cgo'")

    for path in sorted((ROOT / "bindings/go/examples").rglob("*.go")):
        line = first_line(path)
        if line != "//go:build windows && cgo":
            fail(f"{path.relative_to(ROOT)} must use '//go:build windows && cgo'")

    unsupported = ROOT / "bindings/go/mblink/unsupported.go"
    if first_line(unsupported) != "//go:build !windows || !cgo":
        fail("bindings/go/mblink/unsupported.go must cover non-Windows and no-cgo builds")


def check_binding_docs() -> None:
    bindings_doc = (ROOT / "docs/BINDINGS.md").read_text(encoding="utf-8")
    required = [
        "Python | Supported host binding",
        "Rust | Supported host binding",
        "Go | Supported host binding on Windows with cgo",
        "CGO_ENABLED=1",
    ]
    for text in required:
        if text not in bindings_doc:
            fail(f"docs/BINDINGS.md is missing required binding status text: {text}")


def main() -> None:
    check_go_build_tags()
    check_binding_docs()
    print("Binding readiness metadata OK.")


if __name__ == "__main__":
    main()
