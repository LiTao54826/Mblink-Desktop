# LightUI 核心库 CMake配置

message(STATUS "Configuring core library...")

# 添加各个模块
add_subdirectory(window)
add_subdirectory(quickjs)
add_subdirectory(dom)
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
add_library(lightui INTERFACE)

# 链接所有子模块
target_link_libraries(lightui INTERFACE
    lightui_window
    lightui_quickjs
    lightui_dom
    lightui_event
    lightui_layout
    lightui_render
    lightui_bridge
    lightui_api
    lightui_utils
    lightui_network
    lightui_devtools
    lightui_compositor
)

# 包含目录
target_include_directories(lightui INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

message(STATUS "Core library configured.")
