add_library(tilm api.c partition.c variables.c shannon.c bdspga.c)
target_link_libraries(tilm llhdl mapkit ${GMP_LIBRARIES})
