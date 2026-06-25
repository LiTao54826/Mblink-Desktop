use crate::util::{check_rc_raw, to_cstring};
use crate::Result;

pub const RESOURCE_FLAG_BYTECODE: u32 = 1;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ResourceFile {
    data: Vec<u8>,
    flags: u32,
}

impl ResourceFile {
    pub fn new(data: Vec<u8>, flags: u32) -> Self {
        Self { data, flags }
    }

    pub fn data(&self) -> &[u8] {
        &self.data
    }

    pub fn into_bytes(self) -> Vec<u8> {
        self.data
    }

    pub fn flags(&self) -> u32 {
        self.flags
    }

    pub fn is_bytecode(&self) -> bool {
        self.flags & RESOURCE_FLAG_BYTECODE != 0
    }

    pub fn into_utf8_string(self) -> Result<String> {
        String::from_utf8(self.data).map_err(|err| crate::Error::Message(err.to_string()))
    }
}

pub fn compile_resources(input_path: &str, output_file: &str, encryption_key: &str) -> Result<()> {
    let input_path = to_cstring(input_path)?;
    let output_file = to_cstring(output_file)?;
    let encryption_key = to_cstring(encryption_key)?;
    check_rc_raw(unsafe {
        mblink_sys::mblink_compile_resources(
            input_path.as_ptr(),
            output_file.as_ptr(),
            encryption_key.as_ptr(),
        )
    })
}

pub fn load_resource_file(
    package_file: &str,
    resource_path: &str,
    encryption_key: &str,
) -> Result<ResourceFile> {
    let package_file = to_cstring(package_file)?;
    let resource_path = to_cstring(resource_path)?;
    let encryption_key = to_cstring(encryption_key)?;
    let mut out_data = std::ptr::null_mut();
    let mut out_size = 0usize;
    let mut out_flags = 0u32;

    check_rc_raw(unsafe {
        mblink_sys::mblink_load_resource_file(
            package_file.as_ptr(),
            resource_path.as_ptr(),
            encryption_key.as_ptr(),
            &mut out_data,
            &mut out_size,
            &mut out_flags,
        )
    })?;

    let bytes = unsafe {
        if out_data.is_null() || out_size == 0 {
            Vec::new()
        } else {
            let slice = std::slice::from_raw_parts(out_data.cast::<u8>(), out_size);
            let bytes = slice.to_vec();
            mblink_sys::mblink_free(out_data);
            bytes
        }
    };

    Ok(ResourceFile::new(bytes, out_flags))
}
