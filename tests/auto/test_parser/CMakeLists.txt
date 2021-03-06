
project( test.md_parser )

find_package( Qt5Core 5.15 REQUIRED )

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

set( SRC main.cpp )

file( GLOB MD_FILES *.md )
file( COPY ${MD_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

file( COPY a.png
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
file( COPY a.jpg
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
file( COPY b.png
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty )

link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib )

add_executable( test.md_parser ${SRC} )

target_link_libraries( test.md_parser md-parser Qt5::Core )

add_test( NAME test.md_parser
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test.md_parser
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
