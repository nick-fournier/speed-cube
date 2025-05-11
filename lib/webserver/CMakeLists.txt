cmake_minimum_required(VERSION 3.13)

project(webserver LANGUAGES CXX)

# Add the library
add_library(webserver STATIC)

# Specify the source files for the library
target_sources(webserver PRIVATE
    webserver.cpp
    dhcpserver.c
    dnsserver.c
)

# Include directories for the library
target_include_directories(webserver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/L76B
)

# Link Pico SDK libraries
target_link_libraries(webserver PUBLIC
    pico_stdlib
    L76B
    pico_cyw43_arch_lwip_threadsafe_background
)

# Set C++ standard
target_compile_features(webserver PUBLIC cxx_std_17)