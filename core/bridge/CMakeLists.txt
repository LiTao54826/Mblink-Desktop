# Bridge模块 CMakeLists.txt
# 跨语言绑定：StateManager + HostBridge

set(BRIDGE_SOURCES
    bridge.cpp
    native_state_graph.cpp
    state_manager.cpp
    host_bridge.cpp
)

set(BRIDGE_HEADERS
    bridge.h
    native_state_graph.h
    state_manager.h
    host_bridge.h
)

add_library(mblink_bridge STATIC ${BRIDGE_SOURCES})

target_include_directories(mblink_bridge PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(mblink_bridge PUBLIC
    mblink_utils
    mblink_quickjs
    nlohmann_json
)
