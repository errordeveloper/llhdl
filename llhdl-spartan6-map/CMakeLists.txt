add_executable(llhdl-spartan6-map main.c)
target_link_libraries(llhdl-spartan6-map banner llhdl tilm ${GMP_LIBRARIES})
install(TARGETS llhdl-spartan6-map DESTINATION bin)
