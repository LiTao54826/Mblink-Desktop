# API模块 CMakeLists.txt

set(API_SOURCES
    lightui.cpp
)

set(API_HEADERS
    lightui.h
)

add_library(lightui_api STATIC ${API_SOURCES})

target_include_directories(lightui_api PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_api PUBLIC
    lightui_window
    lightui_quickjs
    lightui_dom
    lightui_event
    lightui_layout
    lightui_render
    lightui_bridge
    lightui_utils
)
