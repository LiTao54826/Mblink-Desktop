# Requirements Document

## Introduction

本文档定义了 LightUI 项目代码瘦身计划的需求，旨在清理重构过程中产生的冗余代码、新旧系统并存问题，以及超出规范的大文件。目标是减少编译体积、提高代码可维护性、消除开发者困惑。

## Glossary

- **Hit_Testing_System**: 负责确定用户点击/触摸位置对应的 DOM 元素的系统
- **Layer_Tree_System**: 管理合成层树结构的系统，包括构建和增量更新
- **Layout_Boundary_System**: 检测和管理增量布局边界的系统
- **Redundant_Code**: 功能重复或已被新实现替代但未删除的代码
- **Code_Structure_Standards**: 项目定义的代码结构规范（头文件 ≤300 行，源文件 ≤1500 行等）

## Requirements

### Requirement 1: 清理 Hit Testing 系统冗余

**User Story:** As a developer, I want to have a single Hit Testing implementation, so that I don't get confused about which API to use and reduce compilation size.

#### Acceptance Criteria

1. WHEN the codebase is compiled, THE Build_System SHALL only compile one Hit Testing implementation (hit_test_controller.cpp)
2. WHEN a developer searches for Hit Testing code, THE Codebase SHALL contain only HitTestController class, not the deprecated HitTesting class
3. WHEN hit_testing.h/cpp files are removed, THE Build_System SHALL compile successfully without errors
4. WHEN all callers of HitTesting class are updated, THE System SHALL use HitTestController API instead
5. IF any code references the old HitTesting class, THEN THE Build_System SHALL report a compilation error

### Requirement 2: 清理 Layer Tree 系统职责重复

**User Story:** As a developer, I want clear separation between LayerTreeBuilder and LayerTreeManager, so that I understand which class to use for which purpose.

#### Acceptance Criteria

1. THE LayerTreeBuilder SHALL only contain layer tree construction logic without maintaining state
2. THE LayerTreeManager SHALL be the single source of truth for tree_version_ management
3. WHEN layer tree needs to be built, THE LayerTreeBuilder SHALL be called by LayerTreeManager
4. THE LayerTreeBuilder SHALL NOT maintain its own tree_version_ field
5. WHEN incremental updates are requested, THE LayerTreeManager SHALL coordinate with LayerTreeBuilder

### Requirement 3: 拆分超大头文件

**User Story:** As a developer, I want header files to comply with the 300-line limit, so that the codebase follows project standards.

#### Acceptance Criteria

1. THE layer_tree_manager.h file SHALL be split into multiple files, each under 300 lines
2. WHEN layer_tree_manager.h is split, THE Public_API SHALL remain backward compatible
3. THE layer_tree_builder.h file SHALL be refactored to stay under 300 lines
4. WHEN header files are split, THE CMakeLists.txt SHALL be updated accordingly
5. WHEN header files are split, THE Include_Paths in dependent files SHALL be updated

### Requirement 4: 清理调试文件和日志

**User Story:** As a developer, I want debug files excluded from version control, so that the repository stays clean.

#### Acceptance Criteria

1. THE .gitignore file SHALL include debuglog.txt pattern
2. THE .gitignore file SHALL include build_log.txt pattern
3. WHEN git status is run, THE System SHALL NOT show debuglog.txt or build_log.txt as untracked
4. IF debuglog.txt exists in repository, THEN THE System SHALL remove it from tracking

### Requirement 5: 更新文档和 README

**User Story:** As a developer, I want accurate documentation, so that I can understand the current system architecture.

#### Acceptance Criteria

1. WHEN hit_testing system is cleaned up, THE core/event/input/README.md SHALL be updated to reflect HitTestController
2. WHEN layer tree system is refactored, THE core/compositor/README.md SHALL be updated
3. THE CODE_REDUNDANCY_ANALYSIS.md SHALL be archived or deleted after cleanup is complete
4. WHEN documentation is updated, THE System_Architecture descriptions SHALL match the actual implementation

### Requirement 6: 验证编译体积减少

**User Story:** As a developer, I want to verify that the cleanup reduces compilation output size, so that I can confirm the effectiveness of the slimming effort.

#### Acceptance Criteria

1. WHEN cleanup is complete, THE Compiled_Binary size SHALL be measured and compared to baseline
2. THE Build_System SHALL compile successfully after all cleanup tasks
3. WHEN all tests are run, THE Test_Suite SHALL pass without regressions
4. THE Compilation_Time SHOULD decrease or remain stable after cleanup
