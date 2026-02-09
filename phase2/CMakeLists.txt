cmake_minimum_required(VERSION 3.16)
project(ActionCamera VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(ENABLE_CROSS_COMPILE "Enable cross-compilation for ARM64" OFF)

if(ENABLE_CROSS_COMPILE)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mtune=cortex-a53")
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GPIOD REQUIRED libgpiod>=2.2.2)
if(NOT GPIOD_FOUND)
    message(FATAL_ERROR "libgpiod C library v2.2.2+ required. Install: sudo apt install libgpiod-dev")
endif()

find_program(RPICAM_VID rpicam-vid)
if(NOT RPICAM_VID)
    message(FATAL_ERROR "rpicam-vid not found. Install rpicam-apps: sudo apt install rpicam-apps")
endif()

set(GPIOD_LIBRARIES ${GPIOD_LIBRARIES})
set(GPIOD_INCLUDE_DIRS ${GPIOD_INCLUDE_DIRS})
set(GPIOD_CFLAGS_OTHER ${GPIOD_CFLAGS_OTHER})

add_subdirectory(src)
