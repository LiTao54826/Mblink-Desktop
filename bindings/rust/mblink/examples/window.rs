use mblink::App;

fn main() -> mblink::Result<()> {
    let app = App::new("MBlink Window Controls", 900, 640)?;
    app.set_position(120, 120)?
        .set_min_size(480, 320)?
        .set_max_size(1400, 1000)?
        .set_resizable(true)?
        .set_always_on_top(false)?;

    let (_x, _y) = app.position()?;
    let (_w, _h) = app.size()?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <body>
            <h1>MBlink window controls</h1>
            <p>Rust safe wrapper demo for window utilities.</p>
        </body>
        </html>
        "#,
    )?;

    app.devtools_open()?;
    let mut app = app;
    app.run();
    Ok(())
}
