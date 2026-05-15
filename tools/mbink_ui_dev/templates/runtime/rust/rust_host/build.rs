use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=resources/app.mbrp");
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let resource_package = manifest_dir.join("resources").join("app.mbrp");
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let target = out_dir.join("embedded_resources.rs");

    let content = if resource_package.exists() {
        format!(
            "pub const EMBEDDED_RESOURCE_PACKAGE: Option<&'static [u8]> = Some(include_bytes!(r#\"{}\"#));\n",
            resource_package.display()
        )
    } else {
        "pub const EMBEDDED_RESOURCE_PACKAGE: Option<&'static [u8]> = None;\n".to_string()
    };

    fs::write(target, content).expect("failed to write embedded resource metadata");
}
