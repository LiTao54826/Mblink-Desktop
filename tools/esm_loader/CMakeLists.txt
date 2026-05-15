# ESM Loader - ES 模块加载器（支持嵌入 payload 模式，可作为 app_bundler 基座）

# ============================================================
# 生成嵌入的 JS 文件（自动发现 js/ 下所有 .js）
# ============================================================
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(JS_DIR "${CMAKE_SOURCE_DIR}/js")
file(GLOB_RECURSE EMBEDDED_JS_FILES CONFIGURE_DEPENDS
    RELATIVE "${JS_DIR}"
    "${JS_DIR}/*.js"
)

set(GENERATE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_js.cmake")
set(GENERATE_REGISTRY_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_js_registry.cmake")
set(GENERATED_JS_INCS "")
set(EMBEDDED_JS_REGISTRY_PATHS "")

foreach(JS_REL_PATH IN LISTS EMBEDDED_JS_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${JS_DIR}/${JS_REL_PATH}")
    set(JS_OUTPUT_FILE "${GENERATED_DIR}/${JS_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${JS_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${JS_INPUT_FILE}
            -DOUTPUT_FILE=${JS_OUTPUT_FILE}
            -DVAR_NAME=${JS_VAR_NAME}
            -P ${GENERATE_SCRIPT}
        DEPENDS ${JS_INPUT_FILE} ${GENERATE_SCRIPT}
        COMMENT "Embedding ${JS_REL_PATH}"
    )

    list(APPEND GENERATED_JS_INCS ${JS_OUTPUT_FILE})
    list(APPEND EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(OFFICIAL_PREACT_DIR "${CMAKE_SOURCE_DIR}/third_party/preact")
file(GLOB_RECURSE OFFICIAL_PREACT_FILES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${OFFICIAL_PREACT_DIR}/*.js"
    "${OFFICIAL_PREACT_DIR}/*.json"
)

foreach(JS_REL_PATH IN LISTS OFFICIAL_PREACT_FILES)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
    set(JS_INPUT_FILE "${CMAKE_SOURCE_DIR}/${JS_REL_PATH}")
    set(JS_OUTPUT_FILE "${GENERATED_DIR}/${JS_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${JS_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${JS_INPUT_FILE}
            -DOUTPUT_FILE=${JS_OUTPUT_FILE}
            -DVAR_NAME=${JS_VAR_NAME}
            -P ${GENERATE_SCRIPT}
        DEPENDS ${JS_INPUT_FILE} ${GENERATE_SCRIPT}
        COMMENT "Embedding ${JS_REL_PATH}"
    )

    list(APPEND GENERATED_JS_INCS ${JS_OUTPUT_FILE})
    list(APPEND EMBEDDED_JS_REGISTRY_PATHS ${JS_REL_PATH})
endforeach()

set(EMBEDDED_JS_LIST_FILE "${GENERATED_DIR}/embedded_js_files.txt")
string(REPLACE ";" "\n" EMBEDDED_JS_FILES_CONTENT "${EMBEDDED_JS_REGISTRY_PATHS}")
file(WRITE "${EMBEDDED_JS_LIST_FILE}" "${EMBEDDED_JS_FILES_CONTENT}\n")

set(EMBEDDED_JS_REGISTRY_INC "${GENERATED_DIR}/embedded_js_registry.inc")
add_custom_command(
    OUTPUT ${EMBEDDED_JS_REGISTRY_INC}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${EMBEDDED_JS_REGISTRY_INC}
        -DINPUT_LIST_FILE=${EMBEDDED_JS_LIST_FILE}
        -P ${GENERATE_REGISTRY_SCRIPT}
    DEPENDS ${EMBEDDED_JS_LIST_FILE} ${GENERATED_JS_INCS} ${GENERATE_REGISTRY_SCRIPT}
    COMMENT "Generating embedded JS registry"
)

add_custom_target(generate_embedded_js
    DEPENDS
        ${GENERATED_JS_INCS}
        ${EMBEDDED_JS_REGISTRY_INC}
)

# ============================================================
# esm_loader 目标
# ============================================================
add_executable(esm_loader
    main.cpp
    embedded_js.cpp
    embedded_js.h
    asset_manager.cpp
    asset_manager.h
    ui_dev_snapshot.cpp
    ui_dev_snapshot.h
    ui_dev_control.cpp
    ui_dev_control.h
    ui_dev_runtime_support.cpp
    ui_dev_runtime_support.h
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/payload.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/payload.h
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.h
)

# 确保在编译前生成嵌入的 JS 文件
add_dependencies(esm_loader generate_embedded_js)

target_include_directories(esm_loader PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/tools/app_bundler
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(esm_loader PRIVATE
    mbink_window
    mbink_dom
    mbink_quickjs
    mbink_event
    mbink_devtools
    mbink_render
    mbink_layout
    mbink_lexbor
    mbink_network
    mbink_bridge
    SDL3::SDL3-static
    skia
)

# 输出到 bin/ 目录
set_target_properties(esm_loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
