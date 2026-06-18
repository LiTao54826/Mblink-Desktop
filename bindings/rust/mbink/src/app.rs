use std::ffi::{CStr, CString};
use std::io::{Read, Write};
use std::net::TcpStream;
use std::sync::mpsc;
use std::sync::Once;
use std::thread;

use serde_json::Value;

use crate::callback::{
    bind_async_trampoline, bind_trampoline, bool_trampoline, resize_trampoline,
    state_watch_trampoline, update_trampoline, void_trampoline, AsyncBindHolder, BindHolder,
    BindKind, BindRegistration, BoolHolder, EventRegistry, ResizeHolder, StateWatchHolder,
    StateWatchRegistration, UpdateHolder, VoidHolder,
};
use crate::config::AppBuilder;
use crate::controls::{
    logview_from_handle, terminal_from_handle, ControlHandle, LogView, Terminal,
};
use crate::shared::Shared;
use crate::state::State;
use crate::util::{string_from_const_ptr, string_from_owned_ptr, to_cstring};
use crate::{Error, Result};

static INIT: Once = Once::new();

#[derive(Debug, Clone, Copy)]
pub struct AppHandle {
    handle: mbink_sys::MBinkHandle,
}

#[derive(Debug, Clone)]
pub struct RuntimeOptions {
    pub runtime_epoch: Option<String>,
    pub load_embedded_runtime: bool,
    pub load_official_preact: bool,
}

impl Default for RuntimeOptions {
    fn default() -> Self {
        Self {
            runtime_epoch: None,
            load_embedded_runtime: true,
            load_official_preact: true,
        }
    }
}

#[derive(Debug, Clone)]
pub struct UiDevSnapshotOptions {
    pub runtime_epoch: Option<String>,
    pub max_nodes: usize,
    pub max_depth: i32,
    pub root_selector: Option<String>,
    pub include_screenshot: bool,
    pub inline_screenshot: bool,
    pub screenshot_file: Option<String>,
}

#[derive(Debug, Clone)]
pub struct DevToolsHttpOptions {
    pub bind_host: Option<String>,
    pub port: u16,
    pub auth_token: Option<String>,
    pub require_auth: bool,
}

impl Default for DevToolsHttpOptions {
    fn default() -> Self {
        Self {
            bind_host: None,
            port: 0,
            auth_token: None,
            require_auth: true,
        }
    }
}

#[derive(Debug, Clone)]
pub struct DevToolsHttpSession {
    url: String,
    port: u16,
    auth_token: String,
    require_auth: bool,
    handle: mbink_sys::MBinkHandle,
    next_id: i64,
}

impl DevToolsHttpSession {
    pub fn url(&self) -> &str {
        &self.url
    }

    pub fn port(&self) -> u16 {
        self.port
    }

    pub fn auth_token(&self) -> &str {
        &self.auth_token
    }

    pub fn require_auth(&self) -> bool {
        self.require_auth
    }

    pub fn request(&mut self, method: &str, params: Option<Value>) -> Result<Value> {
        self.next_id += 1;
        let mut payload = serde_json::json!({
            "jsonrpc": "2.0",
            "id": self.next_id,
            "method": method,
        });
        if let Some(params) = params {
            payload["params"] = params;
        }

        let url = self.url.clone();
        let auth_token = self.auth_token.clone();
        let (tx, rx) = mpsc::sync_channel(1);
        thread::spawn(move || {
            let _ = tx.send(post_jsonrpc(&url, &auth_token, &payload));
        });

        loop {
            match rx.try_recv() {
                Ok(result) => return result,
                Err(mpsc::TryRecvError::Empty) => {
                    unsafe { mbink_sys::mbink_wait_events(self.handle) };
                }
                Err(mpsc::TryRecvError::Disconnected) => {
                    return Err(Error::Message(
                        "devtools http request worker stopped before returning".into(),
                    ));
                }
            }
        }
    }

    pub fn stop(self) -> Result<()> {
        check_rc_raw(unsafe { mbink_sys::mbink_devtools_http_stop(self.handle) })
    }
}

impl Default for UiDevSnapshotOptions {
    fn default() -> Self {
        Self {
            runtime_epoch: None,
            max_nodes: 2000,
            max_depth: 64,
            root_selector: None,
            include_screenshot: false,
            inline_screenshot: false,
            screenshot_file: None,
        }
    }
}

