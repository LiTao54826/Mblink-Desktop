#![allow(non_snake_case)]
use std::env;


use std::ffi::{c_char, c_int, c_void};
use std::path::PathBuf;
use std::sync::OnceLock;

use super::{
    MBinkBoolCallback, MBinkCallback, MBinkConfig, MBinkHandle, MBinkResizeCallback,
    MBinkSharedHandle, MBinkType, MBinkUpdateCallback, MBinkVoidCallback,
};

type MBinkLoadResourceFileFn = unsafe extern "C" fn(
    *const c_char,
    *const c_char,
    *const c_char,
    *mut *mut c_void,
    *mut usize,
    *mut u32,
) -> c_int;

struct Api {
    _lib: libloading::Library,
    mbink_init: unsafe extern "C" fn() -> c_int,
    mbink_cleanup: unsafe extern "C" fn(),
    mbink_version: unsafe extern "C" fn() -> *const c_char,
    mbink_last_error: unsafe extern "C" fn() -> *const c_char,
    mbink_create: unsafe extern "C" fn(*const c_char, c_int, c_int) -> MBinkHandle,
    mbink_create_ex: unsafe extern "C" fn(*const MBinkConfig) -> MBinkHandle,
    mbink_default_config: unsafe extern "C" fn() -> MBinkConfig,
    mbink_destroy: unsafe extern "C" fn(MBinkHandle),
    mbink_run: unsafe extern "C" fn(MBinkHandle),
    mbink_stop: unsafe extern "C" fn(MBinkHandle),
    mbink_poll_events: unsafe extern "C" fn(MBinkHandle) -> bool,
    mbink_set_title: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_set_size: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_get_size: unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int,
    mbink_set_position: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_get_position: unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int,
    mbink_show: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_hide: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_minimize: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_maximize: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_restore: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_load_html: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_load_html_file: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_eval_js: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_eval_module: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_load_js_file: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_load_bytecode: unsafe extern "C" fn(MBinkHandle, *const c_void, usize) -> c_int,
    mbink_compile_resources: unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int,
    mbink_load_resource_file: MBinkLoadResourceFileFn,
    mbink_mount_resource_package: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mbink_emit: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_bind: unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkCallback, *mut c_void) -> c_int,
    mbink_unbind: unsafe extern "C" fn(MBinkHandle, *const c_char),
    mbink_on_resize: unsafe extern "C" fn(MBinkHandle, MBinkResizeCallback, *mut c_void) -> c_int,
    mbink_on_close: unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int,
    mbink_on_close_request: unsafe extern "C" fn(MBinkHandle, MBinkBoolCallback, *mut c_void) -> c_int,
    mbink_on_focus: unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int,
    mbink_on_blur: unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int,
    mbink_on_update: unsafe extern "C" fn(MBinkHandle, MBinkUpdateCallback, *mut c_void) -> c_int,
    mbink_state_exists: unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool,
    mbink_state_type: unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkType,
    mbink_state_delete: unsafe extern "C" fn(MBinkHandle, *const c_char),
    mbink_state_get_bool: unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool,
    mbink_state_get_int: unsafe extern "C" fn(MBinkHandle, *const c_char) -> i64,
    mbink_state_get_double: unsafe extern "C" fn(MBinkHandle, *const c_char) -> f64,
    mbink_state_get_string: unsafe extern "C" fn(MBinkHandle, *const c_char) -> *const c_char,
    mbink_state_get_json: unsafe extern "C" fn(MBinkHandle, *const c_char) -> *mut c_char,
    mbink_state_set_null: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_set_bool: unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int,
    mbink_state_set_int: unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int,
    mbink_state_set_double: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_set_string: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_set_json: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_batch_begin: unsafe extern "C" fn(MBinkHandle),
    mbink_state_batch_end: unsafe extern "C" fn(MBinkHandle),
    mbink_shared_create: unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkSharedHandle,
    mbink_shared_destroy: unsafe extern "C" fn(MBinkSharedHandle),
    mbink_shared_set_int: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, i64) -> c_int,
    mbink_shared_set_double: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, f64) -> c_int,
    mbink_shared_set_string: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int,
    mbink_shared_set_bool: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, bool) -> c_int,
    mbink_shared_set_null: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int,
    mbink_shared_set_json: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int,
    mbink_shared_get_int: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> i64,
    mbink_shared_get_double: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> f64,
    mbink_shared_get_string: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char,
    mbink_shared_get_bool: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool,
    mbink_shared_get_json: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char,
    mbink_shared_get_type: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int,
    mbink_shared_delete: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int,
    mbink_shared_has: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool,
    mbink_shared_batch_begin: unsafe extern "C" fn(MBinkSharedHandle),
    mbink_shared_batch_end: unsafe extern "C" fn(MBinkSharedHandle),
    mbink_copy_string: unsafe extern "C" fn(*const c_char) -> *mut c_char,
    mbink_free: unsafe extern "C" fn(*mut c_void),
}


