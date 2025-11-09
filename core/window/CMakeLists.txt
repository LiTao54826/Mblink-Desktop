# Window模块 CMakeLists.txt

set(WINDOW_SOURCES
    window.cpp
)

set(WINDOW_HEADERS
    window.h
)

add_library(lightui_window STATIC ${WINDOW_SOURCES})

target_include_directories(lightui_window PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接SDL3（如果可用）
if(TARGET SDL3::SDL3)
    target_link_libraries(lightui_window PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(lightui_window PUBLIC SDL3-static)
else()
    message(WARNING "SDL3 not found, window module may not build correctly")
endif()

# 链接Skia（如果启用）
message(STATUS "Window module: Checking for skia target...")
if(TARGET skia)
    message(STATUS "Window module: Linking Skia")
    target_link_libraries(lightui_window PUBLIC skia)
    target_include_directories(lightui_window PUBLIC ${CMAKE_SOURCE_DIR}/third_party/skia)
else()
    message(STATUS "Window module: Skia target not found, skipping")
    # 尝试手动添加include目录
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/skia")
        message(STATUS "Window module: Manually adding Skia include directory")
        target_include_directories(lightui_window PUBLIC ${CMAKE_SOURCE_DIR}/third_party/skia)
    endif()
endif()
