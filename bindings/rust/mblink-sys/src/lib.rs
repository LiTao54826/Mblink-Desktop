#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

use std::ffi::{c_char, c_int, c_void};

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
use std::{env, path::PathBuf, sync::OnceLock};

#[cfg(all(target_os = "windows", mblink_runtime_load))]
mod windows_runtime;

#[cfg(all(target_os = "windows", mblink_runtime_load))]
pub use windows_runtime::*;

pub enum MBlinkWindow {}
pub type MBlinkHandle = *mut MBlinkWindow;

pub enum MBlinkSharedObject {}
pub type MBlinkSharedHandle = *mut MBlinkSharedObject;

pub enum MBlinkLogViewObject {}
pub type MBlinkLogViewHandle = *mut MBlinkLogViewObject;

pub enum MBlinkTerminalObject {}
pub type MBlinkTerminalHandle = *mut MBlinkTerminalObject;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBlinkError {
    MBLINK_OK = 0,
    MBLINK_ERROR_INVALID_HANDLE = -1,
    MBLINK_ERROR_INVALID_PARAM = -2,
    MBLINK_ERROR_NOT_FOUND = -3,
    MBLINK_ERROR_TYPE_MISMATCH = -4,
    MBLINK_ERROR_OUT_OF_RANGE = -5,
    MBLINK_ERROR_JS_ERROR = -6,
    MBLINK_ERROR_UNKNOWN = -99,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBlinkType {
    MBLINK_TYPE_NULL = 0,
    MBLINK_TYPE_BOOL = 1,
    MBLINK_TYPE_INT = 2,
    MBLINK_TYPE_DOUBLE = 3,
    MBLINK_TYPE_STRING = 4,
    MBLINK_TYPE_ARRAY = 5,
    MBLINK_TYPE_OBJECT = 6,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBlinkLifecycleState {
    MBLINK_LIFECYCLE_CREATED = 0,
    MBLINK_LIFECYCLE_LOADED = 1,
    MBLINK_LIFECYCLE_RUNNING = 2,
    MBLINK_LIFECYCLE_CLOSE_REQUESTED = 3,
    MBLINK_LIFECYCLE_STOPPED = 4,
    MBLINK_LIFECYCLE_DESTROYED = 5,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MBlinkObserveKind {
    MBLINK_OBSERVE_CONSOLE = 1,
    MBLINK_OBSERVE_ERROR = 2,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBlinkConfig {
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
pub struct MBlinkRuntimeOptions {
    pub runtime_epoch: *const c_char,
    pub load_embedded_runtime: bool,
    pub load_official_preact: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBlinkUiDevSnapshotOptions {
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
pub struct MBlinkDevToolsHttpOptions {
    pub bind_host: *const c_char,
    pub port: u16,
    pub auth_token: *const c_char,
    pub require_auth: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MBlinkDevToolsHttpInfo {
    pub port: u16,
    pub url: *mut c_char,
    pub auth_token: *mut c_char,
}

pub type MBlinkCallback = Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBlinkAsyncCallback =
    Option<unsafe extern "C" fn(*const c_char, *mut c_void) -> *mut c_char>;
pub type MBlinkStateCallback =
    Option<unsafe extern "C" fn(*const c_char, *const c_char, *mut c_void)>;
pub type MBlinkResizeCallback = Option<unsafe extern "C" fn(c_int, c_int, *mut c_void)>;
pub type MBlinkVoidCallback = Option<unsafe extern "C" fn(*mut c_void)>;
pub type MBlinkBoolCallback = Option<unsafe extern "C" fn(*mut c_void) -> bool>;
pub type MBlinkUpdateCallback = Option<unsafe extern "C" fn(f32, *mut c_void)>;
pub type MBlinkObserveCallback =
    Option<unsafe extern "C" fn(MBlinkObserveKind, *const c_char, *mut c_void)>;

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
extern "C" {
    pub fn mblink_init() -> c_int;
    pub fn mblink_cleanup();
    pub fn mblink_version() -> *const c_char;
    pub fn mblink_last_error() -> *const c_char;

    pub fn mblink_create(title: *const c_char, width: c_int, height: c_int) -> MBlinkHandle;
    pub fn mblink_create_ex(config: *const MBlinkConfig) -> MBlinkHandle;
    pub fn mblink_default_config() -> MBlinkConfig;
    pub fn mblink_destroy(handle: MBlinkHandle);
    pub fn mblink_run(handle: MBlinkHandle);
    pub fn mblink_stop(handle: MBlinkHandle);
    pub fn mblink_poll_events(handle: MBlinkHandle) -> bool;
    pub fn mblink_wait_events(handle: MBlinkHandle) -> bool;
    pub fn mblink_default_runtime_options() -> MBlinkRuntimeOptions;
    pub fn mblink_configure_runtime(
        handle: MBlinkHandle,
        options: *const MBlinkRuntimeOptions,
    ) -> c_int;
    pub fn mblink_load_embedded_runtime(handle: MBlinkHandle, include_official_preact: bool)
        -> c_int;
    pub fn mblink_load_entry_file(
        handle: MBlinkHandle,
        entry_path: *const c_char,
        execute_html_scripts: bool,
    ) -> c_int;
    pub fn mblink_load_module_file(handle: MBlinkHandle, entry_path: *const c_char) -> c_int;
    pub fn mblink_render_frame(handle: MBlinkHandle, passes: c_int) -> c_int;
    pub fn mblink_runtime_epoch(handle: MBlinkHandle, out_epoch: *mut *mut c_char) -> c_int;
    pub fn mblink_lifecycle_state(handle: MBlinkHandle) -> MBlinkLifecycleState;
    pub fn mblink_lifecycle_reason(handle: MBlinkHandle, out_reason: *mut *mut c_char) -> c_int;

    pub fn mblink_set_title(handle: MBlinkHandle, title: *const c_char) -> c_int;
    pub fn mblink_tray_create(handle: MBlinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mblink_tray_destroy(handle: MBlinkHandle) -> c_int;
    pub fn mblink_tray_set_tooltip(handle: MBlinkHandle, tooltip: *const c_char) -> c_int;
    pub fn mblink_tray_set_menu(handle: MBlinkHandle, menu_json: *const c_char) -> c_int;
    pub fn mblink_tray_set_left_click_callback(
        handle: MBlinkHandle,
        callback: MBlinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_tray_set_menu_callback(
        handle: MBlinkHandle,
        callback: MBlinkCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_set_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mblink_get_size(handle: MBlinkHandle, width: *mut c_int, height: *mut c_int) -> c_int;
    pub fn mblink_set_position(handle: MBlinkHandle, x: c_int, y: c_int) -> c_int;
    pub fn mblink_get_position(handle: MBlinkHandle, x: *mut c_int, y: *mut c_int) -> c_int;
    pub fn mblink_set_min_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mblink_set_max_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int;
    pub fn mblink_show(handle: MBlinkHandle) -> c_int;
    pub fn mblink_hide(handle: MBlinkHandle) -> c_int;
    pub fn mblink_minimize(handle: MBlinkHandle) -> c_int;
    pub fn mblink_maximize(handle: MBlinkHandle) -> c_int;
    pub fn mblink_restore(handle: MBlinkHandle) -> c_int;
    pub fn mblink_set_fullscreen(handle: MBlinkHandle, fullscreen: bool) -> c_int;
    pub fn mblink_set_resizable(handle: MBlinkHandle, resizable: bool) -> c_int;
    pub fn mblink_set_borderless(handle: MBlinkHandle, borderless: bool) -> c_int;
    pub fn mblink_set_always_on_top(handle: MBlinkHandle, on_top: bool) -> c_int;

    pub fn mblink_load_html(handle: MBlinkHandle, html: *const c_char) -> c_int;
    pub fn mblink_load_html_file(handle: MBlinkHandle, filepath: *const c_char) -> c_int;
    pub fn mblink_eval_js(handle: MBlinkHandle, code: *const c_char) -> c_int;
    pub fn mblink_eval_module(
        handle: MBlinkHandle,
        code: *const c_char,
        filename: *const c_char,
    ) -> c_int;
    pub fn mblink_load_js_file(handle: MBlinkHandle, filepath: *const c_char) -> c_int;
    pub fn mblink_load_bytecode(handle: MBlinkHandle, data: *const c_void, size: usize) -> c_int;
    pub fn mblink_compile_resources(
        input_path: *const c_char,
        output_file: *const c_char,
        encryption_key: *const c_char,
    ) -> c_int;
    pub fn mblink_load_resource_file(
        package_file: *const c_char,
        resource_path: *const c_char,
        encryption_key: *const c_char,
        out_data: *mut *mut c_void,
        out_size: *mut usize,
        out_flags: *mut u32,
    ) -> c_int;
    pub fn mblink_mount_resource_package(
        handle: MBlinkHandle,
        package_file: *const c_char,
        encryption_key: *const c_char,
        mount_point: *const c_char,
    ) -> c_int;
    pub fn mblink_emit(
        handle: MBlinkHandle,
        event_name: *const c_char,
        data_json: *const c_char,
    ) -> c_int;
    pub fn mblink_observe_set_callback(
        handle: MBlinkHandle,
        callback: MBlinkObserveCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_observe_console_json(handle: MBlinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mblink_observe_errors_json(handle: MBlinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mblink_observe_lifecycle_json(handle: MBlinkHandle, out_json: *mut *mut c_char) -> c_int;
    pub fn mblink_observe_clear(handle: MBlinkHandle, kind: MBlinkObserveKind) -> c_int;
    pub fn mblink_state_create_null(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_create_bool(handle: MBlinkHandle, name: *const c_char, value: bool) -> c_int;
    pub fn mblink_state_create_int(handle: MBlinkHandle, name: *const c_char, value: i64) -> c_int;
    pub fn mblink_state_create_double(handle: MBlinkHandle, name: *const c_char, value: f64)
        -> c_int;
    pub fn mblink_state_create_string(
        handle: MBlinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_state_create_array(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_create_object(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_create_json(
        handle: MBlinkHandle,
        name: *const c_char,
        json: *const c_char,
    ) -> c_int;

    pub fn mblink_bind(
        handle: MBlinkHandle,
        name: *const c_char,
        callback: MBlinkCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_bind_async(
        handle: MBlinkHandle,
        name: *const c_char,
        callback: MBlinkAsyncCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_unbind(handle: MBlinkHandle, name: *const c_char);
    pub fn mblink_on_resize(
        handle: MBlinkHandle,
        callback: MBlinkResizeCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_on_close(
        handle: MBlinkHandle,
        callback: MBlinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_on_close_request(
        handle: MBlinkHandle,
        callback: MBlinkBoolCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_on_focus(
        handle: MBlinkHandle,
        callback: MBlinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_on_blur(
        handle: MBlinkHandle,
        callback: MBlinkVoidCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_on_update(
        handle: MBlinkHandle,
        callback: MBlinkUpdateCallback,
        user_data: *mut c_void,
    ) -> c_int;

    pub fn mblink_state_exists(handle: MBlinkHandle, name: *const c_char) -> bool;
    pub fn mblink_state_type(handle: MBlinkHandle, name: *const c_char) -> MBlinkType;
    pub fn mblink_state_delete(handle: MBlinkHandle, name: *const c_char);
    pub fn mblink_state_get_bool(handle: MBlinkHandle, name: *const c_char) -> bool;
    pub fn mblink_state_get_int(handle: MBlinkHandle, name: *const c_char) -> i64;
    pub fn mblink_state_get_double(handle: MBlinkHandle, name: *const c_char) -> f64;
    pub fn mblink_state_get_string(handle: MBlinkHandle, name: *const c_char) -> *const c_char;
    pub fn mblink_state_get_length(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_get_json(handle: MBlinkHandle, name: *const c_char) -> *mut c_char;
    pub fn mblink_state_get_at(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
    ) -> *mut c_char;
    pub fn mblink_state_get_key(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
    ) -> *mut c_char;
    pub fn mblink_state_set_null(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_set_bool(handle: MBlinkHandle, name: *const c_char, value: bool) -> c_int;
    pub fn mblink_state_set_int(handle: MBlinkHandle, name: *const c_char, value: i64) -> c_int;
    pub fn mblink_state_set_double(handle: MBlinkHandle, name: *const c_char, value: f64) -> c_int;
    pub fn mblink_state_set_string(
        handle: MBlinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_state_set_json(
        handle: MBlinkHandle,
        name: *const c_char,
        json: *const c_char,
    ) -> c_int;
    pub fn mblink_state_array_push(
        handle: MBlinkHandle,
        name: *const c_char,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mblink_state_array_push_int(
        handle: MBlinkHandle,
        name: *const c_char,
        value: i64,
    ) -> c_int;
    pub fn mblink_state_array_push_double(
        handle: MBlinkHandle,
        name: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mblink_state_array_push_string(
        handle: MBlinkHandle,
        name: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_state_array_push_bool(
        handle: MBlinkHandle,
        name: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mblink_state_array_pop(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_array_shift(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_array_unshift(
        handle: MBlinkHandle,
        name: *const c_char,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mblink_state_array_remove(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
    ) -> c_int;
    pub fn mblink_state_array_clear(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_array_set(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
        item_json: *const c_char,
    ) -> c_int;
    pub fn mblink_state_array_set_int(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
        value: i64,
    ) -> c_int;
    pub fn mblink_state_array_set_double(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
        value: f64,
    ) -> c_int;
    pub fn mblink_state_array_set_string(
        handle: MBlinkHandle,
        name: *const c_char,
        index: c_int,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_state_object_set(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
        value_json: *const c_char,
    ) -> c_int;
    pub fn mblink_state_object_set_int(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: i64,
    ) -> c_int;
    pub fn mblink_state_object_set_double(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mblink_state_object_set_string(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_state_object_set_bool(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mblink_state_object_remove(
        handle: MBlinkHandle,
        name: *const c_char,
        key: *const c_char,
    ) -> c_int;
    pub fn mblink_state_object_clear(handle: MBlinkHandle, name: *const c_char) -> c_int;
    pub fn mblink_state_increment(handle: MBlinkHandle, name: *const c_char, delta: f64) -> c_int;
    pub fn mblink_state_multiply(handle: MBlinkHandle, name: *const c_char, factor: f64) -> c_int;
    pub fn mblink_state_string_append(
        handle: MBlinkHandle,
        name: *const c_char,
        suffix: *const c_char,
    ) -> c_int;
    pub fn mblink_state_string_prepend(
        handle: MBlinkHandle,
        name: *const c_char,
        prefix: *const c_char,
    ) -> c_int;
    pub fn mblink_state_watch(
        handle: MBlinkHandle,
        name: *const c_char,
        callback: MBlinkStateCallback,
        user_data: *mut c_void,
    ) -> c_int;
    pub fn mblink_state_unwatch(handle: MBlinkHandle, watch_id: c_int);
    pub fn mblink_state_batch_begin(handle: MBlinkHandle);
    pub fn mblink_state_batch_end(handle: MBlinkHandle);
    pub fn mblink_state_set_merge_mode(handle: MBlinkHandle, enable: bool);
    pub fn mblink_process_queue(handle: MBlinkHandle) -> c_int;
    pub fn mblink_queue_size(handle: MBlinkHandle) -> c_int;

    pub fn mblink_shared_create(handle: MBlinkHandle, name: *const c_char) -> MBlinkSharedHandle;
    pub fn mblink_shared_destroy(shared: MBlinkSharedHandle);
    pub fn mblink_shared_set_int(shared: MBlinkSharedHandle, key: *const c_char, value: i64)
        -> c_int;
    pub fn mblink_shared_set_double(
        shared: MBlinkSharedHandle,
        key: *const c_char,
        value: f64,
    ) -> c_int;
    pub fn mblink_shared_set_string(
        shared: MBlinkSharedHandle,
        key: *const c_char,
        value: *const c_char,
    ) -> c_int;
    pub fn mblink_shared_set_bool(
        shared: MBlinkSharedHandle,
        key: *const c_char,
        value: bool,
    ) -> c_int;
    pub fn mblink_shared_set_null(shared: MBlinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mblink_shared_set_json(
        shared: MBlinkSharedHandle,
        key: *const c_char,
        json: *const c_char,
    ) -> c_int;
    pub fn mblink_shared_get_int(shared: MBlinkSharedHandle, key: *const c_char) -> i64;
    pub fn mblink_shared_get_double(shared: MBlinkSharedHandle, key: *const c_char) -> f64;
    pub fn mblink_shared_get_string(shared: MBlinkSharedHandle, key: *const c_char) -> *mut c_char;
    pub fn mblink_shared_get_bool(shared: MBlinkSharedHandle, key: *const c_char) -> bool;
    pub fn mblink_shared_get_json(shared: MBlinkSharedHandle, key: *const c_char) -> *mut c_char;
    pub fn mblink_shared_get_type(shared: MBlinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mblink_shared_delete(shared: MBlinkSharedHandle, key: *const c_char) -> c_int;
    pub fn mblink_shared_has(shared: MBlinkSharedHandle, key: *const c_char) -> bool;
    pub fn mblink_shared_batch_begin(shared: MBlinkSharedHandle);
    pub fn mblink_shared_batch_end(shared: MBlinkSharedHandle);

    pub fn mblink_logview_get(handle: MBlinkHandle, element_id: *const c_char) -> MBlinkLogViewHandle;
    pub fn mblink_logview_destroy(logview: MBlinkLogViewHandle);
    pub fn mblink_logview_append(
        logview: MBlinkLogViewHandle,
        level: *const c_char,
        source: *const c_char,
        message: *const c_char,
    ) -> c_int;
    pub fn mblink_logview_clear(logview: MBlinkLogViewHandle);
    pub fn mblink_logview_export(logview: MBlinkLogViewHandle, format: *const c_char) -> *mut c_char;

    pub fn mblink_terminal_get(
        handle: MBlinkHandle,
        element_id: *const c_char,
    ) -> MBlinkTerminalHandle;
    pub fn mblink_terminal_destroy(terminal: MBlinkTerminalHandle);
    pub fn mblink_terminal_write(terminal: MBlinkTerminalHandle, data: *const c_char) -> c_int;
    pub fn mblink_terminal_clear(terminal: MBlinkTerminalHandle);
    pub fn mblink_terminal_execute(terminal: MBlinkTerminalHandle, command: *const c_char) -> c_int;
    pub fn mblink_terminal_start_shell(terminal: MBlinkTerminalHandle, shell: *const c_char)
        -> c_int;
    pub fn mblink_terminal_send_input(terminal: MBlinkTerminalHandle, input: *const c_char) -> c_int;
    pub fn mblink_terminal_resize(terminal: MBlinkTerminalHandle, rows: c_int, cols: c_int);
    pub fn mblink_terminal_serialize(terminal: MBlinkTerminalHandle) -> *mut c_char;

    pub fn mblink_copy_string(str_: *const c_char) -> *mut c_char;
    pub fn mblink_free(ptr: *mut c_void);
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
struct DevtoolsApi {
    _lib: libloading::Library,
    mblink_devtools_open: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_devtools_close: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_devtools_default_http_options: unsafe extern "C" fn() -> MBlinkDevToolsHttpOptions,
    mblink_devtools_http_start: unsafe extern "C" fn(
        MBlinkHandle,
        *const MBlinkDevToolsHttpOptions,
        *mut MBlinkDevToolsHttpInfo,
    ) -> c_int,
    mblink_devtools_http_stop: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_devtools_http_info_free: unsafe extern "C" fn(*mut MBlinkDevToolsHttpInfo),
    mblink_ui_dev_default_snapshot_options: unsafe extern "C" fn() -> MBlinkUiDevSnapshotOptions,
    mblink_ui_dev_snapshot_json: unsafe extern "C" fn(
        MBlinkHandle,
        *const MBlinkUiDevSnapshotOptions,
        *mut *mut c_char,
    ) -> c_int,
    mblink_ui_dev_snapshot_file:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const MBlinkUiDevSnapshotOptions) -> c_int,
    mblink_ui_dev_command_json:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *mut *mut c_char) -> c_int,
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
static DEVTOOLS_API: OnceLock<DevtoolsApi> = OnceLock::new();

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
fn devtools_api() -> &'static DevtoolsApi {
    DEVTOOLS_API.get_or_init(|| unsafe { load_devtools_api() })
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
unsafe fn load_devtools_api() -> DevtoolsApi {
    let path = devtools_library_path();
    let lib = libloading::Library::new(&path).unwrap_or_else(|err| {
        panic!(
            "failed to load mblink_devtools dynamic library at {}: {err}",
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

    let mblink_devtools_open = load!(
        b"mblink_devtools_open\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_devtools_close = load!(
        b"mblink_devtools_close\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_devtools_default_http_options = load!(
        b"mblink_devtools_default_http_options\0",
        unsafe extern "C" fn() -> MBlinkDevToolsHttpOptions
    );
    let mblink_devtools_http_start = load!(
        b"mblink_devtools_http_start\0",
        unsafe extern "C" fn(
            MBlinkHandle,
            *const MBlinkDevToolsHttpOptions,
            *mut MBlinkDevToolsHttpInfo,
        ) -> c_int
    );
    let mblink_devtools_http_stop = load!(
        b"mblink_devtools_http_stop\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_devtools_http_info_free = load!(
        b"mblink_devtools_http_info_free\0",
        unsafe extern "C" fn(*mut MBlinkDevToolsHttpInfo)
    );
    let mblink_ui_dev_default_snapshot_options = load!(
        b"mblink_ui_dev_default_snapshot_options\0",
        unsafe extern "C" fn() -> MBlinkUiDevSnapshotOptions
    );
    let mblink_ui_dev_snapshot_json = load!(
        b"mblink_ui_dev_snapshot_json\0",
        unsafe extern "C" fn(
            MBlinkHandle,
            *const MBlinkUiDevSnapshotOptions,
            *mut *mut c_char,
        ) -> c_int
    );
    let mblink_ui_dev_snapshot_file = load!(
        b"mblink_ui_dev_snapshot_file\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const MBlinkUiDevSnapshotOptions) -> c_int
    );
    let mblink_ui_dev_command_json = load!(
        b"mblink_ui_dev_command_json\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *mut *mut c_char) -> c_int
    );

    DevtoolsApi {
        _lib: lib,
        mblink_devtools_open,
        mblink_devtools_close,
        mblink_devtools_default_http_options,
        mblink_devtools_http_start,
        mblink_devtools_http_stop,
        mblink_devtools_http_info_free,
        mblink_ui_dev_default_snapshot_options,
        mblink_ui_dev_snapshot_json,
        mblink_ui_dev_snapshot_file,
        mblink_ui_dev_command_json,
    }
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
fn devtools_library_path() -> PathBuf {
    let names = devtools_library_names();

    if let Some(path) = env::var_os("MBLINK_DEVTOOLS_PATH") {
        let path = PathBuf::from(path);
        if !path.is_absolute() {
            panic!("MBLINK_DEVTOOLS_PATH must be an absolute path");
        }
        if path.is_dir() {
            return first_existing_in_dir(&path, &names).unwrap_or_else(|| {
                panic!(
                    "mblink_devtools dynamic library not found in {}",
                    path.display()
                )
            });
        }
        return std::fs::canonicalize(&path).unwrap_or_else(|_| {
            panic!(
                "mblink_devtools dynamic library not found at {}",
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

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
fn devtools_library_names() -> [&'static str; 1] {
    if cfg!(target_os = "windows") {
        ["mblink_devtools.dll"]
    } else if cfg!(target_os = "macos") {
        ["libmblink_devtools.dylib"]
    } else {
        ["libmblink_devtools.so"]
    }
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
fn devtools_search_dirs() -> Vec<PathBuf> {
    let mut dirs = Vec::new();

    if let Some(path) = env::var_os("MBLINK_DLL_PATH") {
        let path = PathBuf::from(path);
        if path.is_absolute() {
            if path.is_dir() {
                dirs.push(path);
            } else if let Some(parent) = path.parent() {
                dirs.push(parent.to_path_buf());
            }
        }
    }

    if let Some(path) = env::var_os("MBLINK_LIB_DIR") {
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

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
fn first_existing_in_dir(dir: &PathBuf, names: &[&str]) -> Option<PathBuf> {
    names
        .iter()
        .map(|name| dir.join(name))
        .find(|path| path.exists())
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_open(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_open)(handle)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_close(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_close)(handle)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_default_http_options() -> MBlinkDevToolsHttpOptions {
    (devtools_api().mblink_devtools_default_http_options)()
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_http_start(
    handle: MBlinkHandle,
    options: *const MBlinkDevToolsHttpOptions,
    out_info: *mut MBlinkDevToolsHttpInfo,
) -> c_int {
    (devtools_api().mblink_devtools_http_start)(handle, options, out_info)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_http_stop(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_http_stop)(handle)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_devtools_http_info_free(info: *mut MBlinkDevToolsHttpInfo) {
    (devtools_api().mblink_devtools_http_info_free)(info)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_ui_dev_default_snapshot_options() -> MBlinkUiDevSnapshotOptions {
    (devtools_api().mblink_ui_dev_default_snapshot_options)()
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_ui_dev_snapshot_json(
    handle: MBlinkHandle,
    options: *const MBlinkUiDevSnapshotOptions,
    out_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mblink_ui_dev_snapshot_json)(handle, options, out_json)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_ui_dev_snapshot_file(
    handle: MBlinkHandle,
    output_path: *const c_char,
    options: *const MBlinkUiDevSnapshotOptions,
) -> c_int {
    (devtools_api().mblink_ui_dev_snapshot_file)(handle, output_path, options)
}

#[cfg(not(all(target_os = "windows", mblink_runtime_load)))]
pub unsafe fn mblink_ui_dev_command_json(
    handle: MBlinkHandle,
    command_json: *const c_char,
    out_response_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mblink_ui_dev_command_json)(handle, command_json, out_response_json)
}
