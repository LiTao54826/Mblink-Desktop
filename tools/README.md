cmake --build build --config Release --target mbink_ui_dev esm_loader

# 基本用法
build\bin\Release\mbink-ui-dev.exe open
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe open "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot
build\bin\Release\mbink-ui-dev.exe info
build\bin\Release\mbink-ui-dev.exe build
build\bin\Release\mbink-ui-dev.exe reload
build\bin\Release\mbink-ui-dev.exe stop

# P3 元素精确控制
build\bin\Release\mbink-ui-dev.exe query "#todo-input"
build\bin\Release\mbink-ui-dev.exe inspect "button[type=\"submit\"]"
build\bin\Release\mbink-ui-dev.exe click "button[type=\"submit\"]"
build\bin\Release\mbink-ui-dev.exe input-text "#todo-input" "hello world"
build\bin\Release\mbink-ui-dev.exe scroll body --y 400
build\bin\Release\mbink-ui-dev.exe highlight "#todo-input" --color "#ff4d4f"

# 自动项目定位
# 下面这些命令会优先按以下顺序解析项目：
# 1) --project <path>
# 2) 命令位置参数中的项目路径（如 open <path>）
# 3) 当前目录/父目录中的 .devui
# 4) 当前目录/父目录中的 mbink.config.json（必要时自动创建 .devui）
# 5) 若当前只管理了 1 个项目，则自动回退到该项目
# 若同时管理了多个项目且无法唯一确定，会提示使用 --project <path>

# 多项目管理
# 每个项目都有自己的 .devui，里面记录 project_id / runtime_id
# daemon state、named pipe、runtime snapshot/command/response/console/errors/stdout/stderr
# 都按 project_id 隔离，可并行管理多个项目

# MCP
build\bin\Release\mbink-ui-dev.exe serve
# open_project 的 path / project_root 可省略；省略时会按当前目录自动解析项目

# 示例：在 todo_app_js 目录内可直接省略项目路径
cd examples\todo_app_js
..\..\build\bin\Release\mbink-ui-dev.exe open
..\..\build\bin\Release\mbink-ui-dev.exe snapshot
..\..\build\bin\Release\mbink-ui-dev.exe query "#todo-input"
..\..\build\bin\Release\mbink-ui-dev.exe input-text "#todo-input" "task from cli"
..\..\build\bin\Release\mbink-ui-dev.exe click "button[type=\"submit\"]"
..\..\build\bin\Release\mbink-ui-dev.exe query span

# 已实现行为说明
# snapshot 现在会在 runtime 首帧主动生成，open 后第一次 snapshot 不再返回 stub-root
# click / input-text / scroll / highlight 会在执行后推进 event loop + render，再返回最新结果
# body / html 选择器已做特殊处理，可直接用于 query / inspect / scroll

# 已知限制
# eval 在 Windows cmd.exe 下仍可能受到 shell quoting 影响；复杂 JS 更建议通过 MCP 调用或 PowerShell 执行

# 手动关闭 runtime 窗口后，该项目会被视为 stopped
# watch / reload / fallback 不会自动重新拉起被手动关闭的 runtime