impl AppHandle {
    pub fn stop(self) {
        unsafe { mbink_sys::mbink_stop(self.handle) };
    }

    pub fn show(self) -> Result<()> {
        check_rc_raw(unsafe { mbink_sys::mbink_show(self.handle) })
    }

    pub fn hide(self) -> Result<()> {
        check_rc_raw(unsafe { mbink_sys::mbink_hide(self.handle) })
    }

    pub fn restore(self) -> Result<()> {
        check_rc_raw(unsafe { mbink_sys::mbink_restore(self.handle) })
    }

    pub fn set_title(self, title: &str) -> Result<()> {
        let title = to_cstring(title)?;
        check_rc_raw(unsafe { mbink_sys::mbink_set_title(self.handle, title.as_ptr()) })
    }

    pub fn set_always_on_top(self, on_top: bool) -> Result<()> {
        check_rc_raw(unsafe { mbink_sys::mbink_set_always_on_top(self.handle, on_top) })
    }

    pub fn show_main_window(self) -> Result<()> {
        self.show()?;
        self.restore()
    }

    pub fn hide_to_tray(self) -> Result<()> {
        self.hide()
    }
}

pub struct App {
    handle: mbink_sys::MBinkHandle,
    shared_handles: Vec<mbink_sys::MBinkSharedHandle>,
    control_handles: Vec<ControlHandle>,
    bind_callbacks: Vec<BindRegistration>,
    state_watchers: Vec<StateWatchRegistration>,
    event_callbacks: EventRegistry,
}

impl App {
    pub fn new(title: &str, width: i32, height: i32) -> Result<Self> {
        Self::builder().title(title).size(width, height).build()
    }

    pub fn builder() -> AppBuilder {
        AppBuilder::default()
    }

    pub fn build(builder: AppBuilder) -> Result<Self> {
        ensure_init();
        let title = CString::new(builder.title_str()).map_err(Error::Nul)?;
        let mut cfg = unsafe { mbink_sys::mbink_default_config() };
        cfg.title = title.as_ptr();
        builder.apply_to_raw(&mut cfg);

        let handle = unsafe { mbink_sys::mbink_create_ex(&cfg) };
        if handle.is_null() {
            return Err(Error::NullHandle);
        }
        let runtime_options = unsafe { mbink_sys::mbink_default_runtime_options() };
        let rc = unsafe { mbink_sys::mbink_configure_runtime(handle, &runtime_options) };
        if rc != 0 {
            unsafe { mbink_sys::mbink_destroy(handle) };
            check_rc_raw(rc)?;
        }

        let mut app = Self {
            handle,
            shared_handles: Vec::new(),
            control_handles: Vec::new(),
            bind_callbacks: Vec::new(),
            state_watchers: Vec::new(),
            event_callbacks: EventRegistry::default(),
        };
        app.install_default_on_close_stop()?;
        Ok(app)
    }

    pub fn handle(&self) -> AppHandle {
        AppHandle {
            handle: self.handle,
        }
    }

    pub fn run(&mut self) {
        if self.handle.is_null() {
            return;
        }
        unsafe { mbink_sys::mbink_run(self.handle) };
        self.cleanup_native();
    }

    pub fn stop(&self) {
        unsafe { mbink_sys::mbink_stop(self.handle) };
    }

