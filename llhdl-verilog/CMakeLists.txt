add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/scanner.c
	COMMAND re2c -o ${CMAKE_CURRENT_SOURCE_DIR}/scanner.c ${CMAKE_CURRENT_SOURCE_DIR}/scanner.re
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scanner.re ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/parser.c ${CMAKE_CURRENT_SOURCE_DIR}/parser.h ${CMAKE_CURRENT_SOURCE_DIR}/parser.out
	COMMAND lemon ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
)

add_executable(llhdl-verilog main.c verilog.c scanner.c parser.c transform.c)
target_link_libraries(llhdl-verilog banner llhdl ${GMP_LIBRARIES})
install(TARGETS llhdl-verilog DESTINATION bin)
