use std::marker::PhantomData;

use serde::de::DeserializeOwned;
use serde::Serialize;

use crate::app::App;
use crate::util::{string_from_const_ptr, string_from_owned_ptr, to_cstring};
use crate::{Result, ValueType};

pub struct State<'a> {
    pub(crate) app: &'a App,
}

pub struct StateBatch<'a> {
    handle: mbink_sys::MBinkHandle,
    _marker: PhantomData<&'a App>,
}

impl<'a> State<'a> {
    pub fn exists(&self, name: &str) -> Result<bool> {
        let name = to_cstring(name)?;
        Ok(unsafe { mbink_sys::mbink_state_exists(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn ty(&self, name: &str) -> Result<ValueType> {
        let name = to_cstring(name)?;
        let raw = unsafe { mbink_sys::mbink_state_type(self.app.raw_handle(), name.as_ptr()) };
        Ok(ValueType::from_raw(raw as i32))
    }

    pub fn delete(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        unsafe { mbink_sys::mbink_state_delete(self.app.raw_handle(), name.as_ptr()) };
        Ok(())
    }

    pub fn get_bool(&self, name: &str) -> Result<bool> {
        let name = to_cstring(name)?;
        Ok(unsafe { mbink_sys::mbink_state_get_bool(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn get_int(&self, name: &str) -> Result<i64> {
        let name = to_cstring(name)?;
        Ok(unsafe { mbink_sys::mbink_state_get_int(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn get_double(&self, name: &str) -> Result<f64> {
        let name = to_cstring(name)?;
        Ok(unsafe { mbink_sys::mbink_state_get_double(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn get_string(&self, name: &str) -> Result<String> {
        let name = to_cstring(name)?;
        unsafe { string_from_const_ptr(mbink_sys::mbink_state_get_string(self.app.raw_handle(), name.as_ptr())) }
    }

    pub fn get_json_string(&self, name: &str) -> Result<String> {
        let name = to_cstring(name)?;
        unsafe { string_from_owned_ptr(mbink_sys::mbink_state_get_json(self.app.raw_handle(), name.as_ptr())) }
    }

    pub fn get_json<T: DeserializeOwned>(&self, name: &str) -> Result<T> {
        Ok(serde_json::from_str(&self.get_json_string(name)?)?)
    }

    pub fn set_null(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_null(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn set_bool(&self, name: &str, value: bool) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_bool(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn set_int(&self, name: &str, value: i64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_int(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn set_double(&self, name: &str, value: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_double(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn set_string(&self, name: &str, value: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let value = to_cstring(value)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_string(self.app.raw_handle(), name.as_ptr(), value.as_ptr()) })
    }

    pub fn set_json_str(&self, name: &str, json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let json = to_cstring(json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_set_json(self.app.raw_handle(), name.as_ptr(), json.as_ptr()) })
    }

    pub fn set_json<T: Serialize>(&self, name: &str, value: &T) -> Result<()> {
        self.set_json_str(name, &serde_json::to_string(value)?)
    }

    pub fn batch(&self) -> StateBatch<'_> {
        unsafe { mbink_sys::mbink_state_batch_begin(self.app.raw_handle()) };
        StateBatch { handle: self.app.raw_handle(), _marker: PhantomData }
    }
}

impl Drop for StateBatch<'_> {
    fn drop(&mut self) {
        unsafe { mbink_sys::mbink_state_batch_end(self.handle) };
    }
}
