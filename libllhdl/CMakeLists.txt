add_library(llhdl structure.c interchange.c tools.c)
target_link_libraries(llhdl ${GMP_LIBRARIES})
