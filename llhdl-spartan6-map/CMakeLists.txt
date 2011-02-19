add_executable(llhdl-spartan6-map main.c flow.c fd.c lut.c arith.c)
target_link_libraries(llhdl-spartan6-map banner netlist llhdl mapkit tilm bd ${GMP_LIBRARIES})
install(TARGETS llhdl-spartan6-map DESTINATION bin)
