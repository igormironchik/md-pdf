
cmake_minimum_required( VERSION 3.1 )

include( ExternalProject )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/podofo-trunk/cmake/modules )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "Release"
		CACHE STRING "Choose the type of build."
		FORCE)
endif( NOT CMAKE_BUILD_TYPE )

set( CMAKE_CXX_STANDARD 14 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )

project( md-pdf )

file( MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

set( PODOFO_BUILD_LIB_ONLY ON )

add_subdirectory( 3rdparty/podofo-trunk )

find_package( LIBJPEG )
find_package( PNG )

if( NOT LIBJPEG_FOUND )
	message( FATAL_ERROR "libjpeg is required!" )
endif()

if( NOT PNG_FOUND )
	message( FATAL_ERROR "libpng is required!" )
endif()

add_subdirectory( md-pdf )

enable_testing()

add_subdirectory( tests )
