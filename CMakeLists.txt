cmake_minimum_required(VERSION 2.8.2)

# a little deGNU'ing is always good
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(LLHDL)

set(CMAKE_C_FLAGS "-O2 -Wall -D_GNU_SOURCE -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

set(LLHDL_VERSION_MAJOR 0)
set(LLHDL_VERSION_MINOR 0)
set(LLHDL_VERSION_PATCH 1)

configure_file(
	"${PROJECT_SOURCE_DIR}/include/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_BINARY_DIR}")

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(libbanner)
add_subdirectory(libnetlist)
add_subdirectory(libllhdl)

add_subdirectory(samples)
add_subdirectory(llhdl-resolveucf)
add_subdirectory(llhdl-verilog)
