#![allow(non_snake_case)]
use std::env;

use std::ffi::{c_char, c_int, c_void};
use std::path::PathBuf;
use std::sync::OnceLock;

use super::{
    MBlinkAsyncCallback, MBlinkBoolCallback, MBlinkCallback, MBlinkConfig, MBlinkDevToolsHttpInfo,
    MBlinkDevToolsHttpOptions, MBlinkHandle, MBlinkLifecycleState, MBlinkLogViewHandle,
    MBlinkObserveCallback, MBlinkObserveKind, MBlinkResizeCallback, MBlinkRuntimeOptions,
    MBlinkSharedHandle, MBlinkStateCallback, MBlinkTerminalHandle, MBlinkType,
    MBlinkUiDevSnapshotOptions, MBlinkUpdateCallback, MBlinkVoidCallback,
};

type MBlinkLoadResourceFileFn = unsafe extern "C" fn(
    *const c_char,
    *const c_char,
    *const c_char,
    *mut *mut c_void,
    *mut usize,
    *mut u32,
) -> c_int;

struct Api {
    _lib: libloading::Library,
    mblink_init: unsafe extern "C" fn() -> c_int,
    mblink_cleanup: unsafe extern "C" fn(),
    mblink_version: unsafe extern "C" fn() -> *const c_char,
    mblink_last_error: unsafe extern "C" fn() -> *const c_char,
    mblink_create: unsafe extern "C" fn(*const c_char, c_int, c_int) -> MBlinkHandle,
    mblink_create_ex: unsafe extern "C" fn(*const MBlinkConfig) -> MBlinkHandle,
    mblink_default_config: unsafe extern "C" fn() -> MBlinkConfig,
    mblink_destroy: unsafe extern "C" fn(MBlinkHandle),
    mblink_run: unsafe extern "C" fn(MBlinkHandle),
    mblink_stop: unsafe extern "C" fn(MBlinkHandle),
    mblink_poll_events: unsafe extern "C" fn(MBlinkHandle) -> bool,
    mblink_wait_events: unsafe extern "C" fn(MBlinkHandle) -> bool,
    mblink_default_runtime_options: unsafe extern "C" fn() -> MBlinkRuntimeOptions,
    mblink_configure_runtime: unsafe extern "C" fn(MBlinkHandle, *const MBlinkRuntimeOptions) -> c_int,
    mblink_load_embedded_runtime: unsafe extern "C" fn(MBlinkHandle, bool) -> c_int,
    mblink_load_entry_file: unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int,
    mblink_load_module_file: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_render_frame: unsafe extern "C" fn(MBlinkHandle, c_int) -> c_int,
    mblink_runtime_epoch: unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int,
    mblink_lifecycle_state: unsafe extern "C" fn(MBlinkHandle) -> MBlinkLifecycleState,
    mblink_lifecycle_reason: unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int,
    mblink_set_title: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_tray_create: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_tray_destroy: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_tray_set_tooltip: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_tray_set_menu: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_tray_set_left_click_callback:
        unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int,
    mblink_tray_set_menu_callback:
        unsafe extern "C" fn(MBlinkHandle, MBlinkCallback, *mut c_void) -> c_int,
    mblink_set_size: unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int,
    mblink_get_size: unsafe extern "C" fn(MBlinkHandle, *mut c_int, *mut c_int) -> c_int,
    mblink_set_position: unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int,
    mblink_get_position: unsafe extern "C" fn(MBlinkHandle, *mut c_int, *mut c_int) -> c_int,
    mblink_set_min_size: unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int,
    mblink_set_max_size: unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int,
    mblink_show: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_hide: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_minimize: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_maximize: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_restore: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_set_fullscreen: unsafe extern "C" fn(MBlinkHandle, bool) -> c_int,
    mblink_set_resizable: unsafe extern "C" fn(MBlinkHandle, bool) -> c_int,
    mblink_set_borderless: unsafe extern "C" fn(MBlinkHandle, bool) -> c_int,
    mblink_set_always_on_top: unsafe extern "C" fn(MBlinkHandle, bool) -> c_int,
    mblink_load_html: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_load_html_file: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_eval_js: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_eval_module: unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_load_js_file: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_load_bytecode: unsafe extern "C" fn(MBlinkHandle, *const c_void, usize) -> c_int,
    mblink_compile_resources:
        unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int,
    mblink_load_resource_file: MBlinkLoadResourceFileFn,
    mblink_mount_resource_package:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mblink_emit: unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_observe_set_callback:
        unsafe extern "C" fn(MBlinkHandle, MBlinkObserveCallback, *mut c_void) -> c_int,
    mblink_observe_console_json: unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int,
    mblink_observe_errors_json: unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int,
    mblink_observe_lifecycle_json: unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int,
    mblink_observe_clear: unsafe extern "C" fn(MBlinkHandle, MBlinkObserveKind) -> c_int,
    mblink_state_create_null: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_create_bool: unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int,
    mblink_state_create_int: unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int,
    mblink_state_create_double: unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int,
    mblink_state_create_string:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_create_array: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_create_object: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_create_json:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_bind:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkCallback, *mut c_void) -> c_int,
    mblink_bind_async:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkAsyncCallback, *mut c_void) -> c_int,
    mblink_unbind: unsafe extern "C" fn(MBlinkHandle, *const c_char),
    mblink_on_resize: unsafe extern "C" fn(MBlinkHandle, MBlinkResizeCallback, *mut c_void) -> c_int,
    mblink_on_close: unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int,
    mblink_on_close_request:
        unsafe extern "C" fn(MBlinkHandle, MBlinkBoolCallback, *mut c_void) -> c_int,
    mblink_on_focus: unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int,
    mblink_on_blur: unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int,
    mblink_on_update: unsafe extern "C" fn(MBlinkHandle, MBlinkUpdateCallback, *mut c_void) -> c_int,
    mblink_state_exists: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> bool,
    mblink_state_type: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkType,
    mblink_state_delete: unsafe extern "C" fn(MBlinkHandle, *const c_char),
    mblink_state_get_bool: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> bool,
    mblink_state_get_int: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> i64,
    mblink_state_get_double: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> f64,
    mblink_state_get_string: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> *const c_char,
    mblink_state_get_length: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_get_json: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> *mut c_char,
    mblink_state_get_at: unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int) -> *mut c_char,
    mblink_state_get_key:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> *mut c_char,
    mblink_state_set_null: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_set_bool: unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int,
    mblink_state_set_int: unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int,
    mblink_state_set_double: unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int,
    mblink_state_set_string:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_set_json: unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_array_push:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_array_push_int: unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int,
    mblink_state_array_push_double: unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int,
    mblink_state_array_push_string:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_array_push_bool: unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int,
    mblink_state_array_pop: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_array_shift: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_array_unshift:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_array_remove: unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int) -> c_int,
    mblink_state_array_clear: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_array_set:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, *const c_char) -> c_int,
    mblink_state_array_set_int:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, i64) -> c_int,
    mblink_state_array_set_double:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, f64) -> c_int,
    mblink_state_array_set_string:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, *const c_char) -> c_int,
    mblink_state_object_set:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mblink_state_object_set_int:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, i64) -> c_int,
    mblink_state_object_set_double:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, f64) -> c_int,
    mblink_state_object_set_string:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int,
    mblink_state_object_set_bool:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, bool) -> c_int,
    mblink_state_object_remove:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_object_clear: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int,
    mblink_state_increment: unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int,
    mblink_state_multiply: unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int,
    mblink_state_string_append:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_string_prepend:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int,
    mblink_state_watch:
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkStateCallback, *mut c_void) -> c_int,
    mblink_state_unwatch: unsafe extern "C" fn(MBlinkHandle, c_int),
    mblink_state_batch_begin: unsafe extern "C" fn(MBlinkHandle),
    mblink_state_batch_end: unsafe extern "C" fn(MBlinkHandle),
    mblink_state_set_merge_mode: unsafe extern "C" fn(MBlinkHandle, bool),
    mblink_process_queue: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_queue_size: unsafe extern "C" fn(MBlinkHandle) -> c_int,
    mblink_shared_create: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkSharedHandle,
    mblink_shared_destroy: unsafe extern "C" fn(MBlinkSharedHandle),
    mblink_shared_set_int: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, i64) -> c_int,
    mblink_shared_set_double: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, f64) -> c_int,
    mblink_shared_set_string:
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, *const c_char) -> c_int,
    mblink_shared_set_bool: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, bool) -> c_int,
    mblink_shared_set_null: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int,
    mblink_shared_set_json:
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, *const c_char) -> c_int,
    mblink_shared_get_int: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> i64,
    mblink_shared_get_double: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> f64,
    mblink_shared_get_string: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> *mut c_char,
    mblink_shared_get_bool: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> bool,
    mblink_shared_get_json: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> *mut c_char,
    mblink_shared_get_type: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int,
    mblink_shared_delete: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int,
    mblink_shared_has: unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> bool,
    mblink_shared_batch_begin: unsafe extern "C" fn(MBlinkSharedHandle),
    mblink_shared_batch_end: unsafe extern "C" fn(MBlinkSharedHandle),
    mblink_logview_get: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkLogViewHandle,
    mblink_logview_destroy: unsafe extern "C" fn(MBlinkLogViewHandle),
    mblink_logview_append: unsafe extern "C" fn(
        MBlinkLogViewHandle,
        *const c_char,
        *const c_char,
        *const c_char,
    ) -> c_int,
    mblink_logview_clear: unsafe extern "C" fn(MBlinkLogViewHandle),
    mblink_logview_export: unsafe extern "C" fn(MBlinkLogViewHandle, *const c_char) -> *mut c_char,
    mblink_terminal_get: unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkTerminalHandle,
    mblink_terminal_destroy: unsafe extern "C" fn(MBlinkTerminalHandle),
    mblink_terminal_write: unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int,
    mblink_terminal_clear: unsafe extern "C" fn(MBlinkTerminalHandle),
    mblink_terminal_execute: unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int,
    mblink_terminal_start_shell: unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int,
    mblink_terminal_send_input: unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int,
    mblink_terminal_resize: unsafe extern "C" fn(MBlinkTerminalHandle, c_int, c_int),
    mblink_terminal_serialize: unsafe extern "C" fn(MBlinkTerminalHandle) -> *mut c_char,
    mblink_copy_string: unsafe extern "C" fn(*const c_char) -> *mut c_char,
    mblink_free: unsafe extern "C" fn(*mut c_void),
}

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
        .unwrap_or_else(|err| panic!("failed to load MBlink DLL {}: {err}", path.display()));

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

    let mblink_init = load!(b"mblink_init\0", unsafe extern "C" fn() -> c_int);
    let mblink_cleanup = load!(b"mblink_cleanup\0", unsafe extern "C" fn());
    let mblink_version = load!(b"mblink_version\0", unsafe extern "C" fn() -> *const c_char);
    let mblink_last_error = load!(
        b"mblink_last_error\0",
        unsafe extern "C" fn() -> *const c_char
    );
    let mblink_create = load!(
        b"mblink_create\0",
        unsafe extern "C" fn(*const c_char, c_int, c_int) -> MBlinkHandle
    );
    let mblink_create_ex = load!(
        b"mblink_create_ex\0",
        unsafe extern "C" fn(*const MBlinkConfig) -> MBlinkHandle
    );
    let mblink_default_config = load!(
        b"mblink_default_config\0",
        unsafe extern "C" fn() -> MBlinkConfig
    );
    let mblink_destroy = load!(b"mblink_destroy\0", unsafe extern "C" fn(MBlinkHandle));
    let mblink_run = load!(b"mblink_run\0", unsafe extern "C" fn(MBlinkHandle));
    let mblink_stop = load!(b"mblink_stop\0", unsafe extern "C" fn(MBlinkHandle));
    let mblink_poll_events = load!(
        b"mblink_poll_events\0",
        unsafe extern "C" fn(MBlinkHandle) -> bool
    );
    let mblink_wait_events = load!(
        b"mblink_wait_events\0",
        unsafe extern "C" fn(MBlinkHandle) -> bool
    );
    let mblink_default_runtime_options = load!(
        b"mblink_default_runtime_options\0",
        unsafe extern "C" fn() -> MBlinkRuntimeOptions
    );
    let mblink_configure_runtime = load!(
        b"mblink_configure_runtime\0",
        unsafe extern "C" fn(MBlinkHandle, *const MBlinkRuntimeOptions) -> c_int
    );
    let mblink_load_embedded_runtime = load!(
        b"mblink_load_embedded_runtime\0",
        unsafe extern "C" fn(MBlinkHandle, bool) -> c_int
    );
    let mblink_load_entry_file = load!(
        b"mblink_load_entry_file\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int
    );
    let mblink_load_module_file = load!(
        b"mblink_load_module_file\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_render_frame = load!(
        b"mblink_render_frame\0",
        unsafe extern "C" fn(MBlinkHandle, c_int) -> c_int
    );
    let mblink_runtime_epoch = load!(
        b"mblink_runtime_epoch\0",
        unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int
    );
    let mblink_lifecycle_state = load!(
        b"mblink_lifecycle_state\0",
        unsafe extern "C" fn(MBlinkHandle) -> MBlinkLifecycleState
    );
    let mblink_lifecycle_reason = load!(
        b"mblink_lifecycle_reason\0",
        unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int
    );
    let mblink_set_title = load!(
        b"mblink_set_title\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_tray_create = load!(
        b"mblink_tray_create\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_tray_destroy = load!(
        b"mblink_tray_destroy\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_tray_set_tooltip = load!(
        b"mblink_tray_set_tooltip\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_tray_set_menu = load!(
        b"mblink_tray_set_menu\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_tray_set_left_click_callback = load!(
        b"mblink_tray_set_left_click_callback\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int
    );
    let mblink_tray_set_menu_callback = load!(
        b"mblink_tray_set_menu_callback\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkCallback, *mut c_void) -> c_int
    );
    let mblink_set_size = load!(
        b"mblink_set_size\0",
        unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int
    );
    let mblink_get_size = load!(
        b"mblink_get_size\0",
        unsafe extern "C" fn(MBlinkHandle, *mut c_int, *mut c_int) -> c_int
    );
    let mblink_set_position = load!(
        b"mblink_set_position\0",
        unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int
    );
    let mblink_get_position = load!(
        b"mblink_get_position\0",
        unsafe extern "C" fn(MBlinkHandle, *mut c_int, *mut c_int) -> c_int
    );
    let mblink_set_min_size = load!(
        b"mblink_set_min_size\0",
        unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int
    );
    let mblink_set_max_size = load!(
        b"mblink_set_max_size\0",
        unsafe extern "C" fn(MBlinkHandle, c_int, c_int) -> c_int
    );
    let mblink_show = load!(b"mblink_show\0", unsafe extern "C" fn(MBlinkHandle) -> c_int);
    let mblink_hide = load!(b"mblink_hide\0", unsafe extern "C" fn(MBlinkHandle) -> c_int);
    let mblink_minimize = load!(
        b"mblink_minimize\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_maximize = load!(
        b"mblink_maximize\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_restore = load!(
        b"mblink_restore\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_set_fullscreen = load!(
        b"mblink_set_fullscreen\0",
        unsafe extern "C" fn(MBlinkHandle, bool) -> c_int
    );
    let mblink_set_resizable = load!(
        b"mblink_set_resizable\0",
        unsafe extern "C" fn(MBlinkHandle, bool) -> c_int
    );
    let mblink_set_borderless = load!(
        b"mblink_set_borderless\0",
        unsafe extern "C" fn(MBlinkHandle, bool) -> c_int
    );
    let mblink_set_always_on_top = load!(
        b"mblink_set_always_on_top\0",
        unsafe extern "C" fn(MBlinkHandle, bool) -> c_int
    );
    let mblink_load_html = load!(
        b"mblink_load_html\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_load_html_file = load!(
        b"mblink_load_html_file\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_eval_js = load!(
        b"mblink_eval_js\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_eval_module = load!(
        b"mblink_eval_module\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_load_js_file = load!(
        b"mblink_load_js_file\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_load_bytecode = load!(
        b"mblink_load_bytecode\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_void, usize) -> c_int
    );
    let mblink_compile_resources = load!(
        b"mblink_compile_resources\0",
        unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> c_int
    );
    let mblink_load_resource_file = load!(b"mblink_load_resource_file\0", MBlinkLoadResourceFileFn);
    let mblink_mount_resource_package = load!(
        b"mblink_mount_resource_package\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mblink_emit = load!(
        b"mblink_emit\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_observe_set_callback = load!(
        b"mblink_observe_set_callback\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkObserveCallback, *mut c_void) -> c_int
    );
    let mblink_observe_console_json = load!(
        b"mblink_observe_console_json\0",
        unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int
    );
    let mblink_observe_errors_json = load!(
        b"mblink_observe_errors_json\0",
        unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int
    );
    let mblink_observe_lifecycle_json = load!(
        b"mblink_observe_lifecycle_json\0",
        unsafe extern "C" fn(MBlinkHandle, *mut *mut c_char) -> c_int
    );
    let mblink_observe_clear = load!(
        b"mblink_observe_clear\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkObserveKind) -> c_int
    );
    let mblink_state_create_null = load!(
        b"mblink_state_create_null\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_create_bool = load!(
        b"mblink_state_create_bool\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int
    );
    let mblink_state_create_int = load!(
        b"mblink_state_create_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int
    );
    let mblink_state_create_double = load!(
        b"mblink_state_create_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int
    );
    let mblink_state_create_string = load!(
        b"mblink_state_create_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_create_array = load!(
        b"mblink_state_create_array\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_create_object = load!(
        b"mblink_state_create_object\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_create_json = load!(
        b"mblink_state_create_json\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_bind = load!(
        b"mblink_bind\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkCallback, *mut c_void) -> c_int
    );
    let mblink_bind_async = load!(
        b"mblink_bind_async\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkAsyncCallback, *mut c_void) -> c_int
    );
    let mblink_unbind = load!(
        b"mblink_unbind\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char)
    );
    let mblink_on_resize = load!(
        b"mblink_on_resize\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkResizeCallback, *mut c_void) -> c_int
    );
    let mblink_on_close = load!(
        b"mblink_on_close\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int
    );
    let mblink_on_close_request = load!(
        b"mblink_on_close_request\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkBoolCallback, *mut c_void) -> c_int
    );
    let mblink_on_focus = load!(
        b"mblink_on_focus\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int
    );
    let mblink_on_blur = load!(
        b"mblink_on_blur\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkVoidCallback, *mut c_void) -> c_int
    );
    let mblink_on_update = load!(
        b"mblink_on_update\0",
        unsafe extern "C" fn(MBlinkHandle, MBlinkUpdateCallback, *mut c_void) -> c_int
    );
    let mblink_state_exists = load!(
        b"mblink_state_exists\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> bool
    );
    let mblink_state_type = load!(
        b"mblink_state_type\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkType
    );
    let mblink_state_delete = load!(
        b"mblink_state_delete\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char)
    );
    let mblink_state_get_bool = load!(
        b"mblink_state_get_bool\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> bool
    );
    let mblink_state_get_int = load!(
        b"mblink_state_get_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> i64
    );
    let mblink_state_get_double = load!(
        b"mblink_state_get_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> f64
    );
    let mblink_state_get_string = load!(
        b"mblink_state_get_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> *const c_char
    );
    let mblink_state_get_length = load!(
        b"mblink_state_get_length\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_get_json = load!(
        b"mblink_state_get_json\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> *mut c_char
    );
    let mblink_state_get_at = load!(
        b"mblink_state_get_at\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int) -> *mut c_char
    );
    let mblink_state_get_key = load!(
        b"mblink_state_get_key\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> *mut c_char
    );
    let mblink_state_set_null = load!(
        b"mblink_state_set_null\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_set_bool = load!(
        b"mblink_state_set_bool\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int
    );
    let mblink_state_set_int = load!(
        b"mblink_state_set_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int
    );
    let mblink_state_set_double = load!(
        b"mblink_state_set_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int
    );
    let mblink_state_set_string = load!(
        b"mblink_state_set_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_set_json = load!(
        b"mblink_state_set_json\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_array_push = load!(
        b"mblink_state_array_push\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_array_push_int = load!(
        b"mblink_state_array_push_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, i64) -> c_int
    );
    let mblink_state_array_push_double = load!(
        b"mblink_state_array_push_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int
    );
    let mblink_state_array_push_string = load!(
        b"mblink_state_array_push_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_array_push_bool = load!(
        b"mblink_state_array_push_bool\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, bool) -> c_int
    );
    let mblink_state_array_pop = load!(
        b"mblink_state_array_pop\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_array_shift = load!(
        b"mblink_state_array_shift\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_array_unshift = load!(
        b"mblink_state_array_unshift\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_array_remove = load!(
        b"mblink_state_array_remove\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int) -> c_int
    );
    let mblink_state_array_clear = load!(
        b"mblink_state_array_clear\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_array_set = load!(
        b"mblink_state_array_set\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, *const c_char) -> c_int
    );
    let mblink_state_array_set_int = load!(
        b"mblink_state_array_set_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, i64) -> c_int
    );
    let mblink_state_array_set_double = load!(
        b"mblink_state_array_set_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, f64) -> c_int
    );
    let mblink_state_array_set_string = load!(
        b"mblink_state_array_set_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, c_int, *const c_char) -> c_int
    );
    let mblink_state_object_set = load!(
        b"mblink_state_object_set\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_object_set_int = load!(
        b"mblink_state_object_set_int\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, i64) -> c_int
    );
    let mblink_state_object_set_double = load!(
        b"mblink_state_object_set_double\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, f64) -> c_int
    );
    let mblink_state_object_set_string = load!(
        b"mblink_state_object_set_string\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_object_set_bool = load!(
        b"mblink_state_object_set_bool\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char, bool) -> c_int
    );
    let mblink_state_object_remove = load!(
        b"mblink_state_object_remove\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_object_clear = load!(
        b"mblink_state_object_clear\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> c_int
    );
    let mblink_state_increment = load!(
        b"mblink_state_increment\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int
    );
    let mblink_state_multiply = load!(
        b"mblink_state_multiply\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, f64) -> c_int
    );
    let mblink_state_string_append = load!(
        b"mblink_state_string_append\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_string_prepend = load!(
        b"mblink_state_string_prepend\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_state_watch = load!(
        b"mblink_state_watch\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char, MBlinkStateCallback, *mut c_void) -> c_int
    );
    let mblink_state_unwatch = load!(
        b"mblink_state_unwatch\0",
        unsafe extern "C" fn(MBlinkHandle, c_int)
    );
    let mblink_state_batch_begin = load!(
        b"mblink_state_batch_begin\0",
        unsafe extern "C" fn(MBlinkHandle)
    );
    let mblink_state_batch_end = load!(
        b"mblink_state_batch_end\0",
        unsafe extern "C" fn(MBlinkHandle)
    );
    let mblink_state_set_merge_mode = load!(
        b"mblink_state_set_merge_mode\0",
        unsafe extern "C" fn(MBlinkHandle, bool)
    );
    let mblink_process_queue = load!(
        b"mblink_process_queue\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_queue_size = load!(
        b"mblink_queue_size\0",
        unsafe extern "C" fn(MBlinkHandle) -> c_int
    );
    let mblink_shared_create = load!(
        b"mblink_shared_create\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkSharedHandle
    );
    let mblink_shared_destroy = load!(
        b"mblink_shared_destroy\0",
        unsafe extern "C" fn(MBlinkSharedHandle)
    );
    let mblink_shared_set_int = load!(
        b"mblink_shared_set_int\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, i64) -> c_int
    );
    let mblink_shared_set_double = load!(
        b"mblink_shared_set_double\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, f64) -> c_int
    );
    let mblink_shared_set_string = load!(
        b"mblink_shared_set_string\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_shared_set_bool = load!(
        b"mblink_shared_set_bool\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, bool) -> c_int
    );
    let mblink_shared_set_null = load!(
        b"mblink_shared_set_null\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int
    );
    let mblink_shared_set_json = load!(
        b"mblink_shared_set_json\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char, *const c_char) -> c_int
    );
    let mblink_shared_get_int = load!(
        b"mblink_shared_get_int\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> i64
    );
    let mblink_shared_get_double = load!(
        b"mblink_shared_get_double\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> f64
    );
    let mblink_shared_get_string = load!(
        b"mblink_shared_get_string\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> *mut c_char
    );
    let mblink_shared_get_bool = load!(
        b"mblink_shared_get_bool\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> bool
    );
    let mblink_shared_get_json = load!(
        b"mblink_shared_get_json\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> *mut c_char
    );
    let mblink_shared_get_type = load!(
        b"mblink_shared_get_type\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int
    );
    let mblink_shared_delete = load!(
        b"mblink_shared_delete\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> c_int
    );
    let mblink_shared_has = load!(
        b"mblink_shared_has\0",
        unsafe extern "C" fn(MBlinkSharedHandle, *const c_char) -> bool
    );
    let mblink_shared_batch_begin = load!(
        b"mblink_shared_batch_begin\0",
        unsafe extern "C" fn(MBlinkSharedHandle)
    );
    let mblink_shared_batch_end = load!(
        b"mblink_shared_batch_end\0",
        unsafe extern "C" fn(MBlinkSharedHandle)
    );
    let mblink_logview_get = load!(
        b"mblink_logview_get\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkLogViewHandle
    );
    let mblink_logview_destroy = load!(
        b"mblink_logview_destroy\0",
        unsafe extern "C" fn(MBlinkLogViewHandle)
    );
    let mblink_logview_append = load!(
        b"mblink_logview_append\0",
        unsafe extern "C" fn(
            MBlinkLogViewHandle,
            *const c_char,
            *const c_char,
            *const c_char,
        ) -> c_int
    );
    let mblink_logview_clear = load!(
        b"mblink_logview_clear\0",
        unsafe extern "C" fn(MBlinkLogViewHandle)
    );
    let mblink_logview_export = load!(
        b"mblink_logview_export\0",
        unsafe extern "C" fn(MBlinkLogViewHandle, *const c_char) -> *mut c_char
    );
    let mblink_terminal_get = load!(
        b"mblink_terminal_get\0",
        unsafe extern "C" fn(MBlinkHandle, *const c_char) -> MBlinkTerminalHandle
    );
    let mblink_terminal_destroy = load!(
        b"mblink_terminal_destroy\0",
        unsafe extern "C" fn(MBlinkTerminalHandle)
    );
    let mblink_terminal_write = load!(
        b"mblink_terminal_write\0",
        unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int
    );
    let mblink_terminal_clear = load!(
        b"mblink_terminal_clear\0",
        unsafe extern "C" fn(MBlinkTerminalHandle)
    );
    let mblink_terminal_execute = load!(
        b"mblink_terminal_execute\0",
        unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int
    );
    let mblink_terminal_start_shell = load!(
        b"mblink_terminal_start_shell\0",
        unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int
    );
    let mblink_terminal_send_input = load!(
        b"mblink_terminal_send_input\0",
        unsafe extern "C" fn(MBlinkTerminalHandle, *const c_char) -> c_int
    );
    let mblink_terminal_resize = load!(
        b"mblink_terminal_resize\0",
        unsafe extern "C" fn(MBlinkTerminalHandle, c_int, c_int)
    );
    let mblink_terminal_serialize = load!(
        b"mblink_terminal_serialize\0",
        unsafe extern "C" fn(MBlinkTerminalHandle) -> *mut c_char
    );
    let mblink_copy_string = load!(
        b"mblink_copy_string\0",
        unsafe extern "C" fn(*const c_char) -> *mut c_char
    );
    let mblink_free = load!(b"mblink_free\0", unsafe extern "C" fn(*mut c_void));

    Api {
        _lib: lib,
        mblink_init,
        mblink_cleanup,
        mblink_version,
        mblink_last_error,
        mblink_create,
        mblink_create_ex,
        mblink_default_config,
        mblink_destroy,
        mblink_run,
        mblink_stop,
        mblink_poll_events,
        mblink_wait_events,
        mblink_default_runtime_options,
        mblink_configure_runtime,
        mblink_load_embedded_runtime,
        mblink_load_entry_file,
        mblink_load_module_file,
        mblink_render_frame,
        mblink_runtime_epoch,
        mblink_lifecycle_state,
        mblink_lifecycle_reason,
        mblink_set_title,
        mblink_tray_create,
        mblink_tray_destroy,
        mblink_tray_set_tooltip,
        mblink_tray_set_menu,
        mblink_tray_set_left_click_callback,
        mblink_tray_set_menu_callback,
        mblink_set_size,
        mblink_get_size,
        mblink_set_position,
        mblink_get_position,
        mblink_set_min_size,
        mblink_set_max_size,
        mblink_show,
        mblink_hide,
        mblink_minimize,
        mblink_maximize,
        mblink_restore,
        mblink_set_fullscreen,
        mblink_set_resizable,
        mblink_set_borderless,
        mblink_set_always_on_top,
        mblink_load_html,
        mblink_load_html_file,
        mblink_eval_js,
        mblink_eval_module,
        mblink_load_js_file,
        mblink_load_bytecode,
        mblink_compile_resources,
        mblink_load_resource_file,
        mblink_mount_resource_package,
        mblink_emit,
        mblink_observe_set_callback,
        mblink_observe_console_json,
        mblink_observe_errors_json,
        mblink_observe_lifecycle_json,
        mblink_observe_clear,
        mblink_state_create_null,
        mblink_state_create_bool,
        mblink_state_create_int,
        mblink_state_create_double,
        mblink_state_create_string,
        mblink_state_create_array,
        mblink_state_create_object,
        mblink_state_create_json,
        mblink_bind,
        mblink_bind_async,
        mblink_unbind,
        mblink_on_resize,
        mblink_on_close,
        mblink_on_close_request,
        mblink_on_focus,
        mblink_on_blur,
        mblink_on_update,
        mblink_state_exists,
        mblink_state_type,
        mblink_state_delete,
        mblink_state_get_bool,
        mblink_state_get_int,
        mblink_state_get_double,
        mblink_state_get_string,
        mblink_state_get_length,
        mblink_state_get_json,
        mblink_state_get_at,
        mblink_state_get_key,
        mblink_state_set_null,
        mblink_state_set_bool,
        mblink_state_set_int,
        mblink_state_set_double,
        mblink_state_set_string,
        mblink_state_set_json,
        mblink_state_array_push,
        mblink_state_array_push_int,
        mblink_state_array_push_double,
        mblink_state_array_push_string,
        mblink_state_array_push_bool,
        mblink_state_array_pop,
        mblink_state_array_shift,
        mblink_state_array_unshift,
        mblink_state_array_remove,
        mblink_state_array_clear,
        mblink_state_array_set,
        mblink_state_array_set_int,
        mblink_state_array_set_double,
        mblink_state_array_set_string,
        mblink_state_object_set,
        mblink_state_object_set_int,
        mblink_state_object_set_double,
        mblink_state_object_set_string,
        mblink_state_object_set_bool,
        mblink_state_object_remove,
        mblink_state_object_clear,
        mblink_state_increment,
        mblink_state_multiply,
        mblink_state_string_append,
        mblink_state_string_prepend,
        mblink_state_watch,
        mblink_state_unwatch,
        mblink_state_batch_begin,
        mblink_state_batch_end,
        mblink_state_set_merge_mode,
        mblink_process_queue,
        mblink_queue_size,
        mblink_shared_create,
        mblink_shared_destroy,
        mblink_shared_set_int,
        mblink_shared_set_double,
        mblink_shared_set_string,
        mblink_shared_set_bool,
        mblink_shared_set_null,
        mblink_shared_set_json,
        mblink_shared_get_int,
        mblink_shared_get_double,
        mblink_shared_get_string,
        mblink_shared_get_bool,
        mblink_shared_get_json,
        mblink_shared_get_type,
        mblink_shared_delete,
        mblink_shared_has,
        mblink_shared_batch_begin,
        mblink_shared_batch_end,
        mblink_logview_get,
        mblink_logview_destroy,
        mblink_logview_append,
        mblink_logview_clear,
        mblink_logview_export,
        mblink_terminal_get,
        mblink_terminal_destroy,
        mblink_terminal_write,
        mblink_terminal_clear,
        mblink_terminal_execute,
        mblink_terminal_start_shell,
        mblink_terminal_send_input,
        mblink_terminal_resize,
        mblink_terminal_serialize,
        mblink_copy_string,
        mblink_free,
    }
}

