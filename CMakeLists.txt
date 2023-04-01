
cmake_minimum_required( VERSION 3.19 )

project( md-pdf )

list( PREPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR} )
list( PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR} )
list( PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

find_package( Qt6Core REQUIRED )

if( UNIX AND NOT APPLE )
	list( PREPEND CMAKE_INSTALL_RPATH "${QT6_INSTALL_PREFIX}/lib:\$ORIGIN/../lib" )
else()
	list( PREPEND CMAKE_INSTALL_RPATH "${QT6_INSTALL_PREFIX}/lib:@executable_path/../lib" )
endif()

set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )
set( CMAKE_MACOSX_RPATH TRUE )

link_directories( ${CMAKE_BINARY_DIR}/lib )

option( BUILD_BENCHMARK "Build benchmark? Default OFF." OFF )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "Release"
		CACHE STRING "Choose the type of build."
		FORCE)
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )

set( PODOFO_BUILD_LIB_ONLY ON CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/podofo )

set( BUILD_WIDGETS_EXAMPLES OFF CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/widgets )

set( BUILD_MD4QT_QT_TESTS OFF CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/md4qt )

set( JKQtPlotter_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE )
set( JKQtPlotter_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/JKQtPlotter )

add_subdirectory( md-pdf )

enable_testing()

add_subdirectory( tests )
