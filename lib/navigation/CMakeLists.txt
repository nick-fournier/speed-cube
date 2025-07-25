cmake_minimum_required(VERSION 3.13)

project(navigation LANGUAGES CXX)

# Add the library
add_library(navigation STATIC)

# Specify the source files for the library
target_sources(navigation PRIVATE
    gui.cpp
    simulation.cpp
    timeseries.cpp
    pointers.cpp
    tack_detector.cpp
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
    ${CMAKE_SOURCE_DIR}/lib/pico_ups
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
    pico_ups
    hardware_spi
    hardware_gpio
    hardware_i2c
)

# Set C++ standard
target_compile_features(navigation PUBLIC cxx_std_17)