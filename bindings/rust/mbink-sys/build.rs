use std::env;
use std::path::{Path, PathBuf};

fn main() {
    println!("cargo:rerun-if-env-changed=MBINK_LIB_DIR");
    println!("cargo:rerun-if-env-changed=MBINK_LIB_NAME");
    println!("cargo:rerun-if-env-changed=MBINK_DLL_PATH");
    println!("cargo:rustc-check-cfg=cfg(mbink_runtime_load)");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let default_lib_dir = manifest_dir.join("runtime");

    let lib_dir = env::var_os("MBINK_LIB_DIR")
        .map(PathBuf::from)
        .unwrap_or(default_lib_dir);

    let lib_name = env::var("MBINK_LIB_NAME").unwrap_or_else(|_| "mbink".to_string());

    println!("cargo:rustc-link-search=native={}", lib_dir.display());

    if cfg!(target_os = "windows") {
        if has_file(&lib_dir, &format!("{lib_name}.lib")) {
            println!("cargo:rustc-link-lib=dylib={lib_name}");
        } else if has_file(&lib_dir, &format!("{lib_name}.dll")) {
            println!("cargo:warning=No import library found for {lib_name}.dll in {}. Falling back to runtime dynamic loading.", lib_dir.display());
            println!("cargo:rustc-cfg=mbink_runtime_load");
        } else {
            panic!(
                "unable to find either {0}.lib or {0}.dll in MBINK_LIB_DIR or default runtime dir: {1}",
                lib_name,
                lib_dir.display()
            );
        }
        return;
    }

    println!("cargo:rustc-link-lib=dylib={lib_name}");
}

fn has_file(dir: &Path, file_name: &str) -> bool {
    dir.join(file_name).exists()
}
