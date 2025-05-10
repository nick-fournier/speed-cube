cmake_minimum_required(VERSION 3.13)

project(L76B LANGUAGES CXX)

# Add the library
add_library(L76B STATIC)

# Specify the source files for the library
target_sources(L76B PRIVATE
    L76B.cpp
    gps_data.cpp
    kalman.cpp
)

# Include directories for the library
target_include_directories(L76B PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/eigen
)

# Link Pico SDK libraries
target_link_libraries(L76B PUBLIC
    pico_stdlib
    hardware_uart
)

# Set C++ standard
target_compile_features(L76B PUBLIC cxx_std_17)