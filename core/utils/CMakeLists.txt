# Utils模块 CMakeLists.txt

set(UTILS_SOURCES
    logger.cpp
    json.cpp
    encoding_utils.cpp
)

set(UTILS_HEADERS
    logger.h
    json.h
    encoding_utils.h
)

add_library(lightui_utils STATIC ${UTILS_SOURCES})

target_include_directories(lightui_utils PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# Utils模块是基础模块，不依赖其他模块
