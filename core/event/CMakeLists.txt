# Event Loop Module

set(EVENT_SOURCES
    event_loop.cpp
    frame_controller.cpp
    input_handler.cpp
    task_scheduler.cpp
    mouse_event.cpp
    keyboard_event.cpp
    hit_testing.cpp
    event.cpp
    event_system.cpp
    event_types.cpp
    focus_manager.cpp
)

set(EVENT_HEADERS
    event_loop.h
    frame_controller.h
    input_handler.h
    task_scheduler.h
    mouse_event.h
    keyboard_event.h
    hit_testing.h
    event.h
    event_system.h
    event_types.h
    focus_manager.h
)

add_library(lightui_event STATIC ${EVENT_SOURCES})

target_include_directories(lightui_event PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接 SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(lightui_event PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(lightui_event PUBLIC SDL3-static)
endif()

# 链接窗口模块
target_link_libraries(lightui_event PUBLIC lightui_window)

# MSVC 特定设置
if(MSVC)
    # 匹配 Skia 的运行时库设置
    target_compile_options(lightui_event PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(lightui_event PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

# 设置编译选项
if(MSVC)
    target_compile_options(lightui_event PRIVATE /W4)
else()
    target_compile_options(lightui_event PRIVATE -Wall -Wextra)
endif()
