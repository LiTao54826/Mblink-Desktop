use std::marker::PhantomData;

use crate::app::App;
use crate::util::{string_from_owned_ptr, to_cstring};
use crate::{Error, Result};

pub struct LogView<'a> {
    pub(crate) handle: mbink_sys::MBinkLogViewHandle,
    pub(crate) _marker: PhantomData<&'a App>,
}

pub struct Terminal<'a> {
    pub(crate) handle: mbink_sys::MBinkTerminalHandle,
    pub(crate) _marker: PhantomData<&'a App>,
}

impl<'a> LogView<'a> {
    pub fn append(&self, level: &str, source: &str, message: &str) -> Result<&Self> {
        let level = to_cstring(level)?;
        let source = to_cstring(source)?;
        let message = to_cstring(message)?;
        crate::app::check_rc_raw(unsafe {
            mbink_sys::mbink_logview_append(self.handle, level.as_ptr(), source.as_ptr(), message.as_ptr())
        })?;
        Ok(self)
    }

    pub fn clear(&self) -> &Self {
        unsafe { mbink_sys::mbink_logview_clear(self.handle) };
        self
    }

    pub fn export(&self, format: &str) -> Result<String> {
        let format = to_cstring(format)?;
        unsafe { string_from_owned_ptr(mbink_sys::mbink_logview_export(self.handle, format.as_ptr())) }
    }
}

impl<'a> Terminal<'a> {
    pub fn write(&self, data: &str) -> Result<&Self> {
        let data = to_cstring(data)?;
        crate::app::check_rc_raw(unsafe { mbink_sys::mbink_terminal_write(self.handle, data.as_ptr()) })?;
        Ok(self)
    }

    pub fn clear(&self) -> &Self {
        unsafe { mbink_sys::mbink_terminal_clear(self.handle) };
        self
    }

    pub fn execute(&self, command: &str) -> Result<&Self> {
        let command = to_cstring(command)?;
        crate::app::check_rc_raw(unsafe { mbink_sys::mbink_terminal_execute(self.handle, command.as_ptr()) })?;
        Ok(self)
    }

    pub fn start_shell(&self, shell: &str) -> Result<&Self> {
        let shell = to_cstring(shell)?;
        crate::app::check_rc_raw(unsafe { mbink_sys::mbink_terminal_start_shell(self.handle, shell.as_ptr()) })?;
        Ok(self)
    }

    pub fn send_input(&self, input: &str) -> Result<&Self> {
        let input = to_cstring(input)?;
        crate::app::check_rc_raw(unsafe { mbink_sys::mbink_terminal_send_input(self.handle, input.as_ptr()) })?;
        Ok(self)
    }

    pub fn resize(&self, rows: i32, cols: i32) -> &Self {
        unsafe { mbink_sys::mbink_terminal_resize(self.handle, rows, cols) };
        self
    }

    pub fn serialize(&self) -> Result<String> {
        unsafe { string_from_owned_ptr(mbink_sys::mbink_terminal_serialize(self.handle)) }
    }
}

pub(crate) enum ControlHandle {
    LogView(mbink_sys::MBinkLogViewHandle),
    Terminal(mbink_sys::MBinkTerminalHandle),
}

pub(crate) fn logview_from_handle<'a>(handle: mbink_sys::MBinkLogViewHandle) -> Result<LogView<'a>> {
    if handle.is_null() {
        return Err(Error::NullHandle);
    }
    Ok(LogView { handle, _marker: PhantomData })
}

pub(crate) fn terminal_from_handle<'a>(handle: mbink_sys::MBinkTerminalHandle) -> Result<Terminal<'a>> {
    if handle.is_null() {
        return Err(Error::NullHandle);
    }
    Ok(Terminal { handle, _marker: PhantomData })
}
