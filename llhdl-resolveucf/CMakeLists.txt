add_executable(llhdl-resolveucf main.c)
target_link_libraries(llhdl-resolveucf netlist)
install(TARGETS llhdl-resolveucf DESTINATION bin)
