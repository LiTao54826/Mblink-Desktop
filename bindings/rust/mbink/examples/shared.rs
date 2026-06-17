use mbink::App;
use serde_json::json;

fn main() -> mbink::Result<()> {
    let mut app = App::new("MBink Rust Shared", 900, 600)?;

    let shared = app.shared("data")?;
    shared.set_int("count", 1)?;
    shared.set_string("title", "Shared state from Rust")?;
    shared.set_json(
        "user",
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
            <h1 id="title"></h1>
            <pre id="out"></pre>
            <script>
                const render = () => {
                    document.getElementById('title').textContent = data.title;
                    document.getElementById('out').textContent = JSON.stringify({
                        count: data.count,
                        user: data.user,
                    }, null, 2);
                };
                render();
            </script>
        </body>
        </html>
        "#,
    )?;

    app.run();
    Ok(())
}
