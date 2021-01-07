cmake_minimum_required (VERSION 3.0)
project( "LavaCake-Examples" )


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(LavaCake)

###############################################################
# Sample projects                                             #
###############################################################


# Sample projects generation
macro( list_samples )
		file( GLOB SAMPLES RELATIVE "${CMAKE_CURRENT_LIST_DIR}/Samples/Source Files" "${CMAKE_CURRENT_LIST_DIR}/Samples/Source Files/*" )

		FOREACH( SAMPLE ${SAMPLES} )
			file( GLOB SAMPLE_SOURCE_FILES "Samples/Source Files/${SAMPLE}/*.cpp" )
			source_group( "" FILES ${SAMPLE_SOURCE_FILES} )

			add_executable( ${SAMPLE} ${SAMPLE_SOURCE_FILES} )
			target_link_libraries( ${SAMPLE} LavaCake )
			set_property( TARGET ${SAMPLE} PROPERTY FOLDER "Samples" )
		ENDFOREACH()
endmacro()

# Sample projects generation
list_samples()
file( COPY "${CMAKE_CURRENT_LIST_DIR}/Samples/Data" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
