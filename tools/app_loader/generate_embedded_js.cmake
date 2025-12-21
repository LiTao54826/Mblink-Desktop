# generate_embedded_js.cmake
# 在构建时将 JS 文件转换为 C++ 字节数组
#
# 用法: cmake -DINPUT_FILE=xxx -DOUTPUT_FILE=xxx -DVAR_NAME=xxx -P generate_embedded_js.cmake

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE not defined")
endif()

if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not defined")
endif()

if(NOT DEFINED VAR_NAME)
    message(FATAL_ERROR "VAR_NAME not defined")
endif()

# 读取输入文件为十六进制
file(READ "${INPUT_FILE}" FILE_CONTENT HEX)

# 获取文件大小（十六进制字符串长度 / 2）
string(LENGTH "${FILE_CONTENT}" HEX_LENGTH)
math(EXPR FILE_SIZE "${HEX_LENGTH} / 2")

# 将十六进制转换为 0xNN 格式的字节数组
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," HEX_ARRAY "${FILE_CONTENT}")

# 每 16 个字节换行，提高可读性
string(REGEX REPLACE "(0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],)" "\\1\n" HEX_ARRAY "${HEX_ARRAY}")

# 生成 C++ 源码
set(OUTPUT_CONTENT "// Auto-generated file - DO NOT EDIT
// Generated from: ${INPUT_FILE}
// Size: ${FILE_SIZE} bytes

namespace {
const unsigned char ${VAR_NAME}_data[] = {
${HEX_ARRAY}
};
const size_t ${VAR_NAME}_size = ${FILE_SIZE};
}
")

# 写入输出文件
file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")

message(STATUS "Generated ${OUTPUT_FILE} from ${INPUT_FILE} (${FILE_SIZE} bytes)")
