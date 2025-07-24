cmake_minimum_required(VERSION 3.13)

project(pico_ups LANGUAGES CXX)

# Add the library
add_library(pico_ups STATIC)

# Specify the source files for the library
target_sources(pico_ups PRIVATE
    pico_ups.cpp
)

# Include directories for the library
target_include_directories(pico_ups PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Pico SDK libraries
target_link_libraries(pico_ups PUBLIC
    pico_stdlib
    hardware_i2c
)

# Set C++ standard
target_compile_features(pico_ups PUBLIC cxx_std_17)