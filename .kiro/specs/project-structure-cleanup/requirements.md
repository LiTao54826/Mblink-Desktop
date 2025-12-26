# Requirements Document

## Introduction

本规范定义了 LightUI 项目结构优化的需求，目标是将项目重构为符合企业级 C++ 项目规范的结构。主要包括：清理根目录杂乱文件、统一命名规范、优化模块划分、修复编码问题等。

## Glossary

- **LightUI**: 轻量级 UI 渲染引擎项目
- **Module**: 项目中的功能模块，位于 core/ 目录下
- **Namespace**: C++ 命名空间，用于避免符号冲突
- **CMakeLists.txt**: CMake 构建配置文件

## Requirements

### Requirement 1: 清理根目录

**User Story:** As a developer, I want a clean project root directory, so that I can quickly understand the project structure and find important files.

#### Acceptance Criteria

1. WHEN the project is built THEN the system SHALL output all test result XML files to build/test_results/ directory
2. WHEN temporary executable files exist in root directory THEN the system SHALL have them listed in .gitignore
3. WHEN .devtools_state.json exists THEN the system SHALL have it listed in .gitignore
4. THE root directory SHALL contain only essential project files (CMakeLists.txt, README.md, LICENSE, .gitignore, .clang-format)

### Requirement 2: 删除空目录和冗余文件

**User Story:** As a developer, I want no empty or unused directories in the codebase, so that the project structure accurately reflects the actual functionality.

#### Acceptance Criteria

1. WHEN core/react/ directory exists and is empty THEN the system SHALL remove it from the project
2. WHEN unused source files exist THEN the system SHALL remove them from the project
3. THE project SHALL not contain any empty directories in the core/ tree

### Requirement 3: 统一命名规范

**User Story:** As a developer, I want consistent naming conventions across the project, so that I can easily understand and navigate the codebase.

#### Acceptance Criteria

1. THE system SHALL use snake_case for all file names
2. THE system SHALL use lightui_ prefix for all internal library targets
3. THE system SHALL use PascalCase for class names
4. THE system SHALL use snake_case for function and variable names
5. WHEN a CMake target is created THEN the system SHALL follow the naming pattern lightui_{module_name}

### Requirement 4: 优化 render 模块结构

**User Story:** As a developer, I want the render module to be well-organized, so that I can easily find and modify rendering-related code.

#### Acceptance Criteria

1. WHEN animation-related files exist in render/ THEN the system SHALL organize them into core/render/animation/ subdirectory
2. WHEN CSS-related files exist in render/ THEN the system SHALL organize them into core/render/css/ subdirectory
3. THE render module SHALL have clear subdirectory organization for animation, css, image, text, and canvas components

### Requirement 5: 修复 CMakeLists.txt 编码问题

**User Story:** As a developer, I want readable comments in CMakeLists.txt files, so that I can understand the build configuration.

#### Acceptance Criteria

1. WHEN CMakeLists.txt contains Chinese comments THEN the system SHALL ensure they display correctly with UTF-8 encoding
2. THE root CMakeLists.txt SHALL have properly encoded comments that display correctly in all editors

### Requirement 6: 添加统一命名空间

**User Story:** As a developer, I want all code to use a consistent namespace, so that symbol conflicts are avoided and code organization is clear.

#### Acceptance Criteria

1. THE system SHALL define a lightui namespace for all public API code
2. WHEN new source files are created THEN the system SHALL wrap code in the lightui namespace
3. THE system SHALL document the namespace convention in coding standards

### Requirement 7: 优化测试输出配置

**User Story:** As a developer, I want test outputs organized in a dedicated directory, so that the project root stays clean.

#### Acceptance Criteria

1. WHEN tests are executed THEN the system SHALL output results to build/test_results/ directory
2. THE tests/CMakeLists.txt SHALL configure test output directory to build/test_results/
3. WHEN test results are generated THEN the system SHALL use consistent naming pattern {test_type}_results.xml