unsafe fn load_devtools_api() -> DevtoolsApi {
    let path = devtools_dll_path();
    let lib = libloading::Library::new(&path).unwrap_or_else(|err| {
        panic!(
            "failed to load mblink_devtools.dll at {}: {err}",
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

fn dll_path() -> PathBuf {
    if let Some(path) = env::var_os("MBLINK_DLL_PATH") {
        return PathBuf::from(path);
    }

    if let Some(path) = dll_next_to_exe() {
        return path;
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let runtime_candidate = manifest_dir.join("runtime").join("mblink.dll");
    if runtime_candidate.exists() {
        return runtime_candidate;
    }

    PathBuf::from("mblink.dll")
}

fn devtools_dll_path() -> PathBuf {
    if let Some(path) = env::var_os("MBLINK_DEVTOOLS_PATH") {
        let path = PathBuf::from(path);
        if !path.is_absolute() {
            panic!("MBLINK_DEVTOOLS_PATH must be an absolute path");
        }
        if path.is_dir() {
            let candidate = path.join("mblink_devtools.dll");
            return std::fs::canonicalize(&candidate)
                .unwrap_or_else(|_| panic!("mblink_devtools.dll not found at {}", candidate.display()));
        }
        return std::fs::canonicalize(&path)
            .unwrap_or_else(|_| panic!("mblink_devtools.dll not found at {}", path.display()));
    }

    let core_path = dll_path();
    if core_path.is_absolute() && core_path.exists() {
        if let Some(parent) = core_path.parent() {
            let candidate = parent.join("mblink_devtools.dll");
            if candidate.exists() {
                return candidate;
            }
        }
    }

    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let runtime_core = manifest_dir.join("runtime").join("mblink.dll");
    if runtime_core.exists() {
        let candidate = runtime_core.with_file_name("mblink_devtools.dll");
        if candidate.exists() {
            return candidate;
        }
    }

    if let Some(candidate) = devtools_dll_next_to_exe() {
        return candidate;
    }

    let runtime_candidate = manifest_dir.join("runtime").join("mblink_devtools.dll");
    if runtime_candidate.exists() {
        return runtime_candidate;
    }

    panic!(
        "mblink_devtools.dll not found. Set MBLINK_DEVTOOLS_PATH or place it next to mblink.dll"
    )
}

fn dll_next_to_exe() -> Option<PathBuf> {
    let exe_dir = env::current_exe().ok()?.parent()?.to_path_buf();
    let candidate = exe_dir.join("mblink.dll");
    candidate.exists().then_some(candidate)
}

fn devtools_dll_next_to_exe() -> Option<PathBuf> {
    let exe_dir = env::current_exe().ok()?.parent()?.to_path_buf();
    let core_candidate = exe_dir.join("mblink.dll");
    if !core_candidate.exists() {
        return None;
    }
    let candidate = exe_dir.join("mblink_devtools.dll");
    candidate.exists().then_some(candidate)
}

pub unsafe fn mblink_init() -> c_int {
    (api().mblink_init)()
}
pub unsafe fn mblink_cleanup() {
    (api().mblink_cleanup)()
}
pub unsafe fn mblink_version() -> *const c_char {
    (api().mblink_version)()
}
pub unsafe fn mblink_last_error() -> *const c_char {
    (api().mblink_last_error)()
}
pub unsafe fn mblink_create(title: *const c_char, width: c_int, height: c_int) -> MBlinkHandle {
    (api().mblink_create)(title, width, height)
}
pub unsafe fn mblink_create_ex(config: *const MBlinkConfig) -> MBlinkHandle {
    (api().mblink_create_ex)(config)
}
pub unsafe fn mblink_default_config() -> MBlinkConfig {
    (api().mblink_default_config)()
}
pub unsafe fn mblink_destroy(handle: MBlinkHandle) {
    (api().mblink_destroy)(handle)
}
pub unsafe fn mblink_run(handle: MBlinkHandle) {
    (api().mblink_run)(handle)
}
pub unsafe fn mblink_stop(handle: MBlinkHandle) {
    (api().mblink_stop)(handle)
}
pub unsafe fn mblink_poll_events(handle: MBlinkHandle) -> bool {
    (api().mblink_poll_events)(handle)
}
pub unsafe fn mblink_wait_events(handle: MBlinkHandle) -> bool {
    (api().mblink_wait_events)(handle)
}
pub unsafe fn mblink_default_runtime_options() -> MBlinkRuntimeOptions {
    (api().mblink_default_runtime_options)()
}
pub unsafe fn mblink_configure_runtime(
    handle: MBlinkHandle,
    options: *const MBlinkRuntimeOptions,
) -> c_int {
    (api().mblink_configure_runtime)(handle, options)
}
pub unsafe fn mblink_load_embedded_runtime(
    handle: MBlinkHandle,
    include_official_preact: bool,
) -> c_int {
    (api().mblink_load_embedded_runtime)(handle, include_official_preact)
}
pub unsafe fn mblink_load_entry_file(
    handle: MBlinkHandle,
    entry_path: *const c_char,
    execute_html_scripts: bool,
) -> c_int {
    (api().mblink_load_entry_file)(handle, entry_path, execute_html_scripts)
}
pub unsafe fn mblink_load_module_file(handle: MBlinkHandle, entry_path: *const c_char) -> c_int {
    (api().mblink_load_module_file)(handle, entry_path)
}
pub unsafe fn mblink_render_frame(handle: MBlinkHandle, passes: c_int) -> c_int {
    (api().mblink_render_frame)(handle, passes)
}
pub unsafe fn mblink_runtime_epoch(handle: MBlinkHandle, out_epoch: *mut *mut c_char) -> c_int {
    (api().mblink_runtime_epoch)(handle, out_epoch)
}
pub unsafe fn mblink_lifecycle_state(handle: MBlinkHandle) -> MBlinkLifecycleState {
    (api().mblink_lifecycle_state)(handle)
}
pub unsafe fn mblink_lifecycle_reason(handle: MBlinkHandle, out_reason: *mut *mut c_char) -> c_int {
    (api().mblink_lifecycle_reason)(handle, out_reason)
}
pub unsafe fn mblink_set_title(handle: MBlinkHandle, title: *const c_char) -> c_int {
    (api().mblink_set_title)(handle, title)
}
pub unsafe fn mblink_tray_create(handle: MBlinkHandle, tooltip: *const c_char) -> c_int {
    (api().mblink_tray_create)(handle, tooltip)
}
pub unsafe fn mblink_tray_destroy(handle: MBlinkHandle) -> c_int {
    (api().mblink_tray_destroy)(handle)
}
pub unsafe fn mblink_tray_set_tooltip(handle: MBlinkHandle, tooltip: *const c_char) -> c_int {
    (api().mblink_tray_set_tooltip)(handle, tooltip)
}
pub unsafe fn mblink_tray_set_menu(handle: MBlinkHandle, menu_json: *const c_char) -> c_int {
    (api().mblink_tray_set_menu)(handle, menu_json)
}
pub unsafe fn mblink_tray_set_left_click_callback(
    handle: MBlinkHandle,
    callback: MBlinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_tray_set_left_click_callback)(handle, callback, user_data)
}
pub unsafe fn mblink_tray_set_menu_callback(
    handle: MBlinkHandle,
    callback: MBlinkCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_tray_set_menu_callback)(handle, callback, user_data)
}
pub unsafe fn mblink_set_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mblink_set_size)(handle, width, height)
}
pub unsafe fn mblink_get_size(handle: MBlinkHandle, width: *mut c_int, height: *mut c_int) -> c_int {
    (api().mblink_get_size)(handle, width, height)
}
pub unsafe fn mblink_set_position(handle: MBlinkHandle, x: c_int, y: c_int) -> c_int {
    (api().mblink_set_position)(handle, x, y)
}
pub unsafe fn mblink_get_position(handle: MBlinkHandle, x: *mut c_int, y: *mut c_int) -> c_int {
    (api().mblink_get_position)(handle, x, y)
}
pub unsafe fn mblink_set_min_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mblink_set_min_size)(handle, width, height)
}
pub unsafe fn mblink_set_max_size(handle: MBlinkHandle, width: c_int, height: c_int) -> c_int {
    (api().mblink_set_max_size)(handle, width, height)
}
pub unsafe fn mblink_show(handle: MBlinkHandle) -> c_int {
    (api().mblink_show)(handle)
}
pub unsafe fn mblink_hide(handle: MBlinkHandle) -> c_int {
    (api().mblink_hide)(handle)
}
pub unsafe fn mblink_minimize(handle: MBlinkHandle) -> c_int {
    (api().mblink_minimize)(handle)
}
pub unsafe fn mblink_maximize(handle: MBlinkHandle) -> c_int {
    (api().mblink_maximize)(handle)
}
pub unsafe fn mblink_restore(handle: MBlinkHandle) -> c_int {
    (api().mblink_restore)(handle)
}
pub unsafe fn mblink_set_fullscreen(handle: MBlinkHandle, fullscreen: bool) -> c_int {
    (api().mblink_set_fullscreen)(handle, fullscreen)
}
pub unsafe fn mblink_set_resizable(handle: MBlinkHandle, resizable: bool) -> c_int {
    (api().mblink_set_resizable)(handle, resizable)
}
pub unsafe fn mblink_set_borderless(handle: MBlinkHandle, borderless: bool) -> c_int {
    (api().mblink_set_borderless)(handle, borderless)
}
pub unsafe fn mblink_set_always_on_top(handle: MBlinkHandle, on_top: bool) -> c_int {
    (api().mblink_set_always_on_top)(handle, on_top)
}
pub unsafe fn mblink_load_html(handle: MBlinkHandle, html: *const c_char) -> c_int {
    (api().mblink_load_html)(handle, html)
}
pub unsafe fn mblink_load_html_file(handle: MBlinkHandle, filepath: *const c_char) -> c_int {
    (api().mblink_load_html_file)(handle, filepath)
}
pub unsafe fn mblink_eval_js(handle: MBlinkHandle, code: *const c_char) -> c_int {
    (api().mblink_eval_js)(handle, code)
}
pub unsafe fn mblink_eval_module(
    handle: MBlinkHandle,
    code: *const c_char,
    filename: *const c_char,
) -> c_int {
    (api().mblink_eval_module)(handle, code, filename)
}
pub unsafe fn mblink_load_js_file(handle: MBlinkHandle, filepath: *const c_char) -> c_int {
    (api().mblink_load_js_file)(handle, filepath)
}
pub unsafe fn mblink_load_bytecode(handle: MBlinkHandle, data: *const c_void, size: usize) -> c_int {
    (api().mblink_load_bytecode)(handle, data, size)
}
pub unsafe fn mblink_compile_resources(
    input_path: *const c_char,
    output_file: *const c_char,
    encryption_key: *const c_char,
) -> c_int {
    (api().mblink_compile_resources)(input_path, output_file, encryption_key)
}
pub unsafe fn mblink_load_resource_file(
    package_file: *const c_char,
    resource_path: *const c_char,
    encryption_key: *const c_char,
    out_data: *mut *mut c_void,
    out_size: *mut usize,
    out_flags: *mut u32,
) -> c_int {
    (api().mblink_load_resource_file)(
        package_file,
        resource_path,
        encryption_key,
        out_data,
        out_size,
        out_flags,
    )
}
pub unsafe fn mblink_mount_resource_package(
    handle: MBlinkHandle,
    package_file: *const c_char,
    encryption_key: *const c_char,
    mount_point: *const c_char,
) -> c_int {
    (api().mblink_mount_resource_package)(handle, package_file, encryption_key, mount_point)
}
pub unsafe fn mblink_emit(
    handle: MBlinkHandle,
    event_name: *const c_char,
    data_json: *const c_char,
) -> c_int {
    (api().mblink_emit)(handle, event_name, data_json)
}
pub unsafe fn mblink_devtools_open(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_open)(handle)
}
pub unsafe fn mblink_devtools_close(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_close)(handle)
}
pub unsafe fn mblink_devtools_default_http_options() -> MBlinkDevToolsHttpOptions {
    (devtools_api().mblink_devtools_default_http_options)()
}
pub unsafe fn mblink_devtools_http_start(
    handle: MBlinkHandle,
    options: *const MBlinkDevToolsHttpOptions,
    out_info: *mut MBlinkDevToolsHttpInfo,
) -> c_int {
    (devtools_api().mblink_devtools_http_start)(handle, options, out_info)
}
pub unsafe fn mblink_devtools_http_stop(handle: MBlinkHandle) -> c_int {
    (devtools_api().mblink_devtools_http_stop)(handle)
}
pub unsafe fn mblink_devtools_http_info_free(info: *mut MBlinkDevToolsHttpInfo) {
    (devtools_api().mblink_devtools_http_info_free)(info)
}
pub unsafe fn mblink_observe_set_callback(
    handle: MBlinkHandle,
    callback: MBlinkObserveCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_observe_set_callback)(handle, callback, user_data)
}
pub unsafe fn mblink_observe_console_json(handle: MBlinkHandle, out_json: *mut *mut c_char) -> c_int {
    (api().mblink_observe_console_json)(handle, out_json)
}
pub unsafe fn mblink_observe_errors_json(handle: MBlinkHandle, out_json: *mut *mut c_char) -> c_int {
    (api().mblink_observe_errors_json)(handle, out_json)
}
pub unsafe fn mblink_observe_lifecycle_json(
    handle: MBlinkHandle,
    out_json: *mut *mut c_char,
) -> c_int {
    (api().mblink_observe_lifecycle_json)(handle, out_json)
}
pub unsafe fn mblink_observe_clear(handle: MBlinkHandle, kind: MBlinkObserveKind) -> c_int {
    (api().mblink_observe_clear)(handle, kind)
}
pub unsafe fn mblink_ui_dev_default_snapshot_options() -> MBlinkUiDevSnapshotOptions {
    (devtools_api().mblink_ui_dev_default_snapshot_options)()
}
pub unsafe fn mblink_ui_dev_snapshot_json(
    handle: MBlinkHandle,
    options: *const MBlinkUiDevSnapshotOptions,
    out_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mblink_ui_dev_snapshot_json)(handle, options, out_json)
}
pub unsafe fn mblink_ui_dev_snapshot_file(
    handle: MBlinkHandle,
    output_path: *const c_char,
    options: *const MBlinkUiDevSnapshotOptions,
) -> c_int {
    (devtools_api().mblink_ui_dev_snapshot_file)(handle, output_path, options)
}
pub unsafe fn mblink_ui_dev_command_json(
    handle: MBlinkHandle,
    command_json: *const c_char,
    out_response_json: *mut *mut c_char,
) -> c_int {
    (devtools_api().mblink_ui_dev_command_json)(handle, command_json, out_response_json)
}
pub unsafe fn mblink_state_create_null(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_create_null)(handle, name)
}
pub unsafe fn mblink_state_create_bool(
    handle: MBlinkHandle,
    name: *const c_char,
    value: bool,
) -> c_int {
    (api().mblink_state_create_bool)(handle, name, value)
}
pub unsafe fn mblink_state_create_int(
    handle: MBlinkHandle,
    name: *const c_char,
    value: i64,
) -> c_int {
    (api().mblink_state_create_int)(handle, name, value)
}
pub unsafe fn mblink_state_create_double(
    handle: MBlinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mblink_state_create_double)(handle, name, value)
}
pub unsafe fn mblink_state_create_string(
    handle: MBlinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mblink_state_create_string)(handle, name, value)
}
pub unsafe fn mblink_state_create_array(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_create_array)(handle, name)
}
pub unsafe fn mblink_state_create_object(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_create_object)(handle, name)
}
pub unsafe fn mblink_state_create_json(
    handle: MBlinkHandle,
    name: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mblink_state_create_json)(handle, name, json)
}
pub unsafe fn mblink_bind(
    handle: MBlinkHandle,
    name: *const c_char,
    callback: MBlinkCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_bind)(handle, name, callback, user_data)
}
pub unsafe fn mblink_bind_async(
    handle: MBlinkHandle,
    name: *const c_char,
    callback: MBlinkAsyncCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_bind_async)(handle, name, callback, user_data)
}
pub unsafe fn mblink_unbind(handle: MBlinkHandle, name: *const c_char) {
    (api().mblink_unbind)(handle, name)
}
pub unsafe fn mblink_on_resize(
    handle: MBlinkHandle,
    callback: MBlinkResizeCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_resize)(handle, callback, user_data)
}
pub unsafe fn mblink_on_close(
    handle: MBlinkHandle,
    callback: MBlinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_close)(handle, callback, user_data)
}
pub unsafe fn mblink_on_close_request(
    handle: MBlinkHandle,
    callback: MBlinkBoolCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_close_request)(handle, callback, user_data)
}
pub unsafe fn mblink_on_focus(
    handle: MBlinkHandle,
    callback: MBlinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_focus)(handle, callback, user_data)
}
pub unsafe fn mblink_on_blur(
    handle: MBlinkHandle,
    callback: MBlinkVoidCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_blur)(handle, callback, user_data)
}
pub unsafe fn mblink_on_update(
    handle: MBlinkHandle,
    callback: MBlinkUpdateCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_on_update)(handle, callback, user_data)
}
pub unsafe fn mblink_state_exists(handle: MBlinkHandle, name: *const c_char) -> bool {
    (api().mblink_state_exists)(handle, name)
}
pub unsafe fn mblink_state_type(handle: MBlinkHandle, name: *const c_char) -> MBlinkType {
    (api().mblink_state_type)(handle, name)
}
pub unsafe fn mblink_state_delete(handle: MBlinkHandle, name: *const c_char) {
    (api().mblink_state_delete)(handle, name)
}
pub unsafe fn mblink_state_get_bool(handle: MBlinkHandle, name: *const c_char) -> bool {
    (api().mblink_state_get_bool)(handle, name)
}
pub unsafe fn mblink_state_get_int(handle: MBlinkHandle, name: *const c_char) -> i64 {
    (api().mblink_state_get_int)(handle, name)
}
pub unsafe fn mblink_state_get_double(handle: MBlinkHandle, name: *const c_char) -> f64 {
    (api().mblink_state_get_double)(handle, name)
}
pub unsafe fn mblink_state_get_string(handle: MBlinkHandle, name: *const c_char) -> *const c_char {
    (api().mblink_state_get_string)(handle, name)
}
pub unsafe fn mblink_state_get_length(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_get_length)(handle, name)
}
pub unsafe fn mblink_state_get_json(handle: MBlinkHandle, name: *const c_char) -> *mut c_char {
    (api().mblink_state_get_json)(handle, name)
}
pub unsafe fn mblink_state_get_at(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
) -> *mut c_char {
    (api().mblink_state_get_at)(handle, name, index)
}
pub unsafe fn mblink_state_get_key(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
) -> *mut c_char {
    (api().mblink_state_get_key)(handle, name, key)
}
pub unsafe fn mblink_state_set_null(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_set_null)(handle, name)
}
pub unsafe fn mblink_state_set_bool(handle: MBlinkHandle, name: *const c_char, value: bool) -> c_int {
    (api().mblink_state_set_bool)(handle, name, value)
}
pub unsafe fn mblink_state_set_int(handle: MBlinkHandle, name: *const c_char, value: i64) -> c_int {
    (api().mblink_state_set_int)(handle, name, value)
}
pub unsafe fn mblink_state_set_double(
    handle: MBlinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mblink_state_set_double)(handle, name, value)
}
pub unsafe fn mblink_state_set_string(
    handle: MBlinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mblink_state_set_string)(handle, name, value)
}
pub unsafe fn mblink_state_set_json(
    handle: MBlinkHandle,
    name: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mblink_state_set_json)(handle, name, json)
}
pub unsafe fn mblink_state_array_push(
    handle: MBlinkHandle,
    name: *const c_char,
    item_json: *const c_char,
) -> c_int {
    (api().mblink_state_array_push)(handle, name, item_json)
}
pub unsafe fn mblink_state_array_push_int(
    handle: MBlinkHandle,
    name: *const c_char,
    value: i64,
) -> c_int {
    (api().mblink_state_array_push_int)(handle, name, value)
}
pub unsafe fn mblink_state_array_push_double(
    handle: MBlinkHandle,
    name: *const c_char,
    value: f64,
) -> c_int {
    (api().mblink_state_array_push_double)(handle, name, value)
}
pub unsafe fn mblink_state_array_push_string(
    handle: MBlinkHandle,
    name: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mblink_state_array_push_string)(handle, name, value)
}
pub unsafe fn mblink_state_array_push_bool(
    handle: MBlinkHandle,
    name: *const c_char,
    value: bool,
) -> c_int {
    (api().mblink_state_array_push_bool)(handle, name, value)
}
pub unsafe fn mblink_state_array_pop(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_array_pop)(handle, name)
}
pub unsafe fn mblink_state_array_shift(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_array_shift)(handle, name)
}
pub unsafe fn mblink_state_array_unshift(
    handle: MBlinkHandle,
    name: *const c_char,
    item_json: *const c_char,
) -> c_int {
    (api().mblink_state_array_unshift)(handle, name, item_json)
}
pub unsafe fn mblink_state_array_remove(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
) -> c_int {
    (api().mblink_state_array_remove)(handle, name, index)
}
pub unsafe fn mblink_state_array_clear(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_array_clear)(handle, name)
}
pub unsafe fn mblink_state_array_set(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
    item_json: *const c_char,
) -> c_int {
    (api().mblink_state_array_set)(handle, name, index, item_json)
}
pub unsafe fn mblink_state_array_set_int(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
    value: i64,
) -> c_int {
    (api().mblink_state_array_set_int)(handle, name, index, value)
}
pub unsafe fn mblink_state_array_set_double(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
    value: f64,
) -> c_int {
    (api().mblink_state_array_set_double)(handle, name, index, value)
}
pub unsafe fn mblink_state_array_set_string(
    handle: MBlinkHandle,
    name: *const c_char,
    index: c_int,
    value: *const c_char,
) -> c_int {
    (api().mblink_state_array_set_string)(handle, name, index, value)
}
pub unsafe fn mblink_state_object_set(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
    value_json: *const c_char,
) -> c_int {
    (api().mblink_state_object_set)(handle, name, key, value_json)
}
pub unsafe fn mblink_state_object_set_int(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: i64,
) -> c_int {
    (api().mblink_state_object_set_int)(handle, name, key, value)
}
pub unsafe fn mblink_state_object_set_double(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: f64,
) -> c_int {
    (api().mblink_state_object_set_double)(handle, name, key, value)
}
pub unsafe fn mblink_state_object_set_string(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mblink_state_object_set_string)(handle, name, key, value)
}
pub unsafe fn mblink_state_object_set_bool(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
    value: bool,
) -> c_int {
    (api().mblink_state_object_set_bool)(handle, name, key, value)
}
pub unsafe fn mblink_state_object_remove(
    handle: MBlinkHandle,
    name: *const c_char,
    key: *const c_char,
) -> c_int {
    (api().mblink_state_object_remove)(handle, name, key)
}
pub unsafe fn mblink_state_object_clear(handle: MBlinkHandle, name: *const c_char) -> c_int {
    (api().mblink_state_object_clear)(handle, name)
}
pub unsafe fn mblink_state_increment(handle: MBlinkHandle, name: *const c_char, delta: f64) -> c_int {
    (api().mblink_state_increment)(handle, name, delta)
}
pub unsafe fn mblink_state_multiply(handle: MBlinkHandle, name: *const c_char, factor: f64) -> c_int {
    (api().mblink_state_multiply)(handle, name, factor)
}
pub unsafe fn mblink_state_string_append(
    handle: MBlinkHandle,
    name: *const c_char,
    suffix: *const c_char,
) -> c_int {
    (api().mblink_state_string_append)(handle, name, suffix)
}
pub unsafe fn mblink_state_string_prepend(
    handle: MBlinkHandle,
    name: *const c_char,
    prefix: *const c_char,
) -> c_int {
    (api().mblink_state_string_prepend)(handle, name, prefix)
}
pub unsafe fn mblink_state_watch(
    handle: MBlinkHandle,
    name: *const c_char,
    callback: MBlinkStateCallback,
    user_data: *mut c_void,
) -> c_int {
    (api().mblink_state_watch)(handle, name, callback, user_data)
}
pub unsafe fn mblink_state_unwatch(handle: MBlinkHandle, watch_id: c_int) {
    (api().mblink_state_unwatch)(handle, watch_id)
}
pub unsafe fn mblink_state_batch_begin(handle: MBlinkHandle) {
    (api().mblink_state_batch_begin)(handle)
}
pub unsafe fn mblink_state_batch_end(handle: MBlinkHandle) {
    (api().mblink_state_batch_end)(handle)
}
pub unsafe fn mblink_state_set_merge_mode(handle: MBlinkHandle, enable: bool) {
    (api().mblink_state_set_merge_mode)(handle, enable)
}
pub unsafe fn mblink_process_queue(handle: MBlinkHandle) -> c_int {
    (api().mblink_process_queue)(handle)
}
pub unsafe fn mblink_queue_size(handle: MBlinkHandle) -> c_int {
    (api().mblink_queue_size)(handle)
}
pub unsafe fn mblink_shared_create(handle: MBlinkHandle, name: *const c_char) -> MBlinkSharedHandle {
    (api().mblink_shared_create)(handle, name)
}
pub unsafe fn mblink_shared_destroy(shared: MBlinkSharedHandle) {
    (api().mblink_shared_destroy)(shared)
}
pub unsafe fn mblink_shared_set_int(
    shared: MBlinkSharedHandle,
    key: *const c_char,
    value: i64,
) -> c_int {
    (api().mblink_shared_set_int)(shared, key, value)
}
pub unsafe fn mblink_shared_set_double(
    shared: MBlinkSharedHandle,
    key: *const c_char,
    value: f64,
) -> c_int {
    (api().mblink_shared_set_double)(shared, key, value)
}
pub unsafe fn mblink_shared_set_string(
    shared: MBlinkSharedHandle,
    key: *const c_char,
    value: *const c_char,
) -> c_int {
    (api().mblink_shared_set_string)(shared, key, value)
}
pub unsafe fn mblink_shared_set_bool(
    shared: MBlinkSharedHandle,
    key: *const c_char,
    value: bool,
) -> c_int {
    (api().mblink_shared_set_bool)(shared, key, value)
}
pub unsafe fn mblink_shared_set_null(shared: MBlinkSharedHandle, key: *const c_char) -> c_int {
    (api().mblink_shared_set_null)(shared, key)
}
pub unsafe fn mblink_shared_set_json(
    shared: MBlinkSharedHandle,
    key: *const c_char,
    json: *const c_char,
) -> c_int {
    (api().mblink_shared_set_json)(shared, key, json)
}
pub unsafe fn mblink_shared_get_int(shared: MBlinkSharedHandle, key: *const c_char) -> i64 {
    (api().mblink_shared_get_int)(shared, key)
}
pub unsafe fn mblink_shared_get_double(shared: MBlinkSharedHandle, key: *const c_char) -> f64 {
    (api().mblink_shared_get_double)(shared, key)
}
pub unsafe fn mblink_shared_get_string(
    shared: MBlinkSharedHandle,
    key: *const c_char,
) -> *mut c_char {
    (api().mblink_shared_get_string)(shared, key)
}
pub unsafe fn mblink_shared_get_bool(shared: MBlinkSharedHandle, key: *const c_char) -> bool {
    (api().mblink_shared_get_bool)(shared, key)
}
pub unsafe fn mblink_shared_get_json(shared: MBlinkSharedHandle, key: *const c_char) -> *mut c_char {
    (api().mblink_shared_get_json)(shared, key)
}
pub unsafe fn mblink_shared_get_type(shared: MBlinkSharedHandle, key: *const c_char) -> c_int {
    (api().mblink_shared_get_type)(shared, key)
}
pub unsafe fn mblink_shared_delete(shared: MBlinkSharedHandle, key: *const c_char) -> c_int {
    (api().mblink_shared_delete)(shared, key)
}
pub unsafe fn mblink_shared_has(shared: MBlinkSharedHandle, key: *const c_char) -> bool {
    (api().mblink_shared_has)(shared, key)
}
pub unsafe fn mblink_shared_batch_begin(shared: MBlinkSharedHandle) {
    (api().mblink_shared_batch_begin)(shared)
}
pub unsafe fn mblink_shared_batch_end(shared: MBlinkSharedHandle) {
    (api().mblink_shared_batch_end)(shared)
}
pub unsafe fn mblink_logview_get(
    handle: MBlinkHandle,
    element_id: *const c_char,
) -> MBlinkLogViewHandle {
    (api().mblink_logview_get)(handle, element_id)
}
pub unsafe fn mblink_logview_destroy(logview: MBlinkLogViewHandle) {
    (api().mblink_logview_destroy)(logview)
}
pub unsafe fn mblink_logview_append(
    logview: MBlinkLogViewHandle,
    level: *const c_char,
    source: *const c_char,
    message: *const c_char,
) -> c_int {
    (api().mblink_logview_append)(logview, level, source, message)
}
pub unsafe fn mblink_logview_clear(logview: MBlinkLogViewHandle) {
    (api().mblink_logview_clear)(logview)
}
pub unsafe fn mblink_logview_export(
    logview: MBlinkLogViewHandle,
    format: *const c_char,
) -> *mut c_char {
    (api().mblink_logview_export)(logview, format)
}
pub unsafe fn mblink_terminal_get(
    handle: MBlinkHandle,
    element_id: *const c_char,
) -> MBlinkTerminalHandle {
    (api().mblink_terminal_get)(handle, element_id)
}
pub unsafe fn mblink_terminal_destroy(terminal: MBlinkTerminalHandle) {
    (api().mblink_terminal_destroy)(terminal)
}
pub unsafe fn mblink_terminal_write(terminal: MBlinkTerminalHandle, data: *const c_char) -> c_int {
    (api().mblink_terminal_write)(terminal, data)
}
pub unsafe fn mblink_terminal_clear(terminal: MBlinkTerminalHandle) {
    (api().mblink_terminal_clear)(terminal)
}
pub unsafe fn mblink_terminal_execute(
    terminal: MBlinkTerminalHandle,
    command: *const c_char,
) -> c_int {
    (api().mblink_terminal_execute)(terminal, command)
}
pub unsafe fn mblink_terminal_start_shell(
    terminal: MBlinkTerminalHandle,
    shell: *const c_char,
) -> c_int {
    (api().mblink_terminal_start_shell)(terminal, shell)
}
pub unsafe fn mblink_terminal_send_input(
    terminal: MBlinkTerminalHandle,
    input: *const c_char,
) -> c_int {
    (api().mblink_terminal_send_input)(terminal, input)
}
pub unsafe fn mblink_terminal_resize(terminal: MBlinkTerminalHandle, rows: c_int, cols: c_int) {
    (api().mblink_terminal_resize)(terminal, rows, cols)
}
pub unsafe fn mblink_terminal_serialize(terminal: MBlinkTerminalHandle) -> *mut c_char {
    (api().mblink_terminal_serialize)(terminal)
}
pub unsafe fn mblink_copy_string(str_: *const c_char) -> *mut c_char {
    (api().mblink_copy_string)(str_)
}
pub unsafe fn mblink_free(ptr: *mut c_void) {
    (api().mblink_free)(ptr)
}
