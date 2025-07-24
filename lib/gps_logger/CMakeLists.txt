cmake_minimum_required(VERSION 3.13)

project(gps_logger LANGUAGES CXX)

# Add the library
add_library(gps_logger STATIC)

# Specify the source files for the library
target_sources(gps_logger PRIVATE
    gps_logger.cpp
)

# Include directories for the library
target_include_directories(gps_logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/L76B  # For gps_data.h and gps_datetime.h
    ${CMAKE_SOURCE_DIR}/lib/fatfs  # For ff.h
)

# Link with necessary libraries
target_link_libraries(gps_logger PUBLIC
    L76B        # For GPS data structures
    pico_stdlib # For standard Pico functionality
)

# Set C++ standard
target_compile_features(gps_logger PUBLIC cxx_std_17)