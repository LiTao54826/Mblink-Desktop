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
file(GLOB_RECURSE MBINK_API_JS_FILES CONFIGURE_DEPENDS
    RELATIVE "${MBINK_API_JS_DIR}"
    "${MBINK_API_JS_DIR}/*.js"
)

set(MBINK_API_GENERATE_SCRIPT "${CMAKE_SOURCE_DIR}/tools/esm_loader/generate_embedded_js.cmake")
set(MBINK_API_GENERATE_REGISTRY_SCRIPT "${CMAKE_SOURCE_DIR}/tools/esm_loader/generate_embedded_js_registry.cmake")
set(MBINK_API_GENERATED_JS_INCS "")

foreach(JS_REL_PATH IN LISTS MBINK_API_JS_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${MBINK_API_JS_DIR}/${JS_REL_PATH}")
    set(JS_OUTPUT_FILE "${MBINK_API_GENERATED_DIR}/${JS_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${JS_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${JS_INPUT_FILE}
            -DOUTPUT_FILE=${JS_OUTPUT_FILE}
            -DVAR_NAME=${JS_VAR_NAME}
            -P ${MBINK_API_GENERATE_SCRIPT}
        DEPENDS ${JS_INPUT_FILE} ${MBINK_API_GENERATE_SCRIPT}
        COMMENT "Embedding ${JS_REL_PATH} for mbink_api"
    )

    list(APPEND MBINK_API_GENERATED_JS_INCS ${JS_OUTPUT_FILE})
endforeach()

set(MBINK_API_EMBEDDED_JS_LIST_FILE "${MBINK_API_GENERATED_DIR}/embedded_js_files.txt")
string(REPLACE ";" "\n" MBINK_API_JS_FILES_CONTENT "${MBINK_API_JS_FILES}")
file(WRITE "${MBINK_API_EMBEDDED_JS_LIST_FILE}" "${MBINK_API_JS_FILES_CONTENT}\n")

set(MBINK_API_EMBEDDED_JS_REGISTRY_INC "${MBINK_API_GENERATED_DIR}/embedded_js_registry.inc")
add_custom_command(
    OUTPUT ${MBINK_API_EMBEDDED_JS_REGISTRY_INC}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${MBINK_API_EMBEDDED_JS_REGISTRY_INC}
        -DINPUT_LIST_FILE=${MBINK_API_EMBEDDED_JS_LIST_FILE}
        -P ${MBINK_API_GENERATE_REGISTRY_SCRIPT}
    DEPENDS ${MBINK_API_EMBEDDED_JS_LIST_FILE} ${MBINK_API_GENERATED_JS_INCS} ${MBINK_API_GENERATE_REGISTRY_SCRIPT}
    COMMENT "Generating embedded JS registry for mbink_api"
)

add_custom_target(mbink_api_generate_embedded_js
    DEPENDS
        ${MBINK_API_GENERATED_JS_INCS}
        ${MBINK_API_EMBEDDED_JS_REGISTRY_INC}
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
