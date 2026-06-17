# ESM Loader - thin CLI wrapper over mbink.dll / C API

add_executable(esm_loader
    main_c_api.cpp
)

add_dependencies(esm_loader mbink_api mbink_devtools)

target_include_directories(esm_loader PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(esm_loader PRIVATE
    mbink_api
)

set_target_properties(esm_loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
