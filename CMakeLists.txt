cmake_minimum_required(VERSION 2.8.2)

# a little deGNU'ing is always good
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(LLHDL)

set(CMAKE_C_FLAGS "-O2 -Wall -D_GNU_SOURCE -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(libnetlist)
add_subdirectory(libllhdl)

add_subdirectory(samples)
add_subdirectory(llhdl-resolveucf)
add_subdirectory(llhdl-verilog)
