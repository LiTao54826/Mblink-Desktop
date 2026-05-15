set(TEMPLATE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(TEMPLATE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_templates")
set(TEMPLATE_GENERATE_FILE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_template_file.cmake")
set(TEMPLATE_GENERATE_REGISTRY_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_template_registry.cmake")
file(MAKE_DIRECTORY ${TEMPLATE_GENERATED_DIR})

file(GLOB_RECURSE EMBEDDED_TEMPLATE_FILES CONFIGURE_DEPENDS
    RELATIVE "${TEMPLATE_SOURCE_DIR}"
    "${TEMPLATE_SOURCE_DIR}/*"
)

set(GENERATED_TEMPLATE_INCS "")
set(EMBEDDED_TEMPLATE_FILE_LIST "")

macro(add_embedded_ui_dev_template_file TEMPLATE_REL_PATH TEMPLATE_INPUT_FILE)
    list(APPEND EMBEDDED_TEMPLATE_FILE_LIST ${TEMPLATE_REL_PATH})
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" TEMPLATE_VAR_NAME "${TEMPLATE_REL_PATH}")
    set(TEMPLATE_OUTPUT_FILE "${TEMPLATE_GENERATED_DIR}/${TEMPLATE_VAR_NAME}.inc")

    add_custom_command(
        OUTPUT ${TEMPLATE_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${TEMPLATE_INPUT_FILE}
            -DOUTPUT_FILE=${TEMPLATE_OUTPUT_FILE}
            -DVAR_NAME=${TEMPLATE_VAR_NAME}
            -P ${TEMPLATE_GENERATE_FILE_SCRIPT}
        DEPENDS ${TEMPLATE_INPUT_FILE} ${TEMPLATE_GENERATE_FILE_SCRIPT}
        COMMENT "Embedding UI template ${TEMPLATE_REL_PATH}"
    )

    list(APPEND GENERATED_TEMPLATE_INCS ${TEMPLATE_OUTPUT_FILE})
endmacro()

foreach(TEMPLATE_REL_PATH IN LISTS EMBEDDED_TEMPLATE_FILES)
    if(IS_DIRECTORY "${TEMPLATE_SOURCE_DIR}/${TEMPLATE_REL_PATH}")
        continue()
    endif()
    if(TEMPLATE_REL_PATH MATCHES "(^|/)[.]")
        continue()
    endif()

    add_embedded_ui_dev_template_file("${TEMPLATE_REL_PATH}" "${TEMPLATE_SOURCE_DIR}/${TEMPLATE_REL_PATH}")
endforeach()

set(RUST_MBINK_VENDOR_TEMPLATE_ROOT "runtime/rust/rust_host/vendor")
set(RUST_MBINK_CRATE_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mbink")
set(RUST_MBINK_SYS_CRATE_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mbink-sys")
if(WIN32)
    set(RUST_MBINK_RUNTIME_LIBRARY_FILE "mbink.dll")
elseif(APPLE)
    set(RUST_MBINK_RUNTIME_LIBRARY_FILE "libmbink.dylib")
else()
    set(RUST_MBINK_RUNTIME_LIBRARY_FILE "libmbink.so")
endif()
set(RUST_MBINK_RUNTIME_LIBRARY "${RUST_MBINK_SYS_CRATE_DIR}/runtime/${RUST_MBINK_RUNTIME_LIBRARY_FILE}")
set(GO_MBINK_VENDOR_TEMPLATE_ROOT "runtime/go/.mbink/mbink-go")
set(GO_MBINK_DIR "${CMAKE_SOURCE_DIR}/bindings/go")
set(PYTHON_MBINK_VENDOR_TEMPLATE_ROOT "runtime/python/vendor/mbink")
set(PYTHON_MBINK_DIR "${CMAKE_SOURCE_DIR}/bindings/python/mbink")

file(GLOB_RECURSE RUST_MBINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${RUST_MBINK_CRATE_DIR}"
    "${RUST_MBINK_CRATE_DIR}/src/*.rs"
)

add_embedded_ui_dev_template_file(
    "${RUST_MBINK_VENDOR_TEMPLATE_ROOT}/mbink/Cargo.toml"
    "${RUST_MBINK_CRATE_DIR}/Cargo.toml"
)
foreach(RUST_MBINK_REL_PATH IN LISTS RUST_MBINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${RUST_MBINK_VENDOR_TEMPLATE_ROOT}/mbink/${RUST_MBINK_REL_PATH}"
        "${RUST_MBINK_CRATE_DIR}/${RUST_MBINK_REL_PATH}"
    )
endforeach()

file(GLOB_RECURSE RUST_MBINK_SYS_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${RUST_MBINK_SYS_CRATE_DIR}"
    "${RUST_MBINK_SYS_CRATE_DIR}/src/*.rs"
)

foreach(RUST_MBINK_SYS_REL_PATH IN ITEMS Cargo.toml build.rs)
    add_embedded_ui_dev_template_file(
        "${RUST_MBINK_VENDOR_TEMPLATE_ROOT}/mbink-sys/${RUST_MBINK_SYS_REL_PATH}"
        "${RUST_MBINK_SYS_CRATE_DIR}/${RUST_MBINK_SYS_REL_PATH}"
    )
endforeach()
foreach(RUST_MBINK_SYS_REL_PATH IN LISTS RUST_MBINK_SYS_SRC_FILES)
add_embedded_ui_dev_template_file(
    "${RUST_MBINK_VENDOR_TEMPLATE_ROOT}/mbink-sys/${RUST_MBINK_SYS_REL_PATH}"
    "${RUST_MBINK_SYS_CRATE_DIR}/${RUST_MBINK_SYS_REL_PATH}"
)
endforeach()

add_embedded_ui_dev_template_file(
    "${GO_MBINK_VENDOR_TEMPLATE_ROOT}/go.mod"
    "${GO_MBINK_DIR}/go.mod"
)
add_embedded_ui_dev_template_file(
    "${GO_MBINK_VENDOR_TEMPLATE_ROOT}/mbink.lib"
    "${GO_MBINK_DIR}/mbink.lib"
)
add_embedded_ui_dev_template_file(
    "${GO_MBINK_VENDOR_TEMPLATE_ROOT}/core/api/mbink.h"
    "${CMAKE_SOURCE_DIR}/core/api/mbink.h"
)
file(GLOB_RECURSE GO_MBINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${GO_MBINK_DIR}"
    "${GO_MBINK_DIR}/mbink/*.go"
)
foreach(GO_MBINK_REL_PATH IN LISTS GO_MBINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${GO_MBINK_VENDOR_TEMPLATE_ROOT}/${GO_MBINK_REL_PATH}"
        "${GO_MBINK_DIR}/${GO_MBINK_REL_PATH}"
    )
endforeach()

file(GLOB_RECURSE PYTHON_MBINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${PYTHON_MBINK_DIR}"
    "${PYTHON_MBINK_DIR}/*.py"
)
foreach(PYTHON_MBINK_REL_PATH IN LISTS PYTHON_MBINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${PYTHON_MBINK_VENDOR_TEMPLATE_ROOT}/${PYTHON_MBINK_REL_PATH}"
        "${PYTHON_MBINK_DIR}/${PYTHON_MBINK_REL_PATH}"
    )
endforeach()

set(EMBEDDED_TEMPLATE_LIST_FILE "${TEMPLATE_GENERATED_DIR}/embedded_template_files.txt")
string(REPLACE ";" "\n" EMBEDDED_TEMPLATE_FILES_CONTENT "${EMBEDDED_TEMPLATE_FILE_LIST}")
file(WRITE "${EMBEDDED_TEMPLATE_LIST_FILE}" "${EMBEDDED_TEMPLATE_FILES_CONTENT}\n")

set(EMBEDDED_TEMPLATE_REGISTRY_INC "${TEMPLATE_GENERATED_DIR}/embedded_template_registry.inc")
add_custom_command(
    OUTPUT ${EMBEDDED_TEMPLATE_REGISTRY_INC}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${EMBEDDED_TEMPLATE_REGISTRY_INC}
        -DINPUT_LIST_FILE=${EMBEDDED_TEMPLATE_LIST_FILE}
        -P ${TEMPLATE_GENERATE_REGISTRY_SCRIPT}
    DEPENDS ${EMBEDDED_TEMPLATE_LIST_FILE} ${GENERATED_TEMPLATE_INCS} ${TEMPLATE_GENERATE_REGISTRY_SCRIPT}
    COMMENT "Generating embedded UI template registry"
)

add_custom_target(generate_embedded_ui_dev_templates
    DEPENDS
        ${GENERATED_TEMPLATE_INCS}
        ${EMBEDDED_TEMPLATE_REGISTRY_INC}
)
add_dependencies(generate_embedded_ui_dev_templates mbink_api)

add_executable(mbink_ui_dev
    main.cpp
    common.cpp
    common.h
    daemon_ipc.cpp
    daemon_ipc.h
    daemon_server.cpp
    daemon_server.h
)

target_include_directories(mbink_ui_dev PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)
add_dependencies(mbink_ui_dev esm_loader generate_embedded_ui_dev_templates)

target_link_libraries(mbink_ui_dev PRIVATE
    mbink_resource_package
    nlohmann_json
)

set_target_properties(mbink_ui_dev PROPERTIES
    OUTPUT_NAME "mbink-ui-dev"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
