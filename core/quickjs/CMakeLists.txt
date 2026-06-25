# QuickJS模块 CMakeLists.txt

set(QUICKJS_RUNTIME_SOURCES
    quickjs_runtime.cpp
    ${CMAKE_SOURCE_DIR}/tools/app_bundler/bytecode_compiler.cpp
    window_bindings.cpp
    document_bindings_impl.cpp
    dom_binding_map.cpp
    bindings/js_node.cpp
    bindings/js_element.cpp
    bindings/js_style_declaration.cpp
    bindings/js_event.cpp
    bindings/js_data_transfer.cpp
    bindings/js_file_list.cpp
    bindings/js_range.cpp
    bindings/js_selection.cpp
    bindings/js_mutation_observer.cpp
)

set(QUICKJS_RUNTIME_HEADERS
    quickjs_runtime.h
    window_bindings.h
    js_value_wrapper.h
    dom_binding_map.h
    bindings/js_node.h
    bindings/js_element.h
    bindings/js_style_declaration.h
    bindings/js_event.h
    bindings/js_data_transfer.h
    bindings/js_file_list.h
    bindings/js_range.h
    bindings/js_selection.h
    bindings/js_mutation_observer.h
)

add_library(mblink_quickjs STATIC ${QUICKJS_RUNTIME_SOURCES})

target_include_directories(mblink_quickjs PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接QuickJS库
if(TARGET quickjs)
    target_link_libraries(mblink_quickjs PUBLIC quickjs)
else()
    message(WARNING "QuickJS not found, quickjs module may not build correctly")
endif()

# 链接nlohmann/json库
if(TARGET nlohmann_json)
    target_link_libraries(mblink_quickjs PUBLIC nlohmann_json)
else()
    message(WARNING "nlohmann/json not found, quickjs module may not build correctly")
endif()

# 链接utils模块（用于JSON转换）
target_link_libraries(mblink_quickjs PUBLIC mblink_utils)

# 链接window和dom模块（用于绑定）
target_link_libraries(mblink_quickjs PUBLIC mblink_window mblink_dom mblink_event)
