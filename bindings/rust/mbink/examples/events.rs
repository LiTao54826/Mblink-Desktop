use mbink::App;

fn main() -> mbink::Result<()> {
    let mut app = App::new("MBink Rust Events", 900, 640)?;

    app.on_resize(|w, h| {
        println!("resize: {}x{}", w, h);
    })?;

    app.on_focus(|| {
        println!("focus");
    })?;

    app.on_blur(|| {
        println!("blur");
    })?;

    app.on_close_request(|| {
        println!("close requested");
        false
    })?;

    app.on_close(|| {
        println!("closed");
    })?;

    app.load_html(
        r#"
        <!doctype html>
        <html>
        <body style="font-family: sans-serif; padding: 24px;">
            <h1>Rust events example</h1>
            <p>Try focusing, resizing, and closing the window.</p>
            <p>Callback output is printed by Rust.</p>
        </body>
        </html>
        "#,
    )?;

    app.run();
    Ok(())
}
