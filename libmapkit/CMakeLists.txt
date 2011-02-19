add_library(mapkit mapkit.c interconnect.c)
target_link_libraries(mapkit llhdl ${GMP_LIBRARIES})
