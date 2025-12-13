# Network 模块 CMakeLists.txt

set(NETWORK_SOURCES
    http_client.cpp
    fetch_bindings.cpp
)

set(NETWORK_HEADERS
    http_client.h
    fetch_bindings.h
)

add_library(lightui_network STATIC
    ${NETWORK_SOURCES}
    ${NETWORK_HEADERS}
)

target_include_directories(lightui_network PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

# 链接 QuickJS 和 Event 模块
target_link_libraries(lightui_network PUBLIC
    quickjs
    lightui_event
)

# Windows specific
if(WIN32)
    target_link_libraries(lightui_network PUBLIC winhttp)
endif()

# Set C++ standard
target_compile_features(lightui_network PUBLIC cxx_std_20)

message(STATUS "Network module configured")

