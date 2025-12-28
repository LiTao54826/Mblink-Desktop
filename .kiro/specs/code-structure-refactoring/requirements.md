# Requirements Document

## Introduction

本文档定义了 LightUI 项目的代码结构重构需求和后续开发的强制规范。随着项目复杂度增加，当前代码结构存在单文件过大、职责边界模糊、目录组织混乱等问题，需要系统性重构以提高可维护性和开发效率。

## Glossary

- **Module**: 具有单一职责的代码单元，通常对应一个 .h/.cpp 文件对
- **Subsystem**: 由多个相关 Module 组成的功能子系统，对应一个目录
- **LOC (Lines of Code)**: 代码行数，用于衡量文件复杂度
- **SRP (Single Responsibility Principle)**: 单一职责原则
- **Cyclomatic Complexity**: 圈复杂度，衡量代码逻辑复杂度的指标

## Requirements

### Requirement 1: 文件大小指导原则

**User Story:** As a developer, I want guidance on file size, so that I can make informed decisions about code organization.

#### Acceptance Criteria

1. WHEN a source file exceeds 1500 lines THEN the system SHALL trigger a review to evaluate if splitting is beneficial
2. WHEN a file contains logically distinct concerns that can be independently tested THEN the system SHALL recommend splitting
3. WHEN a file contains tightly coupled functionality that belongs together THEN the system SHALL allow larger file sizes with justification
4. IF a single function exceeds 150 lines THEN the system SHALL recommend reviewing for potential decomposition
5. WHEN a file grows beyond 2000 lines THEN the system SHALL require documented justification for keeping it unified

### Requirement 2: 目录结构规范

**User Story:** As a developer, I want a clear and consistent directory structure, so that I can quickly locate relevant code.

#### Acceptance Criteria

1. WHEN a directory contains more than 15 source files THEN the system SHALL require creation of subdirectories for logical grouping
2. WHEN creating a new subsystem THEN the system SHALL place it in a dedicated directory with its own CMakeLists.txt
3. WHEN a module has platform-specific code THEN the system SHALL place it in a `platform/` subdirectory
4. WHEN a module has test files THEN the system SHALL place them in a `tests/` subdirectory or adjacent `*_test.cpp` file
5. WHEN organizing HTML element classes THEN the system SHALL group them by category (form/, text/, media/, container/)

### Requirement 3: 单一职责原则

**User Story:** As a developer, I want each module to have a single, well-defined responsibility, so that changes are localized and predictable.

#### Acceptance Criteria

1. WHEN a class handles more than one distinct concern THEN the system SHALL require splitting into separate classes
2. WHEN a file contains both interface definition and implementation details THEN the system SHALL separate them appropriately
3. WHEN a module depends on more than 10 other modules THEN the system SHALL review for potential abstraction or splitting
4. IF a class has more than 20 public methods THEN the system SHALL require interface segregation
5. WHEN event handling code is mixed with rendering code THEN the system SHALL require separation into distinct modules

### Requirement 4: 命名规范

**User Story:** As a developer, I want consistent naming conventions, so that I can understand code purpose from names alone.

#### Acceptance Criteria

1. WHEN naming a class THEN the system SHALL use PascalCase with descriptive noun phrases
2. WHEN naming a function THEN the system SHALL use PascalCase for public methods and snake_case for private helpers
3. WHEN naming a file THEN the system SHALL use snake_case matching the primary class name
4. WHEN naming a directory THEN the system SHALL use lowercase with underscores for multi-word names
5. WHEN naming constants THEN the system SHALL use UPPER_SNAKE_CASE with k prefix for class constants

### Requirement 5: 头文件规范

**User Story:** As a developer, I want clean header files with minimal dependencies, so that compilation is fast and dependencies are clear.

#### Acceptance Criteria

1. WHEN including headers THEN the system SHALL use forward declarations where possible to reduce dependencies
2. WHEN a header file exceeds 300 lines THEN the system SHALL require splitting or moving implementations to .cpp
3. WHEN defining inline functions THEN the system SHALL limit them to 10 lines or less
4. WHEN organizing includes THEN the system SHALL group them in order: own header, project headers, third-party headers, system headers
5. WHEN a header is included by more than 20 files THEN the system SHALL ensure it has minimal dependencies

### Requirement 6: event_loop.cpp 重构

**User Story:** As a developer, I want the event loop code to be modular, so that I can modify event handling without affecting other systems.

#### Acceptance Criteria

