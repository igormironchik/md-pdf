
project( md-pdf-gui )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )

find_package( Qt6Core REQUIRED )
find_package( Qt6Widgets REQUIRED )
find_package( Qt6Network REQUIRED )
find_package( ImageMagick 6 EXACT REQUIRED COMPONENTS Magick++ MagickCore )

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )

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
	progress.ui
	const.hpp
	license_dialog.cpp
	license_dialog.hpp
	license_dialog.ui )
	
qt6_add_resources( GUI_SRC resources.qrc )

if( WIN32 )
	list( APPEND GUI_SRC md-pdf.rc )
endif()

set( LIB_SYNTAX_SRC syntax.cpp
	syntax.hpp )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/podofo-trunk/src
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk
	${ImageMagick_INCLUDE_DIRS} )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk/src/podofo
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo-trunk/src )

add_library( md-parser STATIC ${LIB_SRC} )

add_library( syntax STATIC ${LIB_SYNTAX_SRC} )

target_link_libraries( md-parser Qt6::Core )

target_link_libraries( syntax Qt6::Core )

add_executable( md-pdf-gui WIN32 ${GUI_SRC} )

target_link_libraries( md-pdf-gui md-parser syntax ${PODOFO_LIB} ${ImageMagick_LIBRARIES}
	Qt6::Widgets Qt6::Network Qt6::Core )
