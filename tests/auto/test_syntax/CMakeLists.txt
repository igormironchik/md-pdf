
project( test.syntax )

find_package( Qt5Core 5.15 REQUIRED )

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

set( SRC main.cpp )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty )

link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib )

add_executable( test.syntax ${SRC} )

target_link_libraries( test.syntax syntax Qt5::Core )

add_test( NAME test.syntax
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test.syntax
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
