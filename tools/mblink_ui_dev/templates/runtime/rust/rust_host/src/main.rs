use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::{
    atomic::{AtomicBool, Ordering},
    Arc, Mutex,
};

use mblink::App;
use serde_json::{json, Value};

const EMBEDDED_CONFIG: &str = include_str!("../../mblink.config.json");
const RESOURCE_APP_PATH: &str = "/app/app.js";

include!(concat!(env!("OUT_DIR"), "/embedded_resources.rs"));

fn main() -> mblink::Result<()> {
    let root = project_root();
    let config = load_config(&root);
    let purpose = string_at(&config, "/purpose", "minimal");
    let runtime = string_at(&config, "/runtime", "rust");
    let title = first_non_empty(&[
        string_at(&config, "/window/title", ""),
        string_at(&config, "/name", ""),
        "MBlink".to_string(),
    ]);
    let width = int_at(&config, "/window/width", 900);
    let height = int_at(&config, "/window/height", 640);
    let borderless = bool_at(&config, "/window/borderless", purpose == "desktop-app");
    let resizable = bool_at(&config, "/window/resizable", true);

    let mut app = App::builder()
        .title(title.clone())
        .size(width, height)
        .borderless(borderless)
        .resizable(resizable)
        .build()?;

    let counter = Arc::new(Mutex::new(0_i64));
    let quitting = Arc::new(AtomicBool::new(false));
    let topmost = Arc::new(AtomicBool::new(false));
    let handle = app.handle();

    {
        let purpose = purpose.clone();
        let runtime = runtime.clone();
        app.bind("getTemplateInfo", move |_args| {
            Ok(json!({
                "purpose": purpose.clone(),
                "runtime": runtime.clone(),
                "host": "Rust",
                "mode": "host-runtime",
                "capabilities": {
                    "backend": true,
                    "borderless": borderless,
                    "tray": purpose == "desktop-app",
                    "reload": false,
                    "snapshot": false
                }
            }))
        })?;
    }

    {
        let counter = Arc::clone(&counter);
        app.bind("incrementCounter", move |args| {
            let mut value = counter.lock().expect("counter lock poisoned");
            *value += int_arg(&args, "delta", 1) as i64;
            Ok(json!({ "count": *value, "source": "rust" }))
        })?;
    }

    app.bind("submitValidation", move |args| {
        let value = string_arg(&args, "text");
        let message = if value.is_empty() {
            "Rust host accepted an empty value".to_string()
        } else {
            format!("Rust host accepted: {value}")
        };
        Ok(json!({ "ok": true, "value": value, "message": message }))
    })?;

    {
        let quitting = Arc::clone(&quitting);
        app.bind("trayAction", move |args| {
            let action = first_non_empty(&[string_arg(&args, "action"), "status".to_string()]);
            match action.as_str() {
                "hide" => {
                    handle.hide_to_tray()?;
                }
                "show" => {
                    handle.show_main_window()?;
                }
                "quit" => {
                    quitting.store(true, Ordering::SeqCst);
                    handle.stop();
                }
                _ => {}
            }
            Ok(json!({ "ok": true, "action": action, "message": format!("Rust tray action: {action}") }))
        })?;
    }

    app.load_html(
        r#"<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <style>
    html, body, #root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    * { box-sizing: border-box; }
  </style>
</head>
<body><div id="root"></div></body>
</html>"#,
    )?;

    if purpose == "desktop-app" {
        app.create_tray(&title)?;
        app.set_tray_menu(&json!([
            { "id": "show", "label": "Show window" },
            { "id": "hide", "label": "Hide to tray" },
            { "type": "separator" },
            { "id": "toggle_top", "label": "Toggle always on top" },
            { "id": "quit", "label": "Quit" }
        ]))?;

        {
            let quitting = Arc::clone(&quitting);
            app.on_close_request(move || {
                if quitting.load(Ordering::SeqCst) {
                    true
                } else {
                    let _ = handle.hide_to_tray();
                    false
                }
            })?;
        }

        app.on_tray_click(move || {
            let _ = handle.show_main_window();
        })?;

        {
            let quitting = Arc::clone(&quitting);
            let topmost = Arc::clone(&topmost);
            app.on_tray_menu(move |payload| {
                let item_id = string_arg(&payload, "id");
                match item_id.as_str() {
                    "show" => {
                        handle.show_main_window()?;
                    }
                    "hide" => {
                        handle.hide_to_tray()?;
                    }
                    "toggle_top" => {
                        let next = !topmost.load(Ordering::SeqCst);
                        handle.set_always_on_top(next)?;
                        topmost.store(next, Ordering::SeqCst);
                    }
                    "quit" => {
                        quitting.store(true, Ordering::SeqCst);
                        handle.stop();
                    }
                    _ => {}
                }
                Ok(json!({ "ok": true, "id": item_id }))
            })?;
        }
    }

    if let Some(resource_package) = resolve_resource_package(&root) {
        app.mount_resource_package(&path_string(&resource_package), "", "/")?;
        app.load_js_file(RESOURCE_APP_PATH)?;
    } else {
        app.load_js_file(&path_string(&root.join("ui").join("app.js")))?;
    }
    app.run();
    Ok(())
}

