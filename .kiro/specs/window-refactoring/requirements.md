# window.cpp 拆分重构需求

## Introduction

本文档定义了 `core/window/window.cpp` 文件拆分重构的需求。该文件当前有 3368 行，远超过代码规范的 1500 行阈值，需要拆分为多个独立文件以提高可维护性。

## Glossary

- **Window**: 应用程序窗口管理类，负责窗口创建、渲染、事件处理
- **WindowDOMObserver**: DOM 观察者，监听 DOM 变化并触发窗口重绘
- **WindowRenderer**: 窗口渲染器，负责渲染相关逻辑（已存在但未完全使用）
- **DisplayBackend**: 显示后端，负责无闪烁显示（已独立）

## Requirements

### Requirement 1: 提取 WindowDOMObserver

**User Story:** As a developer, I want WindowDOMObserver to be in a separate file, so that DOM observation logic is isolated and easier to maintain.

#### Acceptance Criteria

1. WHEN the WindowDOMObserver class is extracted THEN the system SHALL create a new file `window_dom_observer.cpp`
2. WHEN the WindowDOMObserver is in a separate file THEN the system SHALL maintain all existing DOM observation functionality
3. WHEN compiling after extraction THEN the system SHALL produce no errors or warnings related to the extraction

### Requirement 2: 迁移渲染逻辑到 WindowRenderer

**User Story:** As a developer, I want rendering logic to be in WindowRenderer, so that Window class focuses on window management.

#### Acceptance Criteria

1. WHEN rendering methods are migrated THEN the system SHALL move `Render()`, `RenderDevTools()`, `UpdateAnimations()`, `ApplyAnimationsToRenderTree()` to WindowRenderer
2. WHEN rendering is delegated THEN the system SHALL maintain all existing rendering functionality
3. WHEN Window::Render() is called THEN the system SHALL delegate to WindowRenderer

### Requirement 3: 提取平台特定代码

**User Story:** As a developer, I want platform-specific code to be in separate files, so that cross-platform maintenance is easier.

#### Acceptance Criteria

1. WHEN Windows-specific code is extracted THEN the system SHALL create `window_win32.cpp` for Windows subclassing logic
2. WHEN platform code is separated THEN the system SHALL use conditional compilation to include platform-specific files
3. WHEN compiling on Windows THEN the system SHALL include Windows-specific code without errors

### Requirement 4: 提取初始化代码

**User Story:** As a developer, I want initialization code to be organized, so that Window constructor is cleaner.

#### Acceptance Criteria

1. WHEN initialization methods are organized THEN the system SHALL group SDL, OpenGL, Skia, and CPU rendering initialization
2. WHEN initialization is refactored THEN the system SHALL maintain all existing initialization functionality
3. WHEN Window is created THEN the system SHALL initialize all subsystems correctly

### Requirement 5: 文件大小目标

**User Story:** As a developer, I want each file to be under 1500 lines, so that code is maintainable per project standards.

#### Acceptance Criteria

1. WHEN refactoring is complete THEN window.cpp SHALL be under 1500 lines
2. WHEN refactoring is complete THEN window_dom_observer.cpp SHALL be under 500 lines
3. WHEN refactoring is complete THEN window_renderer.cpp SHALL be under 1000 lines
4. WHEN refactoring is complete THEN window_win32.cpp SHALL be under 300 lines

### Requirement 6: 编译和功能验证

**User Story:** As a developer, I want the refactored code to compile and work correctly, so that no functionality is lost.

#### Acceptance Criteria

1. WHEN all files are refactored THEN the system SHALL compile without errors
2. WHEN the application runs THEN the system SHALL render correctly as before
3. WHEN DOM changes occur THEN the system SHALL update the display correctly
