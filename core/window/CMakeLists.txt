# Window模块 CMakeLists.txt

set(WINDOW_SOURCES
    window.cpp
    window_manager.cpp
    window_renderer.cpp
    window_dom_observer.cpp
    display_backend.cpp
    app_tray.cpp
)

# Windows 平台特定源文件
if(WIN32)
    list(APPEND WINDOW_SOURCES window_win32.cpp app_tray_win32.cpp)
endif()

set(WINDOW_HEADERS
    window.h
    window_event.h
    window_manager.h
    window_renderer.h
    window_dom_observer.h
    display_backend.h
    app_tray.h
)

# Windows 平台特定头文件
if(WIN32)
    list(APPEND WINDOW_HEADERS window_win32.h)
endif()

add_library(mblink_window STATIC ${WINDOW_SOURCES})

target_include_directories(mblink_window PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

# 链接 mblink_layout
target_link_libraries(mblink_window PUBLIC mblink_layout)
target_link_libraries(mblink_window PUBLIC mblink_devtools_bridge)

# 链接 mblink_compositor（分层合成架构）
target_link_libraries(mblink_window PUBLIC mblink_compositor)

# 链接SDL3（如果可用）
if(TARGET SDL3::SDL3)
    target_link_libraries(mblink_window PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(mblink_window PUBLIC SDL3-static)
else()
    message(WARNING "SDL3 not found, window module may not build correctly")
endif()

# 链接Skia（如果启用）
message(STATUS "Window module: Checking for skia target...")
if(TARGET skia)
    message(STATUS "Window module: Linking Skia")
    target_link_libraries(mblink_window PUBLIC skia)
    # Add Skia include directories based on build configuration
    target_include_directories(mblink_window PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/skia/Debug>
        $<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/skia/Release>
    )

    # Match Skia's runtime library settings (MSVC)
    # Skia 138 is compiled with Release runtime (/MT) and _ITERATOR_DEBUG_LEVEL=0
    # We MUST match these settings even in Debug mode
    if(MSVC)
        target_compile_options(mblink_window PUBLIC
            $<$<CONFIG:Debug>:/MT>
            $<$<CONFIG:Release>:/MT>
        )
        target_compile_definitions(mblink_window PUBLIC
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        )

        # Link D3D12 libraries for Skia
        target_link_libraries(mblink_window PUBLIC
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )
    endif()
else()
    message(STATUS "Window module: Skia target not found, skipping")
endif()

# 链接 OpenGL（用于 DisplayBackend）
find_package(OpenGL)
if(OpenGL_FOUND)
    target_link_libraries(mblink_window PUBLIC OpenGL::GL)
elseif(WIN32)
    target_link_libraries(mblink_window PUBLIC opengl32)
endif()
