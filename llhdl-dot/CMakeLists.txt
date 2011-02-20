add_executable(llhdl-dot main.c)
target_link_libraries(llhdl-dot banner llhdl ${GMP_LIBRARIES})
install(TARGETS llhdl-dot DESTINATION bin)
