cmake_minimum_required(VERSION 3.13)

project(LCD LANGUAGES CXX)

add_library(LCD STATIC)

# Specify the source files for the library
target_sources(LCD PRIVATE
    LCD.cpp
)

# Include directories for the library
target_include_directories(LCD PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link against the Pico SDK libraries
target_link_libraries(LCD PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
)

# Set C++ standard
target_compile_features(LCD PUBLIC cxx_std_17)