mod app;
mod callback;
mod config;
mod controls;
mod error;
mod resources;
mod shared;
mod state;
mod util;

pub use app::{
    App, AppHandle, DevToolsHttpOptions, DevToolsHttpSession, RuntimeOptions, UiDevSnapshotOptions,
};
pub use config::AppBuilder;
pub use controls::{LogView, Terminal};
pub use error::{Error, Result};
pub use resources::{compile_resources, load_resource_file, ResourceFile, RESOURCE_FLAG_BYTECODE};
pub use shared::{Shared, SharedBatch};
pub use state::{State, StateBatch};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LifecycleState {
    Created,
    Loaded,
    Running,
    CloseRequested,
    Stopped,
    Destroyed,
    Unknown(i32),
}

impl LifecycleState {
    pub(crate) fn from_raw(value: mbink_sys::MBinkLifecycleState) -> Self {
        match value {
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_CREATED => Self::Created,
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_LOADED => Self::Loaded,
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_RUNNING => Self::Running,
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_CLOSE_REQUESTED => Self::CloseRequested,
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_STOPPED => Self::Stopped,
            mbink_sys::MBinkLifecycleState::MBINK_LIFECYCLE_DESTROYED => Self::Destroyed,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ObserveKind {
    Console,
    Error,
}

impl ObserveKind {
    pub(crate) fn to_raw(self) -> mbink_sys::MBinkObserveKind {
        match self {
            Self::Console => mbink_sys::MBinkObserveKind::MBINK_OBSERVE_CONSOLE,
            Self::Error => mbink_sys::MBinkObserveKind::MBINK_OBSERVE_ERROR,
        }
    }
}

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
