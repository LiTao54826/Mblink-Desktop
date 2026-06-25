use std::fmt::{Display, Formatter};
use std::str::Utf8Error;

#[derive(Debug)]
pub enum Error {
    Mblink { code: i32, message: String },
    NullHandle,
    Nul(std::ffi::NulError),
    Utf8(Utf8Error),
    Json(serde_json::Error),
    Message(String),
}

impl Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Mblink { code, message } => write!(f, "MBlink error {code}: {message}"),
            Self::NullHandle => write!(f, "received null handle from MBlink"),
            Self::Nul(err) => write!(f, "string contains interior NUL: {err}"),
            Self::Utf8(err) => write!(f, "utf-8 decoding error: {err}"),
            Self::Json(err) => write!(f, "json error: {err}"),
            Self::Message(message) => f.write_str(message),
        }
    }
}

impl std::error::Error for Error {}

impl From<serde_json::Error> for Error {
    fn from(value: serde_json::Error) -> Self {
        Self::Json(value)
    }
}

pub type Result<T> = std::result::Result<T, Error>;
