
project( test.render )

find_package( Qt5Core 5.15 REQUIRED )
find_package( Qt5Test 5.15 REQUIRED )
find_package( Qt5Gui 5.15 REQUIRED )
find_package( Qt5Network 5.15 REQUIRED )

set( CMAKE_AUTOMOC ON )

if( ENABLE_COVERAGE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage" )
endif( ENABLE_COVERAGE )

set( SRC main.cpp
	../../../md-pdf/renderer.cpp
	../../../md-pdf/renderer.hpp
	test_footnotes_data.hpp )

add_definitions( -DMD_PDF_TESTING )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/podofo-trunk/src
	${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo-trunk
	${CMAKE_CURRENT_BINARY_DIR} )

link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo-trunk/src/podofo )
link_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../../3rdparty/podofo-trunk/src )

set( NORMAL_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif.ttf )
set( ITALIC_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_italic.ttf )
set( BOLD_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_bold.ttf )
set( BOLD_ITALIC_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/droid_serif_italic_bold.ttf )
set( MONO_FONT ${CMAKE_CURRENT_SOURCE_DIR}/../../fonts/courier_new_mc_serif.ttf )
set( WORKING_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} )

configure_file( test_const.hpp.in test_const.hpp @ONLY )

link_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib )

add_executable( test.render ${SRC} )

target_link_libraries( test.render syntax md-parser ${PODOFO_LIB} Qt5::Gui Qt5::Network Qt5::Test Qt5::Core )

add_test( NAME test.render
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test.render
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
