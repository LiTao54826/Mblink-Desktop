# DOM Observers 子模块 CMakeLists.txt

set(DOM_OBSERVERS_SOURCES
    dom_observer.cpp
    mutation_observer.cpp
    dirty_node_tracker.cpp
)

set(DOM_OBSERVERS_HEADERS
    dom_observer.h
    mutation_observer.h
    dirty_node_tracker.h
)

# 将源文件列表传递给父目录
set(DOM_OBSERVERS_SOURCES ${DOM_OBSERVERS_SOURCES} PARENT_SCOPE)
set(DOM_OBSERVERS_HEADERS ${DOM_OBSERVERS_HEADERS} PARENT_SCOPE)
