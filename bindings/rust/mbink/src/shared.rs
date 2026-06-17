use std::marker::PhantomData;

use serde::de::DeserializeOwned;
use serde::Serialize;

use crate::util::{string_from_owned_ptr, to_cstring};
use crate::{Result, ValueType};

pub struct Shared<'a> {
    pub(crate) handle: mbink_sys::MBinkSharedHandle,
    pub(crate) _marker: PhantomData<&'a ()>,
}

pub struct SharedBatch<'a> {
    handle: mbink_sys::MBinkSharedHandle,
    _marker: PhantomData<&'a ()>,
}

impl<'a> Shared<'a> {
    pub fn set_null(&self, key: &str) -> Result<()> {
        let key = to_cstring(key)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_null(self.handle, key.as_ptr())
        })
    }

    pub fn set_bool(&self, key: &str, value: bool) -> Result<()> {
        let key = to_cstring(key)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_bool(self.handle, key.as_ptr(), value)
        })
    }

    pub fn set_int(&self, key: &str, value: i64) -> Result<()> {
        let key = to_cstring(key)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_int(self.handle, key.as_ptr(), value)
        })
    }

    pub fn set_double(&self, key: &str, value: f64) -> Result<()> {
        let key = to_cstring(key)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_double(self.handle, key.as_ptr(), value)
        })
    }

    pub fn set_string(&self, key: &str, value: &str) -> Result<()> {
        let key = to_cstring(key)?;
        let value = to_cstring(value)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_string(self.handle, key.as_ptr(), value.as_ptr())
        })
    }

    pub fn set_json_str(&self, key: &str, json: &str) -> Result<()> {
        let key = to_cstring(key)?;
        let json = to_cstring(json)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_set_json(self.handle, key.as_ptr(), json.as_ptr())
        })
    }

    pub fn set_json<T: Serialize>(&self, key: &str, value: &T) -> Result<()> {
        self.set_json_str(key, &serde_json::to_string(value)?)
    }

    pub fn get_bool(&self, key: &str) -> Result<bool> {
        let key = to_cstring(key)?;
        Ok(unsafe { mbink_sys::mbink_shared_get_bool(self.handle, key.as_ptr()) })
    }

    pub fn get_int(&self, key: &str) -> Result<i64> {
        let key = to_cstring(key)?;
        Ok(unsafe { mbink_sys::mbink_shared_get_int(self.handle, key.as_ptr()) })
    }

    pub fn get_double(&self, key: &str) -> Result<f64> {
        let key = to_cstring(key)?;
        Ok(unsafe { mbink_sys::mbink_shared_get_double(self.handle, key.as_ptr()) })
    }

    pub fn get_string(&self, key: &str) -> Result<String> {
        let key = to_cstring(key)?;
        unsafe {
            string_from_owned_ptr(mbink_sys::mbink_shared_get_string(
                self.handle,
                key.as_ptr(),
            ))
        }
    }

    pub fn get_json_string(&self, key: &str) -> Result<String> {
        let key = to_cstring(key)?;
        unsafe {
            string_from_owned_ptr(mbink_sys::mbink_shared_get_json(self.handle, key.as_ptr()))
        }
    }

    pub fn get_json<T: DeserializeOwned>(&self, key: &str) -> Result<T> {
        Ok(serde_json::from_str(&self.get_json_string(key)?)?)
    }

    pub fn has(&self, key: &str) -> Result<bool> {
        let key = to_cstring(key)?;
        Ok(unsafe { mbink_sys::mbink_shared_has(self.handle, key.as_ptr()) })
    }

    pub fn delete(&self, key: &str) -> Result<()> {
        let key = to_cstring(key)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_shared_delete(self.handle, key.as_ptr())
        })
    }

    pub fn ty(&self, key: &str) -> Result<ValueType> {
        let key = to_cstring(key)?;
        Ok(ValueType::from_raw(unsafe {
            mbink_sys::mbink_shared_get_type(self.handle, key.as_ptr())
        }))
    }

    pub fn batch(&self) -> SharedBatch<'_> {
        unsafe { mbink_sys::mbink_shared_batch_begin(self.handle) };
        SharedBatch {
            handle: self.handle,
            _marker: PhantomData,
        }
    }
}

impl Drop for SharedBatch<'_> {
    fn drop(&mut self) {
        unsafe { mbink_sys::mbink_shared_batch_end(self.handle) };
    }
}
