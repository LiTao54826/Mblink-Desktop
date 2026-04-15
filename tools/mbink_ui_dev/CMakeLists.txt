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
    ${CMAKE_SOURCE_DIR}/third_party
)
add_dependencies(mbink_ui_dev esm_loader)


target_link_libraries(mbink_ui_dev PRIVATE
    nlohmann_json
)

set_target_properties(mbink_ui_dev PROPERTIES
    OUTPUT_NAME "mbink-ui-dev"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
