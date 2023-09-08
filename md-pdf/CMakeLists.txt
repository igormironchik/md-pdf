
project( md-pdf-gui )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )

find_package( Qt6Widgets 6.5.0 REQUIRED )
find_package( Qt6Network 6.5.0 REQUIRED )
find_package( ImageMagick 6 EXACT REQUIRED COMPONENTS Magick++ MagickCore )

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
add_definitions( -DPODOFO_SHARED )

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

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
	const.hpp )
	
qt6_add_resources( GUI_SRC resources.qrc )

if( WIN32 )
	list( APPEND GUI_SRC md-pdf.rc )
endif()

set( LIB_SYNTAX_SRC syntax.cpp
	syntax.hpp )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/podofo/src
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/widgets/include
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo/src/podofo
	${ImageMagick_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/JKQtPlotter/lib
	${md4qt_INCLUDE_DIRECTORIES}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ksyntaxhighlighting/src
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/ksyntaxhighlighting/src )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo/src/podofo
	${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/podofo/src )
	
execute_process( COMMAND git rev-parse --short HEAD
	OUTPUT_VARIABLE MDPDF_VERSION_MINOR
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_STRIP_TRAILING_WHITESPACE )

set( MDPDF_VERSION "0.9.18 build ${MDPDF_VERSION_MINOR}" )

configure_file( version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.hpp )

add_library( syntax STATIC ${LIB_SYNTAX_SRC} )

target_link_libraries( syntax KF6SyntaxHighlighting Qt6::Gui Qt6::Network Qt6::Core )

add_executable( md-pdf-gui WIN32 ${GUI_SRC} )

target_link_libraries( md-pdf-gui syntax widgets ${ImageMagick_LIBRARIES}
	JKQTMathTextSharedLib JKQTCommonSharedLib KF6SyntaxHighlighting
	Qt6::Widgets Qt6::Network Qt6::Core
	podofo_shared )
