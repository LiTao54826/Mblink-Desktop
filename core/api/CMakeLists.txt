# API模块 CMakeLists.txt

set(API_SOURCES
    mbink.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/embedded_js.cpp
)

set(API_HEADERS
    mbink.h
)

set(MBINK_API_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${MBINK_API_GENERATED_DIR})

set(MBINK_API_JS_DIR "${CMAKE_SOURCE_DIR}/js")
set(MBINK_API_PREACT_JS "${MBINK_API_JS_DIR}/preact/preact.js")
set(MBINK_API_HOOKS_JS "${MBINK_API_JS_DIR}/preact/hooks.js")
set(MBINK_API_DOM_POLYFILLS_JS "${MBINK_API_JS_DIR}/polyfills/dom.js")
set(MBINK_API_BOOTSTRAP_JS "${MBINK_API_JS_DIR}/runtime/bootstrap.js")

set(MBINK_API_PREACT_JS_INC "${MBINK_API_GENERATED_DIR}/preact_js.inc")
set(MBINK_API_HOOKS_JS_INC "${MBINK_API_GENERATED_DIR}/hooks_js.inc")
set(MBINK_API_DOM_POLYFILLS_JS_INC "${MBINK_API_GENERATED_DIR}/dom_polyfills_js.inc")
set(MBINK_API_BOOTSTRAP_JS_INC "${MBINK_API_GENERATED_DIR}/bootstrap_js.inc")

set(MBINK_API_GENERATE_SCRIPT "${CMAKE_SOURCE_DIR}/tools/esm_loader/generate_embedded_js.cmake")

add_custom_command(
    OUTPUT ${MBINK_API_PREACT_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${MBINK_API_PREACT_JS}
        -DOUTPUT_FILE=${MBINK_API_PREACT_JS_INC}
        -DVAR_NAME=preact_js
        -P ${MBINK_API_GENERATE_SCRIPT}
    DEPENDS ${MBINK_API_PREACT_JS} ${MBINK_API_GENERATE_SCRIPT}
    COMMENT "Embedding preact.js for mbink_api"
)

add_custom_command(
    OUTPUT ${MBINK_API_HOOKS_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${MBINK_API_HOOKS_JS}
        -DOUTPUT_FILE=${MBINK_API_HOOKS_JS_INC}
        -DVAR_NAME=hooks_js
        -P ${MBINK_API_GENERATE_SCRIPT}
    DEPENDS ${MBINK_API_HOOKS_JS} ${MBINK_API_GENERATE_SCRIPT}
    COMMENT "Embedding hooks.js for mbink_api"
)

add_custom_command(
    OUTPUT ${MBINK_API_DOM_POLYFILLS_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${MBINK_API_DOM_POLYFILLS_JS}
        -DOUTPUT_FILE=${MBINK_API_DOM_POLYFILLS_JS_INC}
        -DVAR_NAME=dom_polyfills_js
        -P ${MBINK_API_GENERATE_SCRIPT}
    DEPENDS ${MBINK_API_DOM_POLYFILLS_JS} ${MBINK_API_GENERATE_SCRIPT}
    COMMENT "Embedding dom.js polyfills for mbink_api"
)

add_custom_command(
    OUTPUT ${MBINK_API_BOOTSTRAP_JS_INC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${MBINK_API_BOOTSTRAP_JS}
        -DOUTPUT_FILE=${MBINK_API_BOOTSTRAP_JS_INC}
        -DVAR_NAME=bootstrap_js
        -P ${MBINK_API_GENERATE_SCRIPT}
    DEPENDS ${MBINK_API_BOOTSTRAP_JS} ${MBINK_API_GENERATE_SCRIPT}
    COMMENT "Embedding runtime/bootstrap.js for mbink_api"
)

add_custom_target(mbink_api_generate_embedded_js
    DEPENDS
        ${MBINK_API_PREACT_JS_INC}
        ${MBINK_API_HOOKS_JS_INC}
        ${MBINK_API_DOM_POLYFILLS_JS_INC}
        ${MBINK_API_BOOTSTRAP_JS_INC}
)

# 构建为 SHARED 库 (DLL/SO)，供 Python ctypes / Go cgo / Rust FFI 调用
add_library(mbink_api SHARED ${API_SOURCES})

add_dependencies(mbink_api mbink_api_generate_embedded_js)

# 定义 MBINK_BUILDING_DLL，启用 dllexport
target_compile_definitions(mbink_api PRIVATE MBINK_BUILDING_DLL)

target_include_directories(mbink_api PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tools/esm_loader
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mbink_api PUBLIC
    mbink_window
    mbink_quickjs
    mbink_dom
    mbink_event
    mbink_layout
    mbink_render
    mbink_bridge
    mbink_utils
    mbink_network
    mbink_devtools
    mbink_compositor
)

# 设置输出名称为 mbink
set_target_properties(mbink_api PROPERTIES
    OUTPUT_NAME "mbink"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# 编译后自动复制运行时库到 Python binding 目录
set(PYTHON_MBINK_BIN_DIR "${CMAKE_SOURCE_DIR}/bindings/python/mbink/bin")
add_custom_command(TARGET mbink_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_MBINK_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mbink_api>" "${PYTHON_MBINK_BIN_DIR}/$<TARGET_FILE_NAME:mbink_api>"
    COMMENT "Copying MBink runtime to Python package bin directories"
)
