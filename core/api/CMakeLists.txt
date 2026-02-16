# API模块 CMakeLists.txt

set(API_SOURCES
    lightui.cpp
)

set(API_HEADERS
    lightui.h
)

# 构建为 SHARED 库 (DLL/SO)，供 Python ctypes / Go cgo / Rust FFI 调用
add_library(lightui_api SHARED ${API_SOURCES})

# 定义 LIGHTUI_BUILDING_DLL，启用 dllexport
target_compile_definitions(lightui_api PRIVATE LIGHTUI_BUILDING_DLL)

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
    lightui_network
    lightui_devtools
    lightui_compositor
)

# 设置输出名称为 lightui（不带 _api 后缀）
set_target_properties(lightui_api PROPERTIES
    OUTPUT_NAME "lightui"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
