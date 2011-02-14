add_library(tilm api.c fd.c shannon.c bdspga.c)
target_link_libraries(tilm llhdl ${GMP_LIBRARIES})
