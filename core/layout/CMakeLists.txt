# Layout模块 CMakeLists.txt

set(LAYOUT_SOURCES
    layout_engine.cpp
    style_parser.cpp
)

set(LAYOUT_HEADERS
    layout_engine.h
    style_parser.h
)

add_library(lightui_layout STATIC ${LAYOUT_SOURCES})

target_include_directories(lightui_layout PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(lightui_layout PUBLIC
    lightui_utils
)

# 链接Yoga（如果可用）
if(TARGET yogacore)
    target_link_libraries(lightui_layout PUBLIC yogacore)
elseif(TARGET yoga)
    target_link_libraries(lightui_layout PUBLIC yoga)
else()
    message(WARNING "Yoga not found, layout module may not build correctly")
endif()
