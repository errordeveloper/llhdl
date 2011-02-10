add_library(llhdl structure.c exchange.c tools.c)
target_link_libraries(llhdl ${GMP_LIBRARIES})

