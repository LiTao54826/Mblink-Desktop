# Third-Party Dependencies | 第三方依赖

## Overview | 概览

This directory contains third-party source trees and dependency-related tooling visible in the current repository.
本目录包含当前仓库中可见的第三方源码目录和依赖相关工具。

## Directories Present | 当前目录

The following directories can be confirmed under `third_party/`:
当前 `third_party/` 下可确认存在：

- `SDL3/`
- `depot_tools/`
- `lexbor/`
- `nlohmann/`
- `quickjs/`
- `skia/`
- `stb/`
- `vcpkg/`

## Repository-Visible Usage | 从仓库可见的用途

From the source tree and build files, the repository shows integration traces for:
从源码树和构建文件可见，仓库对以下依赖存在接入痕迹：

- QuickJS
- SDL3
- Skia
- Lexbor
- nlohmann/json
- GoogleTest

Additional helper or dependency-management directories are also present.
此外还存在辅助工具或依赖管理目录。

## Important Notes | 重要说明

- the presence of a directory does not by itself prove active or complete integration
  目录存在本身不代表该依赖已完整接入或当前启用
- exact versions should be verified against the actual dependency source and build scripts
  精确版本应以实际依赖源码和构建脚本为准
- platform-specific dependency setup may still require additional validation
  平台相关依赖配置仍可能需要额外验证

## Dependency Setup | 依赖准备

Check these locations first when preparing dependencies:
准备依赖时建议优先查看：

- `scripts/download_deps.bat`
- `scripts/download_deps.ps1`
- `scripts/download_deps.sh`
- `scripts/download_skia_prebuilt.ps1`
- `docs/BUILD.md`

## Non-Claims | 不应直接承诺的内容

This document should not be used to claim that:
本文档不应被用来宣称：

- all third-party dependencies are pinned and unified / 所有第三方依赖都已统一锁版本
- all dependency setup paths are validated on every platform / 所有平台的依赖准备流程都已验证
- every directory here is required for every build configuration / 此处每个目录都对所有构建配置必需