    fn install_default_on_close_stop(&mut self) -> Result<()> {
        let handle = self.handle;
        let holder = Box::new(VoidHolder {
            callback: Box::new(move || unsafe {
                mbink_sys::mbink_stop(handle);
            }),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_close(self.handle, Some(void_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_close = Some(user_data);
        Ok(())
    }

    fn cleanup_native(&mut self) {
        if self.handle.is_null() {
            return;
        }

        for control in self.control_handles.drain(..).rev() {
            unsafe {
                match control {
                    ControlHandle::LogView(handle) => mbink_sys::mbink_logview_destroy(handle),
                    ControlHandle::Terminal(handle) => mbink_sys::mbink_terminal_destroy(handle),
                }
            }
        }
        for shared in self.shared_handles.drain(..).rev() {
            unsafe { mbink_sys::mbink_shared_destroy(shared) };
        }
        for watcher in self.state_watchers.drain(..).rev() {
            unsafe {
                mbink_sys::mbink_state_unwatch(self.handle, watcher.watch_id);
                drop(Box::from_raw(watcher.user_data));
            }
        }
        unsafe { mbink_sys::mbink_destroy(self.handle) };
        self.handle = std::ptr::null_mut();

        for reg in self.bind_callbacks.drain(..) {
            unsafe { Self::drop_bind_registration(reg) };
        }
        if let Some(ptr) = self.event_callbacks.on_resize.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_close.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_close_request.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_focus.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_blur.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_update.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_tray_click.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        if let Some(ptr) = self.event_callbacks.on_tray_menu.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
    }

    pub fn poll(&self) -> Result<bool> {
        Ok(unsafe { mbink_sys::mbink_poll_events(self.handle) })
    }

    pub fn wait(&self) -> Result<bool> {
        Ok(unsafe { mbink_sys::mbink_wait_events(self.handle) })
    }

    pub fn load_html(&self, html: &str) -> Result<&Self> {
        let html = to_cstring(html)?;
        self.check_rc(unsafe { mbink_sys::mbink_load_html(self.handle, html.as_ptr()) })?;
        Ok(self)
    }

    pub fn load_html_file(&self, path: &str) -> Result<&Self> {
        let path = to_cstring(path)?;
        self.check_rc(unsafe { mbink_sys::mbink_load_html_file(self.handle, path.as_ptr()) })?;
        Ok(self)
    }

    pub fn load_entry_file(&self, path: &str, execute_html_scripts: bool) -> Result<&Self> {
        let path = to_cstring(path)?;
        self.check_rc(unsafe {
            mbink_sys::mbink_load_entry_file(self.handle, path.as_ptr(), execute_html_scripts)
        })?;
        Ok(self)
    }

    pub fn eval_js(&self, code: &str) -> Result<&Self> {
        let code = to_cstring(code)?;
        self.check_rc(unsafe { mbink_sys::mbink_eval_js(self.handle, code.as_ptr()) })?;
        Ok(self)
    }

    pub fn eval_module(&self, code: &str, filename: &str) -> Result<&Self> {
        let code = to_cstring(code)?;
        let filename = to_cstring(filename)?;
        self.check_rc(unsafe {
            mbink_sys::mbink_eval_module(self.handle, code.as_ptr(), filename.as_ptr())
        })?;
        Ok(self)
    }

    pub fn load_js_file(&self, path: &str) -> Result<&Self> {
        let path = to_cstring(path)?;
        self.check_rc(unsafe { mbink_sys::mbink_load_js_file(self.handle, path.as_ptr()) })?;
        Ok(self)
    }

    pub fn load_module_file(&self, path: &str) -> Result<&Self> {
        let path = to_cstring(path)?;
        self.check_rc(unsafe { mbink_sys::mbink_load_module_file(self.handle, path.as_ptr()) })?;
        Ok(self)
    }

    pub fn load_bytecode(&self, data: &[u8]) -> Result<&Self> {
        self.check_rc(unsafe {
            mbink_sys::mbink_load_bytecode(self.handle, data.as_ptr().cast(), data.len())
        })?;
        Ok(self)
    }

    pub fn mount_resource_package(
        &self,
        package_file: &str,
        encryption_key: &str,
        mount_point: &str,
    ) -> Result<&Self> {
        let package_file = to_cstring(package_file)?;
        let encryption_key = to_cstring(encryption_key)?;
        let mount_point = to_cstring(mount_point)?;
        self.check_rc(unsafe {
            mbink_sys::mbink_mount_resource_package(
                self.handle,
                package_file.as_ptr(),
                encryption_key.as_ptr(),
                mount_point.as_ptr(),
            )
        })?;
        Ok(self)
    }

    pub fn configure_runtime(&self, options: RuntimeOptions) -> Result<&Self> {
        let epoch = options
            .runtime_epoch
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let mut raw = unsafe { mbink_sys::mbink_default_runtime_options() };
        raw.runtime_epoch = epoch.as_ref().map_or(std::ptr::null(), |v| v.as_ptr());
        raw.load_embedded_runtime = options.load_embedded_runtime;
        raw.load_official_preact = options.load_official_preact;
        self.check_rc(unsafe { mbink_sys::mbink_configure_runtime(self.handle, &raw) })?;
        Ok(self)
    }

    pub fn load_embedded_runtime(&self, include_official_preact: bool) -> Result<&Self> {
        self.check_rc(unsafe {
            mbink_sys::mbink_load_embedded_runtime(self.handle, include_official_preact)
        })?;
        Ok(self)
    }

    pub fn render_frame(&self, passes: i32) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_render_frame(self.handle, passes) })?;
        Ok(self)
    }

    pub fn runtime_epoch(&self) -> Result<String> {
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe { mbink_sys::mbink_runtime_epoch(self.handle, &mut out) })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn lifecycle_state(&self) -> crate::LifecycleState {
        crate::LifecycleState::from_raw(unsafe { mbink_sys::mbink_lifecycle_state(self.handle) })
    }

    pub fn lifecycle_reason(&self) -> Result<String> {
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe { mbink_sys::mbink_lifecycle_reason(self.handle, &mut out) })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn observe_console_json(&self) -> Result<String> {
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe { mbink_sys::mbink_observe_console_json(self.handle, &mut out) })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn observe_errors_json(&self) -> Result<String> {
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe { mbink_sys::mbink_observe_errors_json(self.handle, &mut out) })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn observe_lifecycle_json(&self) -> Result<String> {
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe { mbink_sys::mbink_observe_lifecycle_json(self.handle, &mut out) })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn observe_console(&self) -> Result<Value> {
        Ok(serde_json::from_str(&self.observe_console_json()?)?)
    }

    pub fn observe_errors(&self) -> Result<Value> {
        Ok(serde_json::from_str(&self.observe_errors_json()?)?)
    }

    pub fn observe_lifecycle(&self) -> Result<Value> {
        Ok(serde_json::from_str(&self.observe_lifecycle_json()?)?)
    }

    pub fn observe_clear(&self, kind: crate::ObserveKind) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_observe_clear(self.handle, kind.to_raw()) })?;
        Ok(self)
    }

    pub fn ui_dev_snapshot_json(&self, options: UiDevSnapshotOptions) -> Result<String> {
        let runtime_epoch = options
            .runtime_epoch
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let root_selector = options
            .root_selector
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let screenshot_file = options
            .screenshot_file
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let mut raw = unsafe { mbink_sys::mbink_ui_dev_default_snapshot_options() };
        raw.runtime_epoch = runtime_epoch
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        raw.max_nodes = options.max_nodes;
        raw.max_depth = options.max_depth;
        raw.root_selector = root_selector
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        raw.include_screenshot = options.include_screenshot;
        raw.inline_screenshot = options.inline_screenshot;
        raw.screenshot_file = screenshot_file
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe {
            mbink_sys::mbink_ui_dev_snapshot_json(self.handle, &raw, &mut out)
        })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn ui_dev_snapshot(&self, options: UiDevSnapshotOptions) -> Result<Value> {
        Ok(serde_json::from_str(&self.ui_dev_snapshot_json(options)?)?)
    }

    pub fn ui_dev_snapshot_file(
        &self,
        output_path: &str,
        options: UiDevSnapshotOptions,
    ) -> Result<&Self> {
        let output_path = to_cstring(output_path)?;
        let runtime_epoch = options
            .runtime_epoch
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let root_selector = options
            .root_selector
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let screenshot_file = options
            .screenshot_file
            .as_deref()
            .map(to_cstring)
            .transpose()?;
        let mut raw = unsafe { mbink_sys::mbink_ui_dev_default_snapshot_options() };
        raw.runtime_epoch = runtime_epoch
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        raw.max_nodes = options.max_nodes;
        raw.max_depth = options.max_depth;
        raw.root_selector = root_selector
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        raw.include_screenshot = options.include_screenshot;
        raw.inline_screenshot = options.inline_screenshot;
        raw.screenshot_file = screenshot_file
            .as_ref()
            .map_or(std::ptr::null(), |v| v.as_ptr());
        self.check_rc(unsafe {
            mbink_sys::mbink_ui_dev_snapshot_file(self.handle, output_path.as_ptr(), &raw)
        })?;
        Ok(self)
    }

    pub fn ui_dev_command_json(&self, command_json: &str) -> Result<String> {
        let command_json = to_cstring(command_json)?;
        let mut out = std::ptr::null_mut();
        self.check_rc(unsafe {
            mbink_sys::mbink_ui_dev_command_json(self.handle, command_json.as_ptr(), &mut out)
        })?;
        unsafe { string_from_owned_ptr(out) }
    }

    pub fn ui_dev_command<T: serde::Serialize>(&self, command: &T) -> Result<Value> {
        let command_json = serde_json::to_string(command)?;
        Ok(serde_json::from_str(
            &self.ui_dev_command_json(&command_json)?,
        )?)
    }

    pub fn set_title(&self, title: &str) -> Result<&Self> {
        let title = to_cstring(title)?;
        self.check_rc(unsafe { mbink_sys::mbink_set_title(self.handle, title.as_ptr()) })?;
        Ok(self)
    }

    pub fn create_tray(&self, tooltip: &str) -> Result<&Self> {
        let tooltip = to_cstring(tooltip)?;
        self.check_rc(unsafe { mbink_sys::mbink_tray_create(self.handle, tooltip.as_ptr()) })?;
        Ok(self)
    }

    pub fn destroy_tray(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_tray_destroy(self.handle) })?;
        Ok(self)
    }

    pub fn set_tray_tooltip(&self, tooltip: &str) -> Result<&Self> {
        let tooltip = to_cstring(tooltip)?;
        self.check_rc(unsafe { mbink_sys::mbink_tray_set_tooltip(self.handle, tooltip.as_ptr()) })?;
        Ok(self)
    }

    pub fn set_tray_menu_json(&self, menu_json: &str) -> Result<&Self> {
        let menu_json = to_cstring(menu_json)?;
        self.check_rc(unsafe { mbink_sys::mbink_tray_set_menu(self.handle, menu_json.as_ptr()) })?;
        Ok(self)
    }

    pub fn set_tray_menu<T: serde::Serialize>(&self, menu: &T) -> Result<&Self> {
        self.set_tray_menu_json(&serde_json::to_string(menu)?)
    }

    pub fn set_size(&self, width: i32, height: i32) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_size(self.handle, width, height) })?;
        Ok(self)
    }

    pub fn size(&self) -> Result<(i32, i32)> {
        let mut width = 0;
        let mut height = 0;
        self.check_rc(unsafe { mbink_sys::mbink_get_size(self.handle, &mut width, &mut height) })?;
        Ok((width, height))
    }

    pub fn set_position(&self, x: i32, y: i32) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_position(self.handle, x, y) })?;
        Ok(self)
    }

    pub fn position(&self) -> Result<(i32, i32)> {
        let mut x = 0;
        let mut y = 0;
        self.check_rc(unsafe { mbink_sys::mbink_get_position(self.handle, &mut x, &mut y) })?;
        Ok((x, y))
    }

    pub fn set_min_size(&self, width: i32, height: i32) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_min_size(self.handle, width, height) })?;
        Ok(self)
    }

    pub fn set_max_size(&self, width: i32, height: i32) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_max_size(self.handle, width, height) })?;
        Ok(self)
    }

    pub fn show(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_show(self.handle) })?;
        Ok(self)
    }

    pub fn hide(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_hide(self.handle) })?;
        Ok(self)
    }

    pub fn minimize(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_minimize(self.handle) })?;
        Ok(self)
    }

    pub fn maximize(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_maximize(self.handle) })?;
        Ok(self)
    }

    pub fn restore(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_restore(self.handle) })?;
        Ok(self)
    }

    pub fn set_fullscreen(&self, fullscreen: bool) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_fullscreen(self.handle, fullscreen) })?;
        Ok(self)
    }

    pub fn set_resizable(&self, resizable: bool) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_resizable(self.handle, resizable) })?;
        Ok(self)
    }

    pub fn set_borderless(&self, borderless: bool) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_borderless(self.handle, borderless) })?;
        Ok(self)
    }

    pub fn set_always_on_top(&self, on_top: bool) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_set_always_on_top(self.handle, on_top) })?;
        Ok(self)
    }

    pub fn show_main_window(&self) -> Result<&Self> {
        self.show()?;
        self.restore()?;
        Ok(self)
    }

    pub fn hide_to_tray(&self) -> Result<&Self> {
        self.hide()?;
        Ok(self)
    }

    pub fn emit_json<T: serde::Serialize>(&self, event: &str, value: &T) -> Result<()> {
        let json = serde_json::to_string(value)?;
        self.emit_str(event, &json)
    }

    pub fn emit_str(&self, event: &str, json: &str) -> Result<()> {
        let event = to_cstring(event)?;
        let json = to_cstring(json)?;
        self.check_rc(unsafe { mbink_sys::mbink_emit(self.handle, event.as_ptr(), json.as_ptr()) })
    }

    pub fn bind<F>(&mut self, name: &str, callback: F) -> Result<()>
    where
        F: Fn(Value) -> Result<Value> + 'static,
    {
        let name_c = to_cstring(name)?;
        let holder = Box::new(BindHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder).cast();
        let rc = unsafe {
            mbink_sys::mbink_bind(
                self.handle,
                name_c.as_ptr(),
                Some(bind_trampoline),
                user_data,
            )
        };
        if rc != 0 {
            unsafe { self.drop_bind_user_data(BindKind::Sync, user_data) };
            return self.check_rc(rc);
        }
        self.bind_callbacks.push(BindRegistration {
            name: name.to_string(),
            kind: BindKind::Sync,
            user_data,
        });
        Ok(())
    }

    pub fn bind_async<F>(&mut self, name: &str, callback: F) -> Result<()>
    where
        F: Fn(Value) -> Result<Value> + 'static,
    {
        let name_c = to_cstring(name)?;
        let holder = Box::new(AsyncBindHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder).cast();
        let rc = unsafe {
            mbink_sys::mbink_bind_async(
                self.handle,
                name_c.as_ptr(),
                Some(bind_async_trampoline),
                user_data,
            )
        };
        if rc != 0 {
            unsafe { self.drop_bind_user_data(BindKind::Async, user_data) };
            return self.check_rc(rc);
        }
        self.bind_callbacks.push(BindRegistration {
            name: name.to_string(),
            kind: BindKind::Async,
            user_data,
        });
        Ok(())
    }

    pub fn unbind(&mut self, name: &str) -> Result<()> {
        let name_c = to_cstring(name)?;
        unsafe { mbink_sys::mbink_unbind(self.handle, name_c.as_ptr()) };
        if let Some(index) = self.bind_callbacks.iter().position(|it| it.name == name) {
            let reg = self.bind_callbacks.swap_remove(index);
            unsafe { Self::drop_bind_registration(reg) };
        }
        Ok(())
    }

    pub fn on_tray_click<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn() + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_tray_click.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(VoidHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_tray_set_left_click_callback(
                self.handle,
                Some(void_trampoline),
                user_data.cast(),
            )
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_tray_click = Some(user_data);
        Ok(())
    }

    pub fn on_tray_menu<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn(Value) -> Result<Value> + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_tray_menu.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(BindHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_tray_set_menu_callback(
                self.handle,
                Some(bind_trampoline),
                user_data.cast(),
            )
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_tray_menu = Some(user_data);
        Ok(())
    }

    pub fn on_resize<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn(i32, i32) + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_resize.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(ResizeHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_resize(self.handle, Some(resize_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_resize = Some(user_data);
        Ok(())
    }

    pub fn on_close<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn() + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_close.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let handle = self.handle;
        let holder = Box::new(VoidHolder {
            callback: Box::new(move || {
                callback();
                unsafe { mbink_sys::mbink_stop(handle) };
            }),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_close(self.handle, Some(void_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_close = Some(user_data);
        Ok(())
    }

    pub fn on_close_request<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn() -> bool + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_close_request.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(BoolHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_close_request(self.handle, Some(bool_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_close_request = Some(user_data);
        Ok(())
    }

    pub fn on_focus<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn() + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_focus.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(VoidHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_focus(self.handle, Some(void_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_focus = Some(user_data);
        Ok(())
    }

    pub fn on_blur<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn() + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_blur.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(VoidHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_blur(self.handle, Some(void_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_blur = Some(user_data);
        Ok(())
    }

    pub fn on_update<F>(&mut self, callback: F) -> Result<()>
    where
        F: Fn(f32) + 'static,
    {
        if let Some(ptr) = self.event_callbacks.on_update.take() {
            unsafe { drop(Box::from_raw(ptr)) };
        }
        let holder = Box::new(UpdateHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let rc = unsafe {
            mbink_sys::mbink_on_update(self.handle, Some(update_trampoline), user_data.cast())
        };
        if rc != 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            return self.check_rc(rc);
        }
        self.event_callbacks.on_update = Some(user_data);
        Ok(())
    }

    pub fn state(&self) -> State<'_> {
        State { app: self }
    }

    pub fn shared(&mut self, name: &str) -> Result<Shared<'_>> {
        let name = to_cstring(name)?;
        let handle = unsafe { mbink_sys::mbink_shared_create(self.handle, name.as_ptr()) };
        if handle.is_null() {
            return Err(Error::NullHandle);
        }
        self.shared_handles.push(handle);
        Ok(Shared {
            handle,
            _marker: std::marker::PhantomData,
        })
    }

    pub fn logview(&mut self, element_id: &str) -> Result<LogView<'_>> {
        let element_id = to_cstring(element_id)?;
        let handle = unsafe { mbink_sys::mbink_logview_get(self.handle, element_id.as_ptr()) };
        let logview = logview_from_handle(handle)?;
        self.control_handles.push(ControlHandle::LogView(handle));
        Ok(logview)
    }

    pub fn terminal(&mut self, element_id: &str) -> Result<Terminal<'_>> {
        let element_id = to_cstring(element_id)?;
        let handle = unsafe { mbink_sys::mbink_terminal_get(self.handle, element_id.as_ptr()) };
        let terminal = terminal_from_handle(handle)?;
        self.control_handles.push(ControlHandle::Terminal(handle));
        Ok(terminal)
    }

    pub fn watch_state<F>(&mut self, name: &str, callback: F) -> Result<i32>
    where
        F: Fn(&str, Value) + 'static,
    {
        self.register_state_watch(name, callback)
    }

    pub fn unwatch_state(&mut self, watch_id: i32) {
        self.unregister_state_watch(watch_id);
    }

    pub(crate) fn register_state_watch<F>(&mut self, name: &str, callback: F) -> Result<i32>
    where
        F: Fn(&str, Value) + 'static,
    {
        let name = to_cstring(name)?;
        let holder = Box::new(StateWatchHolder {
            callback: Box::new(callback),
        });
        let user_data = Box::into_raw(holder);
        let watch_id = unsafe {
            mbink_sys::mbink_state_watch(
                self.handle,
                name.as_ptr(),
                Some(state_watch_trampoline),
                user_data.cast(),
            )
        };
        if watch_id < 0 {
            unsafe { drop(Box::from_raw(user_data)) };
            self.check_rc(watch_id)?;
        }
        self.state_watchers.push(StateWatchRegistration {
            watch_id,
            user_data,
        });
        Ok(watch_id)
    }

    pub(crate) fn unregister_state_watch(&mut self, watch_id: i32) {
        if let Some(index) = self
            .state_watchers
            .iter()
            .position(|entry| entry.watch_id == watch_id)
        {
            let watcher = self.state_watchers.swap_remove(index);
            unsafe {
                mbink_sys::mbink_state_unwatch(self.handle, watch_id);
                drop(Box::from_raw(watcher.user_data));
            }
        }
    }

    pub fn version() -> Result<String> {
        ensure_init();
        unsafe { string_from_const_ptr(mbink_sys::mbink_version()) }
    }

    pub fn devtools_open(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_devtools_open(self.handle) })?;
        Ok(self)
    }

    pub fn devtools_close(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_devtools_close(self.handle) })?;
        Ok(self)
    }

    pub fn enable_devtools(&self) -> Result<&Self> {
        self.devtools_open()
    }

    pub fn enable_devtools_http(
        &self,
        options: DevToolsHttpOptions,
    ) -> Result<DevToolsHttpSession> {
        self.devtools_http_session(options)
    }

    pub fn devtools_http_session(
        &self,
        options: DevToolsHttpOptions,
    ) -> Result<DevToolsHttpSession> {
        let bind_host = options.bind_host.as_deref().map(to_cstring).transpose()?;
        let auth_token = options.auth_token.as_deref().map(to_cstring).transpose()?;
        let mut raw = unsafe { mbink_sys::mbink_devtools_default_http_options() };
        raw.bind_host = bind_host.as_ref().map_or(std::ptr::null(), |v| v.as_ptr());
        raw.port = options.port;
        raw.auth_token = auth_token.as_ref().map_or(std::ptr::null(), |v| v.as_ptr());
        raw.require_auth = options.require_auth;

        let mut info = mbink_sys::MBinkDevToolsHttpInfo {
            port: 0,
            url: std::ptr::null_mut(),
            auth_token: std::ptr::null_mut(),
        };
        self.check_rc(unsafe {
            mbink_sys::mbink_devtools_http_start(self.handle, &raw, &mut info)
        })?;
        let url = unsafe { string_from_raw_http_field(info.url) }?;
        let token = unsafe { string_from_raw_http_field(info.auth_token) }?;
        let port = info.port;
        unsafe { mbink_sys::mbink_devtools_http_info_free(&mut info) };
        Ok(DevToolsHttpSession {
            url,
            port,
            auth_token: token,
            require_auth: options.require_auth,
            handle: self.handle,
            next_id: 0,
        })
    }

    pub fn devtools_http_stop(&self) -> Result<&Self> {
        self.check_rc(unsafe { mbink_sys::mbink_devtools_http_stop(self.handle) })?;
        Ok(self)
    }

    pub(crate) fn raw_handle(&self) -> mbink_sys::MBinkHandle {
        self.handle
    }

    pub(crate) fn check_rc(&self, rc: i32) -> Result<()> {
        check_rc_raw(rc)
    }

    unsafe fn drop_bind_registration(reg: BindRegistration) {
        match reg.kind {
            BindKind::Sync => drop(Box::from_raw(reg.user_data as *mut BindHolder)),
            BindKind::Async => drop(Box::from_raw(reg.user_data as *mut AsyncBindHolder)),
        }
    }

    unsafe fn drop_bind_user_data(&self, kind: BindKind, user_data: *mut std::ffi::c_void) {
        match kind {
            BindKind::Sync => drop(Box::from_raw(user_data as *mut BindHolder)),
            BindKind::Async => drop(Box::from_raw(user_data as *mut AsyncBindHolder)),
        }
    }
}

impl Drop for App {
    fn drop(&mut self) {
        self.cleanup_native();
    }
}

fn ensure_init() {
    INIT.call_once(|| {
        let _ = unsafe { mbink_sys::mbink_init() };
    });
}

pub(crate) fn check_rc_raw(rc: i32) -> Result<()> {
    if rc == 0 {
        return Ok(());
    }

    let message = unsafe { string_from_const_ptr(mbink_sys::mbink_last_error()) }
        .unwrap_or_else(|_| "unknown MBink error".to_string());
    Err(Error::Mbink { code: rc, message })
}

unsafe fn string_from_raw_http_field(ptr: *mut std::ffi::c_char) -> Result<String> {
    if ptr.is_null() {
        return Ok(String::new());
    }
    CStr::from_ptr(ptr)
        .to_str()
        .map(|s| s.to_owned())
        .map_err(Error::Utf8)
}

fn post_jsonrpc(url: &str, auth_token: &str, payload: &Value) -> Result<Value> {
    let (host, port, path) = parse_local_http_url(url)?;
    let body = serde_json::to_string(payload)?;
    let mut request = format!(
        "POST {path} HTTP/1.1\r\nHost: {host}:{port}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n",
        body.as_bytes().len()
    );
    if !auth_token.is_empty() {
        request.push_str("X-MBINK-DevTools-Token: ");
        request.push_str(auth_token);
        request.push_str("\r\n");
    }
    request.push_str("\r\n");
    request.push_str(&body);

    let mut stream = TcpStream::connect((host.as_str(), port))
        .map_err(|err| Error::Message(format!("devtools http connect failed: {err}")))?;
    stream
        .write_all(request.as_bytes())
        .map_err(|err| Error::Message(format!("devtools http request failed: {err}")))?;
    let mut response = String::new();
    stream
        .read_to_string(&mut response)
        .map_err(|err| Error::Message(format!("devtools http response failed: {err}")))?;
    let body_start = response
        .find("\r\n\r\n")
        .map(|index| index + 4)
        .ok_or_else(|| Error::Message("devtools http response missing headers".to_string()))?;
    Ok(serde_json::from_str(&response[body_start..])?)
}

fn parse_local_http_url(url: &str) -> Result<(String, u16, String)> {
    let rest = url
        .strip_prefix("http://")
        .ok_or_else(|| Error::Message("devtools url must use http://".to_string()))?;
    let (authority, path) = rest
        .split_once('/')
        .map(|(authority, path)| (authority, format!("/{path}")))
        .unwrap_or((rest, "/".to_string()));
    let (host, port_text) = authority
        .rsplit_once(':')
        .ok_or_else(|| Error::Message("devtools url is missing a port".to_string()))?;
    let port = port_text
        .parse::<u16>()
        .map_err(|err| Error::Message(format!("devtools url port is invalid: {err}")))?;
    Ok((host.to_string(), port, path))
}
