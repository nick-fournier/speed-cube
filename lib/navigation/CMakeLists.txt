cmake_minimum_required(VERSION 3.13)

project(navigation LANGUAGES CXX)

# Add the library
add_library(navigation STATIC)

# Specify the source files for the library
target_sources(navigation PRIVATE
    gui.cpp
    control.cpp
)

# Include directories for the library
target_include_directories(navigation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/lcd
    ${CMAKE_SOURCE_DIR}/lib/config
    ${CMAKE_SOURCE_DIR}/lib/font
    ${CMAKE_SOURCE_DIR}/lib/fatfs
    ${CMAKE_SOURCE_DIR}/lib/sdcard
    ${CMAKE_SOURCE_DIR}/lib/L76B
    ${CMAKE_SOURCE_DIR}/lib/kalman
    ${CMAKE_SOURCE_DIR}/lib/eigen
)

# Link Pico SDK libraries
target_link_libraries(navigation PUBLIC
    pico_stdlib
    font
    fatfs
    sdcard
    lcd
    config
    L76B
    kalman
    hardware_spi
    hardware_gpio
)

# Set C++ standard
target_compile_features(navigation PUBLIC cxx_std_17)