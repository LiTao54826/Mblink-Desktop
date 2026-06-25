# MBlink 核心库 CMake配置

message(STATUS "Configuring core library...")

# 添加各个模块
add_library(mblink_devtools_bridge STATIC
    devtools/devtools_bridge.cpp
)

target_include_directories(mblink_devtools_bridge PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

add_subdirectory(window)
add_subdirectory(quickjs)
add_subdirectory(dom)
add_subdirectory(editing)
add_subdirectory(event)
add_subdirectory(layout)
add_subdirectory(render)
add_subdirectory(bridge)
add_subdirectory(api)
add_subdirectory(utils)
add_subdirectory(lexbor)
add_subdirectory(network)
add_subdirectory(devtools)
add_subdirectory(compositor)

# 创建主库 (使用INTERFACE库，因为所有实现都在子模块中)
add_library(mblink INTERFACE)

# 链接所有子模块
target_link_libraries(mblink INTERFACE
    mblink_window
    mblink_quickjs
    mblink_dom
    mblink_editing
    mblink_event
    mblink_layout
    mblink_render
    mblink_bridge
    mblink_api
    mblink_utils
    mblink_network
    mblink_devtools_bridge
    mblink_compositor
)

# 包含目录
target_include_directories(mblink INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

message(STATUS "Core library configured.")
