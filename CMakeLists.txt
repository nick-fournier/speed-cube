# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(speed-cube C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ------------------------------------------------------------------
# Load environment variables from `.env` and generate config.h
# ------------------------------------------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.env" DOTENV_CONTENT)
string(REGEX REPLACE "\r?\n" ";" DOTENV_LINES "${DOTENV_CONTENT}")

foreach(line IN LISTS DOTENV_LINES)
    if(line MATCHES "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_BINARY_DIR}/config.h"
    @ONLY
)
# ------------------------------------------------------------------

# Add executable. Default name is the project name, version 0.1
add_subdirectory(lib/L76B)
add_subdirectory(lib/navigation)
add_subdirectory(lib/config)
add_subdirectory(lib/font)
add_subdirectory(lib/fatfs)
add_subdirectory(lib/sdcard)
add_subdirectory(lib/lcd)
add_subdirectory(lib/webserver)
add_subdirectory(lib/gps_logger)
add_executable(speed-cube speed-cube.cpp )

pico_set_program_name(speed-cube "speed-cube")
pico_set_program_version(speed-cube "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(speed-cube 0)
pico_enable_stdio_usb(speed-cube 1)

# Add the standard library to the build
target_link_libraries(speed-cube
        pico_stdlib
        hardware_spi
        hardware_gpio
        pico_multicore
        )

# Add the standard include files to the build
target_include_directories(speed-cube PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_BINARY_DIR}  # so config.h is available
)

# Add any user requested libraries
target_link_libraries(speed-cube 
        L76B
        navigation
        webserver
        gps_logger
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(speed-cube)

