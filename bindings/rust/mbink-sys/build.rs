use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-env-changed=MBINK_LIB_DIR");
    println!("cargo:rerun-if-env-changed=MBINK_LIB_NAME");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let default_lib_dir = manifest_dir
        .parent()
        .and_then(|p| p.parent())
        .map(|p| p.join("python").join("mbink").join("bin"));

    let lib_dir = env::var_os("MBINK_LIB_DIR")
        .map(PathBuf::from)
        .or(default_lib_dir)
        .expect("unable to determine MBINK_LIB_DIR");

    let lib_name = env::var("MBINK_LIB_NAME").unwrap_or_else(|_| "mbink".to_string());

    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=dylib={lib_name}");
}
