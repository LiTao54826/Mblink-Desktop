use std::env;
use std::path::{Path, PathBuf};

fn main() {
    println!("cargo:rerun-if-env-changed=MBLINK_LIB_DIR");
    println!("cargo:rerun-if-env-changed=MBLINK_LIB_NAME");
    println!("cargo:rerun-if-env-changed=MBLINK_DLL_PATH");
    println!("cargo:rustc-check-cfg=cfg(mblink_runtime_load)");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let default_lib_dir = manifest_dir.join("runtime");

    let lib_dir = env::var_os("MBLINK_LIB_DIR")
        .map(PathBuf::from)
        .unwrap_or(default_lib_dir);

    let lib_name = env::var("MBLINK_LIB_NAME").unwrap_or_else(|_| "mblink".to_string());

    println!("cargo:rustc-link-search=native={}", lib_dir.display());

    if cfg!(target_os = "windows") {
        if has_file(&lib_dir, &format!("{lib_name}.dll")) {
            println!("cargo:rustc-cfg=mblink_runtime_load");
        } else if has_file(&lib_dir, &format!("{lib_name}.lib")) {
            println!("cargo:warning=Found {lib_name}.lib but no {lib_name}.dll in {}. Windows bindings use runtime dynamic loading, so the DLL must be available at runtime.", lib_dir.display());
            println!("cargo:rustc-cfg=mblink_runtime_load");
        } else {
            panic!(
                "unable to find either {0}.dll or {0}.lib in MBLINK_LIB_DIR or default runtime dir: {1}",
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
