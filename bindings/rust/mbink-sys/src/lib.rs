#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

use std::ffi::{c_char, c_int, c_void};

#[cfg(all(target_os = "windows", mbink_runtime_load))]
mod windows_runtime;

#[cfg(all(target_os = "windows", mbink_runtime_load))]
pub use windows_runtime::*;

pub enum MBinkWindow {}
pub type MBinkHandle = *mut MBinkWindow;

pub enum MBinkSharedObject {}
pub type MBinkSharedHandle = *mut MBinkSharedObject;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBinkError {
    MBINK_OK = 0,
    MBINK_ERROR_INVALID_HANDLE = -1,
    MBINK_ERROR_INVALID_PARAM = -2,
    MBINK_ERROR_NOT_FOUND = -3,
    MBINK_ERROR_TYPE_MISMATCH = -4,
    MBINK_ERROR_OUT_OF_RANGE = -5,
    MBINK_ERROR_JS_ERROR = -6,
    MBINK_ERROR_UNKNOWN = -99,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBinkType {
    MBINK_TYPE_NULL = 0,
    MBINK_TYPE_BOOL = 1,
    MBINK_TYPE_INT = 2,
    MBINK_TYPE_DOUBLE = 3,
    MBINK_TYPE_STRING = 4,
    MBINK_TYPE_ARRAY = 5,
    MBINK_TYPE_OBJECT = 6,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBinkConfig {
    pub title: *const c_char,
    pub width: c_int,
    pub height: c_int,
    pub headless: bool,
    pub borderless: bool,
    pub transparent: bool,
    pub always_on_top: bool,
    pub resizable: bool,
    pub gpu: bool,
    pub fullscreen: bool,
    pub resize_border_width: c_int,
    pub min_width: c_int,
    pub min_height: c_int,
    pub max_width: c_int,
    pub max_height: c_int,
}

pub type MBinkCallback = Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBinkAsyncCallback = Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBinkResizeCallback = Option<unsafe extern "C" fn(c_int, c_int, *mut c_void)>;
pub type MBinkVoidCallback = Option<unsafe extern "C" fn(*mut c_void)>;
pub type MBinkBoolCallback = Option<unsafe extern "C" fn(*mut c_void) -> bool>;
pub type MBinkUpdateCallback = Option<unsafe extern "C" fn(f32, *mut c_void)>;

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
extern "C" {
    pub fn mbink_init() -> c_int;
    pub fn mbink_cleanup();
    pub fn mbink_version() -> *const c_char;
    pub fn mbink_last_error() -> *const c_char;

    pub fn mbink_create(title: *const c_char, width: c_int, height: c_int) -> MBinkHandle;
    pub fn mbink_create_ex(config: *const MBinkConfig) -> MBinkHandle;
    pub fn mbink_default_config() -> MBinkConfig;
    pub fn mbink_destroy(handle: MBinkHandle);
    pub fn mbink_run(handle: MBinkHandle);
    pub fn mbink_stop(handle: MBinkHandle);
    pub fn mbink_poll_events(handle: MBinkHandle) -> bool;

    pub fn mbink_set_title(handle: MBinkHandle, title: *const c_char) -> c_int;
    pub fn mbink_tray_create(handle: MBinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mbink_tray_destroy(handle: MBinkHandle) -> c_int;
    pub fn mbink_tray_set_tooltip(handle: MBinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mbink_tray_set_menu(handle: MBinkHandle, menu_json: *const c_char) -> c_int;
    pub fn mbink_tray_set_left_click_callback(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_tray_set_menu_callback(handle: MBinkHandle, callback: MBinkCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_set_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mbink_get_size(handle: MBinkHandle, width: *mut c_int, height: *mut c_int) -> c_int;
    pub fn mbink_set_position(handle: MBinkHandle, x: c_int, y: c_int) -> c_int;
    pub fn mbink_get_position(handle: MBinkHandle, x: *mut c_int, y: *mut c_int) -> c_int;
    pub fn mbink_set_min_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mbink_set_max_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mbink_show(handle: MBinkHandle) -> c_int;
    pub fn mbink_hide(handle: MBinkHandle) -> c_int;
    pub fn mbink_minimize(handle: MBinkHandle) -> c_int;
    pub fn mbink_maximize(handle: MBinkHandle) -> c_int;
    pub fn mbink_restore(handle: MBinkHandle) -> c_int;
    pub fn mbink_set_fullscreen(handle: MBinkHandle, fullscreen: bool) -> c_int;
    pub fn mbink_set_resizable(handle: MBinkHandle, resizable: bool) -> c_int;
    pub fn mbink_set_borderless(handle: MBinkHandle, borderless: bool) -> c_int;
    pub fn mbink_set_always_on_top(handle: MBinkHandle, on_top: bool) -> c_int;

    pub fn mbink_load_html(handle: MBinkHandle, html: *const c_char) -> c_int;
    pub fn mbink_load_html_file(handle: MBinkHandle, filepath: *const c_char) -> c_int;
    pub fn mbink_eval_js(handle: MBinkHandle, code: *const c_char) -> c_int;
    pub fn mbink_eval_module(handle: MBinkHandle, code: *const c_char, filename: *const c_char) -> c_int;
    pub fn mbink_load_js_file(handle: MBinkHandle, filepath: *const c_char) -> c_int;
    pub fn mbink_load_bytecode(handle: MBinkHandle, data: *const c_void, size: usize) -> c_int;
    pub fn mbink_compile_resources(input_path: *const c_char, output_file: *const c_char, encryption_key: *const c_char) -> c_int;
    pub fn mbink_load_resource_file(package_file: *const c_char, resource_path: *const c_char, encryption_key: *const c_char, out_data: *mut *mut c_void, out_size: *mut usize, out_flags: *mut u32) -> c_int;
    pub fn mbink_mount_resource_package(handle: MBinkHandle, package_file: *const c_char, encryption_key: *const c_char, mount_point: *const c_char) -> c_int;
    pub fn mbink_emit(handle: MBinkHandle, event_name: *const c_char, data_json: *const c_char) -> c_int;
    pub fn mbink_devtools_open(handle: MBinkHandle) -> c_int;
    pub fn mbink_devtools_close(handle: MBinkHandle) -> c_int;

    pub fn mbink_bind(handle: MBinkHandle, name: *const c_char, callback: MBinkCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_bind_async(handle: MBinkHandle, name: *const c_char, callback: MBinkAsyncCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_unbind(handle: MBinkHandle, name: *const c_char);
    pub fn mbink_on_resize(handle: MBinkHandle, callback: MBinkResizeCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_on_close(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_on_close_request(handle: MBinkHandle, callback: MBinkBoolCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_on_focus(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_on_blur(handle: MBinkHandle, callback: MBinkVoidCallback, user_data: *mut c_void) -> c_int;
    pub fn mbink_on_update(handle: MBinkHandle, callback: MBinkUpdateCallback, user_data: *mut c_void) -> c_int;

    pub fn mbink_state_exists(handle: MBinkHandle, name: *const c_char) -> bool;
    pub fn mbink_state_type(handle: MBinkHandle, name: *const c_char) -> MBinkType;
    pub fn mbink_state_delete(handle: MBinkHandle, name: *const c_char);
    pub fn mbink_state_get_bool(handle: MBinkHandle, name: *const c_char) -> bool;
    pub fn mbink_state_get_int(handle: MBinkHandle, name: *const c_char) -> i64;
    pub fn mbink_state_get_double(handle: MBinkHandle, name: *const c_char) -> f64;
    pub fn mbink_state_get_string(handle: MBinkHandle, name: *const c_char) -> *const c_char;
    pub fn mbink_state_get_json(handle: MBinkHandle, name: *const c_char) -> *mut c_char;
    pub fn mbink_state_set_null(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_set_bool(handle: MBinkHandle, name: *const c_char, value: bool) -> c_int;
    pub fn mbink_state_set_int(handle: MBinkHandle, name: *const c_char, value: i64) -> c_int;
    pub fn mbink_state_set_double(handle: MBinkHandle, name: *const c_char, value: f64) -> c_int;
    pub fn mbink_state_set_string(handle: MBinkHandle, name: *const c_char, value: *const c_char) -> c_int;
    pub fn mbink_state_set_json(handle: MBinkHandle, name: *const c_char, json: *const c_char) -> c_int;
    pub fn mbink_state_batch_begin(handle: MBinkHandle);
    pub fn mbink_state_batch_end(handle: MBinkHandle);

    pub fn mbink_shared_create(handle: MBinkHandle, name: *const c_char) -> MBinkSharedHandle;
    pub fn mbink_shared_destroy(shared: MBinkSharedHandle);
    pub fn mbink_shared_set_int(shared: MBinkSharedHandle, key: *const c_char, value: i64) -> c_int;
    pub fn mbink_shared_set_double(shared: MBinkSharedHandle, key: *const c_char, value: f64) -> c_int;
    pub fn mbink_shared_set_string(shared: MBinkSharedHandle, key: *const c_char, value: *const c_char) -> c_int;
    pub fn mbink_shared_set_bool(shared: MBinkSharedHandle, key: *const c_char, value: bool) -> c_int;
    pub fn mbink_shared_set_null(shared: MBinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mbink_shared_set_json(shared: MBinkSharedHandle, key: *const c_char, json: *const c_char) -> c_int;
    pub fn mbink_shared_get_int(shared: MBinkSharedHandle, key: *const c_char) -> i64;
    pub fn mbink_shared_get_double(shared: MBinkSharedHandle, key: *const c_char) -> f64;
    pub fn mbink_shared_get_string(shared: MBinkSharedHandle, key: *const c_char) -> *mut c_char;
    pub fn mbink_shared_get_bool(shared: MBinkSharedHandle, key: *const c_char) -> bool;
    pub fn mbink_shared_get_json(shared: MBinkSharedHandle, key: *const c_char) -> *mut c_char;
    pub fn mbink_shared_get_type(shared: MBinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mbink_shared_delete(shared: MBinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mbink_shared_has(shared: MBinkSharedHandle, key: *const c_char) -> bool;
    pub fn mbink_shared_batch_begin(shared: MBinkSharedHandle);
    pub fn mbink_shared_batch_end(shared: MBinkSharedHandle);

    pub fn mbink_copy_string(str_: *const c_char) -> *mut c_char;
    pub fn mbink_free(ptr: *mut c_void);
}
