use std::ffi::{CStr, CString, c_char, c_void};

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
