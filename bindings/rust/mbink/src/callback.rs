use std::ffi::{CStr, CString, c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};

use serde_json::{Value, json};

use crate::util::string_from_const_ptr;

pub type BindFn = dyn Fn(Value) -> crate::Result<Value> + 'static;
pub type AsyncBindFn = dyn Fn(Value) -> crate::Result<Value> + 'static;
pub type StateWatchFn = dyn Fn(&str, Value) + 'static;
pub type VoidFn = dyn Fn() + 'static;
pub type BoolFn = dyn Fn() -> bool + 'static;
pub type ResizeFn = dyn Fn(i32, i32) + 'static;
pub type UpdateFn = dyn Fn(f32) + 'static;

pub struct BindHolder {
    pub callback: Box<BindFn>,
}

pub struct AsyncBindHolder {
    pub callback: Box<AsyncBindFn>,
}

pub struct StateWatchHolder {
    pub callback: Box<StateWatchFn>,
}

pub struct VoidHolder {
    pub callback: Box<VoidFn>,
}

pub struct BoolHolder {
    pub callback: Box<BoolFn>,
}

pub struct ResizeHolder {
    pub callback: Box<ResizeFn>,
}

pub struct UpdateHolder {
    pub callback: Box<UpdateFn>,
}

pub struct BindRegistration {
    pub name: String,
    pub kind: BindKind,
    pub user_data: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BindKind {
    Sync,
    Async,
}

#[derive(Default)]
pub struct EventRegistry {
    pub on_resize: Option<*mut ResizeHolder>,
    pub on_close: Option<*mut VoidHolder>,
    pub on_close_request: Option<*mut BoolHolder>,
    pub on_focus: Option<*mut VoidHolder>,
    pub on_blur: Option<*mut VoidHolder>,
    pub on_update: Option<*mut UpdateHolder>,
    pub on_tray_click: Option<*mut VoidHolder>,
    pub on_tray_menu: Option<*mut BindHolder>,
}

pub struct StateWatchRegistration {
    pub watch_id: i32,
    pub user_data: *mut StateWatchHolder,
}

pub unsafe extern "C" fn bind_trampoline(
    args_json: *const c_char,
    user_data: *mut c_void,
) -> *mut c_char {
    let holder = &*(user_data as *mut BindHolder);
    let args = parse_json_arg(args_json);

    let value = match catch_unwind(AssertUnwindSafe(|| (holder.callback)(args))) {
        Ok(Ok(value)) => value,
        Ok(Err(err)) => json!({ "error": err.to_string() }),
        Err(_) => json!({ "error": "panic in Rust bind callback" }),
    };

    let text = serde_json::to_string(&value)
        .unwrap_or_else(|_| "{\"error\":\"serialize failure\"}".to_string());
    let c_text = CString::new(text)
        .unwrap_or_else(|_| CString::new("{\"error\":\"interior nul\"}").unwrap());
    mbink_sys::mbink_copy_string(c_text.as_ptr())
}

pub unsafe extern "C" fn bind_async_trampoline(
    args_json: *const c_char,
    user_data: *mut c_void,
) -> *mut c_char {
    let holder = &*(user_data as *mut AsyncBindHolder);
    let args = parse_json_arg(args_json);

    let value = match catch_unwind(AssertUnwindSafe(|| (holder.callback)(args))) {
        Ok(Ok(value)) => value,
        Ok(Err(err)) => json!({ "error": err.to_string() }),
        Err(_) => json!({ "error": "panic in Rust async bind callback" }),
    };

    let text = serde_json::to_string(&value)
        .unwrap_or_else(|_| "{\"error\":\"serialize failure\"}".to_string());
    let c_text = CString::new(text)
        .unwrap_or_else(|_| CString::new("{\"error\":\"interior nul\"}").unwrap());
    mbink_sys::mbink_copy_string(c_text.as_ptr())
}

pub unsafe extern "C" fn state_watch_trampoline(
    name: *const c_char,
    value_json: *const c_char,
    user_data: *mut c_void,
) {
    let holder = &*(user_data as *mut StateWatchHolder);
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let name = string_from_const_ptr(name).unwrap_or_default();
        let value = parse_json_arg(value_json);
        (holder.callback)(&name, value);
    }));
}

pub unsafe extern "C" fn resize_trampoline(width: i32, height: i32, user_data: *mut c_void) {
    let holder = &*(user_data as *mut ResizeHolder);
    let _ = catch_unwind(AssertUnwindSafe(|| (holder.callback)(width, height)));
}

pub unsafe extern "C" fn void_trampoline(user_data: *mut c_void) {
    let holder = &*(user_data as *mut VoidHolder);
    let _ = catch_unwind(AssertUnwindSafe(|| (holder.callback)()));
}

pub unsafe extern "C" fn bool_trampoline(user_data: *mut c_void) -> bool {
    let holder = &*(user_data as *mut BoolHolder);
    catch_unwind(AssertUnwindSafe(|| (holder.callback)())).unwrap_or(false)
}

pub unsafe extern "C" fn update_trampoline(delta_time: f32, user_data: *mut c_void) {
    let holder = &*(user_data as *mut UpdateHolder);
    let _ = catch_unwind(AssertUnwindSafe(|| (holder.callback)(delta_time)));
}

unsafe fn parse_json_arg(ptr: *const c_char) -> Value {
    if ptr.is_null() {
        return Value::Null;
    }

    match CStr::from_ptr(ptr).to_str() {
        Ok(text) => serde_json::from_str(text).unwrap_or(Value::Null),
        Err(_) => string_from_const_ptr(ptr)
            .ok()
            .and_then(|s| serde_json::from_str(&s).ok())
            .unwrap_or(Value::Null),
    }
}
