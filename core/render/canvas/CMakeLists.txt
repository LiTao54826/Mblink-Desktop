# Canvas Rendering CMakeLists.txt

# 设置源文件
set(CANVAS_SOURCES
    canvas_rendering_context_2d.cpp
)

set(CANVAS_HEADERS
    canvas_rendering_context_2d.h
)

# 添加到父级目标
target_sources(mbink_core PRIVATE ${CANVAS_SOURCES})
target_sources(mbink_core PUBLIC FILE_SET HEADERS FILES ${CANVAS_HEADERS})
