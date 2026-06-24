use std::fs;
use std::path::PathBuf;

use mbink::{compile_resources, load_resource_file, RESOURCE_FLAG_BYTECODE};

fn main() -> mbink::Result<()> {
    let base = std::env::temp_dir().join("mbink-rust-resource-read-example");
    let input_dir = base.join("input");
    let package_file = base.join("assets.mbk");

    if base.exists() {
        fs::remove_dir_all(&base).map_err(|err| mbink::Error::Message(err.to_string()))?;
    }
    fs::create_dir_all(&input_dir).map_err(|err| mbink::Error::Message(err.to_string()))?;

    fs::write(
        input_dir.join("message.txt"),
        "hello from resource package\n",
    )
    .map_err(|err| mbink::Error::Message(err.to_string()))?;

    compile_resources(&path_str(&input_dir), &path_str(&package_file), "")?;

    let file = load_resource_file(&path_str(&package_file), "input/message.txt", "")?;
    println!("flags = {}", file.flags());
    assert_eq!(file.flags() & RESOURCE_FLAG_BYTECODE, 0);
    assert_eq!(file.into_utf8_string()?, "hello from resource package\n");
    Ok(())
}

fn path_str(path: &PathBuf) -> String {
    path.to_string_lossy().into_owned()
}
