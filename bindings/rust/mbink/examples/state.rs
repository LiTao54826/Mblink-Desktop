use mbink::App;
use serde_json::json;

fn main() -> mbink::Result<()> {
    let mut app = App::new("MBink Rust State", 900, 640)?;

    app.state().set_int("count", 1)?;
    app.state().set_string("title", "State from Rust")?;
    app.state().set_json(
        "profile",
        &json!({
            "name": "Alice",
            "role": "admin"
        }),
    )?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <body style="font-family: sans-serif; padding: 24px;">
            <h1>Rust state example</h1>
            <p>This example initializes window state from Rust.</p>
            <pre id="out"></pre>
            <script>
                document.getElementById('out').textContent = JSON.stringify({
                    count: window.count,
                    title: window.title,
                    profile: window.profile,
                }, null, 2);
            </script>
        </body>
        </html>
        "#,
    )?;

    app.run();
    Ok(())
}
