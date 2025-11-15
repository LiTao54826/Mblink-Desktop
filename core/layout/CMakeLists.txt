# Layout模块 CMakeLists.txt

set(LAYOUT_SOURCES
    layout_engine.cpp
)

set(LAYOUT_HEADERS
    layout_engine.h
)

add_library(lightui_layout STATIC ${LAYOUT_SOURCES})

target_include_directories(lightui_layout PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_layout PUBLIC
    lightui_utils
)

# 链接 Taffy 布局引擎
if(TARGET taffy)
    target_link_libraries(lightui_layout PUBLIC taffy)
    message(STATUS "Linking Taffy layout engine")
else()
    message(WARNING "Taffy not found, layout module may not build correctly")
endif()
