#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

use std::ffi::{c_char, c_int, c_void};

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
use std::{env, path::PathBuf, sync::OnceLock};

#[cfg(all(target_os = "windows", mbink_runtime_load))]
mod windows_runtime;

#[cfg(all(target_os = "windows", mbink_runtime_load))]
pub use windows_runtime::*;

pub enum MBinkWindow {}
pub type MBinkHandle = *mut MBinkWindow;

pub enum MBinkSharedObject {}
pub type MBinkSharedHandle = *mut MBinkSharedObject;

pub enum MBinkLogViewObject {}
pub type MBinkLogViewHandle = *mut MBinkLogViewObject;

pub enum MBinkTerminalObject {}
pub type MBinkTerminalHandle = *mut MBinkTerminalObject;

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
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBinkLifecycleState {
    MBINK_LIFECYCLE_CREATED = 0,
    MBINK_LIFECYCLE_LOADED = 1,
    MBINK_LIFECYCLE_RUNNING = 2,
    MBINK_LIFECYCLE_CLOSE_REQUESTED = 3,
    MBINK_LIFECYCLE_STOPPED = 4,
    MBINK_LIFECYCLE_DESTROYED = 5,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBinkObserveKind {
    MBINK_OBSERVE_CONSOLE = 1,
    MBINK_OBSERVE_ERROR = 2,
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

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBinkRuntimeOptions {
    pub runtime_epoch: *const c_char,
    pub load_embedded_runtime: bool,
    pub load_official_preact: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBinkUiDevSnapshotOptions {
    pub runtime_epoch: *const c_char,
    pub max_nodes: usize,
    pub max_depth: c_int,
    pub root_selector: *const c_char,
    pub include_screenshot: bool,
    pub inline_screenshot: bool,
    pub screenshot_file: *const c_char,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBinkDevToolsHttpOptions {
    pub bind_host: *const c_char,
    pub port: u16,
    pub auth_token: *const c_char,
    pub require_auth: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBinkDevToolsHttpInfo {
    pub port: u16,
    pub url: *mut c_char,
    pub auth_token: *mut c_char,
}

pub type MBinkCallback = Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBinkAsyncCallback =
    Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBinkStateCallback =
    Option<unsafe extern "C" fn(*const c_char, *const c_char, *mut c_void)>;
pub type MBinkResizeCallback = Option<unsafe extern "C" fn(c_int, c_int, *mut c_void)>;
pub type MBinkVoidCallback = Option<unsafe extern "C" fn(*mut c_void)>;
pub type MBinkBoolCallback = Option<unsafe extern "C" fn(*mut c_void) -> bool>;
pub type MBinkUpdateCallback = Option<unsafe extern "C" fn(f32, *mut c_void)>;
pub type MBinkObserveCallback =
    Option<unsafe extern "C" fn(MBinkObserveKind, *const c_char, *mut c_void)>;

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
    pub fn mbink_wait_events(handle: MBinkHandle) -> bool;
    pub fn mbink_default_runtime_options() -> MBinkRuntimeOptions;
    pub fn mbink_configure_runtime(
        handle: MBinkHandle,
        options: *const MBinkRuntimeOptions,
    ) -> c_int;
    pub fn mbink_load_embedded_runtime(handle: MBinkHandle, include_official_preact: bool)
        -> c_int;
    pub fn mbink_load_entry_file(
        handle: MBinkHandle,
        entry_path: *const c_char,
        execute_html_scripts: bool,
    ) -> c_int;
    pub fn mbink_load_module_file(handle: MBinkHandle, entry_path: *const c_char) -> c_int;
    pub fn mbink_render_frame(handle: MBinkHandle, passes: c_int) -> c_int;
    pub fn mbink_runtime_epoch(handle: MBinkHandle, out_epoch: *mut *mut c_char) -> c_int;
    pub fn mbink_lifecycle_state(handle: MBinkHandle) -> MBinkLifecycleState;
    pub fn mbink_lifecycle_reason(handle: MBinkHandle, out_reason: *mut *mut c_char) -> c_int;

    pub fn mbink_set_title(handle: MBinkHandle, title: *const c_char) -> c_int;
    pub fn mbink_tray_create(handle: MBinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mbink_tray_destroy(handle: MBinkHandle) -> c_int;
    pub fn mbink_tray_set_tooltip(handle: MBinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mbink_tray_set_menu(handle: MBinkHandle, menu_json: *const c_char) -> c_int;
    pub fn mbink_tray_set_left_click_callback(
        handle: MBinkHandle,
        callback: MBinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_tray_set_menu_callback(
        handle: MBinkHandle,
        callback: MBinkCallback,
        user_data: *mut c_void,
    ) -> c_int;
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
    pub fn mbink_eval_module(
        handle: MBinkHandle,
        code: *const c_char,
        filename: *const c_char,
    ) -> c_int;
    pub fn mbink_load_js_file(handle: MBinkHandle, filepath: *const c_char) -> c_int;
    pub fn mbink_load_bytecode(handle: MBinkHandle, data: *const c_void, size: usize) -> c_int;
    pub fn mbink_compile_resources(
        input_path: *const c_char,
        output_file: *const c_char,
        encryption_key: *const c_char,
    ) -> c_int;
    pub fn mbink_load_resource_file(
        package_file: *const c_char,
        resource_path: *const c_char,
        encryption_key: *const c_char,
        out_data: *mut *mut c_void,
        out_size: *mut usize,
        out_flags: *mut u32,
    ) -> c_int;
    pub fn mbink_mount_resource_package(
        handle: MBinkHandle,
        package_file: *const c_char,
        encryption_key: *const c_char,
        mount_point: *const c_char,
    ) -> c_int;
    pub fn mbink_emit(
        handle: MBinkHandle,
        event_name: *const c_char,
        data_json: *const c_char,
    ) -> c_int;
    pub fn mbink_observe_set_callback(
        handle: MBinkHandle,
        callback: MBinkObserveCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_observe_console_json(handle: MBinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mbink_observe_errors_json(handle: MBinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mbink_observe_lifecycle_json(handle: MBinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mbink_observe_clear(handle: MBinkHandle, kind: MBinkObserveKind) -> c_int;
    pub fn mbink_state_create_null(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_create_bool(handle: MBinkHandle, name: *const c_char, value: bool) -> c_int;
    pub fn mbink_state_create_int(handle: MBinkHandle, name: *const c_char, value: i64) -> c_int;
    pub fn mbink_state_create_double(handle: MBinkHandle, name: *const c_char, value: f64)
        -> c_int;
    pub fn mbink_state_create_string(
        handle: MBinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_state_create_array(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_create_object(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_create_json(
        handle: MBinkHandle,
        name: *const c_char,
        json: *const c_char,
    ) -> c_int;

    pub fn mbink_bind(
        handle: MBinkHandle,
        name: *const c_char,
        callback: MBinkCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_bind_async(
        handle: MBinkHandle,
        name: *const c_char,
        callback: MBinkAsyncCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_unbind(handle: MBinkHandle, name: *const c_char);
    pub fn mbink_on_resize(
        handle: MBinkHandle,
        callback: MBinkResizeCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_on_close(
        handle: MBinkHandle,
        callback: MBinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_on_close_request(
        handle: MBinkHandle,
        callback: MBinkBoolCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_on_focus(
        handle: MBinkHandle,
        callback: MBinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_on_blur(
        handle: MBinkHandle,
        callback: MBinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_on_update(
        handle: MBinkHandle,
        callback: MBinkUpdateCallback,
        user_data: *mut c_void,
    ) -> c_int;

    pub fn mbink_state_exists(handle: MBinkHandle, name: *const c_char) -> bool;
    pub fn mbink_state_type(handle: MBinkHandle, name: *const c_char) -> MBinkType;
    pub fn mbink_state_delete(handle: MBinkHandle, name: *const c_char);
    pub fn mbink_state_get_bool(handle: MBinkHandle, name: *const c_char) -> bool;
    pub fn mbink_state_get_int(handle: MBinkHandle, name: *const c_char) -> i64;
    pub fn mbink_state_get_double(handle: MBinkHandle, name: *const c_char) -> f64;
    pub fn mbink_state_get_string(handle: MBinkHandle, name: *const c_char) -> *const c_char;
    pub fn mbink_state_get_length(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_get_json(handle: MBinkHandle, name: *const c_char) -> *mut c_char;
    pub fn mbink_state_get_at(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
    ) -> *mut c_char;
    pub fn mbink_state_get_key(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
    ) -> *mut c_char;
    pub fn mbink_state_set_null(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_set_bool(handle: MBinkHandle, name: *const c_char, value: bool) -> c_int;
    pub fn mbink_state_set_int(handle: MBinkHandle, name: *const c_char, value: i64) -> c_int;
    pub fn mbink_state_set_double(handle: MBinkHandle, name: *const c_char, value: f64) -> c_int;
    pub fn mbink_state_set_string(
        handle: MBinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_state_set_json(
        handle: MBinkHandle,
        name: *const c_char,
        json: *const c_char,
    ) -> c_int;
    pub fn mbink_state_array_push(
        handle: MBinkHandle,
        name: *const c_char,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mbink_state_array_push_int(
        handle: MBinkHandle,
        name: *const c_char,
        value: i64,
    ) -> c_int;
    pub fn mbink_state_array_push_double(
        handle: MBinkHandle,
        name: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mbink_state_array_push_string(
        handle: MBinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_state_array_push_bool(
        handle: MBinkHandle,
        name: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mbink_state_array_pop(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_array_shift(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_array_unshift(
        handle: MBinkHandle,
        name: *const c_char,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mbink_state_array_remove(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
    ) -> c_int;
    pub fn mbink_state_array_clear(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_array_set(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mbink_state_array_set_int(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
        value: i64,
    ) -> c_int;
    pub fn mbink_state_array_set_double(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
        value: f64,
    ) -> c_int;
    pub fn mbink_state_array_set_string(
        handle: MBinkHandle,
        name: *const c_char,
        index: c_int,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_state_object_set(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
        value_json: *const c_char,
    ) -> c_int;
    pub fn mbink_state_object_set_int(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: i64,
    ) -> c_int;
    pub fn mbink_state_object_set_double(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mbink_state_object_set_string(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_state_object_set_bool(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mbink_state_object_remove(
        handle: MBinkHandle,
        name: *const c_char,
        key: *const c_char,
    ) -> c_int;
    pub fn mbink_state_object_clear(handle: MBinkHandle, name: *const c_char) -> c_int;
    pub fn mbink_state_increment(handle: MBinkHandle, name: *const c_char, delta: f64) -> c_int;
    pub fn mbink_state_multiply(handle: MBinkHandle, name: *const c_char, factor: f64) -> c_int;
    pub fn mbink_state_string_append(
        handle: MBinkHandle,
        name: *const c_char,
        suffix: *const c_char,
    ) -> c_int;
    pub fn mbink_state_string_prepend(
        handle: MBinkHandle,
        name: *const c_char,
        prefix: *const c_char,
    ) -> c_int;
    pub fn mbink_state_watch(
        handle: MBinkHandle,
        name: *const c_char,
        callback: MBinkStateCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mbink_state_unwatch(handle: MBinkHandle, watch_id: c_int);
    pub fn mbink_state_batch_begin(handle: MBinkHandle);
    pub fn mbink_state_batch_end(handle: MBinkHandle);
    pub fn mbink_state_set_merge_mode(handle: MBinkHandle, enable: bool);
    pub fn mbink_process_queue(handle: MBinkHandle) -> c_int;
    pub fn mbink_queue_size(handle: MBinkHandle) -> c_int;

    pub fn mbink_shared_create(handle: MBinkHandle, name: *const c_char) -> MBinkSharedHandle;
    pub fn mbink_shared_destroy(shared: MBinkSharedHandle);
    pub fn mbink_shared_set_int(shared: MBinkSharedHandle, key: *const c_char, value: i64)
        -> c_int;
    pub fn mbink_shared_set_double(
        shared: MBinkSharedHandle,
        key: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mbink_shared_set_string(
        shared: MBinkSharedHandle,
        key: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mbink_shared_set_bool(
        shared: MBinkSharedHandle,
        key: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mbink_shared_set_null(shared: MBinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mbink_shared_set_json(
        shared: MBinkSharedHandle,
        key: *const c_char,
        json: *const c_char,
    ) -> c_int;
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

    pub fn mbink_logview_get(handle: MBinkHandle, element_id: *const c_char) -> MBinkLogViewHandle;
    pub fn mbink_logview_destroy(logview: MBinkLogViewHandle);
    pub fn mbink_logview_append(
        logview: MBinkLogViewHandle,
        level: *const c_char,
        source: *const c_char,
        message: *const c_char,
    ) -> c_int;
    pub fn mbink_logview_clear(logview: MBinkLogViewHandle);
    pub fn mbink_logview_export(logview: MBinkLogViewHandle, format: *const c_char) -> *mut c_char;

    pub fn mbink_terminal_get(
        handle: MBinkHandle,
        element_id: *const c_char,
    ) -> MBinkTerminalHandle;
    pub fn mbink_terminal_destroy(terminal: MBinkTerminalHandle);
    pub fn mbink_terminal_write(terminal: MBinkTerminalHandle, data: *const c_char) -> c_int;
    pub fn mbink_terminal_clear(terminal: MBinkTerminalHandle);
    pub fn mbink_terminal_execute(terminal: MBinkTerminalHandle, command: *const c_char) -> c_int;
    pub fn mbink_terminal_start_shell(terminal: MBinkTerminalHandle, shell: *const c_char)
        -> c_int;
    pub fn mbink_terminal_send_input(terminal: MBinkTerminalHandle, input: *const c_char) -> c_int;
    pub fn mbink_terminal_resize(terminal: MBinkTerminalHandle, rows: c_int, cols: c_int);
    pub fn mbink_terminal_serialize(terminal: MBinkTerminalHandle) -> *mut c_char;

    pub fn mbink_copy_string(str_: *const c_char) -> *mut c_char;
    pub fn mbink_free(ptr: *mut c_void);
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
struct DevtoolsApi {
    _lib: libloading::Library,
    mbink_devtools_open: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_devtools_close: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_devtools_default_http_options: unsafe extern "C" fn() -> MBinkDevToolsHttpOptions,
    mbink_devtools_http_start: unsafe extern "C" fn(
        MBinkHandle,
        *const MBinkDevToolsHttpOptions,
        *mut MBinkDevToolsHttpInfo,
    ) -> c_int,
    mbink_devtools_http_stop: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_devtools_http_info_free: unsafe extern "C" fn(*mut MBinkDevToolsHttpInfo),
    mbink_ui_dev_default_snapshot_options: unsafe extern "C" fn() -> MBinkUiDevSnapshotOptions,
    mbink_ui_dev_snapshot_json: unsafe extern "C" fn(
        MBinkHandle,
        *const MBinkUiDevSnapshotOptions,
        *mut *mut c_char,
    ) -> c_int,
    mbink_ui_dev_snapshot_file:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const MBinkUiDevSnapshotOptions) -> c_int,
    mbink_ui_dev_command_json:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *mut *mut c_char) -> c_int,
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
static DEVTOOLS_API: OnceLock<DevtoolsApi> = OnceLock::new();

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
fn devtools_api() -> &'static DevtoolsApi {
    DEVTOOLS_API.get_or_init(|| unsafe { load_devtools_api() })
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
unsafe fn load_devtools_api() -> DevtoolsApi {
    let path = devtools_library_path();
    let lib = libloading::Library::new(&path).unwrap_or_else(|err| {
        panic!(
            "failed to load mbink_devtools dynamic library at {}: {err}",
            path.display()
        )
    });

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

    let mbink_devtools_open = load!(
        b"mbink_devtools_open\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_devtools_close = load!(
        b"mbink_devtools_close\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_devtools_default_http_options = load!(
        b"mbink_devtools_default_http_options\0",
        unsafe extern "C" fn() -> MBinkDevToolsHttpOptions
    );
    let mbink_devtools_http_start = load!(
        b"mbink_devtools_http_start\0",
        unsafe extern "C" fn(
            MBinkHandle,
            *const MBinkDevToolsHttpOptions,
            *mut MBinkDevToolsHttpInfo,
        ) -> c_int
    );
    let mbink_devtools_http_stop = load!(
        b"mbink_devtools_http_stop\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_devtools_http_info_free = load!(
        b"mbink_devtools_http_info_free\0",
        unsafe extern "C" fn(*mut MBinkDevToolsHttpInfo)
    );
    let mbink_ui_dev_default_snapshot_options = load!(
        b"mbink_ui_dev_default_snapshot_options\0",
        unsafe extern "C" fn() -> MBinkUiDevSnapshotOptions
    );
    let mbink_ui_dev_snapshot_json = load!(
        b"mbink_ui_dev_snapshot_json\0",
        unsafe extern "C" fn(
            MBinkHandle,
            *const MBinkUiDevSnapshotOptions,
            *mut *mut c_char,
        ) -> c_int
    );
    let mbink_ui_dev_snapshot_file = load!(
        b"mbink_ui_dev_snapshot_file\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const MBinkUiDevSnapshotOptions) -> c_int
    );
    let mbink_ui_dev_command_json = load!(
        b"mbink_ui_dev_command_json\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *mut *mut c_char) -> c_int
    );

    DevtoolsApi {
        _lib: lib,
        mbink_devtools_open,
        mbink_devtools_close,
        mbink_devtools_default_http_options,
        mbink_devtools_http_start,
        mbink_devtools_http_stop,
        mbink_devtools_http_info_free,
        mbink_ui_dev_default_snapshot_options,
        mbink_ui_dev_snapshot_json,
        mbink_ui_dev_snapshot_file,
        mbink_ui_dev_command_json,
    }
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
fn devtools_library_path() -> PathBuf {
    let names = devtools_library_names();

    if let Some(path) = env::var_os("MBINK_DEVTOOLS_PATH") {
        let path = PathBuf::from(path);
        if !path.is_absolute() {
            panic!("MBINK_DEVTOOLS_PATH must be an absolute path");
        }
        if path.is_dir() {
            return first_existing_in_dir(&path, &names).unwrap_or_else(|| {
                panic!(
                    "mbink_devtools dynamic library not found in {}",
                    path.display()
                )
            });
        }
        return std::fs::canonicalize(&path).unwrap_or_else(|_| {
            panic!(
                "mbink_devtools dynamic library not found at {}",
                path.display()
            )
        });
    }

    for dir in devtools_search_dirs() {
        if let Some(candidate) = first_existing_in_dir(&dir, &names) {
            return candidate;
        }
    }

    PathBuf::from(names[0])
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
fn devtools_library_names() -> [&'static str; 1] {
    if cfg!(target_os = "windows") {
        ["mbink_devtools.dll"]
    } else if cfg!(target_os = "macos") {
        ["libmbink_devtools.dylib"]
    } else {
        ["libmbink_devtools.so"]
    }
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
fn devtools_search_dirs() -> Vec<PathBuf> {
    let mut dirs = Vec::new();

    if let Some(path) = env::var_os("MBINK_DLL_PATH") {
        let path = PathBuf::from(path);
        if path.is_absolute() {
            if path.is_dir() {
                dirs.push(path);
            } else if let Some(parent) = path.parent() {
                dirs.push(parent.to_path_buf());
            }
        }
    }

    if let Some(path) = env::var_os("MBINK_LIB_DIR") {
        dirs.push(PathBuf::from(path));
    }

    if let Some(exe_dir) = env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|p| p.to_path_buf()))
    {
        dirs.push(exe_dir);
    }

    dirs.push(PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("runtime"));
    dirs
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
fn first_existing_in_dir(dir: &PathBuf, names: &[&str]) -> Option<PathBuf> {
    names
        .iter()
        .map(|name| dir.join(name))
        .find(|path| path.exists())
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_open(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_open)(handle)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_close(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_close)(handle)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_default_http_options() -> MBinkDevToolsHttpOptions {
    (devtools_api().mbink_devtools_default_http_options)()
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_http_start(
    handle: MBinkHandle,
    options: *const MBinkDevToolsHttpOptions,
    out_info: *mut MBinkDevToolsHttpInfo,
) -> c_int {
    (devtools_api().mbink_devtools_http_start)(handle, options, out_info)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_http_stop(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_http_stop)(handle)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_devtools_http_info_free(info: *mut MBinkDevToolsHttpInfo) {
    (devtools_api().mbink_devtools_http_info_free)(info)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_ui_dev_default_snapshot_options() -> MBinkUiDevSnapshotOptions {
    (devtools_api().mbink_ui_dev_default_snapshot_options)()
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_ui_dev_snapshot_json(
    handle: MBinkHandle,
    options: *const MBinkUiDevSnapshotOptions,
    out_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mbink_ui_dev_snapshot_json)(handle, options, out_json)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_ui_dev_snapshot_file(
    handle: MBinkHandle,
    output_path: *const c_char,
    options: *const MBinkUiDevSnapshotOptions,
) -> c_int {
    (devtools_api().mbink_ui_dev_snapshot_file)(handle, output_path, options)
}

#[cfg(not(all(target_os = "windows", mbink_runtime_load)))]
pub unsafe fn mbink_ui_dev_command_json(
    handle: MBinkHandle,
    command_json: *const c_char,
    out_response_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mbink_ui_dev_command_json)(handle, command_json, out_response_json)
}
