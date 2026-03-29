# Editing Subsystem Module
# 文本编辑子系统，包含选择、剪贴板、contentEditable 等功能

set(EDITING_SOURCES
    selection_manager.cpp
    clipboard_manager.cpp
    editor_input_session.cpp
    contenteditable_handler.cpp
    contenteditable_controller.cpp
    contenteditable_geometry.cpp
    input_edit_state.cpp
    input_edit_command.cpp
    input_editing_controller.cpp
    textarea_edit_state.cpp
    textarea_editing_controller.cpp
    drag_manager.cpp
)

set(EDITING_HEADERS
    selection_manager.h
    editor_input_session.h
    input_editing_controller.h
    textarea_edit_state.h
    textarea_editing_controller.h
    input_edit_state.h
    input_edit_command.h
    clipboard_manager.h
    contenteditable_handler.h
    contenteditable_controller.h
    contenteditable_geometry.h
    drag_manager.h
)

add_library(mbink_editing STATIC ${EDITING_SOURCES})

target_include_directories(mbink_editing PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# 链接 SDL3
if(TARGET SDL3::SDL3)
    target_link_libraries(mbink_editing PUBLIC SDL3::SDL3)
elseif(TARGET SDL3-static)
    target_link_libraries(mbink_editing PUBLIC SDL3-static)
endif()

# 链接依赖模块
target_link_libraries(mbink_editing PUBLIC mbink_dom)

# MSVC 特定设置
if(MSVC)
    target_compile_options(mbink_editing PUBLIC
        $<$<CONFIG:Debug>:/MT>
        $<$<CONFIG:Release>:/MT>
    )
    target_compile_definitions(mbink_editing PUBLIC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

# 设置编译选项
if(MSVC)
    target_compile_options(mbink_editing PRIVATE /W4)
else()
    target_compile_options(mbink_editing PRIVATE -Wall -Wextra)
endif()
