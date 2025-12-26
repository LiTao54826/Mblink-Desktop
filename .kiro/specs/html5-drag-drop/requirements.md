# Requirements Document

## Introduction

本功能旨在为 LightUI 实现符合 W3C HTML5 标准的拖拽（Drag and Drop）API。当前项目已有基于 RmlUi 风格的拖拽实现，但与 HTML5 标准存在差异。本次开发将在现有基础上进行改造和扩展，使其完全兼容 HTML5 Drag and Drop 规范，让开发者能够使用标准的 `draggable` 属性和拖拽事件来实现拖拽功能。

## Glossary

- **DragManager**: 拖拽管理器，负责管理拖拽状态和事件分发
- **DataTransfer**: 拖拽数据传输对象，用于在拖拽操作中存储和传输数据
- **draggable**: HTML5 标准属性，设置为 "true" 时表示元素可拖拽
- **effectAllowed**: DataTransfer 属性，指定允许的拖拽效果（copy/move/link）
- **dropEffect**: DataTransfer 属性，指定当前的拖拽效果
- **Drag Source**: 被拖拽的源元素
- **Drop Target**: 拖拽释放的目标元素
- **Drag Threshold**: 拖拽阈值，鼠标移动超过此距离才开始拖拽

## Requirements

### Requirement 1

**User Story:** As a developer, I want to use the standard `draggable="true"` attribute to make elements draggable, so that I can write code compatible with HTML5 standards.

#### Acceptance Criteria

1. WHEN an element has the attribute `draggable="true"` THEN the DragManager SHALL recognize the element as draggable
2. WHEN an element has the attribute `draggable="false"` THEN the DragManager SHALL prevent the element from being dragged
3. WHEN an element has no `draggable` attribute THEN the DragManager SHALL use the default behavior based on element type
4. WHEN the `draggable` attribute value changes dynamically THEN the DragManager SHALL update the draggable state immediately

### Requirement 2

**User Story:** As a developer, I want to receive standard HTML5 drag events (dragstart, drag, dragend, dragenter, dragleave, dragover, drop), so that I can handle drag operations using familiar event handlers.

#### Acceptance Criteria

1. WHEN a user starts dragging a draggable element THEN the system SHALL dispatch a `dragstart` event to the drag source element
2. WHILE a drag operation is in progress THEN the system SHALL dispatch `drag` events to the drag source element at regular intervals
3. WHEN a drag operation ends THEN the system SHALL dispatch a `dragend` event to the drag source element
4. WHEN a dragged element enters a potential drop target THEN the system SHALL dispatch a `dragenter` event to the drop target
5. WHEN a dragged element leaves a potential drop target THEN the system SHALL dispatch a `dragleave` event to the drop target
6. WHILE a dragged element is over a potential drop target THEN the system SHALL dispatch `dragover` events to the drop target at regular intervals
7. WHEN a dragged element is released over a valid drop target THEN the system SHALL dispatch a `drop` event to the drop target

### Requirement 3

**User Story:** As a developer, I want to use the DataTransfer API to transfer data during drag operations, so that I can pass information from the drag source to the drop target.

#### Acceptance Criteria

1. WHEN a `dragstart` event is dispatched THEN the event SHALL contain a DataTransfer object accessible via `event.dataTransfer`
2. WHEN `dataTransfer.setData(format, data)` is called during `dragstart` THEN the system SHALL store the data with the specified format
3. WHEN `dataTransfer.getData(format)` is called during `drop` THEN the system SHALL return the data stored with the specified format
4. WHEN `dataTransfer.clearData(format)` is called THEN the system SHALL remove the data with the specified format
5. WHEN `dataTransfer.types` is accessed THEN the system SHALL return an array of all stored data formats

### Requirement 4

**User Story:** As a developer, I want to control drag effects using effectAllowed and dropEffect, so that I can provide visual feedback about allowed operations.

#### Acceptance Criteria

