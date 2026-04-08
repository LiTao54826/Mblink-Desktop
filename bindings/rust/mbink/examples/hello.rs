use mbink::App;

fn main() -> mbink::Result<()> {
    let mut app = App::builder()
        .title("MBink Rust Hello")
        .size(800, 600)
        .build()?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <body style="font-family: sans-serif; padding: 24px;">
            <h1>Hello from Rust</h1>
            <p>If you can see this window, the Rust binding works.</p>
        </body>
        </html>
        "#,
    )?;

    app.run();
    Ok(())
}
