# DOM模块 CMakeLists.txt

set(DOM_SOURCES
    node.cpp
    element.cpp
    text.cpp
    document.cpp
    event.cpp
    dom_bindings.cpp
    dom_observer.cpp
    selector_engine.cpp
)

set(DOM_HEADERS
    node.h
    element.h
    text.h
    document.h
    event.h
    dom_bindings.h
    dom_observer.h
    selector_engine.h
)

add_library(lightui_dom STATIC ${DOM_SOURCES})

target_include_directories(lightui_dom PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/lexbor/source
)

target_link_libraries(lightui_dom PUBLIC
    lightui_utils
    lexbor_static
)

# 匹配 Skia 的运行时库设置
if(MSVC)
    target_compile_options(lightui_dom PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(lightui_dom PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()
