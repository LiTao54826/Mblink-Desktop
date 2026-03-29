# Event Loop Module

set(EVENT_SOURCES
    # loop/ - 事件循环核心
    loop/event_loop.cpp
    loop/frame_controller.cpp
    loop/task_scheduler.cpp
    # types/ - 事件类型定义
    types/event.cpp
    types/event_types.cpp
    types/mouse_event.cpp
    types/keyboard_event.cpp
    types/data_transfer.cpp
    # input/ - 输入处理
    input/input_handler.cpp
    input/keyboard_utils.cpp
    input/hit_test_controller.cpp
    input/focus_manager.cpp
    # 根目录
    event_system.cpp
    # dispatch/ - 事件分发器
    dispatch/mouse_event_dispatcher.cpp
    dispatch/keyboard_event_dispatcher.cpp
    dispatch/wheel_event_dispatcher.cpp
)

set(EVENT_HEADERS
    # loop/ - 事件循环核心
    loop/event_loop.h
    loop/frame_controller.h
    loop/task_scheduler.h
    # types/ - 事件类型定义
    types/event.h
    types/event_types.h
    types/mouse_event.h
    types/keyboard_event.h
    types/data_transfer.h
    # input/ - 输入处理
    input/input_handler.h
    input/keyboard_utils.h
    input/hit_test_controller.h
    input/focus_manager.h
    # 根目录
    event_system.h
    # dispatch/ - 事件分发器
    dispatch/mouse_event_dispatcher.h
    dispatch/keyboard_event_dispatcher.h
    dispatch/wheel_event_dispatcher.h
)

add_library(mbink_event STATIC ${EVENT_SOURCES})

target_include_directories(mbink_event PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接 SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(mbink_event PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(mbink_event PUBLIC SDL3-static)
endif()

# 链接窗口模块
target_link_libraries(mbink_event PUBLIC mbink_window)

# 链接编辑模块（用于 DragManager, SelectionManager 等）
target_link_libraries(mbink_event PUBLIC mbink_editing)

# MSVC 特定设置
if(MSVC)
    # 匹配 Skia 的运行时库设置
    target_compile_options(mbink_event PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mbink_event PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

# 设置编译选项
if(MSVC)
    target_compile_options(mbink_event PRIVATE /W4)
else()
    target_compile_options(mbink_event PRIVATE -Wall -Wextra)
endif()
