use std::fs;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

use mbink::App;
use serde_json::{json, Value};

include!(concat!(env!("OUT_DIR"), "/embedded_resources.rs"));

fn main() -> mbink::Result<()> {
    let root = project_root();
    let mut app = App::builder()
        .title("MBink Layout Host Matrix (Rust)")
        .size(1180, 820)
        .resizable(true)
        .build()?;

    let state = Arc::new(Mutex::new(MatrixState {
        sequence: 0,
        rows: 8,
    }));

    app.bind("getMatrixInfo", move |args| {
        Ok(json!({
            "host": "Rust",
            "runtime": "rust",
            "mode": "host-runtime",
            "receivedPurpose": string_arg(&args, "purpose"),
            "capabilities": {
                "backend": true,
                "asyncRoundTrip": true,
                "layoutMutation": true,
                "errorPath": true
            }
        }))
    })?;

    {
        let state = Arc::clone(&state);
        app.bind("mutateMatrix", move |args| {
            let mut guard = state.lock().expect("matrix state lock poisoned");
            guard.sequence += 1;
            guard.rows = (guard.rows + int_arg(&args, "delta", 0)).clamp(3, 24);
            Ok(json!({
                "ok": true,
                "source": "rust",
                "sequence": guard.sequence,
                "rows": guard.rows,
                "label": format!("rust mutation {}", guard.sequence)
            }))
        })?;
    }

    app.bind("validatePayload", move |args| {
        let text = string_arg(&args, "text");
        let normalized = text.to_uppercase();
        Ok(json!({
            "ok": !text.is_empty(),
            "source": "rust",
            "normalized": normalized,
            "length": text.chars().count(),
            "message": if text.is_empty() {
                "rust rejected empty text".to_string()
            } else {
                format!("rust accepted {} chars", text.chars().count())
            }
        }))
    })?;

    app.bind("simulateFailure", move |args| {
        let case_name = string_arg(&args, "caseName");
        Ok(json!({
            "ok": false,
            "source": "rust",
            "code": "RUST_INTENTIONAL_FAILURE",
            "message": format!("intentional failure path reached for {case_name}")
        }))
    })?;

    app.load_html(
        r#"<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
</head>
<body><div id="root"></div></body>
</html>"#,
    )?;
    if let Some(resource_package) = resolve_resource_package(&root) {
        app.mount_resource_package(&path_string(&resource_package), "", "/")?;
        app.load_js_file("/app.js")?;
    } else {
        let bundle = root.join(".dist").join("App.js");
        if bundle.exists() {
            app.load_js_file(&path_string(&bundle))?;
        } else {
            app.load_js_file(&path_string(&root.join("ui").join("app.js")))?;
        }
    }
    app.run();
    Ok(())
}

struct MatrixState {
    sequence: i32,
    rows: i32,
}

fn project_root() -> PathBuf {
    find_project_root_near_exe()
        .or_else(|| std::env::current_dir().ok().and_then(find_project_root))
        .unwrap_or_else(exe_dir)
}

fn find_project_root(mut current: PathBuf) -> Option<PathBuf> {
    loop {
        if current.join("mbink.config.json").exists() || current.join(".dist").join("app.mbrp").exists() {
            return Some(current);
        }
        if !current.pop() {
            return None;
        }
    }
}

fn find_project_root_near_exe() -> Option<PathBuf> {
    std::env::current_exe()
        .ok()
        .and_then(|path| path.parent().map(PathBuf::from))
        .and_then(find_project_root)
}

fn exe_dir() -> PathBuf {
    std::env::current_exe()
        .ok()
        .and_then(|path| path.parent().map(Path::to_path_buf))
        .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")))
}

fn executable_stem() -> String {
    std::env::current_exe()
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
        let path = std::env::temp_dir()
            .join("mbink-layout-host-matrix-rust")
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

fn path_string(path: &Path) -> String {
    path.to_string_lossy().into_owned()
}
