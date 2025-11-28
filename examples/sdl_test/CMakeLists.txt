# SDL 纯测试程序
add_executable(sdl_test main.cpp)

target_link_libraries(sdl_test PRIVATE
    SDL3::SDL3-static
)

# 复制资源文件
add_custom_command(TARGET sdl_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "SDL test built"
    COMMENT "Building SDL test"
)

