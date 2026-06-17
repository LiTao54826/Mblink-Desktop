# API模块 CMakeLists.txt

set(API_SOURCES
    mbink.cpp
    resource_package.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/embedded_js.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/module_resolver.cpp
)

set(API_HEADERS
    mbink.h
    resource_package.h
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
set(MBINK_API_EMBEDDED_JS_REGISTRY_PATHS "")

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
    list(APPEND MBINK_API_EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(MBINK_API_OFFICIAL_PREACT_DIR "${CMAKE_SOURCE_DIR}/third_party/preact")
file(GLOB_RECURSE MBINK_API_OFFICIAL_PREACT_FILES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${MBINK_API_OFFICIAL_PREACT_DIR}/*.js"
    "${MBINK_API_OFFICIAL_PREACT_DIR}/*.json"
)

foreach(JS_REL_PATH IN LISTS MBINK_API_OFFICIAL_PREACT_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${CMAKE_SOURCE_DIR}/${JS_REL_PATH}")
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
    list(APPEND MBINK_API_EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(MBINK_API_EMBEDDED_JS_LIST_FILE "${MBINK_API_GENERATED_DIR}/embedded_js_files.txt")
string(REPLACE ";" "\n" MBINK_API_JS_FILES_CONTENT "${MBINK_API_EMBEDDED_JS_REGISTRY_PATHS}")
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
add_library(mbink_resource_package STATIC
    resource_package.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/embedded_js.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/module_resolver.cpp
)

add_dependencies(mbink_resource_package mbink_api_generate_embedded_js)

target_include_directories(mbink_resource_package PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tools/esm_loader
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mbink_resource_package PUBLIC
    mbink_utils
    quickjs
)

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
    mbink_devtools_bridge
    mbink_compositor
)

if(WIN32)
    target_link_libraries(mbink_api PRIVATE dbghelp)
endif()

# 设置输出名称为 mbink
set_target_properties(mbink_api PROPERTIES
    OUTPUT_NAME "mbink"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set(PYTHON_MBINK_BIN_DIR "${CMAKE_SOURCE_DIR}/bindings/python/mbink/bin")
set(RUST_MBINK_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mbink-sys/runtime")
set(GO_MBINK_DIR "${CMAKE_SOURCE_DIR}/bindings/go")
add_custom_command(TARGET mbink_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_MBINK_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mbink_api>" "${PYTHON_MBINK_BIN_DIR}/$<TARGET_FILE_NAME:mbink_api>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUST_MBINK_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mbink_api>" "${RUST_MBINK_RUNTIME_DIR}/$<TARGET_FILE_NAME:mbink_api>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GO_MBINK_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:mbink_api>" "${GO_MBINK_DIR}/$<TARGET_FILE_NAME:mbink_api>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_LINKER_FILE:mbink_api>" "${GO_MBINK_DIR}/$<TARGET_LINKER_FILE_NAME:mbink_api>"
    COMMENT "Copying MBink runtime to binding directories"
)
