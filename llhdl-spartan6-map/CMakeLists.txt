add_executable(llhdl-spartan6-map main.c flow.c commonstruct.c dsp.c carryarith.c srl16.c lut.c fd.c)
target_link_libraries(llhdl-spartan6-map banner netlist llhdl mapkit tilm bd ${GMP_LIBRARIES})
install(TARGETS llhdl-spartan6-map DESTINATION bin)