1. WHEN `dataTransfer.effectAllowed` is set during `dragstart` THEN the system SHALL restrict the allowed drag effects accordingly
2. WHEN `dataTransfer.dropEffect` is set during `dragover` THEN the system SHALL update the current drag effect
3. WHEN the dropEffect is incompatible with effectAllowed THEN the system SHALL set dropEffect to "none"
4. WHEN a drop occurs with dropEffect set to "none" THEN the system SHALL cancel the drop operation

### Requirement 5

**User Story:** As a developer, I want drag events to follow the standard event propagation model (capture and bubble phases), so that I can handle events at different levels of the DOM tree.

#### Acceptance Criteria

1. WHEN a drag event is dispatched THEN the system SHALL propagate the event through the capture phase from root to target
2. WHEN a drag event reaches the target THEN the system SHALL trigger handlers registered on the target element
3. WHEN a drag event completes the target phase THEN the system SHALL propagate the event through the bubble phase from target to root
4. WHEN `event.stopPropagation()` is called THEN the system SHALL stop further propagation of the event
5. WHEN `event.preventDefault()` is called on `dragover` THEN the system SHALL allow the drop operation on that element

### Requirement 6

**User Story:** As a developer, I want the drag operation to start only after the mouse moves beyond a threshold distance, so that accidental drags are prevented.

#### Acceptance Criteria

1. WHEN a user presses the mouse button on a draggable element THEN the system SHALL enter drag detection mode
2. WHILE in drag detection mode AND the mouse moves less than 4 pixels THEN the system SHALL NOT start the drag operation
3. WHEN in drag detection mode AND the mouse moves 4 or more pixels THEN the system SHALL start the drag operation and dispatch `dragstart`
4. WHEN the mouse button is released before the threshold is reached THEN the system SHALL cancel drag detection and process as a normal click

### Requirement 7

**User Story:** As a developer, I want backward compatibility with the existing RmlUi-style drag attributes, so that existing code continues to work.

#### Acceptance Criteria

1. WHEN an element has the attribute `drag="drag"` THEN the DragManager SHALL treat the element as draggable with simple mode
2. WHEN an element has the attribute `drag="drag-drop"` THEN the DragManager SHALL treat the element as draggable with full event mode
3. WHEN an element has the attribute `drag="clone"` THEN the DragManager SHALL create a visual clone during drag
4. WHEN an element has the attribute `drag="block"` THEN the DragManager SHALL prevent dragging of the element and its children
5. WHEN both `draggable` and `drag` attributes are present THEN the `draggable` attribute SHALL take precedence

### Requirement 8

**User Story:** As a developer, I want the DragEvent to contain mouse position and modifier key information, so that I can implement position-aware drag behavior.

#### Acceptance Criteria

1. WHEN a drag event is created THEN the event SHALL contain `clientX` and `clientY` properties with mouse coordinates
2. WHEN a drag event is created THEN the event SHALL contain `screenX` and `screenY` properties with screen coordinates
3. WHEN a drag event is created THEN the event SHALL contain `ctrlKey`, `shiftKey`, `altKey`, and `metaKey` properties
4. WHEN a drag event is created THEN the event SHALL contain a `button` property indicating which mouse button initiated the drag

### Requirement 9

**User Story:** As a developer, I want to access the DataTransfer API from JavaScript, so that I can implement drag-and-drop functionality in my web applications.

#### Acceptance Criteria

1. WHEN JavaScript code accesses `event.dataTransfer` THEN the system SHALL return a JavaScript object wrapping the native DataTransfer
2. WHEN JavaScript code calls `dataTransfer.setData(format, data)` THEN the system SHALL store the data in the native DataTransfer object
3. WHEN JavaScript code calls `dataTransfer.getData(format)` THEN the system SHALL return the data from the native DataTransfer object
4. WHEN JavaScript code accesses `dataTransfer.effectAllowed` or `dataTransfer.dropEffect` THEN the system SHALL return the current effect values
5. WHEN JavaScript code sets `dataTransfer.effectAllowed` or `dataTransfer.dropEffect` THEN the system SHALL update the native DataTransfer object
