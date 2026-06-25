if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE not defined")
endif()

if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not defined")
endif()

if(NOT DEFINED VAR_NAME)
    message(FATAL_ERROR "VAR_NAME not defined")
endif()

file(READ "${INPUT_FILE}" FILE_CONTENT HEX)
string(LENGTH "${FILE_CONTENT}" HEX_LENGTH)
math(EXPR FILE_SIZE "${HEX_LENGTH} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," HEX_ARRAY "${FILE_CONTENT}")
string(REGEX REPLACE "(0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],0x[0-9a-f][0-9a-f],)" "\\1\n" HEX_ARRAY "${HEX_ARRAY}")

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

file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")
message(STATUS "Generated ${OUTPUT_FILE} from ${INPUT_FILE} (${FILE_SIZE} bytes)")
