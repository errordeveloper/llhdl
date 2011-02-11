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

# system libraries

find_package(GMP REQUIRED)
include_directories("${GMP_INCLUDES}")

# subdirectories

add_subdirectory(libbanner)
add_subdirectory(libllhdl)
add_subdirectory(libbd)
add_subdirectory(libtilm)
add_subdirectory(libnetlist)

add_subdirectory(llhdl-verilog)
add_subdirectory(llhdl-spartan6-map)
add_subdirectory(llhdl-resolveucf)
add_subdirectory(samples)

# packaging

include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An experimental logic synthesis and manipulation infrastructure for FPGAs")
set(CPACK_PACKAGE_VENDOR "Sebastien Bourdeauducq")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "${LLHDL_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${LLHDL_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${LLHDL_VERSION_PATCH}")
include(CPack)

