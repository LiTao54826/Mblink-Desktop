# Utils模块 CMakeLists.txt

set(UTILS_SOURCES
    logger.cpp
    json.cpp
    encoding_utils.cpp
    background_task_runner.cpp
)

set(UTILS_HEADERS
    logger.h
    json.h
    encoding_utils.h
    background_task_runner.h
    async_resource_context.h
)

add_library(mbink_utils STATIC ${UTILS_SOURCES})

target_include_directories(mbink_utils PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# Utils模块是基础模块，不依赖其他模块
