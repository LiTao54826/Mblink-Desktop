# CMake 脚本：将 UPX.exe 转换为 C++ 字节数组
# 用法: cmake -DINPUT_FILE=upx.exe -DOUTPUT_FILE=upx_exe.inc -P generate_embedded_upx.cmake

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE not defined")
endif()

if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not defined")
endif()

# 检查输入文件是否存在
if(NOT EXISTS "${INPUT_FILE}")
    message(WARNING "UPX file not found: ${INPUT_FILE}")
    # 生成空的占位文件
    file(WRITE "${OUTPUT_FILE}" 
"// UPX not available - place upx.exe in tools/app_bundler/ to enable compression
static const unsigned char upx_exe_data[] = {};
static const size_t upx_exe_size = 0;
")
    return()
endif()

# 读取二进制文件
file(READ "${INPUT_FILE}" FILE_CONTENT HEX)

# 获取文件大小
file(SIZE "${INPUT_FILE}" FILE_SIZE)

# 将十六进制字符串转换为 C 数组格式
string(LENGTH "${FILE_CONTENT}" HEX_LENGTH)
set(OUTPUT_CONTENT "// Auto-generated from ${INPUT_FILE}\n")
string(APPEND OUTPUT_CONTENT "// Size: ${FILE_SIZE} bytes\n\n")
string(APPEND OUTPUT_CONTENT "static const unsigned char upx_exe_data[] = {\n    ")

set(BYTE_COUNT 0)
set(LINE_BYTES 0)

math(EXPR TOTAL_BYTES "${HEX_LENGTH} / 2")

while(BYTE_COUNT LESS TOTAL_BYTES)
    math(EXPR HEX_POS "${BYTE_COUNT} * 2")
    string(SUBSTRING "${FILE_CONTENT}" ${HEX_POS} 2 HEX_BYTE)
    
    string(APPEND OUTPUT_CONTENT "0x${HEX_BYTE}")
    
    math(EXPR BYTE_COUNT "${BYTE_COUNT} + 1")
    
    if(BYTE_COUNT LESS TOTAL_BYTES)
        string(APPEND OUTPUT_CONTENT ",")
    endif()
    
    math(EXPR LINE_BYTES "${LINE_BYTES} + 1")
    
    if(LINE_BYTES EQUAL 16 AND BYTE_COUNT LESS TOTAL_BYTES)
        string(APPEND OUTPUT_CONTENT "\n    ")
        set(LINE_BYTES 0)
    endif()
endwhile()

string(APPEND OUTPUT_CONTENT "\n};\n\n")
string(APPEND OUTPUT_CONTENT "static const size_t upx_exe_size = ${FILE_SIZE};\n")

# 写入输出文件
file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")

message(STATUS "Generated ${OUTPUT_FILE} from ${INPUT_FILE} (${FILE_SIZE} bytes)")
