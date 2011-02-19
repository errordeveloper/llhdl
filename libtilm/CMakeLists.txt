add_library(tilm api.c shannon.c bdspga.c)
target_link_libraries(tilm llhdl mapkit ${GMP_LIBRARIES})
