# DOM模块 CMakeLists.txt

set(DOM_SOURCES
    node.cpp
    element.cpp
    text.cpp
    document.cpp
    dom_bindings.cpp
)

set(DOM_HEADERS
    node.h
    element.h
    text.h
    document.h
    dom_bindings.h
)

add_library(lightui_dom STATIC ${DOM_SOURCES})

target_include_directories(lightui_dom PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_dom PUBLIC
    lightui_utils
)
