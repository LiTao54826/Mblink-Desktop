#![allow(non_snake_case)]
use std::env;

use std::ffi::{c_char, c_int, c_void};
use std::path::PathBuf;
use std::sync::OnceLock;

use super::{
    MBinkAsyncCallback, MBinkBoolCallback, MBinkCallback, MBinkConfig, MBinkDevToolsHttpInfo,
    MBinkDevToolsHttpOptions, MBinkHandle, MBinkLifecycleState, MBinkLogViewHandle,
    MBinkObserveCallback, MBinkObserveKind, MBinkResizeCallback, MBinkRuntimeOptions,
    MBinkSharedHandle, MBinkStateCallback, MBinkTerminalHandle, MBinkType,
    MBinkUiDevSnapshotOptions, MBinkUpdateCallback, MBinkVoidCallback,
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
    mbink_default_runtime_options: unsafe extern "C" fn() -> MBinkRuntimeOptions,
    mbink_configure_runtime: unsafe extern "C" fn(MBinkHandle, *const MBinkRuntimeOptions) -> c_int,
    mbink_load_embedded_runtime: unsafe extern "C" fn(MBinkHandle, bool) -> c_int,
    mbink_load_entry_file: unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int,
    mbink_load_module_file: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_render_frame: unsafe extern "C" fn(MBinkHandle, c_int) -> c_int,
    mbink_runtime_epoch: unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int,
    mbink_lifecycle_state: unsafe extern "C" fn(MBinkHandle) -> MBinkLifecycleState,
    mbink_lifecycle_reason: unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int,
    mbink_set_title: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_tray_create: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_tray_destroy: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_tray_set_tooltip: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_tray_set_menu: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_tray_set_left_click_callback:
        unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int,
    mbink_tray_set_menu_callback:
        unsafe extern "C" fn(MBinkHandle, MBinkCallback, *mut c_void) -> c_int,
    mbink_set_size: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_get_size: unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int,
    mbink_set_position: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_get_position: unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int,
    mbink_set_min_size: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_set_max_size: unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int,
    mbink_show: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_hide: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_minimize: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_maximize: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_restore: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_set_fullscreen: unsafe extern "C" fn(MBinkHandle, bool) -> c_int,
    mbink_set_resizable: unsafe extern "C" fn(MBinkHandle, bool) -> c_int,
    mbink_set_borderless: unsafe extern "C" fn(MBinkHandle, bool) -> c_int,
    mbink_set_always_on_top: unsafe extern "C" fn(MBinkHandle, bool) -> c_int,
    mbink_load_html: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_load_html_file: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_eval_js: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_eval_module: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_load_js_file: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_load_bytecode: unsafe extern "C" fn(MBinkHandle, *const c_void, usize) -> c_int,
    mbink_compile_resources:
        unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int,
    mbink_load_resource_file: MBinkLoadResourceFileFn,
    mbink_mount_resource_package:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mbink_emit: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_observe_set_callback:
        unsafe extern "C" fn(MBinkHandle, MBinkObserveCallback, *mut c_void) -> c_int,
    mbink_observe_console_json: unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int,
    mbink_observe_errors_json: unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int,
    mbink_observe_lifecycle_json: unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int,
    mbink_observe_clear: unsafe extern "C" fn(MBinkHandle, MBinkObserveKind) -> c_int,
    mbink_state_create_null: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_create_bool: unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int,
    mbink_state_create_int: unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int,
    mbink_state_create_double: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_create_string:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_create_array: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_create_object: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_create_json:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_bind:
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkCallback, *mut c_void) -> c_int,
    mbink_bind_async:
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkAsyncCallback, *mut c_void) -> c_int,
    mbink_unbind: unsafe extern "C" fn(MBinkHandle, *const c_char),
    mbink_on_resize: unsafe extern "C" fn(MBinkHandle, MBinkResizeCallback, *mut c_void) -> c_int,
    mbink_on_close: unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int,
    mbink_on_close_request:
        unsafe extern "C" fn(MBinkHandle, MBinkBoolCallback, *mut c_void) -> c_int,
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
    mbink_state_get_length: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_get_json: unsafe extern "C" fn(MBinkHandle, *const c_char) -> *mut c_char,
    mbink_state_get_at: unsafe extern "C" fn(MBinkHandle, *const c_char, c_int) -> *mut c_char,
    mbink_state_get_key:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> *mut c_char,
    mbink_state_set_null: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_set_bool: unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int,
    mbink_state_set_int: unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int,
    mbink_state_set_double: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_set_string:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_set_json: unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_array_push:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_array_push_int: unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int,
    mbink_state_array_push_double: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_array_push_string:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_array_push_bool: unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int,
    mbink_state_array_pop: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_array_shift: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_array_unshift:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_array_remove: unsafe extern "C" fn(MBinkHandle, *const c_char, c_int) -> c_int,
    mbink_state_array_clear: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_array_set:
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, *const c_char) -> c_int,
    mbink_state_array_set_int:
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, i64) -> c_int,
    mbink_state_array_set_double:
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, f64) -> c_int,
    mbink_state_array_set_string:
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, *const c_char) -> c_int,
    mbink_state_object_set:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mbink_state_object_set_int:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, i64) -> c_int,
    mbink_state_object_set_double:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, f64) -> c_int,
    mbink_state_object_set_string:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mbink_state_object_set_bool:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, bool) -> c_int,
    mbink_state_object_remove:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_object_clear: unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int,
    mbink_state_increment: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_multiply: unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int,
    mbink_state_string_append:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_string_prepend:
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int,
    mbink_state_watch:
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkStateCallback, *mut c_void) -> c_int,
    mbink_state_unwatch: unsafe extern "C" fn(MBinkHandle, c_int),
    mbink_state_batch_begin: unsafe extern "C" fn(MBinkHandle),
    mbink_state_batch_end: unsafe extern "C" fn(MBinkHandle),
    mbink_state_set_merge_mode: unsafe extern "C" fn(MBinkHandle, bool),
    mbink_process_queue: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_queue_size: unsafe extern "C" fn(MBinkHandle) -> c_int,
    mbink_shared_create: unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkSharedHandle,
    mbink_shared_destroy: unsafe extern "C" fn(MBinkSharedHandle),
    mbink_shared_set_int: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, i64) -> c_int,
    mbink_shared_set_double: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, f64) -> c_int,
    mbink_shared_set_string:
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int,
    mbink_shared_set_bool: unsafe extern "C" fn(MBinkSharedHandle, *const c_char, bool) -> c_int,
    mbink_shared_set_null: unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int,
    mbink_shared_set_json:
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int,
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
    mbink_logview_get: unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkLogViewHandle,
    mbink_logview_destroy: unsafe extern "C" fn(MBinkLogViewHandle),
    mbink_logview_append: unsafe extern "C" fn(
        MBinkLogViewHandle,
        *const c_char,
        *const c_char,
        *const c_char,
    ) -> c_int,
    mbink_logview_clear: unsafe extern "C" fn(MBinkLogViewHandle),
    mbink_logview_export: unsafe extern "C" fn(MBinkLogViewHandle, *const c_char) -> *mut c_char,
    mbink_terminal_get: unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkTerminalHandle,
    mbink_terminal_destroy: unsafe extern "C" fn(MBinkTerminalHandle),
    mbink_terminal_write: unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int,
    mbink_terminal_clear: unsafe extern "C" fn(MBinkTerminalHandle),
    mbink_terminal_execute: unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int,
    mbink_terminal_start_shell: unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int,
    mbink_terminal_send_input: unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int,
    mbink_terminal_resize: unsafe extern "C" fn(MBinkTerminalHandle, c_int, c_int),
    mbink_terminal_serialize: unsafe extern "C" fn(MBinkTerminalHandle) -> *mut c_char,
    mbink_copy_string: unsafe extern "C" fn(*const c_char) -> *mut c_char,
    mbink_free: unsafe extern "C" fn(*mut c_void),
}

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

