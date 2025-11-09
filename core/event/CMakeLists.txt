# Event模块 CMakeLists.txt

set(EVENT_SOURCES
    event.cpp
    mouse_event.cpp
    keyboard_event.cpp
    event_system.cpp
)

set(EVENT_HEADERS
    event.h
    mouse_event.h
    keyboard_event.h
    event_system.h
)

add_library(lightui_event STATIC ${EVENT_SOURCES})

target_include_directories(lightui_event PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_event PUBLIC
    lightui_utils
)
