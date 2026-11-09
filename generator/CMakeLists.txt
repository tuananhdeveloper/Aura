cmake_minimum_required(VERSION 3.20)
project(aura_generator VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8)
FetchContent_Declare(raygui GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 25c8c65a6e5f0f4d4b564a0343861898c6f2778b SOURCE_SUBDIR no-cmake-build)
FetchContent_Declare(portable_file_dialogs GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG 8ccf2a93f70d021842e72fd950eac2a0bb244eec SOURCE_SUBDIR no-cmake-build)
FetchContent_MakeAvailable(raylib raygui portable_file_dialogs)

add_library(generator_core src/core.cpp)
target_include_directories(generator_core PUBLIC src)
target_compile_features(generator_core PUBLIC cxx_std_17)

add_executable(aura-generator src/main.cpp src/dialogs.cpp)
target_include_directories(aura-generator PRIVATE "${raygui_SOURCE_DIR}/src" "${portable_file_dialogs_SOURCE_DIR}")
target_link_libraries(aura-generator PRIVATE generator_core raylib)
if(APPLE)
    set_target_properties(aura-generator PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "Aura Generator"
        MACOSX_BUNDLE_BUNDLE_NAME "Aura Generator"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tuananhdeveloper.aura-generator"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in")
endif()
if(WIN32)
    target_link_libraries(aura-generator PRIVATE ole32 shell32)
endif()
if(MINGW)
    target_link_options(aura-generator PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(STAGE "${CMAKE_CURRENT_BINARY_DIR}/distribution")
add_custom_target(stage_template ALL
    COMMAND ${CMAKE_COMMAND} "-DROOT=${ROOT}" "-DDEST=${STAGE}"
        "-DRAYLIB_LICENSE=${raylib_SOURCE_DIR}/LICENSE"
        "-DRAYGUI_LICENSE=${raygui_SOURCE_DIR}/LICENSE"
        "-DPFD_LICENSE=${portable_file_dialogs_SOURCE_DIR}/COPYING"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/stage.cmake"
    VERBATIM)
add_dependencies(aura-generator stage_template)
set_target_properties(aura-generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STAGE}")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(aura-generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${STAGE}")
endforeach()

if(APPLE)
    install(TARGETS aura-generator BUNDLE DESTINATION . COMPONENT generator)
    install(DIRECTORY "${STAGE}/template" "${STAGE}/resources" "${STAGE}/third-party-licenses"
        DESTINATION "Aura Generator.app/Contents/MacOS" COMPONENT generator)

    if(CMAKE_OSX_ARCHITECTURES)
        set(DMG_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    else()
        set(DMG_ARCH "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
    if(DMG_ARCH STREQUAL "arm64" OR DMG_ARCH STREQUAL "x86_64")
        add_custom_target(package-dmg
            COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config $<CONFIG>
                --component generator --prefix "${CMAKE_BINARY_DIR}/dmg-contents"
            COMMAND hdiutil create -volname "Aura Generator" -srcfolder "${CMAKE_BINARY_DIR}/dmg-contents"
                -ov -format UDZO "${CMAKE_BINARY_DIR}/aura-generator-macos-${DMG_ARCH}.dmg"
            DEPENDS aura-generator
            VERBATIM)
    endif()
else()
    install(TARGETS aura-generator RUNTIME DESTINATION . COMPONENT generator)
    install(DIRECTORY "${STAGE}/template" "${STAGE}/resources" "${STAGE}/third-party-licenses"
        DESTINATION . COMPONENT generator)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 3.10 COMPONENTS Interpreter REQUIRED)
    add_executable(generator_tests tests/generator_tests.cpp)
    target_link_libraries(generator_tests PRIVATE generator_core)
    add_dependencies(generator_tests stage_template)
    add_test(NAME generator_tests COMMAND generator_tests "${STAGE}" "${Python3_EXECUTABLE}" "${CMAKE_COMMAND}")
endif()
