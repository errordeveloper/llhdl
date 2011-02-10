add_library(tilm api.c rg.c shannon.c bdspga.c)
target_link_libraries(tilm llhdl ${GMP_LIBRARIES})
