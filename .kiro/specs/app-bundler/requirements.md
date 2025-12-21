# Requirements Document

## Introduction

App Bundler 是一个独立的命令行工具，用于将 Preact/JS 应用打包成单个可执行文件。该工具无需 CMake 或 C 编译器环境，只需一个 exe 即可完成打包。打包后的应用包含完整的运行时环境（QuickJS、DOM 绑定、渲染引擎等），可以独立运行。

支持 ES6 模块语法，自动解析 `import/export` 依赖并打包所有相关文件。

## Glossary

- **App Bundler**: 将 JS 应用打包成独立 exe 的命令行工具
- **Template Exe**: 作为打包基础的 app_loader.exe 模板文件
- **Bytecode**: QuickJS 编译后的字节码，比源码执行更快
- **Payload**: 嵌入到 exe 末尾的数据（字节码 + 元数据）
- **Magic Number**: 用于标识 payload 存在的特殊字节序列
- **ES6 Module**: 使用 import/export 语法的 JavaScript 模块

## Requirements

### Requirement 1

**User Story:** As a developer, I want to bundle my Preact application into a single executable, so that I can distribute it without requiring users to install any runtime or dependencies.

#### Acceptance Criteria

1. WHEN a user runs `app_bundler app.js -o my_app.exe` THEN the App Bundler SHALL produce a standalone executable containing the application
2. WHEN the bundled executable is run THEN the System SHALL execute the embedded application without requiring external JS files
3. WHEN bundling an application THEN the App Bundler SHALL compile JS source to QuickJS bytecode for faster startup
4. WHEN the output file already exists THEN the App Bundler SHALL overwrite it unless --no-overwrite flag is specified

### Requirement 2

**User Story:** As a developer, I want to use ES6 modules (import/export) in my application, so that I can organize my code into multiple files and use modern JavaScript syntax.

#### Acceptance Criteria

1. WHEN the main app.js contains `import` statements THEN the App Bundler SHALL resolve and bundle all imported modules
2. WHEN resolving imports THEN the App Bundler SHALL support relative paths (e.g., `import { Component } from './components/Button.js'`)
3. WHEN resolving imports THEN the App Bundler SHALL support bare module specifiers for built-in libraries (e.g., `import { h, render } from 'preact'`)
4. WHEN a circular dependency is detected THEN the App Bundler SHALL handle it correctly following ES6 module semantics
5. WHEN an imported module is not found THEN the App Bundler SHALL report the error with the import path and source location
6. WHEN bundling ES6 modules THEN the App Bundler SHALL compile ALL resolved modules (including dependencies) to QuickJS bytecode
7. WHEN compiling modules THEN the App Bundler SHALL preserve module dependency order for correct initialization

### Requirement 3

**User Story:** As a developer, I want the bundler to work without requiring a C compiler or CMake, so that I can bundle applications on any Windows machine.

#### Acceptance Criteria

1. WHEN bundling an application THEN the App Bundler SHALL NOT require any external compiler or build tools
2. WHEN bundling an application THEN the App Bundler SHALL use an embedded template exe or locate app_loader.exe in the same directory
3. WHEN the template exe is not found THEN the App Bundler SHALL report a clear error message with instructions

### Requirement 4

**User Story:** As a developer, I want to customize the bundled application's window properties, so that I can control the initial appearance of my application.

#### Acceptance Criteria

1. WHEN a user specifies `--width <value>` THEN the App Bundler SHALL embed the window width configuration
2. WHEN a user specifies `--height <value>` THEN the App Bundler SHALL embed the window height configuration
3. WHEN a user specifies `--title <value>` THEN the App Bundler SHALL embed the window title configuration
4. WHEN no window options are specified THEN the App Bundler SHALL use default values (800x600, "MBink App")

### Requirement 5

**User Story:** As a developer, I want to include additional JS libraries in my bundle, so that I can use third-party libraries without external dependencies.

#### Acceptance Criteria

1. WHEN a user specifies `--include <file.js>` THEN the App Bundler SHALL include the specified JS file before the main app
2. WHEN multiple --include options are provided THEN the App Bundler SHALL load them in the specified order
3. WHEN an included file does not exist THEN the App Bundler SHALL report an error and exit with non-zero status

### Requirement 6

**User Story:** As a developer, I want to see the bundling progress and any errors, so that I can troubleshoot issues with my application.

#### Acceptance Criteria

1. WHEN bundling an application THEN the App Bundler SHALL display progress information (resolving modules, compiling, embedding)
2. WHEN a JS syntax error is detected THEN the App Bundler SHALL report the error location and message
3. WHEN bundling completes successfully THEN the App Bundler SHALL display the output file path and size
4. WHEN the --verbose flag is specified THEN the App Bundler SHALL display detailed information including all resolved modules

### Requirement 7

**User Story:** As a developer, I want the bundled executable to detect and run embedded code automatically, so that the same app_loader can work both standalone and as a bundled app.

#### Acceptance Criteria

1. WHEN app_loader starts THEN the System SHALL check for embedded payload at the end of its own executable
2. WHEN embedded payload is detected THEN the System SHALL load and execute the bytecode instead of requiring command-line arguments
3. WHEN no embedded payload is detected THEN the System SHALL operate in normal mode requiring an app.js path argument
4. WHEN embedded payload is corrupted THEN the System SHALL report an error and exit gracefully

### Requirement 8

**User Story:** As a developer, I want to verify the integrity of bundled applications, so that I can ensure the bundle was created correctly.

#### Acceptance Criteria

1. WHEN bundling an application THEN the App Bundler SHALL include a CRC32 checksum in the payload metadata
2. WHEN loading embedded payload THEN the System SHALL verify the checksum before execution
3. WHEN checksum verification fails THEN the System SHALL report a corruption error and refuse to execute
