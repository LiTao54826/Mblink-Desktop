# Bridge模块 CMakeLists.txt
# 跨语言绑定：StateManager + HostBridge

set(BRIDGE_SOURCES
    bridge.cpp
    state_manager.cpp
    host_bridge.cpp
)

set(BRIDGE_HEADERS
    bridge.h
    state_manager.h
    host_bridge.h
)

add_library(mbink_bridge STATIC ${BRIDGE_SOURCES})

target_include_directories(mbink_bridge PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(mbink_bridge PUBLIC
    mbink_utils
    mbink_quickjs
    nlohmann_json
)
