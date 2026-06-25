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
    pub(crate) fn from_raw(value: mblink_sys::MBlinkLifecycleState) -> Self {
        match value {
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_CREATED => Self::Created,
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_LOADED => Self::Loaded,
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_RUNNING => Self::Running,
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_CLOSE_REQUESTED => Self::CloseRequested,
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_STOPPED => Self::Stopped,
            mblink_sys::MBlinkLifecycleState::MBLINK_LIFECYCLE_DESTROYED => Self::Destroyed,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ObserveKind {
    Console,
    Error,
}

impl ObserveKind {
    pub(crate) fn to_raw(self) -> mblink_sys::MBlinkObserveKind {
        match self {
            Self::Console => mblink_sys::MBlinkObserveKind::MBLINK_OBSERVE_CONSOLE,
            Self::Error => mblink_sys::MBlinkObserveKind::MBLINK_OBSERVE_ERROR,
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
