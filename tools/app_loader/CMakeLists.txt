# MBink App Loader - 通用 app.js 加载器

# 创建生成目录
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# JS 源文件路径
set(JS_DIR "${CMAKE_SOURCE_DIR}/js")
set(PREACT_JS "${JS_DIR}/preact/preact.js")
set(HOOKS_JS "${JS_DIR}/preact/hooks.js")
set(DOM_POLYFILLS_JS "${JS_DIR}/polyfills/dom.js")

# 生成的 .inc 文件路径
set(PREACT_JS_INC "${GENERATED_DIR}/preact_js.inc")
set(HOOKS_JS_INC "${GENERATED_DIR}/hooks_js.inc")
set(DOM_POLYFILLS_JS_INC "${GENERATED_DIR}/dom_polyfills_js.inc")

# CMake 脚本路径
set(GENERATE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_js.cmake")

# 添加自定义命令生成嵌入的 JS 代码
add_custom_command(
    OUTPUT ${PREACT_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${PREACT_JS}
        -DOUTPUT_FILE=${PREACT_JS_INC}
        -DVAR_NAME=preact_js
        -P ${GENERATE_SCRIPT}
    DEPENDS ${PREACT_JS} ${GENERATE_SCRIPT}
    COMMENT "Embedding preact.js"
)

add_custom_command(
    OUTPUT ${HOOKS_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${HOOKS_JS}
        -DOUTPUT_FILE=${HOOKS_JS_INC}
        -DVAR_NAME=hooks_js
        -P ${GENERATE_SCRIPT}
    DEPENDS ${HOOKS_JS} ${GENERATE_SCRIPT}
    COMMENT "Embedding hooks.js"
)

add_custom_command(
    OUTPUT ${DOM_POLYFILLS_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${DOM_POLYFILLS_JS}
        -DOUTPUT_FILE=${DOM_POLYFILLS_JS_INC}
        -DVAR_NAME=dom_polyfills_js
        -P ${GENERATE_SCRIPT}
    DEPENDS ${DOM_POLYFILLS_JS} ${GENERATE_SCRIPT}
    COMMENT "Embedding dom.js polyfills"
)

# 创建自定义目标确保生成文件
add_custom_target(generate_embedded_js
    DEPENDS
        ${PREACT_JS_INC}
        ${HOOKS_JS_INC}
        ${DOM_POLYFILLS_JS_INC}
)

add_executable(app_loader
    main.cpp
    embedded_js.cpp
    embedded_js.h
)

# 确保在编译前生成嵌入的 JS 文件
add_dependencies(app_loader generate_embedded_js)

# 添加生成目录到包含路径
target_include_directories(app_loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(app_loader
    lightui_window
    lightui_dom
    lightui_event
    lightui_render
    lightui_quickjs
    lightui_lexbor
    lightui_devtools
    SDL3::SDL3-static
    skia
)

# 输出到 bin/ 目录
set_target_properties(app_loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