fn project_root() -> PathBuf {
    find_project_root_near_exe().unwrap_or_else(exe_dir)
}

fn find_project_root_near_exe() -> Option<PathBuf> {
    let exe_path = env::current_exe().ok()?;
    let mut current = exe_path.parent()?.to_path_buf();
    loop {
        if current.join("mblink.config.json").exists() || current.join(".dist").join("app.mbrp").exists() {
            return Some(current);
        }
        if !current.pop() {
            return None;
        }
    }
}

fn exe_dir() -> PathBuf {
    env::current_exe()
        .ok()
        .and_then(|path| path.parent().map(Path::to_path_buf))
        .unwrap_or_else(|| env::current_dir().unwrap_or_else(|_| PathBuf::from(".")))
}

fn executable_stem() -> String {
    env::current_exe()
        .ok()
        .and_then(|path| path.file_stem().map(|value| value.to_string_lossy().into_owned()))
        .filter(|value| !value.is_empty())
        .unwrap_or_else(|| "app".to_string())
}

fn resolve_resource_package(root: &Path) -> Option<PathBuf> {
    for candidate in resource_package_candidates(root) {
        if candidate.exists() {
            return Some(candidate);
        }
    }
    if let Some(bytes) = EMBEDDED_RESOURCE_PACKAGE {
        let path = env::temp_dir()
            .join("mblink-template-rust")
            .join(executable_stem())
            .join("app.mbrp");
        write_embedded_file(&path, bytes);
        return Some(path);
    }
    None
}

fn resource_package_candidates(root: &Path) -> Vec<PathBuf> {
    let exe_dir = exe_dir();
    vec![
        exe_dir.join("app.mbrp"),
        exe_dir.join("resources").join("app.mbrp"),
        root.join(".dist").join("app.mbrp"),
        root.join("rust_host").join("resources").join("app.mbrp"),
    ]
}

fn write_embedded_file(path: &Path, data: &[u8]) {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).expect("failed to create embedded runtime directory");
    }
    if fs::read(path).ok().as_deref() == Some(data) {
        return;
    }
    fs::write(path, data).expect("failed to write embedded runtime file");
}

fn load_config(root: &Path) -> Value {
    let content = fs::read_to_string(root.join("mblink.config.json"))
        .unwrap_or_else(|_| EMBEDDED_CONFIG.to_string());
    serde_json::from_str(&content).expect("invalid mblink.config.json")
}

fn string_at(value: &Value, pointer: &str, fallback: &str) -> String {
    value
        .pointer(pointer)
        .and_then(Value::as_str)
        .filter(|v| !v.is_empty())
        .unwrap_or(fallback)
        .to_string()
}

fn int_at(value: &Value, pointer: &str, fallback: i32) -> i32 {
    value
        .pointer(pointer)
        .and_then(Value::as_i64)
        .map(|v| v as i32)
        .unwrap_or(fallback)
}

fn bool_at(value: &Value, pointer: &str, fallback: bool) -> bool {
    value
        .pointer(pointer)
        .and_then(Value::as_bool)
        .unwrap_or(fallback)
}

fn string_arg(value: &Value, key: &str) -> String {
    value
        .get(key)
        .and_then(Value::as_str)
        .unwrap_or("")
        .trim()
        .to_string()
}

fn int_arg(value: &Value, key: &str, fallback: i32) -> i32 {
    value
        .get(key)
        .and_then(Value::as_i64)
        .map(|v| v as i32)
        .unwrap_or(fallback)
}

fn first_non_empty(values: &[String]) -> String {
    values
        .iter()
        .find(|value| !value.is_empty())
        .cloned()
        .unwrap_or_default()
}

fn path_string(path: &Path) -> String {
    path.to_string_lossy().into_owned()
}
