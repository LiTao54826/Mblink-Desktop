# API模块 CMakeLists.txt

set(API_SOURCES
    mblink.cpp
    resource_package.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/embedded_js.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/module_resolver.cpp
)

set(API_HEADERS
    mblink.h
    resource_package.h
)

set(MBLINK_API_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${MBLINK_API_GENERATED_DIR})

set(MBLINK_API_JS_DIR "${CMAKE_SOURCE_DIR}/js")
file(GLOB_RECURSE MBLINK_API_JS_FILES CONFIGURE_DEPENDS
    RELATIVE "${MBLINK_API_JS_DIR}"
    "${MBLINK_API_JS_DIR}/*.js"
)

set(MBLINK_API_GENERATE_SCRIPT "${CMAKE_SOURCE_DIR}/tools/esm_loader/generate_embedded_js.cmake")
set(MBLINK_API_GENERATE_REGISTRY_SCRIPT "${CMAKE_SOURCE_DIR}/tools/esm_loader/generate_embedded_js_registry.cmake")
set(MBLINK_API_GENERATED_JS_INCS "")
set(MBLINK_API_EMBEDDED_JS_REGISTRY_PATHS "")

foreach(JS_REL_PATH IN LISTS MBLINK_API_JS_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${MBLINK_API_JS_DIR}/${JS_REL_PATH}")
    set(JS_OUTPUT_FILE "${MBLINK_API_GENERATED_DIR}/${JS_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${JS_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${JS_INPUT_FILE}
            -DOUTPUT_FILE=${JS_OUTPUT_FILE}
            -DVAR_NAME=${JS_VAR_NAME}
            -P ${MBLINK_API_GENERATE_SCRIPT}
        DEPENDS ${JS_INPUT_FILE} ${MBLINK_API_GENERATE_SCRIPT}
        COMMENT "Embedding ${JS_REL_PATH} for mblink_api"
    )

    list(APPEND MBLINK_API_GENERATED_JS_INCS ${JS_OUTPUT_FILE})
    list(APPEND MBLINK_API_EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(MBLINK_API_OFFICIAL_PREACT_DIR "${CMAKE_SOURCE_DIR}/third_party/preact")
file(GLOB_RECURSE MBLINK_API_OFFICIAL_PREACT_FILES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${MBLINK_API_OFFICIAL_PREACT_DIR}/*.js"
    "${MBLINK_API_OFFICIAL_PREACT_DIR}/*.json"
)

foreach(JS_REL_PATH IN LISTS MBLINK_API_OFFICIAL_PREACT_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${CMAKE_SOURCE_DIR}/${JS_REL_PATH}")
    set(JS_OUTPUT_FILE "${MBLINK_API_GENERATED_DIR}/${JS_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${JS_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${JS_INPUT_FILE}
            -DOUTPUT_FILE=${JS_OUTPUT_FILE}
            -DVAR_NAME=${JS_VAR_NAME}
            -P ${MBLINK_API_GENERATE_SCRIPT}
        DEPENDS ${JS_INPUT_FILE} ${MBLINK_API_GENERATE_SCRIPT}
        COMMENT "Embedding ${JS_REL_PATH} for mblink_api"
    )

    list(APPEND MBLINK_API_GENERATED_JS_INCS ${JS_OUTPUT_FILE})
    list(APPEND MBLINK_API_EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(MBLINK_API_EMBEDDED_JS_LIST_FILE "${MBLINK_API_GENERATED_DIR}/embedded_js_files.txt")
string(REPLACE ";" "\n" MBLINK_API_JS_FILES_CONTENT "${MBLINK_API_EMBEDDED_JS_REGISTRY_PATHS}")
file(WRITE "${MBLINK_API_EMBEDDED_JS_LIST_FILE}" "${MBLINK_API_JS_FILES_CONTENT}\n")

set(MBLINK_API_EMBEDDED_JS_REGISTRY_INC "${MBLINK_API_GENERATED_DIR}/embedded_js_registry.inc")
add_custom_command(
    OUTPUT ${MBLINK_API_EMBEDDED_JS_REGISTRY_INC}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${MBLINK_API_EMBEDDED_JS_REGISTRY_INC}
        -DINPUT_LIST_FILE=${MBLINK_API_EMBEDDED_JS_LIST_FILE}
        -P ${MBLINK_API_GENERATE_REGISTRY_SCRIPT}
    DEPENDS ${MBLINK_API_EMBEDDED_JS_LIST_FILE} ${MBLINK_API_GENERATED_JS_INCS} ${MBLINK_API_GENERATE_REGISTRY_SCRIPT}
    COMMENT "Generating embedded JS registry for mblink_api"
)

add_custom_target(mblink_api_generate_embedded_js
    DEPENDS
        ${MBLINK_API_GENERATED_JS_INCS}
        ${MBLINK_API_EMBEDDED_JS_REGISTRY_INC}
)

# 构建为 SHARED 库 (DLL/SO)，供 Python ctypes / Go cgo / Rust FFI 调用
add_library(mblink_resource_package STATIC
    resource_package.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/embedded_js.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/module_resolver.cpp
)

add_dependencies(mblink_resource_package mblink_api_generate_embedded_js)

target_include_directories(mblink_resource_package PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tools/esm_loader
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mblink_resource_package PUBLIC
    mblink_utils
    quickjs
)

add_library(mblink_api SHARED ${API_SOURCES})

add_dependencies(mblink_api mblink_api_generate_embedded_js)

# 定义 MBLINK_BUILDING_DLL，启用 dllexport
target_compile_definitions(mblink_api PRIVATE MBLINK_BUILDING_DLL)

target_include_directories(mblink_api PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tools/esm_loader
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mblink_api PUBLIC
    mblink_window
    mblink_quickjs
    mblink_dom
    mblink_event
    mblink_layout
    mblink_render
    mblink_bridge
    mblink_utils
    mblink_network
    mblink_devtools_bridge
    mblink_compositor
)

if(WIN32)
    target_link_libraries(mblink_api PRIVATE dbghelp)
endif()

# 设置输出名称为 mblink
set_target_properties(mblink_api PROPERTIES
    OUTPUT_NAME "mblink"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set(PYTHON_MBLINK_BIN_DIR "${CMAKE_SOURCE_DIR}/bindings/python/mblink/bin")
set(RUST_MBLINK_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mblink-sys/runtime")
set(GO_MBLINK_DIR "${CMAKE_SOURCE_DIR}/bindings/go")
add_custom_command(TARGET mblink_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_MBLINK_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mblink_api>" "${PYTHON_MBLINK_BIN_DIR}/$<TARGET_FILE_NAME:mblink_api>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUST_MBLINK_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mblink_api>" "${RUST_MBLINK_RUNTIME_DIR}/$<TARGET_FILE_NAME:mblink_api>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GO_MBLINK_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mblink_api>" "${GO_MBLINK_DIR}/$<TARGET_FILE_NAME:mblink_api>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_LINKER_FILE:mblink_api>" "${GO_MBLINK_DIR}/$<TARGET_LINKER_FILE_NAME:mblink_api>"
    COMMENT "Copying MBlink runtime to binding directories"
)
