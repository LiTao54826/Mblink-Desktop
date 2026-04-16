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
..\..\build\bin\Release\mbink-ui-dev.exe eval "document.querySelector('input').value=9999"
..\..\build\bin\Release\mbink-ui-dev.exe eval "document.querySelector('button').click()"

# 手动关闭 runtime 窗口后，该项目会被视为 stopped
# watch / reload / fallback 不会自动重新拉起被手动关闭的 runtime