add_executable(simple-mapper main.c)
target_link_libraries(simple-mapper netlist)
target_link_libraries(simple-mapper llhdl)
install(TARGETS simple-mapper DESTINATION bin)
