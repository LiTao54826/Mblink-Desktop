if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not defined")
endif()

set(JS_FILE_LIST "")
if(DEFINED INPUT_LIST_FILE AND EXISTS "${INPUT_LIST_FILE}")
    file(READ "${INPUT_LIST_FILE}" JS_FILES_RAW)
    string(REPLACE "\r\n" ";" JS_FILE_LIST "${JS_FILES_RAW}")
    string(REPLACE "\n" ";" JS_FILE_LIST "${JS_FILE_LIST}")
    list(FILTER JS_FILE_LIST EXCLUDE REGEX "^$")
elseif(DEFINED JS_FILES AND NOT JS_FILES STREQUAL "")
    string(REPLACE "|" ";" JS_FILE_LIST "${JS_FILES}")
endif()

set(OUTPUT_CONTENT "// Auto-generated file - DO NOT EDIT\n")

if(JS_FILE_LIST)
    foreach(JS_REL_PATH IN LISTS JS_FILE_LIST)
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
        string(APPEND OUTPUT_CONTENT "#include \"generated/${JS_VAR_NAME}.inc\"\n")
    endforeach()

    string(APPEND OUTPUT_CONTENT "\n")
    string(APPEND OUTPUT_CONTENT "static const EmbeddedJSEntry kEmbeddedJSEntries[] = {\n")

    foreach(JS_REL_PATH IN LISTS JS_FILE_LIST)
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" JS_VAR_NAME "${JS_REL_PATH}")
        string(APPEND OUTPUT_CONTENT
            "    {\"${JS_REL_PATH}\", ${JS_VAR_NAME}_data, ${JS_VAR_NAME}_size},\n"
        )
    endforeach()

    string(APPEND OUTPUT_CONTENT "};\n")
else()
    string(APPEND OUTPUT_CONTENT "static const EmbeddedJSEntry kEmbeddedJSEntries[] = {};\n")
endif()

file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")
message(STATUS "Generated ${OUTPUT_FILE}")

