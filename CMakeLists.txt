
cmake_minimum_required( VERSION 3.19 )

project( md-pdf )

option( BUILD_MDPDF_TESTS "Build tests? Default ON." ON )

list( PREPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR} )
list( PREPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../ecm/share/ECM/cmake )
list( PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR} )
list( PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin )

find_package( Qt6Core 6.5.0 REQUIRED )

if( UNIX AND NOT APPLE )
	list( PREPEND CMAKE_INSTALL_RPATH "${QT6_INSTALL_PREFIX}/lib:\$ORIGIN/../lib" )
else()
	list( PREPEND CMAKE_INSTALL_RPATH "${QT6_INSTALL_PREFIX}/lib" )
	list( PREPEND CMAKE_INSTALL_RPATH "@executable_path/../lib" )
	list( PREPEND CMAKE_INSTALL_RPATH "@loader_path/../lib" )
endif()

if( WIN32 )
	set( OPENSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/OpenSSL )
	list( APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/ImageMagick )

	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/OpenSSL/bin/libcrypto-3-x64.dll
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/OpenSSL/bin/libssl-3-x64.dll
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
	file( GLOB ENGINES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/OpenSSL/lib/engines-3/* )
	file( COPY ${ENGINES_FILES}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib/engines-3 )
	file( GLOB OSSL_MODULES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/OpenSSL/lib/ossl-modules/* )
	file( COPY ${OSSL_MODULES_FILES}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib/ossl-modules )

	file( GLOB IM_DLL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/ImageMagick/*.dll )
	file( COPY ${IM_DLL_FILES}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
	file( GLOB IM_XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/ImageMagick/*.xml )
	file( COPY ${IM_XML_FILES}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
	file( GLOB IM_MODULES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Windows/ImageMagick/modules/* )
	file( COPY ${IM_MODULES_FILES}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin/modules )

	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/md-pdf-gui.bat
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/test.render.bat
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin )
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

find_package( Perl REQUIRED )
find_package( Python REQUIRED )

add_subdirectory( 3rdparty/ksyntaxhighlighting )

include( ExternalProject )
set( SKIA_ARGS "is_official_build=true skia_use_system_expat=false skia_use_system_harfbuzz=false skia_use_system_icu=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false" )
ExternalProject_Add( skia
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia
	TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/skia/tmp
	STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/skia/stamp
	BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/skia
	GIT_REPOSITORY https://skia.googlesource.com/skia.git
	GIT_TAG a9db1213b17966d0344957ba1d3834b420b2a329
	CONFIGURE_COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia/tools/git-sync-deps
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia/bin/fetch-ninja
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia/bin/gn gen ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/skia --args=${SKIA_ARGS}
	BUILD_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/skia/third_party/ninja/ninja -C ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/skia
	INSTALL_COMMAND ""
	TEST_COMMAND ""
)

add_subdirectory( src )

if( BUILD_MDPDF_TESTS )
	enable_testing()

	add_subdirectory( tests )
endif()
