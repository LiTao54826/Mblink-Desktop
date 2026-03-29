# Utils Module | 工具模块

## Overview | 概览

`core/utils/` provides shared helper infrastructure used across the repository.  
`core/utils/` 提供在整个仓库中复用的通用辅助基础设施。

## Repository-Visible Areas | 仓库可见范围

- logging  
  日志
- JSON helpers  
  JSON 工具
- string helpers  
  字符串工具
- file and path helpers  
  文件与路径工具
- time helpers  
  时间工具
- color conversion helpers  
  颜色转换工具
- URL helpers  
  URL 工具

## Important Files | 重要文件

- `logger.h` / `.cpp` — logging / 日志系统
- `json_utils.h` / `.cpp` — JSON helpers / JSON 工具
- `string_utils.h` / `.cpp` — string helpers / 字符串工具
- `file_utils.h` / `.cpp` — file helpers / 文件工具
- `time_utils.h` / `.cpp` — time helpers / 时间工具
- `color_utils.h` / `.cpp` — color helpers / 颜色工具
- `url_utils.h` / `.cpp` — URL helpers / URL 工具
- `CMakeLists.txt` — build integration / 构建接入

## Notes | 说明

- utility behavior should be verified from implementation rather than old descriptive examples  
  工具行为应以实现而不是旧示例说明为准
- these helpers are widely shared, so stability changes can have broad impact  
  这些辅助模块被广泛复用，稳定性变动影响面较大
- platform-specific path, file, and timing behavior may still need validation  
  平台相关的路径、文件和时间行为仍可能需要验证

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `docs/BUILD.md`
- `docs/KNOWN_LIMITATIONS.md`

