set(DEVTOOLS_CORE_SOURCES
    devtools_manager.cpp
    devtools_panel.cpp
    devtools_state.cpp
    inspector/dom_tree_view.cpp
    inspector/dom_tree_node.cpp
    inspector/element_highlighter.cpp
    inspector/element_picker.cpp
    inspector/attributes_view.cpp
    styles/styles_panel.cpp
    styles/inline_styles_view.cpp
    styles/computed_styles_view.cpp
    styles/box_model_view.cpp
    editor/style_editor.cpp
    editor/attribute_editor.cpp
    search/element_search.cpp
    serializer/dom_serializer.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/ui_dev_snapshot.cpp
    ${CMAKE_SOURCE_DIR}/tools/esm_loader/ui_dev_control.cpp
)

set(DEVTOOLS_CORE_HEADERS
    devtools_manager.h
    devtools_panel.h
    devtools_state.h
    inspector/dom_tree_view.h
    inspector/dom_tree_node.h
    inspector/element_highlighter.h
    inspector/element_picker.h
    inspector/attributes_view.h
    styles/styles_panel.h
    styles/inline_styles_view.h
    styles/computed_styles_view.h
    styles/box_model_view.h
    editor/style_editor.h
    editor/attribute_editor.h
    search/element_search.h
    serializer/dom_serializer.h
)

add_library(mbink_devtools_core STATIC
    ${DEVTOOLS_CORE_SOURCES}
    ${DEVTOOLS_CORE_HEADERS}
)

target_include_directories(mbink_devtools_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/tools/esm_loader
    ${CMAKE_SOURCE_DIR}/third_party/quickjs
    ${CMAKE_SOURCE_DIR}/third_party/SDL/include
)

target_link_libraries(mbink_devtools_core PUBLIC
    mbink_dom
    mbink_render
    mbink_event
    mbink_bridge
    mbink_utils
    mbink_network
    mbink_lexbor
    quickjs
    nlohmann_json
)

if(TARGET skia)
    target_link_libraries(mbink_devtools_core PUBLIC skia)
endif()

if(MSVC)
    target_compile_options(mbink_devtools_core PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mbink_devtools_core PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

add_library(mbink_devtools SHARED devtools_entry.cpp)

target_include_directories(mbink_devtools PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(mbink_devtools PRIVATE
    nlohmann_json
)

if(WIN32)
    target_link_libraries(mbink_devtools PRIVATE ws2_32 bcrypt)
endif()

if(MSVC)
    target_compile_options(mbink_devtools PRIVATE
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mbink_devtools PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

set_target_properties(mbink_devtools PROPERTIES
    OUTPUT_NAME "mbink_devtools"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
