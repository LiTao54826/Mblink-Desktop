use std::ffi::CString;
use std::sync::Once;

use serde_json::Value;

use crate::callback::{
    bind_async_trampoline, bind_trampoline, bool_trampoline, resize_trampoline,
    update_trampoline, void_trampoline, AsyncBindHolder, BindHolder, BindKind,
    BindRegistration, BoolHolder, EventRegistry, ResizeHolder, UpdateHolder, VoidHolder,
};
use crate::config::AppBuilder;
use crate::shared::Shared;
use crate::state::State;
use crate::util::{string_from_const_ptr, to_cstring};
use crate::{Error, Result};

static INIT: Once = Once::new();

pub struct App {
    handle: mbink_sys::MBinkHandle,
    shared_handles: Vec<mbink_sys::MBinkSharedHandle>,
    bind_callbacks: Vec<BindRegistration>,
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

        let mut app = Self {
            handle,
            shared_handles: Vec::new(),
            bind_callbacks: Vec::new(),
            event_callbacks: EventRegistry::default(),
        };
        app.install_default_on_close_stop()?;
        Ok(app)
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

        for shared in self.shared_handles.drain(..).rev() {
            unsafe { mbink_sys::mbink_shared_destroy(shared) };
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
            mbink_sys::mbink_bind(self.handle, name_c.as_ptr(), Some(bind_trampoline), user_data)
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
        Ok(Shared { handle, _marker: std::marker::PhantomData })
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