static API: OnceLock<Api> = OnceLock::new();

fn api() -> &'static Api {
    API.get_or_init(|| unsafe { load_api() })
}

unsafe fn load_api() -> Api {
    let path = dll_path();
    let lib = libloading::Library::new(&path)
        .unwrap_or_else(|err| panic!("failed to load MBink DLL {}: {err}", path.display()));

    macro_rules! load {
        ($name:literal, $ty:ty) => {{
            *lib.get::<$ty>($name).unwrap_or_else(|err| {
                panic!(
                    "failed to load symbol {} from {}: {err}",
                    String::from_utf8_lossy($name),
                    path.display()
                )
            })
        }};
    }

    let mbink_init = load!(b"mbink_init\0", unsafe extern "C" fn() -> c_int);
    let mbink_cleanup = load!(b"mbink_cleanup\0", unsafe extern "C" fn());
    let mbink_version = load!(b"mbink_version\0", unsafe extern "C" fn() -> *const c_char);
    let mbink_last_error = load!(b"mbink_last_error\0", unsafe extern "C" fn() -> *const c_char);
    let mbink_create = load!(b"mbink_create\0", unsafe extern "C" fn(*const c_char, c_int, c_int) -> MBinkHandle);
    let mbink_create_ex = load!(b"mbink_create_ex\0", unsafe extern "C" fn(*const MBinkConfig) -> MBinkHandle);
    let mbink_default_config = load!(b"mbink_default_config\0", unsafe extern "C" fn() -> MBinkConfig);
    let mbink_destroy = load!(b"mbink_destroy\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_run = load!(b"mbink_run\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_stop = load!(b"mbink_stop\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_poll_events = load!(b"mbink_poll_events\0", unsafe extern "C" fn(MBinkHandle) -> bool);
    let mbink_set_title = load!(b"mbink_set_title\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_set_size = load!(b"mbink_set_size\0", unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int);
    let mbink_get_size = load!(b"mbink_get_size\0", unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int);
    let mbink_set_position = load!(b"mbink_set_position\0", unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int);
    let mbink_get_position = load!(b"mbink_get_position\0", unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int);
    let mbink_show = load!(b"mbink_show\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_hide = load!(b"mbink_hide\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_minimize = load!(b"mbink_minimize\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_maximize = load!(b"mbink_maximize\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_restore = load!(b"mbink_restore\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_load_html = load!(b"mbink_load_html\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_load_html_file = load!(b"mbink_load_html_file\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_eval_js = load!(b"mbink_eval_js\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_eval_module = load!(b"mbink_eval_module\0", unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int);
    let mbink_load_js_file = load!(b"mbink_load_js_file\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_load_bytecode = load!(b"mbink_load_bytecode\0", unsafe extern "C" fn(MBinkHandle, *const c_void, usize) -> c_int);
    let mbink_compile_resources = load!(b"mbink_compile_resources\0", unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int);
    let mbink_load_resource_file = load!(b"mbink_load_resource_file\0", MBinkLoadResourceFileFn);
    let mbink_mount_resource_package = load!(b"mbink_mount_resource_package\0", unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int);
    let mbink_emit = load!(b"mbink_emit\0", unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int);
    let mbink_bind = load!(b"mbink_bind\0", unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkCallback, *mut c_void) -> c_int);
    let mbink_unbind = load!(b"mbink_unbind\0", unsafe extern "C" fn(MBinkHandle, *const c_char));
    let mbink_on_resize = load!(b"mbink_on_resize\0", unsafe extern "C" fn(MBinkHandle, MBinkResizeCallback, *mut c_void) -> c_int);
    let mbink_on_close = load!(b"mbink_on_close\0", unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int);
    let mbink_on_close_request = load!(b"mbink_on_close_request\0", unsafe extern "C" fn(MBinkHandle, MBinkBoolCallback, *mut c_void) -> c_int);
    let mbink_on_focus = load!(b"mbink_on_focus\0", unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int);
    let mbink_on_blur = load!(b"mbink_on_blur\0", unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int);
    let mbink_on_update = load!(b"mbink_on_update\0", unsafe extern "C" fn(MBinkHandle, MBinkUpdateCallback, *mut c_void) -> c_int);
    let mbink_state_exists = load!(b"mbink_state_exists\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool);
    let mbink_state_type = load!(b"mbink_state_type\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkType);
    let mbink_state_delete = load!(b"mbink_state_delete\0", unsafe extern "C" fn(MBinkHandle, *const c_char));
    let mbink_state_get_bool = load!(b"mbink_state_get_bool\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool);
    let mbink_state_get_int = load!(b"mbink_state_get_int\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> i64);
    let mbink_state_get_double = load!(b"mbink_state_get_double\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> f64);
    let mbink_state_get_string = load!(b"mbink_state_get_string\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> *const c_char);
    let mbink_state_get_json = load!(b"mbink_state_get_json\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> *mut c_char);
    let mbink_state_set_null = load!(b"mbink_state_set_null\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int);
    let mbink_state_set_bool = load!(b"mbink_state_set_bool\0", unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int);
    let mbink_state_set_int = load!(b"mbink_state_set_int\0", unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int);
    let mbink_state_set_double = load!(b"mbink_state_set_double\0", unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int);
    let mbink_state_set_string = load!(b"mbink_state_set_string\0", unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int);
    let mbink_state_set_json = load!(b"mbink_state_set_json\0", unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int);
    let mbink_state_batch_begin = load!(b"mbink_state_batch_begin\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_state_batch_end = load!(b"mbink_state_batch_end\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_shared_create = load!(b"mbink_shared_create\0", unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkSharedHandle);
    let mbink_shared_destroy = load!(b"mbink_shared_destroy\0", unsafe extern "C" fn(MBinkSharedHandle));
    let mbink_shared_set_int = load!(b"mbink_shared_set_int\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char, i64) -> c_int);
    let mbink_shared_set_double = load!(b"mbink_shared_set_double\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char, f64) -> c_int);
    let mbink_shared_set_string = load!(b"mbink_shared_set_string\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int);
    let mbink_shared_set_bool = load!(b"mbink_shared_set_bool\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char, bool) -> c_int);
    let mbink_shared_set_null = load!(b"mbink_shared_set_null\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int);
    let mbink_shared_set_json = load!(b"mbink_shared_set_json\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int);
    let mbink_shared_get_int = load!(b"mbink_shared_get_int\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> i64);
    let mbink_shared_get_double = load!(b"mbink_shared_get_double\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> f64);
    let mbink_shared_get_string = load!(b"mbink_shared_get_string\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char);
    let mbink_shared_get_bool = load!(b"mbink_shared_get_bool\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool);
    let mbink_shared_get_json = load!(b"mbink_shared_get_json\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char);
    let mbink_shared_get_type = load!(b"mbink_shared_get_type\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int);
    let mbink_shared_delete = load!(b"mbink_shared_delete\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int);
    let mbink_shared_has = load!(b"mbink_shared_has\0", unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool);
    let mbink_shared_batch_begin = load!(b"mbink_shared_batch_begin\0", unsafe extern "C" fn(MBinkSharedHandle));
    let mbink_shared_batch_end = load!(b"mbink_shared_batch_end\0", unsafe extern "C" fn(MBinkSharedHandle));
    let mbink_copy_string = load!(b"mbink_copy_string\0", unsafe extern "C" fn(*const c_char) -> *mut c_char);
    let mbink_free = load!(b"mbink_free\0", unsafe extern "C" fn(*mut c_void));

    Api {
        _lib: lib,
        mbink_init,
        mbink_cleanup,
        mbink_version,
        mbink_last_error,
        mbink_create,
        mbink_create_ex,
        mbink_default_config,
        mbink_destroy,
        mbink_run,
        mbink_stop,
        mbink_poll_events,
        mbink_set_title,
        mbink_set_size,
        mbink_get_size,
        mbink_set_position,
        mbink_get_position,
        mbink_show,
        mbink_hide,
        mbink_minimize,
        mbink_maximize,
        mbink_restore,
        mbink_load_html,
        mbink_load_html_file,
        mbink_eval_js,
        mbink_eval_module,
        mbink_load_js_file,
        mbink_load_bytecode,
        mbink_compile_resources,
        mbink_load_resource_file,
        mbink_mount_resource_package,
        mbink_emit,
        mbink_bind,
        mbink_unbind,
        mbink_on_resize,
        mbink_on_close,
        mbink_on_close_request,
        mbink_on_focus,
        mbink_on_blur,
        mbink_on_update,
        mbink_state_exists,
        mbink_state_type,
        mbink_state_delete,
        mbink_state_get_bool,
        mbink_state_get_int,
        mbink_state_get_double,
        mbink_state_get_string,
        mbink_state_get_json,
        mbink_state_set_null,
        mbink_state_set_bool,
        mbink_state_set_int,
        mbink_state_set_double,
        mbink_state_set_string,
        mbink_state_set_json,
        mbink_state_batch_begin,
        mbink_state_batch_end,
        mbink_shared_create,
        mbink_shared_destroy,
        mbink_shared_set_int,
        mbink_shared_set_double,
        mbink_shared_set_string,
        mbink_shared_set_bool,
        mbink_shared_set_null,
        mbink_shared_set_json,
        mbink_shared_get_int,
        mbink_shared_get_double,
        mbink_shared_get_string,
        mbink_shared_get_bool,
        mbink_shared_get_json,
        mbink_shared_get_type,
        mbink_shared_delete,
        mbink_shared_has,
        mbink_shared_batch_begin,
        mbink_shared_batch_end,
        mbink_copy_string,
        mbink_free,
    }
}


fn dll_path() -> PathBuf {
    if let Some(path) = env::var_os("MBINK_DLL_PATH") {
        return PathBuf::from(path);
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let candidate = manifest_dir
        .parent()
        .and_then(|p| p.parent())
        .map(|p| p.join("python").join("mbink").join("bin").join("mbink.dll"))
        .expect("unable to determine default MBink DLL path");

    if candidate.exists() {
        return candidate;
    }

    PathBuf::from("mbink.dll")
}

pub unsafe fn mbink_init() -> c_int { (api().mbink_init)() }
pub unsafe fn mbink_cleanup() { (api().mbink_cleanup)() }
pub unsafe fn mbink_version() -> *const c_char { (api().mbink_version)() }
pub unsafe fn mbink_last_error() -> *const c_char { (api().mbink_last_error)() }
pub unsafe fn mbink_create(title: *const c_char, width: c_int, height: c_int) -> MBinkHandle { (api().mbink_create)(title, width, height) }
pub unsafe fn mbink_create_ex(config: *const MBinkConfig) -> MBinkHandle { (api().mbink_create_ex)(config) }
pub unsafe fn mbink_default_config() -> MBinkConfig { (api().mbink_default_config)() }
pub unsafe fn mbink_destroy(handle: MBinkHandle) { (api().mbink_destroy)(handle) }
pub unsafe fn mbink_run(handle: MBinkHandle) { (api().mbink_run)(handle) }
pub unsafe fn mbink_stop(handle: MBinkHandle) { (api().mbink_stop)(handle) }
pub unsafe fn mbink_poll_events(handle: MBinkHandle) -> bool { (api().mbink_poll_events)(handle) }
pub unsafe fn mbink_set_title(handle: MBinkHandle, title: *const c_char) -> c_int { (api().mbink_set_title)(handle, title) }
pub unsafe fn mbink_set_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int { (api().mbink_set_size)(handle, width, height) }
pub unsafe fn mbink_get_size(handle: MBinkHandle, width: *mut c_int, height: *mut c_int) -> c_int { (api().mbink_get_size)(handle, width, height) }
pub unsafe fn mbink_set_position(handle: MBinkHandle, x: c_int, y: c_int) -> c_int { (api().mbink_set_position)(handle, x, y) }
pub unsafe fn mbink_get_position(handle: MBinkHandle, x: *mut c_int, y: *mut c_int) -> c_int { (api().mbink_get_position)(handle, x, y) }
pub unsafe fn mbink_show(handle: MBinkHandle) -> c_int { (api().mbink_show)(handle) }
pub unsafe fn mbink_hide(handle: MBinkHandle) -> c_int { (api().mbink_hide)(handle) }
pub unsafe fn mbink_minimize(handle: MBinkHandle) -> c_int { (api().mbink_minimize)(handle) }
pub unsafe fn mbink_maximize(handle: MBinkHandle) -> c_int { (api().mbink_maximize)(handle) }
pub unsafe fn mbink_restore(handle: MBinkHandle) -> c_int { (api().mbink_restore)(handle) }
pub unsafe fn mbink_load_html(handle: MBinkHandle, html: *const c_char) -> c_int { (api().mbink_load_html)(handle, html) }
pub unsafe fn mbink_load_html_file(handle: MBinkHandle, filepath: *const c_char) -> c_int { (api().mbink_load_html_file)(handle, filepath) }
pub unsafe fn mbink_eval_js(handle: MBinkHandle, code: *const c_char) -> c_int { (api().mbink_eval_js)(handle, code) }
pub unsafe fn mbink_eval_module(handle: MBinkHandle, code: *const c_char, filename: *const c_char) -> c_int { (api().mbink_eval_module)(handle, code, filename) }
pub unsafe fn mbink_load_js_file(handle: MBinkHandle, filepath: *const c_char) -> c_int { (api().mbink_load_js_file)(handle, filepath) }
pub unsafe fn mbink_load_bytecode(handle: MBinkHandle, data: *const c_void, size: usize) -> c_int { (api().mbink_load_bytecode)(handle, data, size) }
pub unsafe fn mbink_compile_resources(input_path: *const c_char, output_file: *const c_char, encryption_key: *const c_char) -> c_int { (api().mbink_compile_resources)(input_path, output_file, encryption_key) }
pub unsafe fn mbink_load_resource_file(package_file: *const c_char, resource_path: *const c_char, encryption_key: *const c_char, out_data: *mut *mut c_void, out_size: *mut usize, out_flags: *mut u32) -> c_int { (api().mbink_load_resource_file)(package_file, resource_path, encryption_key, out_data, out_size, out_flags) }
pub unsafe fn mbink_mount_resource_package(handle: MBinkHandle, package_file: *const c_char, encryption_key: *const c_char, mount_point: *const c_char) -> c_int { (api().mbink_mount_resource_package)(handle, package_file, encryption_key, mount_point) }
pub unsafe fn mbink_emit(handle: MBinkHandle, event_name: *const c_char, data_json: *const c_char) -> c_int { (api().mbink_emit)(handle, event_name, data_json) }
pub unsafe fn mbink_bind(handle: MBinkHandle, name: *const c_char, callback: MBinkCallback, user_data: *mut c_void) -> c_int { (api().mbink_bind)(handle, name, callback, user_data) }
pub unsafe fn mbink_unbind(handle: MBinkHandle, name: *const c_char) { (api().mbink_unbind)(handle, name) }
pub unsafe fn mbink_on_resize(handle: MBinkHandle, callback: MBinkResizeCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_resize)(handle, callback, user_data) }
pub unsafe fn mbink_on_close(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_close)(handle, callback, user_data) }
pub unsafe fn mbink_on_close_request(handle: MBinkHandle, callback: MBinkBoolCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_close_request)(handle, callback, user_data) }
pub unsafe fn mbink_on_focus(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_focus)(handle, callback, user_data) }
pub unsafe fn mbink_on_blur(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_blur)(handle, callback, user_data) }
pub unsafe fn mbink_on_update(handle: MBinkHandle, callback: MBinkUpdateCallback, user_data: *mut c_void) -> c_int { (api().mbink_on_update)(handle, callback, user_data) }
pub unsafe fn mbink_state_exists(handle: MBinkHandle, name: *const c_char) -> bool { (api().mbink_state_exists)(handle, name) }
pub unsafe fn mbink_state_type(handle: MBinkHandle, name: *const c_char) -> MBinkType { (api().mbink_state_type)(handle, name) }
pub unsafe fn mbink_state_delete(handle: MBinkHandle, name: *const c_char) { (api().mbink_state_delete)(handle, name) }
pub unsafe fn mbink_state_get_bool(handle: MBinkHandle, name: *const c_char) -> bool { (api().mbink_state_get_bool)(handle, name) }
pub unsafe fn mbink_state_get_int(handle: MBinkHandle, name: *const c_char) -> i64 { (api().mbink_state_get_int)(handle, name) }
pub unsafe fn mbink_state_get_double(handle: MBinkHandle, name: *const c_char) -> f64 { (api().mbink_state_get_double)(handle, name) }
pub unsafe fn mbink_state_get_string(handle: MBinkHandle, name: *const c_char) -> *const c_char { (api().mbink_state_get_string)(handle, name) }
pub unsafe fn mbink_state_get_json(handle: MBinkHandle, name: *const c_char) -> *mut c_char { (api().mbink_state_get_json)(handle, name) }
pub unsafe fn mbink_state_set_null(handle: MBinkHandle, name: *const c_char) -> c_int { (api().mbink_state_set_null)(handle, name) }
pub unsafe fn mbink_state_set_bool(handle: MBinkHandle, name: *const c_char, value: bool) -> c_int { (api().mbink_state_set_bool)(handle, name, value) }
pub unsafe fn mbink_state_set_int(handle: MBinkHandle, name: *const c_char, value: i64) -> c_int { (api().mbink_state_set_int)(handle, name, value) }
pub unsafe fn mbink_state_set_double(handle: MBinkHandle, name: *const c_char, value: f64) -> c_int { (api().mbink_state_set_double)(handle, name, value) }
pub unsafe fn mbink_state_set_string(handle: MBinkHandle, name: *const c_char, value: *const c_char) -> c_int { (api().mbink_state_set_string)(handle, name, value) }
pub unsafe fn mbink_state_set_json(handle: MBinkHandle, name: *const c_char, json: *const c_char) -> c_int { (api().mbink_state_set_json)(handle, name, json) }
pub unsafe fn mbink_state_batch_begin(handle: MBinkHandle) { (api().mbink_state_batch_begin)(handle) }
pub unsafe fn mbink_state_batch_end(handle: MBinkHandle) { (api().mbink_state_batch_end)(handle) }
pub unsafe fn mbink_shared_create(handle: MBinkHandle, name: *const c_char) -> MBinkSharedHandle { (api().mbink_shared_create)(handle, name) }
pub unsafe fn mbink_shared_destroy(shared: MBinkSharedHandle) { (api().mbink_shared_destroy)(shared) }
pub unsafe fn mbink_shared_set_int(shared: MBinkSharedHandle, key: *const c_char, value: i64) -> c_int { (api().mbink_shared_set_int)(shared, key, value) }
pub unsafe fn mbink_shared_set_double(shared: MBinkSharedHandle, key: *const c_char, value: f64) -> c_int { (api().mbink_shared_set_double)(shared, key, value) }
pub unsafe fn mbink_shared_set_string(shared: MBinkSharedHandle, key: *const c_char, value: *const c_char) -> c_int { (api().mbink_shared_set_string)(shared, key, value) }
pub unsafe fn mbink_shared_set_bool(shared: MBinkSharedHandle, key: *const c_char, value: bool) -> c_int { (api().mbink_shared_set_bool)(shared, key, value) }
pub unsafe fn mbink_shared_set_null(shared: MBinkSharedHandle, key: *const c_char) -> c_int { (api().mbink_shared_set_null)(shared, key) }
pub unsafe fn mbink_shared_set_json(shared: MBinkSharedHandle, key: *const c_char, json: *const c_char) -> c_int { (api().mbink_shared_set_json)(shared, key, json) }
pub unsafe fn mbink_shared_get_int(shared: MBinkSharedHandle, key: *const c_char) -> i64 { (api().mbink_shared_get_int)(shared, key) }
pub unsafe fn mbink_shared_get_double(shared: MBinkSharedHandle, key: *const c_char) -> f64 { (api().mbink_shared_get_double)(shared, key) }
pub unsafe fn mbink_shared_get_string(shared: MBinkSharedHandle, key: *const c_char) -> *mut c_char { (api().mbink_shared_get_string)(shared, key) }
pub unsafe fn mbink_shared_get_bool(shared: MBinkSharedHandle, key: *const c_char) -> bool { (api().mbink_shared_get_bool)(shared, key) }
pub unsafe fn mbink_shared_get_json(shared: MBinkSharedHandle, key: *const c_char) -> *mut c_char { (api().mbink_shared_get_json)(shared, key) }
pub unsafe fn mbink_shared_get_type(shared: MBinkSharedHandle, key: *const c_char) -> c_int { (api().mbink_shared_get_type)(shared, key) }
pub unsafe fn mbink_shared_delete(shared: MBinkSharedHandle, key: *const c_char) -> c_int { (api().mbink_shared_delete)(shared, key) }
pub unsafe fn mbink_shared_has(shared: MBinkSharedHandle, key: *const c_char) -> bool { (api().mbink_shared_has)(shared, key) }
pub unsafe fn mbink_shared_batch_begin(shared: MBinkSharedHandle) { (api().mbink_shared_batch_begin)(shared) }
pub unsafe fn mbink_shared_batch_end(shared: MBinkSharedHandle) { (api().mbink_shared_batch_end)(shared) }
pub unsafe fn mbink_copy_string(str_: *const c_char) -> *mut c_char { (api().mbink_copy_string)(str_) }
pub unsafe fn mbink_free(ptr: *mut c_void) { (api().mbink_free)(ptr) }
