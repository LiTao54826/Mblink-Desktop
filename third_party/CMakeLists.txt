# LightUI 第三方库 CMake配置
#
# 功能：
# - 管理第三方依赖库
# - 配置QuickJS, Skia, SDL3, Yoga, nlohmann/json

message(STATUS "Configuring third-party libraries...")

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================
# nlohmann/json (header-only)
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/nlohmann/json.hpp")
    message(STATUS "  Found nlohmann/json")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "${THIRD_PARTY_DIR}")
    set(NLOHMANN_JSON_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  nlohmann/json not found")
    set(NLOHMANN_JSON_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# SDL3
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/SDL3/CMakeLists.txt")
    message(STATUS "  Found SDL3")
    set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as static library")
    set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library")
    add_subdirectory(SDL3)
    set(SDL3_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  SDL3 not found. Please run scripts/download_deps.sh")
    set(SDL3_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# QuickJS-ng
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/quickjs/CMakeLists.txt")
    message(STATUS "  Found QuickJS-ng")

    # 使用 quickjs-ng 自带的 CMakeLists.txt
    # 禁用不需要的构建选项
    set(QJS_BUILD_EXAMPLES OFF CACHE BOOL "Build QuickJS examples" FORCE)
    set(QJS_BUILD_CLI_STATIC OFF CACHE BOOL "Build static qjs executable" FORCE)
    set(QJS_BUILD_LIBC ON CACHE BOOL "Build QuickJS with libc support" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build QuickJS as static library" FORCE)

    # 添加 quickjs-ng 子目录
    add_subdirectory(quickjs)

    # quickjs-ng 创建的库名为 "qjs"，我们需要创建一个别名 "quickjs" 以保持兼容性
    if(TARGET qjs)
        add_library(quickjs ALIAS qjs)
        message(STATUS "  QuickJS-ng library target: qjs (alias: quickjs)")
    else()
        message(WARNING "  QuickJS-ng target 'qjs' not found")
    endif()

    set(QUICKJS_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  QuickJS-ng not found. Please clone quickjs-ng to third_party/quickjs")
    set(QUICKJS_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# Yoga
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/yoga/CMakeLists.txt")
    message(STATUS "  Found Yoga")
    add_subdirectory(yoga)
    set(YOGA_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  Yoga not found. Please run scripts/download_deps.sh")
    set(YOGA_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# Skia (可选)
# ============================================
message(STATUS "  Checking Skia: LIGHTUI_USE_SKIA=${LIGHTUI_USE_SKIA}")
if(LIGHTUI_USE_SKIA)
    if(EXISTS "${THIRD_PARTY_DIR}/skia")
        message(STATUS "  Found Skia")

        # 根据构建类型选择 Skia 库路径
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(SKIA_LIB_DIR "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64")
            set(SKIA_INCLUDE_DIR "${THIRD_PARTY_DIR}/skia/Debug")
            message(STATUS "  Using Debug Skia libraries from: ${SKIA_LIB_DIR}")
        else()
            set(SKIA_LIB_DIR "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64")
            set(SKIA_INCLUDE_DIR "${THIRD_PARTY_DIR}/skia/Release")
            message(STATUS "  Using Release Skia libraries from: ${SKIA_LIB_DIR}")
        endif()

        # Skia主库
        add_library(skia STATIC IMPORTED GLOBAL)
        set_target_properties(skia PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/skia.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/skia.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/skia.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${SKIA_INCLUDE_DIR}"
        )

        # Skia依赖库
        add_library(skia_harfbuzz STATIC IMPORTED GLOBAL)
        set_target_properties(skia_harfbuzz PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/harfbuzz.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/harfbuzz.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/harfbuzz.lib"
        )

        add_library(skia_icu STATIC IMPORTED GLOBAL)
        set_target_properties(skia_icu PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/icu.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/icu.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/icu.lib"
        )

        add_library(skia_libpng STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libpng PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/libpng.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/libpng.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/libpng.lib"
        )

        add_library(skia_libjpeg STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libjpeg PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/libjpeg.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/libjpeg.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/libjpeg.lib"
        )

        add_library(skia_libwebp STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libwebp PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/libwebp.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/libwebp.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/libwebp.lib"
        )

        add_library(skia_zlib STATIC IMPORTED GLOBAL)
        set_target_properties(skia_zlib PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/zlib.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/zlib.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/zlib.lib"
        )

        add_library(skia_skcms STATIC IMPORTED GLOBAL)
        set_target_properties(skia_skcms PROPERTIES
            IMPORTED_LOCATION_DEBUG "${THIRD_PARTY_DIR}/skia/Debug/out/Debug-windows-x64/skcms.lib"
            IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/skia/Release/out/Release-windows-x64/skcms.lib"
            IMPORTED_LOCATION "${SKIA_LIB_DIR}/skcms.lib"
        )

        # 链接所有依赖
        target_link_libraries(skia INTERFACE
            skia_harfbuzz
            skia_icu
            skia_libpng
            skia_libjpeg
            skia_libwebp
            skia_zlib
            skia_skcms
            opengl32
            gdi32
            user32
        )

        set(SKIA_FOUND TRUE PARENT_SCOPE)
        message(STATUS "  Skia configured successfully")
        message(STATUS "  Skia target created: skia")
    else()
        message(WARNING "  Skia not found. Please download Skia to third_party/skia")
        set(SKIA_FOUND FALSE PARENT_SCOPE)
    endif()
else()
    message(STATUS "  Skia disabled (LIGHTUI_USE_SKIA=OFF)")
    set(SKIA_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# stb_image (header-only)
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/stb/stb_image.h")
    message(STATUS "  Found stb_image")
    add_library(stb_image INTERFACE)
    target_include_directories(stb_image INTERFACE "${THIRD_PARTY_DIR}")
    set(STB_IMAGE_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  stb_image not found")
    set(STB_IMAGE_FOUND FALSE PARENT_SCOPE)
endif()

# ============================================
# Lexbor - HTML/CSS Parser
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/lexbor/CMakeLists.txt")
    message(STATUS "  Found Lexbor")

    # 配置 Lexbor 构建选项
    set(LEXBOR_BUILD_SHARED OFF CACHE BOOL "Build Lexbor as static library" FORCE)
    set(LEXBOR_BUILD_STATIC ON CACHE BOOL "Build Lexbor as static library" FORCE)
    set(LEXBOR_BUILD_TESTS OFF CACHE BOOL "Build Lexbor tests" FORCE)
    set(LEXBOR_BUILD_EXAMPLES OFF CACHE BOOL "Build Lexbor examples" FORCE)

    add_subdirectory(lexbor)
    set(LEXBOR_FOUND TRUE PARENT_SCOPE)
    message(STATUS "  Lexbor configured successfully")
else()
    message(WARNING "  Lexbor not found. Please clone lexbor to third_party/lexbor")
    set(LEXBOR_FOUND FALSE PARENT_SCOPE)
endif()

message(STATUS "Third-party libraries configured.")
