//! Values types for C FFI

use taffy::prelude as core;

use super::{TaffyFFIDefault, TaffyReturnCode};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum TaffyEdge {
    /// The top edge of the box
    Top,
    /// The bottom edge of the box
    Bottom,
    /// The left edge of the box
    Left,
    /// The right edge of the box
    Right,
    /// Both the top and bottom edges of the box
    Vertical,
    /// Both the left and right edges of the box
    Horizontal,
    /// All four edges of the box
    All,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum TaffyUnit {
    /// A none value (used to unset optional fields)
    None,
    /// Fixed Length (pixel) value
    Length,
    /// Percentage value
    Percent,
    /// Min-content size
    MinContent,
    /// Max-content size
    MaxContent,
    /// fit-content() function with a pixel limit
    FitContentPx,
    /// fit-content() function with a percentage limit
    FitContentPercent,
    /// Automatic values
    Auto,
    /// fr unit
    Fr,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum TaffyMeasureMode {
    /// A none value (used to unset optional fields)
    Exact,
    /// Fixed Length (pixel) value
    FitContent,
    /// Percentage value
    MinContent,
    /// Min-content size
    MaxContent,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct TaffySize {
    width: f32,
    height: f32,
}
impl From<TaffySize> for core::Size<f32> {
    #[inline(always)]
    fn from(value: TaffySize) -> Self {
        core::Size { width: value.width, height: value.height }
    }
}

#[repr(C)]
pub struct TaffyLayout {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}
impl TaffyFFIDefault for TaffyLayout {
    fn default() -> Self {
        TaffyLayout { x: 0.0, y: 0.0, width: 0.0, height: 0.0 }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct TaffyDimension {
    /// The value. If the unit is variant that doesn't require a value (e.g. Auto) then the value is ignored.
    pub value: f32,
    pub unit: TaffyUnit,
}
impl TaffyFFIDefault for TaffyDimension {
    fn default() -> Self {
        Self { unit: TaffyUnit::None, value: 0.0 }
    }
}

impl TaffyDimension {
    #[inline(always)]
    pub fn from_raw(unit: TaffyUnit, value: f32) -> Self {
        Self { unit, value }
    }
}

impl From<core::LengthPercentage> for TaffyDimension {
    fn from(value: core::LengthPercentage) -> Self {
        use core::CompactLength;
        let raw = value.into_raw();
        match raw.tag() {
            CompactLength::LENGTH_TAG => Self { unit: TaffyUnit::Length, value: raw.value() },
            CompactLength::PERCENT_TAG => Self { unit: TaffyUnit::Percent, value: raw.value() },
            _ => Self { unit: TaffyUnit::Length, value: 0.0 },
        }
    }
}

impl TryFrom<TaffyDimension> for core::LengthPercentage {
    type Error = TaffyReturnCode;

    fn try_from(value: TaffyDimension) -> Result<Self, Self::Error> {
        match value.unit {
            TaffyUnit::Length => Ok(core::LengthPercentage::length(value.value)),
            TaffyUnit::Percent => Ok(core::LengthPercentage::percent(value.value)),
            TaffyUnit::None => Err(TaffyReturnCode::InvalidNone),
            TaffyUnit::Auto => Err(TaffyReturnCode::InvalidAuto),
            TaffyUnit::MinContent => Err(TaffyReturnCode::InvalidMinContent),
            TaffyUnit::MaxContent => Err(TaffyReturnCode::InvalidMaxContent),
            TaffyUnit::FitContentPx => Err(TaffyReturnCode::InvalidFitContentPx),
            TaffyUnit::FitContentPercent => Err(TaffyReturnCode::InvalidFitContentPercent),
            TaffyUnit::Fr => Err(TaffyReturnCode::InvalidFr),
        }
    }
}

impl From<core::LengthPercentageAuto> for TaffyDimension {
    fn from(value: core::LengthPercentageAuto) -> Self {
        use core::CompactLength;
        let raw = value.into_raw();
        match raw.tag() {
            CompactLength::AUTO_TAG => Self { unit: TaffyUnit::Auto, value: 0.0 },
            CompactLength::LENGTH_TAG => Self { unit: TaffyUnit::Length, value: raw.value() },
            CompactLength::PERCENT_TAG => Self { unit: TaffyUnit::Percent, value: raw.value() },
            _ => Self { unit: TaffyUnit::Auto, value: 0.0 },
        }
    }
}

impl TryFrom<TaffyDimension> for core::LengthPercentageAuto {
    type Error = TaffyReturnCode;

    fn try_from(value: TaffyDimension) -> Result<Self, Self::Error> {
        match value.unit {
            TaffyUnit::Auto => Ok(core::LengthPercentageAuto::auto()),
            TaffyUnit::Length => Ok(core::LengthPercentageAuto::length(value.value)),
            TaffyUnit::Percent => Ok(core::LengthPercentageAuto::percent(value.value)),
            TaffyUnit::None => Err(TaffyReturnCode::InvalidNone),
            TaffyUnit::MinContent => Err(TaffyReturnCode::InvalidMinContent),
            TaffyUnit::MaxContent => Err(TaffyReturnCode::InvalidMaxContent),
            TaffyUnit::FitContentPx => Err(TaffyReturnCode::InvalidFitContentPx),
            TaffyUnit::FitContentPercent => Err(TaffyReturnCode::InvalidFitContentPercent),
            TaffyUnit::Fr => Err(TaffyReturnCode::InvalidFr),
        }
    }
}

impl From<core::Dimension> for TaffyDimension {
    fn from(value: core::Dimension) -> Self {
        use core::CompactLength;
        let raw = value.into_raw();
        match raw.tag() {
            CompactLength::AUTO_TAG => Self { unit: TaffyUnit::Auto, value: 0.0 },
            CompactLength::LENGTH_TAG => Self { unit: TaffyUnit::Length, value: raw.value() },
            CompactLength::PERCENT_TAG => Self { unit: TaffyUnit::Percent, value: raw.value() },
            _ => Self { unit: TaffyUnit::Auto, value: 0.0 },
        }
    }
}

impl TryFrom<TaffyDimension> for core::Dimension {
    type Error = TaffyReturnCode;

    fn try_from(value: TaffyDimension) -> Result<Self, Self::Error> {
        match value.unit {
            TaffyUnit::Auto => Ok(core::Dimension::auto()),
            TaffyUnit::Length => Ok(core::Dimension::length(value.value)),
            TaffyUnit::Percent => Ok(core::Dimension::percent(value.value)),
            TaffyUnit::None => Err(TaffyReturnCode::InvalidNone),
            TaffyUnit::MinContent => Err(TaffyReturnCode::InvalidMinContent),
            TaffyUnit::MaxContent => Err(TaffyReturnCode::InvalidMaxContent),
            TaffyUnit::FitContentPx => Err(TaffyReturnCode::InvalidFitContentPx),
            TaffyUnit::FitContentPercent => Err(TaffyReturnCode::InvalidFitContentPercent),
            TaffyUnit::Fr => Err(TaffyReturnCode::InvalidFr),
        }
    }
}

/// For all fields, zero represents not set
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct TaffyGridPlacement {
    pub start: i16,
    pub end: i16,
    pub span: u16,
}

impl TaffyFFIDefault for TaffyGridPlacement {
    fn default() -> Self {
        Self { start: 0, end: 0, span: 0 }
    }
}

impl From<TaffyGridPlacement> for core::Line<core::GridPlacement> {
    fn from(placement: TaffyGridPlacement) -> Self {
        use core::GridPlacement as GP;

        // Convert raw parts (start, span, end) into Line<GridPlacement>
        // Zero is not a valid value and indicates unset
        match (placement.start, placement.span, placement.end) {
            (0, 0, 0) => core::Line { start: GP::Auto, end: GP::Auto },
            (start, 0, 0) => core::Line { start: GP::Line(start.into()), end: GP::Auto },
            (0, 0, end) => core::Line { start: GP::Auto, end: GP::Line(end.into()) },
            (0, span, 0) => core::Line { start: GP::Span(span), end: GP::Auto },
            (start, span, 0) => core::Line { start: GP::Line(start.into()), end: GP::Span(span) },
            (0, span, end) => core::Line { start: GP::Span(span), end: GP::Line(end.into()) },
            (start, _, end) => core::Line { start: GP::Line(start.into()), end: GP::Line(end.into()) },
        }
    }
}

impl From<core::Line<core::GridPlacement>> for TaffyGridPlacement {
    fn from(placement: core::Line<core::GridPlacement>) -> Self {
        use core::GridPlacement as GP;

        // Convert Line<GridPlacement> into raw parts (start, span, end)
        let (start, span, end) = match (placement.start, placement.end) {
            (GP::Line(start), GP::Line(end)) => (start.as_i16(), 0, end.as_i16()),
            (GP::Line(start), GP::Span(span)) => (start.as_i16(), span, 0),
            (GP::Line(start), GP::Auto) => (start.as_i16(), 1, 0),
            (GP::Line(start), GP::NamedLine(_, _)) => (start.as_i16(), 0, 0),
            (GP::Line(start), GP::NamedSpan(_, _)) => (start.as_i16(), 0, 0),
            (GP::Span(span), GP::Line(end)) => (0, span, end.as_i16()),
            (GP::Span(span), GP::Span(_)) => (0, span, 0),
            (GP::Span(span), GP::Auto) => (0, span, 0),
            (GP::Span(span), GP::NamedLine(_, _)) => (0, span, 0),
            (GP::Span(span), GP::NamedSpan(_, _)) => (0, span, 0),
            (GP::Auto, GP::Line(end)) => (0, 1, end.as_i16()),
            (GP::Auto, GP::Span(span)) => (0, span, 0),
            (GP::Auto, GP::Auto) => (0, 1, 0),
            (GP::Auto, GP::NamedLine(_, _)) => (0, 0, 0),
            (GP::Auto, GP::NamedSpan(_, _)) => (0, 0, 0),
            (GP::NamedLine(_, _), _) => (0, 0, 0),
            (GP::NamedSpan(_, _), _) => (0, 0, 0),
        };
        let (start, span, end) = (start, span, end);
        Self { start, span, end }
    }
}

/// Represents a single grid track sizing function
/// This is a simplified C representation of Taffy's TrackSizingFunction
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct TaffyGridTrack {
    /// The type of track sizing function (length, fr, auto, etc.)
    pub unit: TaffyUnit,
    /// The value for the track (e.g., 100.0 for 100px, 1.0 for 1fr)
    pub value: f32,
}

impl TaffyFFIDefault for TaffyGridTrack {
    fn default() -> Self {
        Self { unit: TaffyUnit::Auto, value: 0.0 }
    }
}

impl From<TaffyGridTrack> for core::TrackSizingFunction {
    fn from(track: TaffyGridTrack) -> Self {
        use core::*;
        match track.unit {
            TaffyUnit::Length => length(track.value),
            TaffyUnit::Percent => percent(track.value),
            TaffyUnit::Fr => fr(track.value),
            TaffyUnit::Auto => auto(),
            TaffyUnit::MinContent => min_content(),
            TaffyUnit::MaxContent => max_content(),
            TaffyUnit::FitContentPx => fit_content(LengthPercentage::length(track.value)),
            TaffyUnit::FitContentPercent => fit_content(LengthPercentage::percent(track.value)),
            TaffyUnit::None => auto(),
        }
    }
}

impl From<TaffyGridTrack> for core::GridTemplateComponent<String> {
    fn from(track: TaffyGridTrack) -> Self {
        // Convert TaffyGridTrack to TrackSizingFunction, then wrap in GridTemplateComponent::Single
        let track_sizing_function: core::TrackSizingFunction = track.into();
        core::GridTemplateComponent::Single(track_sizing_function)
    }
}
