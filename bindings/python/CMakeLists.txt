# MBink Python Binding CMakeLists.txt
#
# 使用 pybind11 构建 Python 扩展模块
# 输出: mbink_core.pyd (Windows) / mbink_core.so (Linux/macOS)

cmake_minimum_required(VERSION 3.15)

# 查找 Python - 优先使用 venv 中的 Python
# 检查是否在 venv 中
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe" CACHE FILEPATH "Python executable")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python" CACHE FILEPATH "Python executable")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# 从 Python 包获取 pybind11 cmake 目录
execute_process(
    COMMAND ${Python3_EXECUTABLE} -m pybind11 --cmakedir
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE pybind11_result
)

if(pybind11_result EQUAL 0 AND pybind11_DIR)
    find_package(pybind11 CONFIG REQUIRED PATHS ${pybind11_DIR} NO_DEFAULT_PATH)
else()
    # 尝试系统安装的 pybind11
    find_package(pybind11 CONFIG QUIET)
    if(NOT pybind11_FOUND)
        message(FATAL_ERROR "pybind11 not found. Install with: pip install pybind11")
    endif()
endif()

message(STATUS "Python: ${Python3_EXECUTABLE}")
message(STATUS "Python version: ${Python3_VERSION}")
message(STATUS "pybind11 version: ${pybind11_VERSION}")

# 创建 Python 模块
pybind11_add_module(mbink_core
    src/bindings.cpp
)

# 包含目录
target_include_directories(mbink_core PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

# 链接 mbink 核心库
# Phase 1: 仅 bridge（状态管理）
# Phase 2: 完整库（Window, EventLoop, Document 等）
target_link_libraries(mbink_core PRIVATE
    mbink_bridge
    mbink_window
    mbink_event
    mbink_dom
    mbink_quickjs
    mbink_devtools
    mbink_network
)

# 设置输出目录 - 直接输出到 bindings/python，便于 import mbink_core
# 运行时 DLL 仍由 core/api 复制到 mbink/bin
set_target_properties(mbink_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bindings/python
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bindings/python
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bindings/python
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bindings/python
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bindings/python
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bindings/python
)

# Windows 特定设置
if(WIN32)
    # 确保输出为 .pyd
    set_target_properties(mbink_core PROPERTIES
        SUFFIX ".pyd"
    )
    
    # 优化体积：启用 LTCG 和去除调试信息
    if(MSVC)
        target_compile_options(mbink_core PRIVATE
            /O2           # 最大优化
            /GL           # 全程序优化
            /Gy           # 函数级链接
            /GS-          # 禁用安全检查（减小体积）
        )
        target_link_options(mbink_core PRIVATE
            /LTCG         # 链接时代码生成
            /OPT:REF      # 移除未引用函数
            /OPT:ICF      # 相同 COMDAT 折叠
            /DEBUG:NONE   # 不生成 PDB 文件
        )
    endif()
else()
    # Linux/macOS 优化
    target_compile_options(mbink_core PRIVATE
        -O3                    # 最大优化
        -fvisibility=hidden    # 隐藏符号
        -ffunction-sections    # 函数分段
        -fdata-sections        # 数据分段
    )
    target_link_options(mbink_core PRIVATE
        -Wl,--gc-sections      # 移除未使用段
        -s                     # 去除符号表
    )
endif()

# 安装规则
install(TARGETS mbink_core
    LIBRARY DESTINATION python
    RUNTIME DESTINATION python
)
