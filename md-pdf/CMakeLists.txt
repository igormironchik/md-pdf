
project( md-pdf-gui )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )

find_package(Qt5Core 5.15 REQUIRED)
find_package(Qt5Widgets 5.15 REQUIRED)
find_package(Qt5Network 5.15 REQUIRED)

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

set( LIB_SRC md_doc.hpp
    md_doc.cpp
    md_parser.hpp
    md_parser.cpp )

set( GUI_SRC main.cpp
	main_window.cpp
	main_window.hpp
	main_window.ui
	color_widget.hpp
	color_widget.cpp
	renderer.hpp
	renderer.cpp
	progress.hpp
	progress.cpp
	progress.ui )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/podofo-trunk/src
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk/src/podofo )

find_package( Threads )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib )

add_library( md-parser STATIC ${LIB_SRC} )

target_link_libraries( md-parser Qt5::Core )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk/src )

add_executable( md-pdf-gui ${GUI_SRC} )

target_link_libraries( md-pdf-gui md-parser ${PODOFO_LIB} Qt5::Widgets Qt5::Network )
