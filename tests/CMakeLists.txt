
project( tests )

add_subdirectory( auto )

add_subdirectory( dev )

if( BUILD_BENCHMARK )
	add_subdirectory( md_benchmark )
endif( BUILD_BENCHMARK )
