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
# QuickJS
# ============================================
if(EXISTS "${THIRD_PARTY_DIR}/quickjs")
    message(STATUS "  Found QuickJS")

    # QuickJS源文件
    set(QUICKJS_SOURCES
        ${THIRD_PARTY_DIR}/quickjs/quickjs.c
        ${THIRD_PARTY_DIR}/quickjs/libregexp.c
        ${THIRD_PARTY_DIR}/quickjs/libunicode.c
        ${THIRD_PARTY_DIR}/quickjs/cutils.c
        ${THIRD_PARTY_DIR}/quickjs/quickjs-libc.c
        ${THIRD_PARTY_DIR}/quickjs/libbf.c
    )

    # 创建QuickJS静态库
    add_library(quickjs STATIC ${QUICKJS_SOURCES})
    target_include_directories(quickjs PUBLIC ${THIRD_PARTY_DIR}/quickjs)

    # QuickJS编译选项
    if(MSVC)
        target_compile_definitions(quickjs PRIVATE
            _CRT_SECURE_NO_WARNINGS
            CONFIG_VERSION="2024-01-13"
            CONFIG_BIGNUM=1
        )
    else()
        target_compile_definitions(quickjs PRIVATE
            CONFIG_VERSION="2024-01-13"
            CONFIG_BIGNUM=1
        )
        target_compile_options(quickjs PRIVATE -Wno-unused-parameter)
    endif()

    set(QUICKJS_FOUND TRUE PARENT_SCOPE)
else()
    message(WARNING "  QuickJS not found. Please run scripts/download_deps.sh")
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

        # Skia主库
        add_library(skia STATIC IMPORTED GLOBAL)
        set_target_properties(skia PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/skia.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_DIR}/skia"
        )

        # Skia依赖库
        add_library(skia_harfbuzz STATIC IMPORTED GLOBAL)
        set_target_properties(skia_harfbuzz PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/harfbuzz.lib"
        )

        add_library(skia_icu STATIC IMPORTED GLOBAL)
        set_target_properties(skia_icu PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/icu.lib"
        )

        add_library(skia_libpng STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libpng PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/libpng.lib"
        )

        add_library(skia_libjpeg STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libjpeg PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/libjpeg.lib"
        )

        add_library(skia_libwebp STATIC IMPORTED GLOBAL)
        set_target_properties(skia_libwebp PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/libwebp.lib"
        )

        add_library(skia_zlib STATIC IMPORTED GLOBAL)
        set_target_properties(skia_zlib PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/zlib.lib"
        )

        add_library(skia_skcms STATIC IMPORTED GLOBAL)
        set_target_properties(skia_skcms PROPERTIES
            IMPORTED_LOCATION "${THIRD_PARTY_DIR}/skia/out/Release-windows-x64/skcms.lib"
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

message(STATUS "Third-party libraries configured.")
