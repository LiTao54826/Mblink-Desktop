use std::path::PathBuf;

use mbink::App;
use serde_json::json;

fn main() -> mbink::Result<()> {
    let mut app = App::new("MBink Rust Native Controls", 1180, 760)?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <head>
            <meta charset="utf-8" />
            <title>MBink Rust Native Controls</title>
            <style>
                * { box-sizing: border-box; }
                html, body { width: 100%; height: 100%; margin: 0; background: #0f172a; color: #e2e8f0; }
                body { font-family: "Segoe UI", sans-serif; }
                .layout { display: grid; grid-template-columns: 340px minmax(0, 1fr); gap: 12px; width: 100vw; height: 100vh; min-height: 0; padding: 12px; }
                .panel { background: #111827; border: 1px solid #334155; border-radius: 12px; padding: 14px; min-height: 0; }
                .control-host { min-height: 0; padding: 0; overflow: hidden; }
                .right { display: grid; grid-template-rows: minmax(0, 1fr) minmax(0, 1fr); gap: 12px; min-height: 0; }
                logview, terminal { width: 100%; height: 100%; min-height: 0; display: block; background: #020617; border: 1px solid #334155; border-radius: 12px; overflow: hidden; }
            </style>
        </head>
        <body>
            <div class="layout">
                <div class="panel"><div id="root"></div></div>
                <div class="right">
                    <div class="panel control-host"><logview id="logs"></logview></div>
                    <div class="panel control-host"><terminal id="term"></terminal></div>
                </div>
            </div>
        </body>
        </html>
        "#,
    )?;

    let log_preview = {
        let logs = app.logview("logs")?;
        logs.append("INFO", "rust", "native controls demo booted")?;
        logs.append("INFO", "rust", "logview acquired from Rust safe wrapper")?;
        logs.append("WARN", "rust", "terminal below is also controlled from Rust")?;
        logs.export("text")?
    };

    let term_snapshot = {
        let term = app.terminal("term")?;
        term.resize(28, 100)
            .write("MBink terminal ready.\r\n")?
            .write("This output is written by Rust before app.run().\r\n")?
            .write("Try editing this example to stream more content.\r\n")?;
        let _ = term.execute("echo hello from terminal.execute");
        term.serialize()?
    };

    let shared = app.shared("demo")?;

    shared.set_string("title", "Rust Native Controls Demo")?;
    shared.set_string("status", "logview + terminal initialized from Rust")?;
    shared.set_string("log_preview", &tail(&log_preview, 320))?;
    shared.set_string("terminal_preview", &tail(&term_snapshot, 320))?;

    app.bind("ping", |_args| {
        Ok(json!({
            "ok": true,
            "message": "Hello from Rust bind()",
            "controls": ["logview", "terminal"]
        }))
    })?;

    let app_js = example_root().join("native_controls/ui/app.js");
    app.load_js_file(&path_str(&app_js))?;
    app.run();
    Ok(())
}

fn example_root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("examples")
}

fn path_str(path: &PathBuf) -> String {
    path.to_string_lossy().into_owned()
}

fn tail(text: &str, max: usize) -> String {
    let mut chars: Vec<char> = text.chars().collect();
    if chars.len() > max {
        chars.drain(..chars.len() - max);
    }
    chars.into_iter().collect()
}
