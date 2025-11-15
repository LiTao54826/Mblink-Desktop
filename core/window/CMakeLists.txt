# Window模块 CMakeLists.txt

set(WINDOW_SOURCES
    window.cpp
    window_manager.cpp
)

set(WINDOW_HEADERS
    window.h
    window_event.h
    window_manager.h
)

add_library(lightui_window STATIC ${WINDOW_SOURCES})

target_include_directories(lightui_window PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

# 链接 lightui_layout
target_link_libraries(lightui_window PUBLIC lightui_layout)

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

    # Match Skia's runtime library settings (MSVC)
    # Skia 138 is compiled with Release runtime (/MT) and _ITERATOR_DEBUG_LEVEL=0
    # We MUST match these settings even in Debug mode
    if(MSVC)
        target_compile_options(lightui_window PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(lightui_window PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )

        # Link D3D12 libraries for Skia
        target_link_libraries(lightui_window PUBLIC
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
else()
    message(STATUS "Window module: Skia target not found, skipping")
    # 尝试手动添加include目录
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/skia")
        message(STATUS "Window module: Manually adding Skia include directory")
        target_include_directories(lightui_window PUBLIC ${CMAKE_SOURCE_DIR}/third_party/skia)
    endif()
endif()
