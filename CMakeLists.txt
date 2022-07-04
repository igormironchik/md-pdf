
cmake_minimum_required( VERSION 3.1 )

project( md-pdf )

include( ExternalProject )

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/podofo-trunk/cmake/modules )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

link_directories( ${CMAKE_BINARY_DIR}/lib )

option( BUILD_BENCHMARK "Build benchmark? Default OFF." OFF )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "Release"
		CACHE STRING "Choose the type of build."
		FORCE)
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )

if( MDPDF_USE_CONAN )
	include( conanbuildinfo.cmake )
	conan_basic_setup()

	configure_file( path.cmd.in ${CMAKE_CURRENT_SOURCE_DIR}/path.cmd )
endif()

set( PODOFO_BUILD_LIB_ONLY ON )

add_subdirectory( 3rdparty/podofo-trunk )

set( BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/widgets )

find_package( LIBJPEG )
find_package( PNG )

if( NOT LIBJPEG_FOUND )
	message( FATAL_ERROR "libjpeg is required!" )
endif()

if( NOT PNG_FOUND )
	message( FATAL_ERROR "libpng is required!" )
endif()

set( JKQtPlotter_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE )
add_subdirectory( 3rdparty/JKQtPlotter )

add_subdirectory( md-pdf )

enable_testing()

add_subdirectory( tests )
