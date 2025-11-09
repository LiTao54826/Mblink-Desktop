# QuickJS模块 CMakeLists.txt

set(QUICKJS_RUNTIME_SOURCES
    quickjs_runtime.cpp
)

set(QUICKJS_RUNTIME_HEADERS
    quickjs_runtime.h
)

add_library(lightui_quickjs STATIC ${QUICKJS_RUNTIME_SOURCES})

target_include_directories(lightui_quickjs PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接QuickJS库
if(TARGET quickjs)
    target_link_libraries(lightui_quickjs PUBLIC quickjs)
else()
    message(WARNING "QuickJS not found, quickjs module may not build correctly")
endif()

# 链接nlohmann/json库
if(TARGET nlohmann_json)
    target_link_libraries(lightui_quickjs PUBLIC nlohmann_json)
else()
    message(WARNING "nlohmann/json not found, quickjs module may not build correctly")
endif()

# 链接utils模块（用于JSON转换）
target_link_libraries(lightui_quickjs PUBLIC lightui_utils)
