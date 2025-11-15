# Taffy CSS Layout Engine
# Precompiled library integration

# Create imported library target
add_library(taffy STATIC IMPORTED GLOBAL)

# Set library location based on platform
if(WIN32)
    set(TAFFY_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/taffy.lib")
    message(STATUS "Taffy: Using Windows library: ${TAFFY_LIB_PATH}")
elseif(APPLE)
    set(TAFFY_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libtaffy.a")
    message(STATUS "Taffy: Using macOS library: ${TAFFY_LIB_PATH}")
else()
    set(TAFFY_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libtaffy.a")
    message(STATUS "Taffy: Using Linux library: ${TAFFY_LIB_PATH}")
endif()

# Check if library exists
if(EXISTS "${TAFFY_LIB_PATH}")
    set_target_properties(taffy PROPERTIES
        IMPORTED_LOCATION "${TAFFY_LIB_PATH}"
    )
    message(STATUS "Taffy: Library found and configured")
else()
    message(WARNING "Taffy: Library not found at ${TAFFY_LIB_PATH}")
    message(WARNING "Taffy: Please build the library first (see README.md)")
endif()

# Set include directories
target_include_directories(taffy INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Export variables for parent scope
set(TAFFY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
set(TAFFY_FOUND TRUE PARENT_SCOPE)

