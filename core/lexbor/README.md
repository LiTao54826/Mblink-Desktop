# Lexbor Layer | Lexbor 层

## Overview | 概览

`core/lexbor/` is the repository-visible HTML/CSS parsing and style-management layer built around Lexbor-related integration.  
`core/lexbor/` 是围绕 Lexbor 接入构建的 HTML/CSS 解析与样式管理层。

## Main Areas | 主要区域

- HTML document parsing  
  HTML 文档解析
- stylesheet parsing  
  样式表解析
- style management  
  样式管理
- cascade calculation  
  级联计算
- style caching  
  样式缓存

## Repository-Visible Files | 仓库可见文件

- `lexbor_document.h` / `.cpp` — document parsing / 文档解析
- `lexbor_stylesheet.h` / `.cpp` — stylesheet parsing / 样式表解析
- `style_manager.h` / `.cpp` — style manager / 样式管理器
- `cascade_engine.h` / `.cpp` — cascade logic / 级联逻辑
- `style_cache.h` / `.cpp` — style cache / 样式缓存
- `CMakeLists.txt` — build integration / 构建接入

## Dependencies | 依赖关系

This layer should be read together with:  
该层应结合以下模块理解：

- `third_party/lexbor`
- `core/dom`
- `core/render`
- `core/utils`

## Notes | 说明

- exact parser coverage and CSS support should be verified from implementation, tests, and vendored Lexbor sources  
  具体解析覆盖范围与 CSS 支持应结合实现、测试和 vendored Lexbor 源码验证
- this page should not be used to claim full web-platform parity  
  本页不应被用来宣称完整 Web 平台一致性
- cache and cascade behavior may evolve with style-system changes  
  缓存与级联行为可能随着样式系统演进而变化

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/dom/README.md`
- `core/render/README.md`

