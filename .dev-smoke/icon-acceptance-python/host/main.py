import json
import os
import sys
from pathlib import Path


EMBEDDED_CONFIG = {
    "name": "MBink",
    "purpose": "minimal",
    "runtime": "python",
    "window": {"title": "MBink", "width": 900, "height": 640, "resizable": True},
}
RESOURCE_CONFIG_PATH = "app/mbink.config.json"
RESOURCE_APP_PATH = "/app/app.js"


def app_base_dir():
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parents[1]


def pyinstaller_base_dir():
    return Path(getattr(sys, "_MEIPASS", app_base_dir()))


PROJECT_ROOT = app_base_dir()
LOCAL_BINDING_ROOT = PROJECT_ROOT / "vendor"
LOCAL_DLL_DIR = LOCAL_BINDING_ROOT / "mbink" / "bin"
FROZEN_ROOT = pyinstaller_base_dir()
FROZEN_DLL_DIR = FROZEN_ROOT / "mbink" / "bin"

if LOCAL_BINDING_ROOT.exists():
    sys.path.insert(0, str(LOCAL_BINDING_ROOT))
    if (LOCAL_DLL_DIR / "mbink.dll").exists():
        os.environ.setdefault("MBINK_DLL_PATH", str(LOCAL_DLL_DIR))
if getattr(sys, "frozen", False):
    sys.path.insert(0, str(FROZEN_ROOT))
    if (FROZEN_DLL_DIR / "mbink.dll").exists():
        os.environ.setdefault("MBINK_DLL_PATH", str(FROZEN_DLL_DIR))

try:
    from mbink import App
    from mbink import load_resource_file
except ImportError as exc:
    raise SystemExit(
        "Cannot import the MBink Python binding. Run mbink-ui-dev build so the local binding is packaged."
    ) from exc


def executable_stem():
    return Path(sys.executable if getattr(sys, "frozen", False) else __file__).stem or "app"


def resource_package_candidates():
    base = pyinstaller_base_dir()
    return [
        PROJECT_ROOT / "app.mbrp",
        PROJECT_ROOT / "resources" / "app.mbrp",
        base / "app.mbrp",
        base / "resources" / "app.mbrp",
        PROJECT_ROOT / ".dist" / "app.mbrp",
        PROJECT_ROOT / "host" / "resources" / "app.mbrp",
    ]


def resolve_resource_package():
    for candidate in resource_package_candidates():
        if candidate.exists():
            return candidate
    return None


def load_config(resource_package):
    config_path = PROJECT_ROOT / "mbink.config.json"
    if config_path.exists():
        return json.loads(config_path.read_text(encoding="utf-8"))
    if resource_package:
        try:
            data, _flags = load_resource_file(str(resource_package), RESOURCE_CONFIG_PATH)
            return json.loads(data.decode("utf-8"))
        except Exception:
            pass
    return dict(EMBEDDED_CONFIG)


def clean_text(value):
    return str(value or "").strip()


def main():
    resource_package = resolve_resource_package()
    config = load_config(resource_package)
    window = config.get("window", {})
    purpose = config.get("purpose", "minimal")
    runtime = config.get("runtime", "python")
    title = window.get("title") or config.get("name") or "MBink"
    width = int(window.get("width", 900))
    height = int(window.get("height", 640))
    borderless = bool(window.get("borderless", purpose == "desktop-app"))
    resizable = bool(window.get("resizable", True))

    app = App(title, width, height, borderless=borderless, resizable=resizable)
    counter = {"value": 0}

    @app.bind("getTemplateInfo")
    def _get_template_info(args):
        return {
            "purpose": purpose,
            "runtime": runtime,
            "host": "Python",
            "mode": "host-runtime",
            "capabilities": {
                "backend": True,
                "borderless": borderless,
                "tray": purpose == "desktop-app",
                "reload": False,
                "snapshot": False,
            },
        }

    @app.bind("incrementCounter")
    def _increment_counter(args):
        counter["value"] += int((args or {}).get("delta", 1))
        return {"count": counter["value"], "source": "python"}

    @app.bind("submitValidation")
    def _submit_validation(args):
        value = clean_text((args or {}).get("text"))
        return {
            "ok": True,
            "value": value,
            "message": f"Python host accepted: {value}" if value else "Python host accepted an empty value",
        }

    @app.bind("trayAction")
    def _tray_action(args):
        action = (args or {}).get("action", "status")
        if action == "hide":
            app.hide_to_tray()
        elif action == "show":
            app.show_main_window()
        elif action == "quit":
            app.stop()
        return {"ok": True, "action": action, "message": f"Python tray action: {action}"}

    if purpose == "desktop-app":
        app.create_tray(
            tooltip=title,
            menu=[
                {"id": "show", "label": "Show window"},
                {"id": "hide", "label": "Hide to tray"},
                {"type": "separator"},
                {"id": "quit", "label": "Quit"},
            ],
        )

        @app.on_tray_click
        def _tray_click():
            app.show_main_window()

        @app.tray_action("show")
        def _tray_show():
            app.show_main_window()

        @app.tray_action("hide")
        def _tray_hide():
            app.hide_to_tray()

        @app.tray_action("quit")
        def _tray_quit():
            app.stop()

        @app.on_close_request
        def _close_request():
            app.hide_to_tray()
            return True

    app.load_html(
        """<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>"""
    )
    if resource_package:
        app.mount_resource_package(str(resource_package), "", "/")
        app.load_js_file(RESOURCE_APP_PATH)
    else:
        app.load_js_file(str(PROJECT_ROOT / "ui" / "app.js"))
    app.run()


if __name__ == "__main__":
    main()
