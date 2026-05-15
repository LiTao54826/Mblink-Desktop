if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not defined")
endif()

set(TEMPLATE_FILE_LIST "")
if(DEFINED INPUT_LIST_FILE AND EXISTS "${INPUT_LIST_FILE}")
    file(READ "${INPUT_LIST_FILE}" TEMPLATE_FILES_RAW)
    string(REPLACE "\r\n" ";" TEMPLATE_FILE_LIST "${TEMPLATE_FILES_RAW}")
    string(REPLACE "\n" ";" TEMPLATE_FILE_LIST "${TEMPLATE_FILE_LIST}")
    list(FILTER TEMPLATE_FILE_LIST EXCLUDE REGEX "^$")
endif()

set(OUTPUT_CONTENT "// Auto-generated file - DO NOT EDIT\n")

if(TEMPLATE_FILE_LIST)
    foreach(TEMPLATE_REL_PATH IN LISTS TEMPLATE_FILE_LIST)
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" TEMPLATE_VAR_NAME "${TEMPLATE_REL_PATH}")
        string(APPEND OUTPUT_CONTENT "#include \"generated_templates/${TEMPLATE_VAR_NAME}.inc\"\n")
    endforeach()

    string(APPEND OUTPUT_CONTENT "\nstatic const EmbeddedTemplateEntry kEmbeddedTemplateEntries[] = {\n")
    foreach(TEMPLATE_REL_PATH IN LISTS TEMPLATE_FILE_LIST)
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" TEMPLATE_VAR_NAME "${TEMPLATE_REL_PATH}")
        string(APPEND OUTPUT_CONTENT
            "    {\"${TEMPLATE_REL_PATH}\", ${TEMPLATE_VAR_NAME}_data, ${TEMPLATE_VAR_NAME}_size},\n"
        )
    endforeach()
    string(APPEND OUTPUT_CONTENT "};\n")
else()
    string(APPEND OUTPUT_CONTENT "static const EmbeddedTemplateEntry kEmbeddedTemplateEntries[] = {{nullptr, nullptr, 0}};\n")
endif()

file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")
message(STATUS "Generated ${OUTPUT_FILE}")