1. WHEN refactoring event_loop.cpp THEN the system SHALL extract mouse event handling to a dedicated MouseEventDispatcher class
2. WHEN refactoring event_loop.cpp THEN the system SHALL extract keyboard event handling to a dedicated KeyboardEventDispatcher class
3. WHEN refactoring event_loop.cpp THEN the system SHALL extract DevTools integration to a dedicated DevToolsEventHandler class
4. WHEN refactoring event_loop.cpp THEN the system SHALL extract scrollbar handling to a dedicated ScrollbarController class
5. WHEN refactoring event_loop.cpp THEN the system SHALL keep the core loop under 200 lines

### Requirement 7: render_object.cpp 重构

**User Story:** As a developer, I want render object code to be organized by concern, so that I can work on layout without affecting painting.

#### Acceptance Criteria

1. WHEN refactoring render_object.cpp THEN the system SHALL extract layout logic to RenderObjectLayout class
2. WHEN refactoring render_object.cpp THEN the system SHALL extract painting logic to RenderObjectPainter class
3. WHEN refactoring render_object.cpp THEN the system SHALL extract scrolling logic to RenderObjectScroll class
4. WHEN refactoring render_object.cpp THEN the system SHALL extract transform handling to RenderObjectTransform class
5. WHEN refactoring render_object.cpp THEN the system SHALL keep the base class under 500 lines

### Requirement 8: window.cpp 重构

**User Story:** As a developer, I want window management code to be platform-agnostic where possible, so that porting is easier.

#### Acceptance Criteria

1. WHEN refactoring window.cpp THEN the system SHALL extract WindowDOMObserver to a separate file
2. WHEN refactoring window.cpp THEN the system SHALL extract platform-specific code to platform/win32_window.cpp
3. WHEN refactoring window.cpp THEN the system SHALL extract rendering pipeline integration to WindowRenderer class
4. WHEN refactoring window.cpp THEN the system SHALL keep the core Window class under 600 lines
5. WHEN adding new platform support THEN the system SHALL only require adding new files in platform/ directory

### Requirement 9: DOM 元素目录重组

**User Story:** As a developer, I want HTML element classes organized by category, so that I can find related elements quickly.

#### Acceptance Criteria

1. WHEN organizing form elements THEN the system SHALL place input, textarea, button, select, option, form in dom/elements/form/
2. WHEN organizing text elements THEN the system SHALL place span, paragraph, heading, anchor, label in dom/elements/text/
3. WHEN organizing container elements THEN the system SHALL place div, li, ul, ol, table in dom/elements/container/
4. WHEN organizing media elements THEN the system SHALL place image, canvas, svg in dom/elements/media/
5. WHEN adding a new HTML element THEN the system SHALL place it in the appropriate category subdirectory

### Requirement 10: 编辑子系统整合

**User Story:** As a developer, I want all text editing code in one place, so that I can understand and modify editing behavior holistically.

#### Acceptance Criteria

1. WHEN organizing editing code THEN the system SHALL create a dedicated core/editing/ directory
2. WHEN refactoring THEN the system SHALL move contenteditable_handler to core/editing/
3. WHEN refactoring THEN the system SHALL move contenteditable_controller to core/editing/
4. WHEN refactoring THEN the system SHALL move selection_manager to core/editing/
5. WHEN refactoring THEN the system SHALL move clipboard_manager to core/editing/

### Requirement 11: 文档和注释规范

**User Story:** As a developer, I want consistent documentation, so that I can understand code without reading every line.

#### Acceptance Criteria

1. WHEN creating a new public class THEN the system SHALL include a Doxygen-style class description
2. WHEN creating a new public function THEN the system SHALL document parameters, return value, and exceptions
3. WHEN a function has non-obvious behavior THEN the system SHALL include inline comments explaining the logic
4. WHEN creating a new directory THEN the system SHALL include a README.md explaining the subsystem
5. WHEN modifying complex algorithms THEN the system SHALL update or add explanatory comments

### Requirement 12: 依赖管理规范

**User Story:** As a developer, I want clear dependency boundaries, so that I can understand module relationships.

#### Acceptance Criteria

1. WHEN a module in core/dom/ depends on core/render/ THEN the system SHALL use forward declarations or interfaces
2. WHEN circular dependencies are detected THEN the system SHALL require refactoring to break the cycle
3. WHEN a low-level module depends on a high-level module THEN the system SHALL require dependency inversion
4. WHEN adding a new dependency THEN the system SHALL document it in the module's README
5. WHEN a module has more than 15 direct dependencies THEN the system SHALL review for potential abstraction
