# Requirements Document

## Introduction

本规范定义了 `core/dom` 目录的重组计划。当前 `core/dom/` 根目录包含 18 个源文件，超过了代码规范中规定的 15 个文件上限。这些文件混合了多种不同职责的模块，需要按功能进行子目录分组以提高代码可维护性。

重组目标是将 DOM 模块按职责划分为逻辑子目录，同时保持 `elements/` 子目录不变（因其具有单一职责和高内聚性）。

## Glossary

- **DOM**: Document Object Model，文档对象模型
- **DOM Core**: DOM 核心类，包括 Node、Element、Document、Text 等基础节点类
- **Bindings**: JavaScript 绑定层，负责将 DOM API 暴露给 QuickJS 运行时
- **Observers**: 观察者模式实现，包括 MutationObserver、DOMObserver 等
- **Selection**: 文本选择相关功能，包括 Range、Selection 等
- **Style**: 样式相关功能，包括 CSSStyleDeclaration、IncrementalStyleRecalc 等

## Requirements

### Requirement 1

**User Story:** As a developer, I want the DOM module to be organized into logical subdirectories, so that I can easily navigate and maintain the codebase.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN the `core/dom/` root directory SHALL contain no more than 10 source files
2. WHEN files are moved to subdirectories THEN the system SHALL preserve all existing functionality without breaking changes
3. WHEN a new subdirectory is created THEN the subdirectory SHALL have its own CMakeLists.txt file
4. WHEN a new subdirectory contains 3 or more source files THEN the subdirectory SHALL have a README.md file

### Requirement 2

**User Story:** As a developer, I want DOM core classes to remain in the root directory, so that the most fundamental classes are easily accessible.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN the following files SHALL remain in `core/dom/`: node.cpp/h, element.cpp/h, document.cpp/h, text.cpp/h
2. WHEN accessing DOM core classes THEN the include paths SHALL remain unchanged as `core/dom/node.h`, `core/dom/element.h`, etc.

### Requirement 3

**User Story:** As a developer, I want JavaScript bindings to be grouped in a dedicated subdirectory, so that binding-related code is clearly separated from DOM logic.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `dom_bindings.cpp/h` and `canvas_bindings.cpp/h` SHALL be moved to `core/dom/bindings/`
2. WHEN bindings are moved THEN all include paths referencing these files SHALL be updated accordingly
3. WHEN the bindings subdirectory is created THEN the subdirectory SHALL have a CMakeLists.txt and README.md

### Requirement 4

**User Story:** As a developer, I want observer-related classes to be grouped together, so that the observer pattern implementations are easy to find.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `dom_observer.cpp/h`, `mutation_observer.cpp/h`, and `dirty_node_tracker.cpp/h` SHALL be moved to `core/dom/observers/`
2. WHEN observers are moved THEN all include paths referencing these files SHALL be updated accordingly
3. WHEN the observers subdirectory is created THEN the subdirectory SHALL have a CMakeLists.txt and README.md

### Requirement 5

**User Story:** As a developer, I want selection-related classes to be grouped together, so that text selection functionality is clearly organized.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `selection.cpp/h`, `range.cpp/h`, and `selector_engine.cpp/h` SHALL be moved to `core/dom/selection/`
2. WHEN selection classes are moved THEN all include paths referencing these files SHALL be updated accordingly
3. WHEN the selection subdirectory is created THEN the subdirectory SHALL have a CMakeLists.txt and README.md

### Requirement 6

**User Story:** As a developer, I want style-related classes to be grouped together, so that CSS and style functionality is clearly organized.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `css_style_declaration.cpp/h` and `incremental_style_recalc.cpp/h` SHALL be moved to `core/dom/style/`
2. WHEN style classes are moved THEN all include paths referencing these files SHALL be updated accordingly
3. WHEN the style subdirectory is created THEN the subdirectory SHALL have a CMakeLists.txt and README.md

### Requirement 7

**User Story:** As a developer, I want utility classes to be grouped together, so that helper classes are clearly separated from core functionality.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `dom_token_list.cpp/h` and `dom_string_map.cpp/h` SHALL be moved to `core/dom/utils/`
2. WHEN utility classes are moved THEN all include paths referencing these files SHALL be updated accordingly
3. WHEN the utils subdirectory is created THEN the subdirectory SHALL have a CMakeLists.txt and README.md

### Requirement 8

**User Story:** As a developer, I want event-related classes to remain in the root directory or be clearly organized, so that event handling code is accessible.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN `event.cpp/h` and `drag_event.cpp/h` SHALL remain in `core/dom/` root directory
2. IF the number of event-related files grows beyond 4 THEN the system SHALL consider creating an `events/` subdirectory

### Requirement 9

**User Story:** As a developer, I want the build system to be updated correctly, so that the project compiles successfully after reorganization.

#### Acceptance Criteria

1. WHEN files are moved THEN the `core/dom/CMakeLists.txt` SHALL be updated to reference new file locations
2. WHEN subdirectories are created THEN each subdirectory SHALL have its own CMakeLists.txt that defines its sources
3. WHEN the reorganization is complete THEN the project SHALL compile without errors on all supported platforms
4. WHEN the reorganization is complete THEN all existing tests SHALL pass

### Requirement 10

**User Story:** As a developer, I want the elements subdirectory to remain unchanged, so that the high-cohesion HTML element implementations are not disrupted.

#### Acceptance Criteria

1. WHEN the reorganization is complete THEN the `core/dom/elements/` directory structure SHALL remain unchanged
2. WHEN the reorganization is complete THEN all HTML element files SHALL remain in `core/dom/elements/`
