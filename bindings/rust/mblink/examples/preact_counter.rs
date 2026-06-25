use std::path::PathBuf;

use mblink::App;
use serde_json::json;

fn main() -> mblink::Result<()> {
    let mut app = App::new("MBlink Rust Preact", 480, 360)?;

    let shared = app.shared("data")?;
    shared.set_int("count", 0)?;
    shared.set_string("title", "Rust + Preact Counter")?;

    app.bind("greet", |_args| {
        Ok(json!({
            "message": "Hello from Rust backend",
            "runtime": "rust",
        }))
    })?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <head>
            <meta charset="utf-8" />
            <title>MBlink Rust Preact</title>
            <style>
                html, body, #root { height: 100%; margin: 0; }
                body { font-family: "Segoe UI", sans-serif; background: #0f172a; color: #e2e8f0; }
            </style>
        </head>
        <body>
            <div id="root"></div>
        </body>
        </html>
        "#,
    )?;

    let app_js = example_root().join("preact_counter/ui/app.js");
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
