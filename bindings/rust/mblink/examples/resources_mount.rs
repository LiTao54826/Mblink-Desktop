use std::fs;
use std::path::PathBuf;

use mblink::{compile_resources, App};

fn main() -> mblink::Result<()> {
    let base = std::env::temp_dir().join("mblink-rust-resource-example");
    let input_dir = base.join("input");
    let package_file = base.join("demo.mbk");

    if base.exists() {
        fs::remove_dir_all(&base).map_err(|err| mblink::Error::Message(err.to_string()))?;
    }
    fs::create_dir_all(&input_dir).map_err(|err| mblink::Error::Message(err.to_string()))?;

    let html = r#"
<!doctype html>
<html>
<body style="font-family: sans-serif; padding: 24px;">
    <h1>MBlink Rust Resource Mount</h1>
    <p>This page is loaded from a compiled resource package.</p>
</body>
</html>
"#;
    fs::write(input_dir.join("index.html"), html)
        .map_err(|err| mblink::Error::Message(err.to_string()))?;

    compile_resources(&path_str(&input_dir), &path_str(&package_file), "")?;

    let mut app = App::new("MBlink Rust Resources", 800, 600)?;
    app.mount_resource_package(&path_str(&package_file), "", "/")?;
    app.load_html_file("/input/index.html")?;
    app.run();
    Ok(())
}

fn path_str(path: &PathBuf) -> String {
    path.to_string_lossy().into_owned()
}
