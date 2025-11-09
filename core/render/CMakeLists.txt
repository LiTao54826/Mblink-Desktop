# Render模块 CMakeLists.txt

set(RENDER_SOURCES
    renderer.cpp
    text_renderer.cpp
)

set(RENDER_HEADERS
    renderer.h
    text_renderer.h
)

add_library(lightui_render STATIC ${RENDER_SOURCES})

target_include_directories(lightui_render PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_render PUBLIC
    lightui_utils
)

# 链接Skia（如果启用）
if(LIGHTUI_USE_SKIA AND TARGET skia)
    target_link_libraries(lightui_render PUBLIC skia)
endif()
