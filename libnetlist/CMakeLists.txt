add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/xilprims.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_xilprims.py c > ${CMAKE_CURRENT_SOURCE_DIR}/xilprims.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_xilprims.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/netlist/xilprims.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_xilprims.py h > ${PROJECT_SOURCE_DIR}/include/netlist/xilprims.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_xilprims.py
)

set_property(
	SOURCE xilprims.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/netlist/xilprims.h
)

add_library(netlist net.c manager.c edif.c io.c xilprims.c)
