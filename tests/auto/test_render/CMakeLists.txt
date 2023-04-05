
project( test.render )

find_package( Qt6Test REQUIRED )
find_package( Qt6Gui REQUIRED )
find_package( Qt6Widgets REQUIRED )
find_package( Qt6Network REQUIRED )
find_package( ImageMagick 6 EXACT REQUIRED COMPONENTS Magick++ MagickCore )

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
add_definitions( -DPODOFO_SHARED )

set( CMAKE_AUTOMOC ON )

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

set( SRC main.cpp
	../../../md-pdf/renderer.cpp
	../../../md-pdf/renderer.hpp )

add_definitions( -DMD_PDF_TESTING )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/podofo/src
	${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo/src/podofo
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/JKQtPlotter/lib
	${md4qt_INCLUDE_DIRECTORIES}
	${CMAKE_CURRENT_BINARY_DIR}
	${ImageMagick_INCLUDE_DIRS} )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo/src/podofo )
link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo/src )

set( NORMAL_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif.ttf )
set( ITALIC_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_italic.ttf )
set( BOLD_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_bold.ttf )
set( BOLD_ITALIC_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_italic_bold.ttf )
set( MONO_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/courier_new_mc_serif.ttf )
set( WORKING_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} )

configure_file( test_const.hpp.in test_const.hpp @ONLY )

link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib )

add_executable( test.render ${SRC} )

target_link_libraries( test.render syntax podofo_shared
	${ImageMagick_LIBRARIES}
	JKQTMathTextSharedLib JKQTCommonSharedLib
	Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Test Qt6::Core )

if( WIN32 )
	set( SUFFIX ".bat" )
endif()

add_test( NAME test.render
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../../../bin/test.render${SUFFIX}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../../bin )
