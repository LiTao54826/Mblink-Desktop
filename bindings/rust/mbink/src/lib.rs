mod app;
mod callback;
mod config;
mod error;
mod shared;
mod state;
mod util;

pub use app::App;
pub use config::AppBuilder;
pub use error::{Error, Result};
pub use shared::{Shared, SharedBatch};
pub use state::{State, StateBatch};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ValueType {
    Null,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object,
    Unknown(i32),
}

impl ValueType {
    pub(crate) fn from_raw(value: i32) -> Self {
        match value {
            0 => Self::Null,
            1 => Self::Bool,
            2 => Self::Int,
            3 => Self::Double,
            4 => Self::String,
            5 => Self::Array,
            6 => Self::Object,
            other => Self::Unknown(other),
        }
    }
}
