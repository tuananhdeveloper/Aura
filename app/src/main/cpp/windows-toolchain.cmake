# app/src/main/cpp/windows-toolchain.cmake
if (APPLE)
    set(MINGW_PATH /opt/homebrew/bin)
    set(ROOT_PATH /opt/homebrew)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(MINGW_PATH /usr/bin)
    set(ROOT_PATH /usr)
endif ()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# change these paths if your install is different
set(CMAKE_C_COMPILER ${MINGW_PATH}/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PATH}/x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER ${MINGW_PATH}/x86_64-w64-mingw32-windres)

# IMPORTANT: force the MinGW binutils, not the host macOS ones
set(CMAKE_AR ${MINGW_PATH}/x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB ${MINGW_PATH}/x86_64-w64-mingw32-ranlib)
set(CMAKE_STRIP ${MINGW_PATH}/x86_64-w64-mingw32-strip)

# set the GCC archiver variables too
set(CMAKE_C_COMPILER_AR ${MINGW_PATH}/x86_64-w64-mingw32-ar)
set(CMAKE_C_COMPILER_RANLIB ${MINGW_PATH}/x86_64-w64-mingw32-ranlib)
set(CMAKE_CXX_COMPILER_AR ${MINGW_PATH}/x86_64-w64-mingw32-ar)
set(CMAKE_CXX_COMPILER_RANLIB ${MINGW_PATH}/x86_64-w64-mingw32-ranlib)

set(CMAKE_FIND_ROOT_PATH ${ROOT_PATH})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)