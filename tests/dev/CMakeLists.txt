
project( test.dev )

find_package( Qt6Core REQUIRED )

set( SRC main.cpp
    ../../md-pdf/md_doc.cpp
	../../md-pdf/md_doc.hpp
	../../md-pdf/md_parser.cpp
	../../md-pdf/md_parser.hpp )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty )

file( COPY dev.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( -DDEV )

add_executable( test.dev ${SRC} )

target_link_libraries( test.dev Qt6::Core )
