# ESM Loader - ES 模块加载器（支持嵌入 payload 模式，可作为 app_bundler 基座）

add_executable(esm_loader
    main.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_loader/embedded_js.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_loader/embedded_js.h
    ${CMAKE_SOURCE_DIR}/tools/app_loader/asset_manager.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_loader/asset_manager.h
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/payload.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/payload.h
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.h
)

# 确保在编译前生成嵌入的 JS 文件
add_dependencies(esm_loader generate_embedded_js)

target_include_directories(esm_loader PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tools/app_loader
    ${CMAKE_BINARY_DIR}/tools/app_loader
    ${CMAKE_SOURCE_DIR}/tools/app_bundler
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(esm_loader PRIVATE
    lightui_window
    lightui_dom
    lightui_quickjs
    lightui_event
    lightui_devtools
    lightui_render
    lightui_layout
    lightui_lexbor
    lightui_network
    lightui_bridge
    SDL3::SDL3-static
    skia
)

# 输出到 bin/ 目录
set_target_properties(esm_loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
