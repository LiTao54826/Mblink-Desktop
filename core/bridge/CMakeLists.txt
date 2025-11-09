# Bridge模块 CMakeLists.txt

set(BRIDGE_SOURCES
    bridge.cpp
)

set(BRIDGE_HEADERS
    bridge.h
)

add_library(lightui_bridge STATIC ${BRIDGE_SOURCES})

target_include_directories(lightui_bridge PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_bridge PUBLIC
    lightui_utils
    lightui_quickjs
)