static API: OnceLock<Api> = OnceLock::new();
static DEVTOOLS_API: OnceLock<DevtoolsApi> = OnceLock::new();

fn api() -> &'static Api {
    API.get_or_init(|| unsafe { load_api() })
}

fn devtools_api() -> &'static DevtoolsApi {
    DEVTOOLS_API.get_or_init(|| unsafe { load_devtools_api() })
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
    let mbink_last_error = load!(
        b"mbink_last_error\0",
        unsafe extern "C" fn() -> *const c_char
    );
    let mbink_create = load!(
        b"mbink_create\0",
        unsafe extern "C" fn(*const c_char, c_int, c_int) -> MBinkHandle
    );
    let mbink_create_ex = load!(
        b"mbink_create_ex\0",
        unsafe extern "C" fn(*const MBinkConfig) -> MBinkHandle
    );
    let mbink_default_config = load!(
        b"mbink_default_config\0",
        unsafe extern "C" fn() -> MBinkConfig
    );
    let mbink_destroy = load!(b"mbink_destroy\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_run = load!(b"mbink_run\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_stop = load!(b"mbink_stop\0", unsafe extern "C" fn(MBinkHandle));
    let mbink_poll_events = load!(
        b"mbink_poll_events\0",
        unsafe extern "C" fn(MBinkHandle) -> bool
    );
    let mbink_default_runtime_options = load!(
        b"mbink_default_runtime_options\0",
        unsafe extern "C" fn() -> MBinkRuntimeOptions
    );
    let mbink_configure_runtime = load!(
        b"mbink_configure_runtime\0",
        unsafe extern "C" fn(MBinkHandle, *const MBinkRuntimeOptions) -> c_int
    );
    let mbink_load_embedded_runtime = load!(
        b"mbink_load_embedded_runtime\0",
        unsafe extern "C" fn(MBinkHandle, bool) -> c_int
    );
    let mbink_load_entry_file = load!(
        b"mbink_load_entry_file\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int
    );
    let mbink_load_module_file = load!(
        b"mbink_load_module_file\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_render_frame = load!(
        b"mbink_render_frame\0",
        unsafe extern "C" fn(MBinkHandle, c_int) -> c_int
    );
    let mbink_runtime_epoch = load!(
        b"mbink_runtime_epoch\0",
        unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int
    );
    let mbink_lifecycle_state = load!(
        b"mbink_lifecycle_state\0",
        unsafe extern "C" fn(MBinkHandle) -> MBinkLifecycleState
    );
    let mbink_lifecycle_reason = load!(
        b"mbink_lifecycle_reason\0",
        unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int
    );
    let mbink_set_title = load!(
        b"mbink_set_title\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_tray_create = load!(
        b"mbink_tray_create\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_tray_destroy = load!(
        b"mbink_tray_destroy\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_tray_set_tooltip = load!(
        b"mbink_tray_set_tooltip\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_tray_set_menu = load!(
        b"mbink_tray_set_menu\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_tray_set_left_click_callback = load!(
        b"mbink_tray_set_left_click_callback\0",
        unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int
    );
    let mbink_tray_set_menu_callback = load!(
        b"mbink_tray_set_menu_callback\0",
        unsafe extern "C" fn(MBinkHandle, MBinkCallback, *mut c_void) -> c_int
    );
    let mbink_set_size = load!(
        b"mbink_set_size\0",
        unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int
    );
    let mbink_get_size = load!(
        b"mbink_get_size\0",
        unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int
    );
    let mbink_set_position = load!(
        b"mbink_set_position\0",
        unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int
    );
    let mbink_get_position = load!(
        b"mbink_get_position\0",
        unsafe extern "C" fn(MBinkHandle, *mut c_int, *mut c_int) -> c_int
    );
    let mbink_set_min_size = load!(
        b"mbink_set_min_size\0",
        unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int
    );
    let mbink_set_max_size = load!(
        b"mbink_set_max_size\0",
        unsafe extern "C" fn(MBinkHandle, c_int, c_int) -> c_int
    );
    let mbink_show = load!(b"mbink_show\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_hide = load!(b"mbink_hide\0", unsafe extern "C" fn(MBinkHandle) -> c_int);
    let mbink_minimize = load!(
        b"mbink_minimize\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_maximize = load!(
        b"mbink_maximize\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_restore = load!(
        b"mbink_restore\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_set_fullscreen = load!(
        b"mbink_set_fullscreen\0",
        unsafe extern "C" fn(MBinkHandle, bool) -> c_int
    );
    let mbink_set_resizable = load!(
        b"mbink_set_resizable\0",
        unsafe extern "C" fn(MBinkHandle, bool) -> c_int
    );
    let mbink_set_borderless = load!(
        b"mbink_set_borderless\0",
        unsafe extern "C" fn(MBinkHandle, bool) -> c_int
    );
    let mbink_set_always_on_top = load!(
        b"mbink_set_always_on_top\0",
        unsafe extern "C" fn(MBinkHandle, bool) -> c_int
    );
    let mbink_load_html = load!(
        b"mbink_load_html\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_load_html_file = load!(
        b"mbink_load_html_file\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_eval_js = load!(
        b"mbink_eval_js\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_eval_module = load!(
        b"mbink_eval_module\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_load_js_file = load!(
        b"mbink_load_js_file\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_load_bytecode = load!(
        b"mbink_load_bytecode\0",
        unsafe extern "C" fn(MBinkHandle, *const c_void, usize) -> c_int
    );
    let mbink_compile_resources = load!(
        b"mbink_compile_resources\0",
        unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int
    );
    let mbink_load_resource_file = load!(b"mbink_load_resource_file\0", MBinkLoadResourceFileFn);
    let mbink_mount_resource_package = load!(
        b"mbink_mount_resource_package\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mbink_emit = load!(
        b"mbink_emit\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_observe_set_callback = load!(
        b"mbink_observe_set_callback\0",
        unsafe extern "C" fn(MBinkHandle, MBinkObserveCallback, *mut c_void) -> c_int
    );
    let mbink_observe_console_json = load!(
        b"mbink_observe_console_json\0",
        unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int
    );
    let mbink_observe_errors_json = load!(
        b"mbink_observe_errors_json\0",
        unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int
    );
    let mbink_observe_lifecycle_json = load!(
        b"mbink_observe_lifecycle_json\0",
        unsafe extern "C" fn(MBinkHandle, *mut *mut c_char) -> c_int
    );
    let mbink_observe_clear = load!(
        b"mbink_observe_clear\0",
        unsafe extern "C" fn(MBinkHandle, MBinkObserveKind) -> c_int
    );
    let mbink_state_create_null = load!(
        b"mbink_state_create_null\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_create_bool = load!(
        b"mbink_state_create_bool\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int
    );
    let mbink_state_create_int = load!(
        b"mbink_state_create_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int
    );
    let mbink_state_create_double = load!(
        b"mbink_state_create_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int
    );
    let mbink_state_create_string = load!(
        b"mbink_state_create_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_create_array = load!(
        b"mbink_state_create_array\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_create_object = load!(
        b"mbink_state_create_object\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_create_json = load!(
        b"mbink_state_create_json\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_bind = load!(
        b"mbink_bind\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkCallback, *mut c_void) -> c_int
    );
    let mbink_bind_async = load!(
        b"mbink_bind_async\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkAsyncCallback, *mut c_void) -> c_int
    );
    let mbink_unbind = load!(
        b"mbink_unbind\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char)
    );
    let mbink_on_resize = load!(
        b"mbink_on_resize\0",
        unsafe extern "C" fn(MBinkHandle, MBinkResizeCallback, *mut c_void) -> c_int
    );
    let mbink_on_close = load!(
        b"mbink_on_close\0",
        unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int
    );
    let mbink_on_close_request = load!(
        b"mbink_on_close_request\0",
        unsafe extern "C" fn(MBinkHandle, MBinkBoolCallback, *mut c_void) -> c_int
    );
    let mbink_on_focus = load!(
        b"mbink_on_focus\0",
        unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int
    );
    let mbink_on_blur = load!(
        b"mbink_on_blur\0",
        unsafe extern "C" fn(MBinkHandle, MBinkVoidCallback, *mut c_void) -> c_int
    );
    let mbink_on_update = load!(
        b"mbink_on_update\0",
        unsafe extern "C" fn(MBinkHandle, MBinkUpdateCallback, *mut c_void) -> c_int
    );
    let mbink_state_exists = load!(
        b"mbink_state_exists\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool
    );
    let mbink_state_type = load!(
        b"mbink_state_type\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkType
    );
    let mbink_state_delete = load!(
        b"mbink_state_delete\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char)
    );
    let mbink_state_get_bool = load!(
        b"mbink_state_get_bool\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> bool
    );
    let mbink_state_get_int = load!(
        b"mbink_state_get_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> i64
    );
    let mbink_state_get_double = load!(
        b"mbink_state_get_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> f64
    );
    let mbink_state_get_string = load!(
        b"mbink_state_get_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> *const c_char
    );
    let mbink_state_get_length = load!(
        b"mbink_state_get_length\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_get_json = load!(
        b"mbink_state_get_json\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> *mut c_char
    );
    let mbink_state_get_at = load!(
        b"mbink_state_get_at\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int) -> *mut c_char
    );
    let mbink_state_get_key = load!(
        b"mbink_state_get_key\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> *mut c_char
    );
    let mbink_state_set_null = load!(
        b"mbink_state_set_null\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_set_bool = load!(
        b"mbink_state_set_bool\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int
    );
    let mbink_state_set_int = load!(
        b"mbink_state_set_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int
    );
    let mbink_state_set_double = load!(
        b"mbink_state_set_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int
    );
    let mbink_state_set_string = load!(
        b"mbink_state_set_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_set_json = load!(
        b"mbink_state_set_json\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_array_push = load!(
        b"mbink_state_array_push\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_array_push_int = load!(
        b"mbink_state_array_push_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, i64) -> c_int
    );
    let mbink_state_array_push_double = load!(
        b"mbink_state_array_push_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int
    );
    let mbink_state_array_push_string = load!(
        b"mbink_state_array_push_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_array_push_bool = load!(
        b"mbink_state_array_push_bool\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, bool) -> c_int
    );
    let mbink_state_array_pop = load!(
        b"mbink_state_array_pop\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_array_shift = load!(
        b"mbink_state_array_shift\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_array_unshift = load!(
        b"mbink_state_array_unshift\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_array_remove = load!(
        b"mbink_state_array_remove\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int) -> c_int
    );
    let mbink_state_array_clear = load!(
        b"mbink_state_array_clear\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_array_set = load!(
        b"mbink_state_array_set\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, *const c_char) -> c_int
    );
    let mbink_state_array_set_int = load!(
        b"mbink_state_array_set_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, i64) -> c_int
    );
    let mbink_state_array_set_double = load!(
        b"mbink_state_array_set_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, f64) -> c_int
    );
    let mbink_state_array_set_string = load!(
        b"mbink_state_array_set_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, c_int, *const c_char) -> c_int
    );
    let mbink_state_object_set = load!(
        b"mbink_state_object_set\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_object_set_int = load!(
        b"mbink_state_object_set_int\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, i64) -> c_int
    );
    let mbink_state_object_set_double = load!(
        b"mbink_state_object_set_double\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, f64) -> c_int
    );
    let mbink_state_object_set_string = load!(
        b"mbink_state_object_set_string\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_object_set_bool = load!(
        b"mbink_state_object_set_bool\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char, bool) -> c_int
    );
    let mbink_state_object_remove = load!(
        b"mbink_state_object_remove\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_object_clear = load!(
        b"mbink_state_object_clear\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> c_int
    );
    let mbink_state_increment = load!(
        b"mbink_state_increment\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int
    );
    let mbink_state_multiply = load!(
        b"mbink_state_multiply\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, f64) -> c_int
    );
    let mbink_state_string_append = load!(
        b"mbink_state_string_append\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_string_prepend = load!(
        b"mbink_state_string_prepend\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_state_watch = load!(
        b"mbink_state_watch\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char, MBinkStateCallback, *mut c_void) -> c_int
    );
    let mbink_state_unwatch = load!(
        b"mbink_state_unwatch\0",
        unsafe extern "C" fn(MBinkHandle, c_int)
    );
    let mbink_state_batch_begin = load!(
        b"mbink_state_batch_begin\0",
        unsafe extern "C" fn(MBinkHandle)
    );
    let mbink_state_batch_end = load!(
        b"mbink_state_batch_end\0",
        unsafe extern "C" fn(MBinkHandle)
    );
    let mbink_state_set_merge_mode = load!(
        b"mbink_state_set_merge_mode\0",
        unsafe extern "C" fn(MBinkHandle, bool)
    );
    let mbink_process_queue = load!(
        b"mbink_process_queue\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_queue_size = load!(
        b"mbink_queue_size\0",
        unsafe extern "C" fn(MBinkHandle) -> c_int
    );
    let mbink_shared_create = load!(
        b"mbink_shared_create\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkSharedHandle
    );
    let mbink_shared_destroy = load!(
        b"mbink_shared_destroy\0",
        unsafe extern "C" fn(MBinkSharedHandle)
    );
    let mbink_shared_set_int = load!(
        b"mbink_shared_set_int\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, i64) -> c_int
    );
    let mbink_shared_set_double = load!(
        b"mbink_shared_set_double\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, f64) -> c_int
    );
    let mbink_shared_set_string = load!(
        b"mbink_shared_set_string\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_shared_set_bool = load!(
        b"mbink_shared_set_bool\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, bool) -> c_int
    );
    let mbink_shared_set_null = load!(
        b"mbink_shared_set_null\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int
    );
    let mbink_shared_set_json = load!(
        b"mbink_shared_set_json\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char, *const c_char) -> c_int
    );
    let mbink_shared_get_int = load!(
        b"mbink_shared_get_int\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> i64
    );
    let mbink_shared_get_double = load!(
        b"mbink_shared_get_double\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> f64
    );
    let mbink_shared_get_string = load!(
        b"mbink_shared_get_string\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char
    );
    let mbink_shared_get_bool = load!(
        b"mbink_shared_get_bool\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool
    );
    let mbink_shared_get_json = load!(
        b"mbink_shared_get_json\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> *mut c_char
    );
    let mbink_shared_get_type = load!(
        b"mbink_shared_get_type\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int
    );
    let mbink_shared_delete = load!(
        b"mbink_shared_delete\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> c_int
    );
    let mbink_shared_has = load!(
        b"mbink_shared_has\0",
        unsafe extern "C" fn(MBinkSharedHandle, *const c_char) -> bool
    );
    let mbink_shared_batch_begin = load!(
        b"mbink_shared_batch_begin\0",
        unsafe extern "C" fn(MBinkSharedHandle)
    );
    let mbink_shared_batch_end = load!(
        b"mbink_shared_batch_end\0",
        unsafe extern "C" fn(MBinkSharedHandle)
    );
    let mbink_logview_get = load!(
        b"mbink_logview_get\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkLogViewHandle
    );
    let mbink_logview_destroy = load!(
        b"mbink_logview_destroy\0",
        unsafe extern "C" fn(MBinkLogViewHandle)
    );
    let mbink_logview_append = load!(
        b"mbink_logview_append\0",
        unsafe extern "C" fn(
            MBinkLogViewHandle,
            *const c_char,
            *const c_char,
            *const c_char,
        ) -> c_int
    );
    let mbink_logview_clear = load!(
        b"mbink_logview_clear\0",
        unsafe extern "C" fn(MBinkLogViewHandle)
    );
    let mbink_logview_export = load!(
        b"mbink_logview_export\0",
        unsafe extern "C" fn(MBinkLogViewHandle, *const c_char) -> *mut c_char
    );
    let mbink_terminal_get = load!(
        b"mbink_terminal_get\0",
        unsafe extern "C" fn(MBinkHandle, *const c_char) -> MBinkTerminalHandle
    );
    let mbink_terminal_destroy = load!(
        b"mbink_terminal_destroy\0",
        unsafe extern "C" fn(MBinkTerminalHandle)
    );
    let mbink_terminal_write = load!(
        b"mbink_terminal_write\0",
        unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int
    );
    let mbink_terminal_clear = load!(
        b"mbink_terminal_clear\0",
        unsafe extern "C" fn(MBinkTerminalHandle)
    );
    let mbink_terminal_execute = load!(
        b"mbink_terminal_execute\0",
        unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int
    );
    let mbink_terminal_start_shell = load!(
        b"mbink_terminal_start_shell\0",
        unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int
    );
    let mbink_terminal_send_input = load!(
        b"mbink_terminal_send_input\0",
        unsafe extern "C" fn(MBinkTerminalHandle, *const c_char) -> c_int
    );
    let mbink_terminal_resize = load!(
        b"mbink_terminal_resize\0",
        unsafe extern "C" fn(MBinkTerminalHandle, c_int, c_int)
    );
    let mbink_terminal_serialize = load!(
        b"mbink_terminal_serialize\0",
        unsafe extern "C" fn(MBinkTerminalHandle) -> *mut c_char
    );
    let mbink_copy_string = load!(
        b"mbink_copy_string\0",
        unsafe extern "C" fn(*const c_char) -> *mut c_char
    );
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
        mbink_default_runtime_options,
        mbink_configure_runtime,
        mbink_load_embedded_runtime,
        mbink_load_entry_file,
        mbink_load_module_file,
        mbink_render_frame,
        mbink_runtime_epoch,
        mbink_lifecycle_state,
        mbink_lifecycle_reason,
        mbink_set_title,
        mbink_tray_create,
        mbink_tray_destroy,
        mbink_tray_set_tooltip,
        mbink_tray_set_menu,
        mbink_tray_set_left_click_callback,
        mbink_tray_set_menu_callback,
        mbink_set_size,
        mbink_get_size,
        mbink_set_position,
        mbink_get_position,
        mbink_set_min_size,
        mbink_set_max_size,
        mbink_show,
        mbink_hide,
        mbink_minimize,
        mbink_maximize,
        mbink_restore,
        mbink_set_fullscreen,
        mbink_set_resizable,
        mbink_set_borderless,
        mbink_set_always_on_top,
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
        mbink_observe_set_callback,
        mbink_observe_console_json,
        mbink_observe_errors_json,
        mbink_observe_lifecycle_json,
        mbink_observe_clear,
        mbink_state_create_null,
        mbink_state_create_bool,
        mbink_state_create_int,
        mbink_state_create_double,
        mbink_state_create_string,
        mbink_state_create_array,
        mbink_state_create_object,
        mbink_state_create_json,
        mbink_bind,
        mbink_bind_async,
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
        mbink_state_get_length,
        mbink_state_get_json,
        mbink_state_get_at,
        mbink_state_get_key,
        mbink_state_set_null,
        mbink_state_set_bool,
        mbink_state_set_int,
        mbink_state_set_double,
        mbink_state_set_string,
        mbink_state_set_json,
        mbink_state_array_push,
        mbink_state_array_push_int,
        mbink_state_array_push_double,
        mbink_state_array_push_string,
        mbink_state_array_push_bool,
        mbink_state_array_pop,
        mbink_state_array_shift,
        mbink_state_array_unshift,
        mbink_state_array_remove,
        mbink_state_array_clear,
        mbink_state_array_set,
        mbink_state_array_set_int,
        mbink_state_array_set_double,
        mbink_state_array_set_string,
        mbink_state_object_set,
        mbink_state_object_set_int,
        mbink_state_object_set_double,
        mbink_state_object_set_string,
        mbink_state_object_set_bool,
        mbink_state_object_remove,
        mbink_state_object_clear,
        mbink_state_increment,
        mbink_state_multiply,
        mbink_state_string_append,
        mbink_state_string_prepend,
        mbink_state_watch,
        mbink_state_unwatch,
        mbink_state_batch_begin,
        mbink_state_batch_end,
        mbink_state_set_merge_mode,
        mbink_process_queue,
        mbink_queue_size,
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
        mbink_logview_get,
        mbink_logview_destroy,
        mbink_logview_append,
        mbink_logview_clear,
        mbink_logview_export,
        mbink_terminal_get,
        mbink_terminal_destroy,
        mbink_terminal_write,
        mbink_terminal_clear,
        mbink_terminal_execute,
        mbink_terminal_start_shell,
        mbink_terminal_send_input,
        mbink_terminal_resize,
        mbink_terminal_serialize,
        mbink_copy_string,
        mbink_free,
    }
}

