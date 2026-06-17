# MBink 核心库 CMake配置

message(STATUS "Configuring core library...")

# 添加各个模块
add_library(mbink_devtools_bridge STATIC
    devtools/devtools_bridge.cpp
)

target_include_directories(mbink_devtools_bridge PUBLIC
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
add_library(mbink INTERFACE)

# 链接所有子模块
target_link_libraries(mbink INTERFACE
    mbink_window
    mbink_quickjs
    mbink_dom
    mbink_editing
    mbink_event
    mbink_layout
    mbink_render
    mbink_bridge
    mbink_api
    mbink_utils
    mbink_network
    mbink_devtools_bridge
    mbink_compositor
)

# 包含目录
target_include_directories(mbink INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

message(STATUS "Core library configured.")
