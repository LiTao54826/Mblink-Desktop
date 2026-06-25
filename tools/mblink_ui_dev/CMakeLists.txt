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

set(RUST_MBLINK_VENDOR_TEMPLATE_ROOT "runtime/rust/rust_host/vendor")
set(RUST_MBLINK_CRATE_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mblink")
set(RUST_MBLINK_SYS_CRATE_DIR "${CMAKE_SOURCE_DIR}/bindings/rust/mblink-sys")
if(WIN32)
    set(RUST_MBLINK_RUNTIME_LIBRARY_FILE "mblink.dll")
elseif(APPLE)
    set(RUST_MBLINK_RUNTIME_LIBRARY_FILE "libmblink.dylib")
else()
    set(RUST_MBLINK_RUNTIME_LIBRARY_FILE "libmblink.so")
endif()
set(RUST_MBLINK_RUNTIME_LIBRARY "${RUST_MBLINK_SYS_CRATE_DIR}/runtime/${RUST_MBLINK_RUNTIME_LIBRARY_FILE}")
set(GO_MBLINK_VENDOR_TEMPLATE_ROOT "runtime/go/.mblink/mblink-go")
set(GO_MBLINK_DIR "${CMAKE_SOURCE_DIR}/bindings/go")
set(PYTHON_MBLINK_VENDOR_TEMPLATE_ROOT "runtime/python/vendor/mblink")
set(PYTHON_MBLINK_DIR "${CMAKE_SOURCE_DIR}/bindings/python/mblink")

file(GLOB_RECURSE RUST_MBLINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${RUST_MBLINK_CRATE_DIR}"
    "${RUST_MBLINK_CRATE_DIR}/src/*.rs"
)

add_embedded_ui_dev_template_file(
    "${RUST_MBLINK_VENDOR_TEMPLATE_ROOT}/mblink/Cargo.toml"
    "${RUST_MBLINK_CRATE_DIR}/Cargo.toml"
)
foreach(RUST_MBLINK_REL_PATH IN LISTS RUST_MBLINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${RUST_MBLINK_VENDOR_TEMPLATE_ROOT}/mblink/${RUST_MBLINK_REL_PATH}"
        "${RUST_MBLINK_CRATE_DIR}/${RUST_MBLINK_REL_PATH}"
    )
endforeach()

file(GLOB_RECURSE RUST_MBLINK_SYS_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${RUST_MBLINK_SYS_CRATE_DIR}"
    "${RUST_MBLINK_SYS_CRATE_DIR}/src/*.rs"
)

foreach(RUST_MBLINK_SYS_REL_PATH IN ITEMS Cargo.toml build.rs)
    add_embedded_ui_dev_template_file(
        "${RUST_MBLINK_VENDOR_TEMPLATE_ROOT}/mblink-sys/${RUST_MBLINK_SYS_REL_PATH}"
        "${RUST_MBLINK_SYS_CRATE_DIR}/${RUST_MBLINK_SYS_REL_PATH}"
    )
endforeach()
foreach(RUST_MBLINK_SYS_REL_PATH IN LISTS RUST_MBLINK_SYS_SRC_FILES)
add_embedded_ui_dev_template_file(
    "${RUST_MBLINK_VENDOR_TEMPLATE_ROOT}/mblink-sys/${RUST_MBLINK_SYS_REL_PATH}"
    "${RUST_MBLINK_SYS_CRATE_DIR}/${RUST_MBLINK_SYS_REL_PATH}"
)
endforeach()

add_embedded_ui_dev_template_file(
    "${GO_MBLINK_VENDOR_TEMPLATE_ROOT}/go.mod"
    "${GO_MBLINK_DIR}/go.mod"
)
add_embedded_ui_dev_template_file(
    "${GO_MBLINK_VENDOR_TEMPLATE_ROOT}/mblink.lib"
    "${GO_MBLINK_DIR}/mblink.lib"
)
add_embedded_ui_dev_template_file(
    "${GO_MBLINK_VENDOR_TEMPLATE_ROOT}/core/api/mblink.h"
    "${CMAKE_SOURCE_DIR}/core/api/mblink.h"
)
file(GLOB_RECURSE GO_MBLINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${GO_MBLINK_DIR}"
    "${GO_MBLINK_DIR}/mblink/*.go"
)
foreach(GO_MBLINK_REL_PATH IN LISTS GO_MBLINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${GO_MBLINK_VENDOR_TEMPLATE_ROOT}/${GO_MBLINK_REL_PATH}"
        "${GO_MBLINK_DIR}/${GO_MBLINK_REL_PATH}"
    )
endforeach()

file(GLOB_RECURSE PYTHON_MBLINK_SRC_FILES CONFIGURE_DEPENDS
    RELATIVE "${PYTHON_MBLINK_DIR}"
    "${PYTHON_MBLINK_DIR}/*.py"
)
foreach(PYTHON_MBLINK_REL_PATH IN LISTS PYTHON_MBLINK_SRC_FILES)
    add_embedded_ui_dev_template_file(
        "${PYTHON_MBLINK_VENDOR_TEMPLATE_ROOT}/${PYTHON_MBLINK_REL_PATH}"
        "${PYTHON_MBLINK_DIR}/${PYTHON_MBLINK_REL_PATH}"
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
add_dependencies(generate_embedded_ui_dev_templates mblink_api)

add_executable(mblink_ui_dev
    main.cpp
    common.cpp
    common.h
    daemon_ipc.cpp
    daemon_ipc.h
    daemon_server.cpp
    daemon_server.h
)

target_include_directories(mblink_ui_dev PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)
add_dependencies(mblink_ui_dev esm_loader mblink_devtools generate_embedded_ui_dev_templates)

target_link_libraries(mblink_ui_dev PRIVATE
    mblink_resource_package
    nlohmann_json
)

set_target_properties(mblink_ui_dev PROPERTIES
    OUTPUT_NAME "mblink-ui-dev"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
