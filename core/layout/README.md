# Layout Module | 布局模块

## Overview | 概览

`core/layout/` contains the native layout engine visible in the current repository.  
`core/layout/` 包含当前仓库中可见的原生布局引擎实现。

## Scope | 范围

Repository-visible areas include:  
从仓库可见的范围包括：

- block layout  
  Block 布局
- flex layout  
  Flexbox 布局
- grid layout  
  Grid 布局
- inline formatting context (`ifc/`)  
  内联格式化上下文（`ifc/`）
- layout types, geometry, cache, and utility helpers  
  布局类型、几何、缓存与工具辅助

## Important Files | 重要文件

- `layout_engine.h` — abstract interface / 抽象接口
- `native_layout_engine.h` / `.cpp` — native engine / 原生引擎
- `block_layout.h` / `.cpp` — block layout / Block 布局
- `flex_layout.h` / `.cpp` — flex layout / Flex 布局
- `grid/` — grid layout implementation / Grid 布局实现
- `ifc/` — inline text and line layout / 内联文本与行布局
- `types/` — geometry, style, layout results, cache / 几何、样式、布局结果、缓存
- `util/` — math and resolver helpers / 数学与解析辅助

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/render`
- `core/utils`
- `third_party/skia`

Used by | 被依赖：

- `core/render`
- `core/event`

## Notes | 说明

- algorithm completeness should be judged from source, not from legacy prose  
  算法完整度应以源码而不是旧说明为准
- text measurement and layout correctness may still depend on platform validation  
  文本测量与布局正确性仍可能依赖平台验证
- this page should not be used to claim full browser parity  
  本页不应被用来宣称与浏览器完全一致

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/render/README.md`
- `docs/KNOWN_LIMITATIONS.md`

