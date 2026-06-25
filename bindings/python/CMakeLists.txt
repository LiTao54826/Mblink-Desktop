# MBlink Python Binding CMakeLists.txt
#
# 当前 Python 绑定仅保留 ctypes + C ABI 路径。
# 运行时动态库由 core/api/mblink_api 构建并复制到 bindings/python/mblink/bin。

cmake_minimum_required(VERSION 3.15)

message(STATUS "Python binding uses ctypes + C ABI only; no pybind11 module is built.")
