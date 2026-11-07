cmake_minimum_required(VERSION 3.28)
project(myraylibgame LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

set(RAYLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/raylib/src)

set(APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${RAYLIB_PATH}
)

set(RAYLIB_SOURCES
    ${RAYLIB_PATH}/external/glad_gles2.h
    ${RAYLIB_PATH}/rcore.c
    ${RAYLIB_PATH}/rshapes.c
    ${RAYLIB_PATH}/rtext.c
    ${RAYLIB_PATH}/rtextures.c
    ${RAYLIB_PATH}/rmodels.c
    ${RAYLIB_PATH}/raudio.c
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_library(raylib STATIC ${RAYLIB_SOURCES})
target_include_directories(raylib PUBLIC ${RAYLIB_PATH})

if(ANDROID)
    target_compile_definitions(raylib PRIVATE PLATFORM_ANDROID GRAPHICS_API_OPENGL_ES2)

    target_include_directories(raylib PRIVATE
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
    )

    add_library(native_app_glue STATIC
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )

    target_include_directories(native_app_glue PUBLIC
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
    )

    add_library(myraylibgame SHARED main.cpp)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    target_link_options(myraylibgame PRIVATE -Wl,--wrap=fopen)

    target_link_libraries(
        myraylibgame
        PRIVATE
        raylib
        native_app_glue
        GLESv2
        EGL
        android
        log
        m
    )
elseif(EMSCRIPTEN)
    string(REPLACE "/cpp" "" SOURCES_DIR "${CMAKE_SOURCE_DIR}")
    set(ASSETS_DIR "${SOURCES_DIR}/assets")
    file(GLOB_RECURSE WEB_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false "${ASSETS_DIR}/*")

    target_compile_definitions(raylib PRIVATE PLATFORM_WEB GRAPHICS_API_OPENGL_ES2)
    target_link_options(raylib PUBLIC
        "-sUSE_GLFW=3"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
    )

    add_executable(myraylibgame main.cpp)
    target_link_libraries(myraylibgame PRIVATE raylib)
    target_link_options(myraylibgame PRIVATE "SHELL:--preload-file \"${ASSETS_DIR}@/\"")
    set_property(TARGET myraylibgame APPEND PROPERTY LINK_DEPENDS ${WEB_ASSETS})
    set_target_properties(myraylibgame PROPERTIES SUFFIX ".html")
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_SYSROOT iphoneos)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "18.0")
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "18.0")
    set(CMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")

    set(RAYLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(RAYLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_GLFW OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)

    set(IOS_FRAMEWORKS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ios-frameworks)
    set(ANGLE_EGL ${IOS_FRAMEWORKS_DIR}/libEGL.xcframework)
    set(ANGLE_GLES ${IOS_FRAMEWORKS_DIR}/libGLESv2.xcframework)
    set(ANGLE_EGL_HEADERS ${IOS_FRAMEWORKS_DIR}/libEGL.xcframework/ios-arm64/libEGL.framework/Headers)

    set(ANGLE_GLES_HEADERS ${IOS_FRAMEWORKS_DIR}/libGLESv2.xcframework/ios-arm64/libGLESv2.framework/Headers)

    add_executable(myraylibgame main.cpp)
    set_target_properties(myraylibgame PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.tinybytelabs.myraylibgame"
            MACOSX_BUNDLE_BUNDLE_NAME "MyRaylibGame"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.tinybytelabs.myraylibgame"
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
    set_target_properties(myraylibgame PROPERTIES
            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
            "@executable_path/Frameworks"

            XCODE_EMBED_FRAMEWORKS
            "${CMAKE_CURRENT_SOURCE_DIR}/ios-frameworks/libEGL.xcframework;${CMAKE_CURRENT_SOURCE_DIR}/ios-frameworks/libGLESv2.xcframework"

            XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
    )
    target_link_libraries(myraylibgame PRIVATE raylib)

    string(REPLACE "/cpp" "" SOURCES_DIR "${CMAKE_SOURCE_DIR}")
    set(ASSETS_DIR "${SOURCES_DIR}/assets")
    file(GLOB_RECURSE APP_ASSETS
            CONFIGURE_DEPENDS
            LIST_DIRECTORIES false
            "${ASSETS_DIR}/*"
    )
    foreach(asset IN LISTS APP_ASSETS)
        file(RELATIVE_PATH relative_path "${ASSETS_DIR}" "${asset}")
        get_filename_component(relative_dir "${relative_path}" DIRECTORY)

        if(relative_dir STREQUAL "")
            set(resource_location "assets")
        else()
            set(resource_location "assets/${relative_dir}")
        endif()

        set_source_files_properties("${asset}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "${resource_location}"
        )
    endforeach()
    target_sources(myraylibgame PRIVATE ${APP_ASSETS})

    target_compile_definitions(raylib PRIVATE
            PLATFORM_IOS
            GRAPHICS_API_OPENGL_ES3
            GL_GLEXT_PROTOTYPES
            SUPPORT_FILEFORMAT_MP3=1
    )
    target_link_libraries(raylib PUBLIC
            ${ANGLE_EGL}
            ${ANGLE_GLES}
    )
    target_include_directories(raylib PRIVATE
            ${ANGLE_EGL_HEADERS}
            ${ANGLE_GLES_HEADERS}
    )
    target_link_libraries(raylib PUBLIC
            "${ANGLE_EGL}"
            "${ANGLE_GLES}"
            "-framework UIKit"
            "-framework Foundation"
            "-framework QuartzCore"
            "-framework CoreGraphics"
            "-framework AVFoundation"
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework OpenAL"
    )

    set_source_files_properties(
            ${RAYLIB_PATH}/raudio.c
            PROPERTIES LANGUAGE OBJC
    )
    set_source_files_properties(
            ${RAYLIB_PATH}/rcore.c
            PROPERTIES LANGUAGE OBJC
    )
else()
    string(REPLACE "/cpp" "" SOURCES_DIR "${CMAKE_SOURCE_DIR}")
    file(COPY "${SOURCES_DIR}/assets/" DESTINATION "${CMAKE_BINARY_DIR}/assets")
    add_subdirectory(${RAYLIB_PATH}/external/glfw ${CMAKE_BINARY_DIR}/glfw)

    target_link_libraries(raylib PUBLIC glfw)
    if(APPLE)
        target_compile_definitions(raylib PRIVATE PLATFORM_DESKTOP GRAPHICS_API_OPENGL_33)
        target_link_libraries(raylib PUBLIC
                "-framework Cocoa"
                "-framework OpenGL"
                "-framework IOKit"
                "-framework CoreVideo"
        )
    else()
        if(CMAKE_SYSTEM_NAME MATCHES "Linux")
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
                target_compile_definitions(raylib PRIVATE PLATFORM_DESKTOP GRAPHICS_API_OPENGL_21)
            else()
                target_compile_definitions(raylib PRIVATE PLATFORM_DESKTOP GRAPHICS_API_OPENGL_33)
            endif()
        else()
            target_compile_definitions(raylib PRIVATE PLATFORM_DESKTOP GRAPHICS_API_OPENGL_33)
        endif()
    endif()

    add_executable(myraylibgame main.cpp)

    target_link_libraries(myraylibgame PRIVATE raylib)

    if (WIN32)
        target_link_libraries(raylib PUBLIC winmm)
        target_link_options(myraylibgame PRIVATE
                -static-libgcc
                -static-libstdc++
                -static
        )
    endif()
endif()

target_include_directories(myraylibgame PRIVATE ${APP_INCLUDE_DIRS})
target_sources(myraylibgame PRIVATE
        framework/tilemap.cpp
        third_party/tinyxml2/tinyxml2.cpp)