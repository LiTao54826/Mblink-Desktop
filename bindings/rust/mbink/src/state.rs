use std::marker::PhantomData;

use serde::de::DeserializeOwned;
use serde::Serialize;
use serde_json::Value;

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
    pub fn create_null(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_null(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn create_bool(&self, name: &str, value: bool) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_bool(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn create_int(&self, name: &str, value: i64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_int(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn create_double(&self, name: &str, value: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_double(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn create_string(&self, name: &str, value: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let value = to_cstring(value)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_string(self.app.raw_handle(), name.as_ptr(), value.as_ptr()) })
    }

    pub fn create_array(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_array(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn create_object(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_object(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn create_json_str(&self, name: &str, json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let json = to_cstring(json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_create_json(self.app.raw_handle(), name.as_ptr(), json.as_ptr()) })
    }

    pub fn create_json<T: Serialize>(&self, name: &str, value: &T) -> Result<()> {
        self.create_json_str(name, &serde_json::to_string(value)?)
    }

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

    pub fn get_length(&self, name: &str) -> Result<i32> {
        let name = to_cstring(name)?;
        Ok(unsafe { mbink_sys::mbink_state_get_length(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn get_at_json_string(&self, name: &str, index: i32) -> Result<String> {
        let name = to_cstring(name)?;
        unsafe { string_from_owned_ptr(mbink_sys::mbink_state_get_at(self.app.raw_handle(), name.as_ptr(), index)) }
    }

    pub fn get_at<T: DeserializeOwned>(&self, name: &str, index: i32) -> Result<T> {
        Ok(serde_json::from_str(&self.get_at_json_string(name, index)?)?)
    }

    pub fn get_key_json_string(&self, name: &str, key: &str) -> Result<String> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        unsafe { string_from_owned_ptr(mbink_sys::mbink_state_get_key(self.app.raw_handle(), name.as_ptr(), key.as_ptr())) }
    }

    pub fn get_key<T: DeserializeOwned>(&self, name: &str, key: &str) -> Result<T> {
        Ok(serde_json::from_str(&self.get_key_json_string(name, key)?)?)
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

    pub fn array_push_json_str(&self, name: &str, item_json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let item_json = to_cstring(item_json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_push(self.app.raw_handle(), name.as_ptr(), item_json.as_ptr()) })
    }

    pub fn array_push_json<T: Serialize>(&self, name: &str, value: &T) -> Result<()> {
        self.array_push_json_str(name, &serde_json::to_string(value)?)
    }

    pub fn array_push_int(&self, name: &str, value: i64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_push_int(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn array_push_double(&self, name: &str, value: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_push_double(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn array_push_string(&self, name: &str, value: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let value = to_cstring(value)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_push_string(self.app.raw_handle(), name.as_ptr(), value.as_ptr()) })
    }

    pub fn array_push_bool(&self, name: &str, value: bool) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_push_bool(self.app.raw_handle(), name.as_ptr(), value) })
    }

    pub fn array_pop(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_pop(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn array_shift(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_shift(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn array_unshift_json_str(&self, name: &str, item_json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let item_json = to_cstring(item_json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_unshift(self.app.raw_handle(), name.as_ptr(), item_json.as_ptr()) })
    }

    pub fn array_unshift_json<T: Serialize>(&self, name: &str, value: &T) -> Result<()> {
        self.array_unshift_json_str(name, &serde_json::to_string(value)?)
    }

    pub fn array_remove(&self, name: &str, index: i32) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_remove(self.app.raw_handle(), name.as_ptr(), index) })
    }

    pub fn array_clear(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_clear(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn array_set_json_str(&self, name: &str, index: i32, item_json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let item_json = to_cstring(item_json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_set(self.app.raw_handle(), name.as_ptr(), index, item_json.as_ptr()) })
    }

    pub fn array_set_json<T: Serialize>(&self, name: &str, index: i32, value: &T) -> Result<()> {
        self.array_set_json_str(name, index, &serde_json::to_string(value)?)
    }

    pub fn array_set_int(&self, name: &str, index: i32, value: i64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_set_int(self.app.raw_handle(), name.as_ptr(), index, value) })
    }

    pub fn array_set_double(&self, name: &str, index: i32, value: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_set_double(self.app.raw_handle(), name.as_ptr(), index, value) })
    }

    pub fn array_set_string(&self, name: &str, index: i32, value: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let value = to_cstring(value)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_array_set_string(self.app.raw_handle(), name.as_ptr(), index, value.as_ptr()) })
    }

    pub fn object_set_json_str(&self, name: &str, key: &str, value_json: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        let value_json = to_cstring(value_json)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_set(self.app.raw_handle(), name.as_ptr(), key.as_ptr(), value_json.as_ptr()) })
    }

    pub fn object_set_json<T: Serialize>(&self, name: &str, key: &str, value: &T) -> Result<()> {
        self.object_set_json_str(name, key, &serde_json::to_string(value)?)
    }

    pub fn object_set_int(&self, name: &str, key: &str, value: i64) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_set_int(self.app.raw_handle(), name.as_ptr(), key.as_ptr(), value) })
    }

    pub fn object_set_double(&self, name: &str, key: &str, value: f64) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_set_double(self.app.raw_handle(), name.as_ptr(), key.as_ptr(), value) })
    }

    pub fn object_set_string(&self, name: &str, key: &str, value: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        let value = to_cstring(value)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_set_string(self.app.raw_handle(), name.as_ptr(), key.as_ptr(), value.as_ptr()) })
    }

    pub fn object_set_bool(&self, name: &str, key: &str, value: bool) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_set_bool(self.app.raw_handle(), name.as_ptr(), key.as_ptr(), value) })
    }

    pub fn object_remove(&self, name: &str, key: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let key = to_cstring(key)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_remove(self.app.raw_handle(), name.as_ptr(), key.as_ptr()) })
    }

    pub fn object_clear(&self, name: &str) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_object_clear(self.app.raw_handle(), name.as_ptr()) })
    }

    pub fn increment(&self, name: &str, delta: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_increment(self.app.raw_handle(), name.as_ptr(), delta) })
    }

    pub fn multiply(&self, name: &str, factor: f64) -> Result<()> {
        let name = to_cstring(name)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_multiply(self.app.raw_handle(), name.as_ptr(), factor) })
    }

    pub fn string_append(&self, name: &str, suffix: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let suffix = to_cstring(suffix)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_string_append(self.app.raw_handle(), name.as_ptr(), suffix.as_ptr()) })
    }

    pub fn string_prepend(&self, name: &str, prefix: &str) -> Result<()> {
        let name = to_cstring(name)?;
        let prefix = to_cstring(prefix)?;
        self.app.check_rc(unsafe { mbink_sys::mbink_state_string_prepend(self.app.raw_handle(), name.as_ptr(), prefix.as_ptr()) })
    }

    pub fn set_merge_mode(&self, enable: bool) {
        unsafe { mbink_sys::mbink_state_set_merge_mode(self.app.raw_handle(), enable) };
    }

    pub fn process_queue(&self) -> Result<()> {
        self.app.check_rc(unsafe { mbink_sys::mbink_process_queue(self.app.raw_handle()) })
    }

    pub fn queue_size(&self) -> Result<i32> {
        let size = unsafe { mbink_sys::mbink_queue_size(self.app.raw_handle()) };
        if size < 0 {
            self.app.check_rc(size)?;
        }
        Ok(size)
    }

    pub fn get_value(&self, name: &str) -> Result<Value> {
        self.get_json(name)
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
