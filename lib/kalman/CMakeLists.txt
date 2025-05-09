cmake_minimum_required(VERSION 3.13)

project(kalman LANGUAGES CXX)

# Add the library
add_library(kalman STATIC)

# Specify the source files for the library
target_sources(kalman PRIVATE
    kalman.cpp
)

# Include directories for the library
target_include_directories(kalman PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/eigen
)

# Link Pico SDK libraries
target_link_libraries(kalman PUBLIC
    pico_stdlib
    hardware_uart
)

# Set C++ standard
target_compile_features(kalman PUBLIC cxx_std_17)