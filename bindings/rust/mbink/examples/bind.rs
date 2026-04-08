use mbink::App;
use serde_json::json;

fn main() -> mbink::Result<()> {
    let mut app = App::new("MBink Rust Bind", 900, 600)?;

    app.bind("greet", |args| {
        let name = args
            .get("name")
            .and_then(|v| v.as_str())
            .unwrap_or("world");

        Ok(json!({
            "message": format!("Hello, {name}!"),
            "from": "Rust",
        }))
    })?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <body style="font-family: sans-serif; padding: 24px;">
            <h1>Rust bind example</h1>
            <button id="btn">Call backend.greet()</button>
            <pre id="out"></pre>
            <script>
                document.getElementById('btn').onclick = async () => {
                    const result = await backend.greet({ name: 'MBink' });
                    document.getElementById('out').textContent = JSON.stringify(result, null, 2);
                };
            </script>
        </body>
        </html>
        "#,
    )?;

    app.run();
    Ok(())
}
