# Network 模块 CMakeLists.txt

set(NETWORK_SOURCES
    http_client.cpp
    fetch_bindings.cpp
)

set(NETWORK_HEADERS
    http_client.h
    fetch_bindings.h
)

add_library(mbink_network STATIC
    ${NETWORK_SOURCES}
    ${NETWORK_HEADERS}
)

target_include_directories(mbink_network PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

# 链接 QuickJS 和 Event 模块
target_link_libraries(mbink_network PUBLIC
    quickjs
    mbink_event
)

# Windows specific
if(WIN32)
    target_link_libraries(mbink_network PUBLIC winhttp)
endif()

# Set C++ standard
target_compile_features(mbink_network PUBLIC cxx_std_20)

message(STATUS "Network module configured")

