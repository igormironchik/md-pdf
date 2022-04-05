
project( tests )

add_subdirectory( auto )

if( BUILD_BENCHMARK )
	add_subdirectory( md_benchmark )
endif( BUILD_BENCHMARK )
