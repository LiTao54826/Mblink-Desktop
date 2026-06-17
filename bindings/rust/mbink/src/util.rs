use std::ffi::{c_char, c_void, CStr, CString};

use crate::{Error, Result};

pub fn to_cstring(value: &str) -> Result<CString> {
    CString::new(value).map_err(Error::Nul)
}

pub unsafe fn string_from_const_ptr(ptr: *const c_char) -> Result<String> {
    if ptr.is_null() {
        return Ok(String::new());
    }
    CStr::from_ptr(ptr)
        .to_str()
        .map(|s| s.to_owned())
        .map_err(Error::Utf8)
}

pub unsafe fn string_from_owned_ptr(ptr: *mut c_char) -> Result<String> {
    if ptr.is_null() {
        return Ok(String::new());
    }

    let result = CStr::from_ptr(ptr)
        .to_str()
        .map(|s| s.to_owned())
        .map_err(Error::Utf8);

    mbink_sys::mbink_free(ptr.cast::<c_void>());
    result
}

pub fn check_rc_raw(rc: i32) -> Result<()> {
    if rc == 0 {
        return Ok(());
    }

    let message = unsafe { string_from_const_ptr(mbink_sys::mbink_last_error()) }
        .unwrap_or_else(|_| "unknown MBink error".to_string());
    Err(Error::Mbink { code: rc, message })
}
