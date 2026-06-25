# MBlink App Bundler - 将 JS 应用打包成独立 exe

# 创建生成目录
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# UPX 下载和嵌入支持
set(UPX_VERSION "4.2.4")
set(UPX_URL "https://github.com/upx/upx/releases/download/v${UPX_VERSION}/upx-${UPX_VERSION}-win64.zip")
set(UPX_ZIP "${CMAKE_CURRENT_BINARY_DIR}/upx-${UPX_VERSION}-win64.zip")
set(UPX_DIR "${CMAKE_CURRENT_BINARY_DIR}/upx-${UPX_VERSION}-win64")
set(UPX_EXE "${UPX_DIR}/upx.exe")
set(UPX_INC "${GENERATED_DIR}/upx_exe.inc")
set(GENERATE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_binary.py")

# 下载 UPX（如果不存在）
if(NOT EXISTS ${UPX_EXE})
    message(STATUS "Downloading UPX ${UPX_VERSION}...")
    file(DOWNLOAD ${UPX_URL} ${UPX_ZIP}
        STATUS DOWNLOAD_STATUS
        SHOW_PROGRESS
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(STATUS_CODE EQUAL 0)
        message(STATUS "Extracting UPX...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${UPX_ZIP}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        message(STATUS "UPX downloaded and extracted to ${UPX_DIR}")
    else()
        message(WARNING "Failed to download UPX, compression will not be available")
    endif()
endif()

# 查找 Python
find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_FOUND)
    add_custom_command(
        OUTPUT ${UPX_INC}
        COMMAND ${Python3_EXECUTABLE} ${GENERATE_SCRIPT} ${UPX_EXE} ${UPX_INC} upx_exe
        DEPENDS ${GENERATE_SCRIPT}
        COMMENT "Embedding UPX"
    )
else()
    add_custom_command(
        OUTPUT ${UPX_INC}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${UPX_EXE}
            -DOUTPUT_FILE=${UPX_INC}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_upx.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded_upx.cmake
        COMMENT "Embedding UPX"
    )
endif()

add_custom_target(generate_embedded_upx DEPENDS ${UPX_INC})

add_executable(app_bundler
    main.cpp
    payload.cpp
    payload.h
    module_resolver.cpp
    module_resolver.h
    bytecode_compiler.cpp
    bytecode_compiler.h
    exe_writer.cpp
    exe_writer.h
    upx_compressor.cpp
    upx_compressor.h
)

add_dependencies(app_bundler generate_embedded_upx)

target_link_libraries(app_bundler
    mblink_quickjs
)

# 添加包含目录
target_include_directories(app_bundler PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 输出到 bin/ 目录
set_target_properties(app_bundler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