unsafe fn load_devtools_api() -> DevtoolsApi {
    let path = devtools_dll_path();
    let lib = libloading::Library::new(&path).unwrap_or_else(|err| {
        panic!(
            "failed to load mbink_devtools.dll at {}: {err}",
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

fn dll_path() -> PathBuf {
    if let Some(path) = env::var_os("MBINK_DLL_PATH") {
        return PathBuf::from(path);
    }

    if let Some(path) = dll_next_to_exe() {
        return path;
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let runtime_candidate = manifest_dir.join("runtime").join("mbink.dll");
    if runtime_candidate.exists() {
        return runtime_candidate;
    }

    PathBuf::from("mbink.dll")
}

fn devtools_dll_path() -> PathBuf {
    if let Some(path) = env::var_os("MBINK_DEVTOOLS_PATH") {
        let path = PathBuf::from(path);
        if !path.is_absolute() {
            panic!("MBINK_DEVTOOLS_PATH must be an absolute path");
        }
        if path.is_dir() {
            let candidate = path.join("mbink_devtools.dll");
            return std::fs::canonicalize(&candidate).unwrap_or_else(|_| {
                panic!("mbink_devtools.dll not found at {}", candidate.display())
            });
        }
        return std::fs::canonicalize(&path)
            .unwrap_or_else(|_| panic!("mbink_devtools.dll not found at {}", path.display()));
    }

    let core_path = dll_path();
    if core_path.is_absolute() && core_path.exists() {
        if let Some(parent) = core_path.parent() {
            let candidate = parent.join("mbink_devtools.dll");
            if candidate.exists() {
                return candidate;
            }
        }
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let runtime_core = manifest_dir.join("runtime").join("mbink.dll");
    if runtime_core.exists() {
        let candidate = runtime_core.with_file_name("mbink_devtools.dll");
        if candidate.exists() {
            return candidate;
        }
    }

    if let Some(candidate) = devtools_dll_next_to_exe() {
        return candidate;
    }

    let runtime_candidate = manifest_dir.join("runtime").join("mbink_devtools.dll");
    if runtime_candidate.exists() {
        return runtime_candidate;
    }

    panic!(
        "mbink_devtools.dll not found. Set MBINK_DEVTOOLS_PATH or place it next to mbink.dll"
    )
}

fn dll_next_to_exe() -> Option<PathBuf> {
    let exe_dir = env::current_exe().ok()?.parent()?.to_path_buf();
    let candidate = exe_dir.join("mbink.dll");
    candidate.exists().then_some(candidate)
}

fn devtools_dll_next_to_exe() -> Option<PathBuf> {
    let exe_dir = env::current_exe().ok()?.parent()?.to_path_buf();
    let core_candidate = exe_dir.join("mbink.dll");
    if !core_candidate.exists() {
        return None;
    }
    let candidate = exe_dir.join("mbink_devtools.dll");
    candidate.exists().then_some(candidate)
}

pub unsafe fn mbink_init() -> c_int {
    (api().mbink_init)()
}
pub unsafe fn mbink_cleanup() {
    (api().mbink_cleanup)()
}
pub unsafe fn mbink_version() -> *const c_char {
    (api().mbink_version)()
}
pub unsafe fn mbink_last_error() -> *const c_char {
    (api().mbink_last_error)()
}
pub unsafe fn mbink_create(title: *const c_char, width: c_int, height: c_int) -> MBinkHandle {
    (api().mbink_create)(title, width, height)
}
pub unsafe fn mbink_create_ex(config: *const MBinkConfig) -> MBinkHandle {
    (api().mbink_create_ex)(config)
}
pub unsafe fn mbink_default_config() -> MBinkConfig {
    (api().mbink_default_config)()
}
pub unsafe fn mbink_destroy(handle: MBinkHandle) {
    (api().mbink_destroy)(handle)
}
pub unsafe fn mbink_run(handle: MBinkHandle) {
    (api().mbink_run)(handle)
}
pub unsafe fn mbink_stop(handle: MBinkHandle) {
    (api().mbink_stop)(handle)
}
pub unsafe fn mbink_poll_events(handle: MBinkHandle) -> bool {
    (api().mbink_poll_events)(handle)
}
pub unsafe fn mbink_default_runtime_options() -> MBinkRuntimeOptions {
    (api().mbink_default_runtime_options)()
}
pub unsafe fn mbink_configure_runtime(
    handle: MBinkHandle,
    options: *const MBinkRuntimeOptions,
) -> c_int {
    (api().mbink_configure_runtime)(handle, options)
}
pub unsafe fn mbink_load_embedded_runtime(
    handle: MBinkHandle,
    include_official_preact: bool,
) -> c_int {
    (api().mbink_load_embedded_runtime)(handle, include_official_preact)
}
pub unsafe fn mbink_load_entry_file(
    handle: MBinkHandle,
    entry_path: *const c_char,
    execute_html_scripts: bool,
) -> c_int {
    (api().mbink_load_entry_file)(handle, entry_path, execute_html_scripts)
}
pub unsafe fn mbink_load_module_file(handle: MBinkHandle, entry_path: *const c_char) -> c_int {
    (api().mbink_load_module_file)(handle, entry_path)
}
pub unsafe fn mbink_render_frame(handle: MBinkHandle, passes: c_int) -> c_int {
    (api().mbink_render_frame)(handle, passes)
}
pub unsafe fn mbink_runtime_epoch(handle: MBinkHandle, out_epoch: *mut *mut c_char) -> c_int {
    (api().mbink_runtime_epoch)(handle, out_epoch)
}
pub unsafe fn mbink_lifecycle_state(handle: MBinkHandle) -> MBinkLifecycleState {
    (api().mbink_lifecycle_state)(handle)
}
pub unsafe fn mbink_lifecycle_reason(handle: MBinkHandle, out_reason: *mut *mut c_char) -> c_int {
    (api().mbink_lifecycle_reason)(handle, out_reason)
}
pub unsafe fn mbink_set_title(handle: MBinkHandle, title: *const c_char) -> c_int {
    (api().mbink_set_title)(handle, title)
}
pub unsafe fn mbink_tray_create(handle: MBinkHandle, tooltip: *const c_char) -> c_int {
    (api().mbink_tray_create)(handle, tooltip)
}
pub unsafe fn mbink_tray_destroy(handle: MBinkHandle) -> c_int {
    (api().mbink_tray_destroy)(handle)
}
pub unsafe fn mbink_tray_set_tooltip(handle: MBinkHandle, tooltip: *const c_char) -> c_int {
    (api().mbink_tray_set_tooltip)(handle, tooltip)
}
pub unsafe fn mbink_tray_set_menu(handle: MBinkHandle, menu_json: *const c_char) -> c_int {
    (api().mbink_tray_set_menu)(handle, menu_json)
}
pub unsafe fn mbink_tray_set_left_click_callback(
    handle: MBinkHandle,
    callback: MBinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_tray_set_left_click_callback)(handle, callback, user_data)
}
pub unsafe fn mbink_tray_set_menu_callback(
    handle: MBinkHandle,
    callback: MBinkCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_tray_set_menu_callback)(handle, callback, user_data)
}
pub unsafe fn mbink_set_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mbink_set_size)(handle, width, height)
}
pub unsafe fn mbink_get_size(handle: MBinkHandle, width: *mut c_int, height: *mut c_int) -> c_int {
    (api().mbink_get_size)(handle, width, height)
}
pub unsafe fn mbink_set_position(handle: MBinkHandle, x: c_int, y: c_int) -> c_int {
    (api().mbink_set_position)(handle, x, y)
}
pub unsafe fn mbink_get_position(handle: MBinkHandle, x: *mut c_int, y: *mut c_int) -> c_int {
    (api().mbink_get_position)(handle, x, y)
}
pub unsafe fn mbink_set_min_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mbink_set_min_size)(handle, width, height)
}
pub unsafe fn mbink_set_max_size(handle: MBinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mbink_set_max_size)(handle, width, height)
}
pub unsafe fn mbink_show(handle: MBinkHandle) -> c_int {
    (api().mbink_show)(handle)
}
pub unsafe fn mbink_hide(handle: MBinkHandle) -> c_int {
    (api().mbink_hide)(handle)
}
pub unsafe fn mbink_minimize(handle: MBinkHandle) -> c_int {
    (api().mbink_minimize)(handle)
}
pub unsafe fn mbink_maximize(handle: MBinkHandle) -> c_int {
    (api().mbink_maximize)(handle)
}
pub unsafe fn mbink_restore(handle: MBinkHandle) -> c_int {
    (api().mbink_restore)(handle)
}
pub unsafe fn mbink_set_fullscreen(handle: MBinkHandle, fullscreen: bool) -> c_int {
    (api().mbink_set_fullscreen)(handle, fullscreen)
}
pub unsafe fn mbink_set_resizable(handle: MBinkHandle, resizable: bool) -> c_int {
    (api().mbink_set_resizable)(handle, resizable)
}
pub unsafe fn mbink_set_borderless(handle: MBinkHandle, borderless: bool) -> c_int {
    (api().mbink_set_borderless)(handle, borderless)
}
pub unsafe fn mbink_set_always_on_top(handle: MBinkHandle, on_top: bool) -> c_int {
    (api().mbink_set_always_on_top)(handle, on_top)
}
pub unsafe fn mbink_load_html(handle: MBinkHandle, html: *const c_char) -> c_int {
    (api().mbink_load_html)(handle, html)
}
pub unsafe fn mbink_load_html_file(handle: MBinkHandle, filepath: *const c_char) -> c_int {
    (api().mbink_load_html_file)(handle, filepath)
}
pub unsafe fn mbink_eval_js(handle: MBinkHandle, code: *const c_char) -> c_int {
    (api().mbink_eval_js)(handle, code)
}
pub unsafe fn mbink_eval_module(
    handle: MBinkHandle,
    code: *const c_char,
    filename: *const c_char,
) -> c_int {
    (api().mbink_eval_module)(handle, code, filename)
}
pub unsafe fn mbink_load_js_file(handle: MBinkHandle, filepath: *const c_char) -> c_int {
    (api().mbink_load_js_file)(handle, filepath)
}
pub unsafe fn mbink_load_bytecode(handle: MBinkHandle, data: *const c_void, size: usize) -> c_int {
    (api().mbink_load_bytecode)(handle, data, size)
}
pub unsafe fn mbink_compile_resources(
    input_path: *const c_char,
    output_file: *const c_char,
    encryption_key: *const c_char,
) -> c_int {
    (api().mbink_compile_resources)(input_path, output_file, encryption_key)
}
pub unsafe fn mbink_load_resource_file(
    package_file: *const c_char,
    resource_path: *const c_char,
    encryption_key: *const c_char,
    out_data: *mut *mut c_void,
    out_size: *mut usize,
    out_flags: *mut u32,
) -> c_int {
    (api().mbink_load_resource_file)(
        package_file,
        resource_path,
        encryption_key,
        out_data,
        out_size,
        out_flags,
    )
}
pub unsafe fn mbink_mount_resource_package(
    handle: MBinkHandle,
    package_file: *const c_char,
    encryption_key: *const c_char,
    mount_point: *const c_char,
) -> c_int {
    (api().mbink_mount_resource_package)(handle, package_file, encryption_key, mount_point)
}
pub unsafe fn mbink_emit(
    handle: MBinkHandle,
    event_name: *const c_char,
    data_json: *const c_char,
) -> c_int {
    (api().mbink_emit)(handle, event_name, data_json)
}
pub unsafe fn mbink_devtools_open(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_open)(handle)
}
pub unsafe fn mbink_devtools_close(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_close)(handle)
}
pub unsafe fn mbink_devtools_default_http_options() -> MBinkDevToolsHttpOptions {
    (devtools_api().mbink_devtools_default_http_options)()
}
pub unsafe fn mbink_devtools_http_start(
    handle: MBinkHandle,
    options: *const MBinkDevToolsHttpOptions,
    out_info: *mut MBinkDevToolsHttpInfo,
) -> c_int {
    (devtools_api().mbink_devtools_http_start)(handle, options, out_info)
}
pub unsafe fn mbink_devtools_http_stop(handle: MBinkHandle) -> c_int {
    (devtools_api().mbink_devtools_http_stop)(handle)
}
pub unsafe fn mbink_devtools_http_info_free(info: *mut MBinkDevToolsHttpInfo) {
    (devtools_api().mbink_devtools_http_info_free)(info)
}
pub unsafe fn mbink_observe_set_callback(
    handle: MBinkHandle,
    callback: MBinkObserveCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_observe_set_callback)(handle, callback, user_data)
}
pub unsafe fn mbink_observe_console_json(handle: MBinkHandle, out_json: *mut *mut c_char) -> c_int {
    (api().mbink_observe_console_json)(handle, out_json)
}
pub unsafe fn mbink_observe_errors_json(handle: MBinkHandle, out_json: *mut *mut c_char) -> c_int {
    (api().mbink_observe_errors_json)(handle, out_json)
}
pub unsafe fn mbink_observe_lifecycle_json(
    handle: MBinkHandle,
    out_json: *mut *mut c_char,
) -> c_int {
    (api().mbink_observe_lifecycle_json)(handle, out_json)
}
pub unsafe fn mbink_observe_clear(handle: MBinkHandle, kind: MBinkObserveKind) -> c_int {
    (api().mbink_observe_clear)(handle, kind)
}
pub unsafe fn mbink_ui_dev_default_snapshot_options() -> MBinkUiDevSnapshotOptions {
    (devtools_api().mbink_ui_dev_default_snapshot_options)()
}
pub unsafe fn mbink_ui_dev_snapshot_json(
    handle: MBinkHandle,
    options: *const MBinkUiDevSnapshotOptions,
    out_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mbink_ui_dev_snapshot_json)(handle, options, out_json)
}
pub unsafe fn mbink_ui_dev_snapshot_file(
    handle: MBinkHandle,
    output_path: *const c_char,
    options: *const MBinkUiDevSnapshotOptions,
) -> c_int {
    (devtools_api().mbink_ui_dev_snapshot_file)(handle, output_path, options)
}
pub unsafe fn mbink_ui_dev_command_json(
    handle: MBinkHandle,
    command_json: *const c_char,
    out_response_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mbink_ui_dev_command_json)(handle, command_json, out_response_json)
}
pub unsafe fn mbink_state_create_null(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_create_null)(handle, name)
}
pub unsafe fn mbink_state_create_bool(
    handle: MBinkHandle,
    name: *const c_char,
    value: bool,
) -> c_int {
    (api().mbink_state_create_bool)(handle, name, value)
}
pub unsafe fn mbink_state_create_int(
    handle: MBinkHandle,
    name: *const c_char,
    value: i64,
) -> c_int {
    (api().mbink_state_create_int)(handle, name, value)
}
pub unsafe fn mbink_state_create_double(
    handle: MBinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mbink_state_create_double)(handle, name, value)
}
pub unsafe fn mbink_state_create_string(
    handle: MBinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mbink_state_create_string)(handle, name, value)
}
pub unsafe fn mbink_state_create_array(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_create_array)(handle, name)
}
pub unsafe fn mbink_state_create_object(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_create_object)(handle, name)
}
pub unsafe fn mbink_state_create_json(
    handle: MBinkHandle,
    name: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mbink_state_create_json)(handle, name, json)
}
pub unsafe fn mbink_bind(
    handle: MBinkHandle,
    name: *const c_char,
    callback: MBinkCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_bind)(handle, name, callback, user_data)
}
pub unsafe fn mbink_bind_async(
    handle: MBinkHandle,
    name: *const c_char,
    callback: MBinkAsyncCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_bind_async)(handle, name, callback, user_data)
}
pub unsafe fn mbink_unbind(handle: MBinkHandle, name: *const c_char) {
    (api().mbink_unbind)(handle, name)
}
pub unsafe fn mbink_on_resize(
    handle: MBinkHandle,
    callback: MBinkResizeCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_resize)(handle, callback, user_data)
}
pub unsafe fn mbink_on_close(
    handle: MBinkHandle,
    callback: MBinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_close)(handle, callback, user_data)
}
pub unsafe fn mbink_on_close_request(
    handle: MBinkHandle,
    callback: MBinkBoolCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_close_request)(handle, callback, user_data)
}
pub unsafe fn mbink_on_focus(
    handle: MBinkHandle,
    callback: MBinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_focus)(handle, callback, user_data)
}
pub unsafe fn mbink_on_blur(
    handle: MBinkHandle,
    callback: MBinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_blur)(handle, callback, user_data)
}
pub unsafe fn mbink_on_update(
    handle: MBinkHandle,
    callback: MBinkUpdateCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_on_update)(handle, callback, user_data)
}
pub unsafe fn mbink_state_exists(handle: MBinkHandle, name: *const c_char) -> bool {
    (api().mbink_state_exists)(handle, name)
}
pub unsafe fn mbink_state_type(handle: MBinkHandle, name: *const c_char) -> MBinkType {
    (api().mbink_state_type)(handle, name)
}
pub unsafe fn mbink_state_delete(handle: MBinkHandle, name: *const c_char) {
    (api().mbink_state_delete)(handle, name)
}
pub unsafe fn mbink_state_get_bool(handle: MBinkHandle, name: *const c_char) -> bool {
    (api().mbink_state_get_bool)(handle, name)
}
pub unsafe fn mbink_state_get_int(handle: MBinkHandle, name: *const c_char) -> i64 {
    (api().mbink_state_get_int)(handle, name)
}
pub unsafe fn mbink_state_get_double(handle: MBinkHandle, name: *const c_char) -> f64 {
    (api().mbink_state_get_double)(handle, name)
}
pub unsafe fn mbink_state_get_string(handle: MBinkHandle, name: *const c_char) -> *const c_char {
    (api().mbink_state_get_string)(handle, name)
}
pub unsafe fn mbink_state_get_length(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_get_length)(handle, name)
}
pub unsafe fn mbink_state_get_json(handle: MBinkHandle, name: *const c_char) -> *mut c_char {
    (api().mbink_state_get_json)(handle, name)
}
pub unsafe fn mbink_state_get_at(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
) -> *mut c_char {
    (api().mbink_state_get_at)(handle, name, index)
}
pub unsafe fn mbink_state_get_key(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
) -> *mut c_char {
    (api().mbink_state_get_key)(handle, name, key)
}
pub unsafe fn mbink_state_set_null(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_set_null)(handle, name)
}
pub unsafe fn mbink_state_set_bool(handle: MBinkHandle, name: *const c_char, value: bool) -> c_int {
    (api().mbink_state_set_bool)(handle, name, value)
}
pub unsafe fn mbink_state_set_int(handle: MBinkHandle, name: *const c_char, value: i64) -> c_int {
    (api().mbink_state_set_int)(handle, name, value)
}
pub unsafe fn mbink_state_set_double(
    handle: MBinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mbink_state_set_double)(handle, name, value)
}
pub unsafe fn mbink_state_set_string(
    handle: MBinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mbink_state_set_string)(handle, name, value)
}
pub unsafe fn mbink_state_set_json(
    handle: MBinkHandle,
    name: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mbink_state_set_json)(handle, name, json)
}
pub unsafe fn mbink_state_array_push(
    handle: MBinkHandle,
    name: *const c_char,
    item_json: *const c_char,
) -> c_int {
    (api().mbink_state_array_push)(handle, name, item_json)
}
pub unsafe fn mbink_state_array_push_int(
    handle: MBinkHandle,
    name: *const c_char,
    value: i64,
) -> c_int {
    (api().mbink_state_array_push_int)(handle, name, value)
}
pub unsafe fn mbink_state_array_push_double(
    handle: MBinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mbink_state_array_push_double)(handle, name, value)
}
pub unsafe fn mbink_state_array_push_string(
    handle: MBinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mbink_state_array_push_string)(handle, name, value)
}
pub unsafe fn mbink_state_array_push_bool(
    handle: MBinkHandle,
    name: *const c_char,
    value: bool,
) -> c_int {
    (api().mbink_state_array_push_bool)(handle, name, value)
}
pub unsafe fn mbink_state_array_pop(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_array_pop)(handle, name)
}
pub unsafe fn mbink_state_array_shift(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_array_shift)(handle, name)
}
pub unsafe fn mbink_state_array_unshift(
    handle: MBinkHandle,
    name: *const c_char,
    item_json: *const c_char,
) -> c_int {
    (api().mbink_state_array_unshift)(handle, name, item_json)
}
pub unsafe fn mbink_state_array_remove(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
) -> c_int {
    (api().mbink_state_array_remove)(handle, name, index)
}
pub unsafe fn mbink_state_array_clear(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_array_clear)(handle, name)
}
pub unsafe fn mbink_state_array_set(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
    item_json: *const c_char,
) -> c_int {
    (api().mbink_state_array_set)(handle, name, index, item_json)
}
pub unsafe fn mbink_state_array_set_int(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
    value: i64,
) -> c_int {
    (api().mbink_state_array_set_int)(handle, name, index, value)
}
pub unsafe fn mbink_state_array_set_double(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
    value: f64,
) -> c_int {
    (api().mbink_state_array_set_double)(handle, name, index, value)
}
pub unsafe fn mbink_state_array_set_string(
    handle: MBinkHandle,
    name: *const c_char,
    index: c_int,
    value: *const c_char,
) -> c_int {
    (api().mbink_state_array_set_string)(handle, name, index, value)
}
pub unsafe fn mbink_state_object_set(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
    value_json: *const c_char,
) -> c_int {
    (api().mbink_state_object_set)(handle, name, key, value_json)
}
pub unsafe fn mbink_state_object_set_int(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: i64,
) -> c_int {
    (api().mbink_state_object_set_int)(handle, name, key, value)
}
pub unsafe fn mbink_state_object_set_double(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: f64,
) -> c_int {
    (api().mbink_state_object_set_double)(handle, name, key, value)
}
pub unsafe fn mbink_state_object_set_string(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mbink_state_object_set_string)(handle, name, key, value)
}
pub unsafe fn mbink_state_object_set_bool(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: bool,
) -> c_int {
    (api().mbink_state_object_set_bool)(handle, name, key, value)
}
pub unsafe fn mbink_state_object_remove(
    handle: MBinkHandle,
    name: *const c_char,
    key: *const c_char,
) -> c_int {
    (api().mbink_state_object_remove)(handle, name, key)
}
pub unsafe fn mbink_state_object_clear(handle: MBinkHandle, name: *const c_char) -> c_int {
    (api().mbink_state_object_clear)(handle, name)
}
pub unsafe fn mbink_state_increment(handle: MBinkHandle, name: *const c_char, delta: f64) -> c_int {
    (api().mbink_state_increment)(handle, name, delta)
}
pub unsafe fn mbink_state_multiply(handle: MBinkHandle, name: *const c_char, factor: f64) -> c_int {
    (api().mbink_state_multiply)(handle, name, factor)
}
pub unsafe fn mbink_state_string_append(
    handle: MBinkHandle,
    name: *const c_char,
    suffix: *const c_char,
) -> c_int {
    (api().mbink_state_string_append)(handle, name, suffix)
}
pub unsafe fn mbink_state_string_prepend(
    handle: MBinkHandle,
    name: *const c_char,
    prefix: *const c_char,
) -> c_int {
    (api().mbink_state_string_prepend)(handle, name, prefix)
}
pub unsafe fn mbink_state_watch(
    handle: MBinkHandle,
    name: *const c_char,
    callback: MBinkStateCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mbink_state_watch)(handle, name, callback, user_data)
}
pub unsafe fn mbink_state_unwatch(handle: MBinkHandle, watch_id: c_int) {
    (api().mbink_state_unwatch)(handle, watch_id)
}
pub unsafe fn mbink_state_batch_begin(handle: MBinkHandle) {
    (api().mbink_state_batch_begin)(handle)
}
pub unsafe fn mbink_state_batch_end(handle: MBinkHandle) {
    (api().mbink_state_batch_end)(handle)
}
pub unsafe fn mbink_state_set_merge_mode(handle: MBinkHandle, enable: bool) {
    (api().mbink_state_set_merge_mode)(handle, enable)
}
pub unsafe fn mbink_process_queue(handle: MBinkHandle) -> c_int {
    (api().mbink_process_queue)(handle)
}
pub unsafe fn mbink_queue_size(handle: MBinkHandle) -> c_int {
    (api().mbink_queue_size)(handle)
}
pub unsafe fn mbink_shared_create(handle: MBinkHandle, name: *const c_char) -> MBinkSharedHandle {
    (api().mbink_shared_create)(handle, name)
}
pub unsafe fn mbink_shared_destroy(shared: MBinkSharedHandle) {
    (api().mbink_shared_destroy)(shared)
}
pub unsafe fn mbink_shared_set_int(
    shared: MBinkSharedHandle,
    key: *const c_char,
    value: i64,
) -> c_int {
    (api().mbink_shared_set_int)(shared, key, value)
}
pub unsafe fn mbink_shared_set_double(
    shared: MBinkSharedHandle,
    key: *const c_char,
    value: f64,
) -> c_int {
    (api().mbink_shared_set_double)(shared, key, value)
}
pub unsafe fn mbink_shared_set_string(
    shared: MBinkSharedHandle,
    key: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mbink_shared_set_string)(shared, key, value)
}
pub unsafe fn mbink_shared_set_bool(
    shared: MBinkSharedHandle,
    key: *const c_char,
    value: bool,
) -> c_int {
    (api().mbink_shared_set_bool)(shared, key, value)
}
pub unsafe fn mbink_shared_set_null(shared: MBinkSharedHandle, key: *const c_char) -> c_int {
    (api().mbink_shared_set_null)(shared, key)
}
pub unsafe fn mbink_shared_set_json(
    shared: MBinkSharedHandle,
    key: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mbink_shared_set_json)(shared, key, json)
}
pub unsafe fn mbink_shared_get_int(shared: MBinkSharedHandle, key: *const c_char) -> i64 {
    (api().mbink_shared_get_int)(shared, key)
}
pub unsafe fn mbink_shared_get_double(shared: MBinkSharedHandle, key: *const c_char) -> f64 {
    (api().mbink_shared_get_double)(shared, key)
}
pub unsafe fn mbink_shared_get_string(
    shared: MBinkSharedHandle,
    key: *const c_char,
) -> *mut c_char {
    (api().mbink_shared_get_string)(shared, key)
}
pub unsafe fn mbink_shared_get_bool(shared: MBinkSharedHandle, key: *const c_char) -> bool {
    (api().mbink_shared_get_bool)(shared, key)
}
pub unsafe fn mbink_shared_get_json(shared: MBinkSharedHandle, key: *const c_char) -> *mut c_char {
    (api().mbink_shared_get_json)(shared, key)
}
pub unsafe fn mbink_shared_get_type(shared: MBinkSharedHandle, key: *const c_char) -> c_int {
    (api().mbink_shared_get_type)(shared, key)
}
pub unsafe fn mbink_shared_delete(shared: MBinkSharedHandle, key: *const c_char) -> c_int {
    (api().mbink_shared_delete)(shared, key)
}
pub unsafe fn mbink_shared_has(shared: MBinkSharedHandle, key: *const c_char) -> bool {
    (api().mbink_shared_has)(shared, key)
}
pub unsafe fn mbink_shared_batch_begin(shared: MBinkSharedHandle) {
    (api().mbink_shared_batch_begin)(shared)
}
pub unsafe fn mbink_shared_batch_end(shared: MBinkSharedHandle) {
    (api().mbink_shared_batch_end)(shared)
}
pub unsafe fn mbink_logview_get(
    handle: MBinkHandle,
    element_id: *const c_char,
) -> MBinkLogViewHandle {
    (api().mbink_logview_get)(handle, element_id)
}
pub unsafe fn mbink_logview_destroy(logview: MBinkLogViewHandle) {
    (api().mbink_logview_destroy)(logview)
}
pub unsafe fn mbink_logview_append(
    logview: MBinkLogViewHandle,
    level: *const c_char,
    source: *const c_char,
    message: *const c_char,
) -> c_int {
    (api().mbink_logview_append)(logview, level, source, message)
}
pub unsafe fn mbink_logview_clear(logview: MBinkLogViewHandle) {
    (api().mbink_logview_clear)(logview)
}
pub unsafe fn mbink_logview_export(
    logview: MBinkLogViewHandle,
    format: *const c_char,
) -> *mut c_char {
    (api().mbink_logview_export)(logview, format)
}
pub unsafe fn mbink_terminal_get(
    handle: MBinkHandle,
    element_id: *const c_char,
) -> MBinkTerminalHandle {
    (api().mbink_terminal_get)(handle, element_id)
}
pub unsafe fn mbink_terminal_destroy(terminal: MBinkTerminalHandle) {
    (api().mbink_terminal_destroy)(terminal)
}
pub unsafe fn mbink_terminal_write(terminal: MBinkTerminalHandle, data: *const c_char) -> c_int {
    (api().mbink_terminal_write)(terminal, data)
}
pub unsafe fn mbink_terminal_clear(terminal: MBinkTerminalHandle) {
    (api().mbink_terminal_clear)(terminal)
}
pub unsafe fn mbink_terminal_execute(
    terminal: MBinkTerminalHandle,
    command: *const c_char,
) -> c_int {
    (api().mbink_terminal_execute)(terminal, command)
}
pub unsafe fn mbink_terminal_start_shell(
    terminal: MBinkTerminalHandle,
    shell: *const c_char,
) -> c_int {
    (api().mbink_terminal_start_shell)(terminal, shell)
}
pub unsafe fn mbink_terminal_send_input(
    terminal: MBinkTerminalHandle,
    input: *const c_char,
) -> c_int {
    (api().mbink_terminal_send_input)(terminal, input)
}
pub unsafe fn mbink_terminal_resize(terminal: MBinkTerminalHandle, rows: c_int, cols: c_int) {
    (api().mbink_terminal_resize)(terminal, rows, cols)
}
pub unsafe fn mbink_terminal_serialize(terminal: MBinkTerminalHandle) -> *mut c_char {
    (api().mbink_terminal_serialize)(terminal)
}
pub unsafe fn mbink_copy_string(str_: *const c_char) -> *mut c_char {
    (api().mbink_copy_string)(str_)
}
pub unsafe fn mbink_free(ptr: *mut c_void) {
    (api().mbink_free)(ptr)
}
