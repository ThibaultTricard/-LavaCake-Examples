cmake_minimum_required (VERSION 3.0)
project( "LavaCake-Examples" )


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_subdirectory(LavaCake)

###############################################################
# Sample projects                                             #
###############################################################

file( COPY "${CMAKE_CURRENT_LIST_DIR}/Samples/Data" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )


#Console renderer

clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/ConsoleRenderer/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/ConsoleRenderer/${FILENAME}.spv spv1.3)
endforeach()
add_executable( ConsoleRenderer  "Samples/Source Files/ConsoleRenderer/main.cpp")
AutoSPIRV(ConsoleRenderer)
target_link_libraries( ConsoleRenderer LavaCake )
set_property( TARGET ConsoleRenderer PROPERTY FOLDER "Samples" )


#Fourier Transform
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/FourierTransform/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/FourierTransform/${FILENAME}.spv spv1.3)
endforeach()
add_executable( FourierTransform  "Samples/Source Files/FourierTransform/main.cpp")
AutoSPIRV(FourierTransform)
target_link_libraries( FourierTransform LavaCake )
set_property( TARGET FourierTransform PROPERTY FOLDER "Samples" )



#Hello World
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/HelloWorld/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/HelloWorld/${FILENAME}.spv spv1.3)
endforeach()
add_executable( HelloWorld  "Samples/Source Files/HelloWorld/main.cpp")
AutoSPIRV(HelloWorld)
target_link_libraries( HelloWorld LavaCake )
set_property( TARGET HelloWorld PROPERTY FOLDER "Samples" )


#ImGui
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/ImGui/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/ImGui/${FILENAME}.spv spv1.3)
endforeach()
add_executable( ImGui  "Samples/Source Files/ImGui/main.cpp")
AutoSPIRV(ImGui)
target_link_libraries( ImGui LavaCake )
set_property( TARGET ImGui PROPERTY FOLDER "Samples" )


#Instancing
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/Instancing/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/Instancing/${FILENAME}.spv spv1.3)
endforeach()
add_executable( Instancing  "Samples/Source Files/Instancing/main.cpp")
AutoSPIRV(Instancing)
target_link_libraries( Instancing LavaCake )
set_property( TARGET Instancing PROPERTY FOLDER "Samples" )


#JumpFlooding
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/JumpFlooding/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME ${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/JumpFlooding/${FILENAME}.spv spv1.3)
endforeach()
add_executable( JumpFlooding  "Samples/Source Files/JumpFlooding/main.cpp")
AutoSPIRV(JumpFlooding)
target_link_libraries( JumpFlooding LavaCake )
set_property( TARGET JumpFlooding PROPERTY FOLDER "Samples" )


option(BUILD_RAYTRACING_EXAMPLE "Build the raytracing example" off)


#RT Hello World

if(BUILD_RAYTRACING_EXAMPLE)
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/RaytracingHelloWorld/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME RaytracingHelloWorld/${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/RaytracingHelloWorld/${FILENAME}.spv spv1.4)
endforeach()
add_executable( RaytracingHelloWorld  "Samples/Source Files/RaytracingHelloWorld/main.cpp")
AutoSPIRV(RaytracingHelloWorld)
target_link_libraries( RaytracingHelloWorld LavaCake )
set_property( TARGET RaytracingHelloWorld PROPERTY FOLDER "Samples" )

#RT Cornell box
clearShader()
file( GLOB SHADER_SOURCE_FILES "Samples/Data/Shaders/CornellBox/*")
FOREACH( shader ${SHADER_SOURCE_FILES} )
	string(REGEX MATCH "[A-Za-z0-9_\\-]*\\.[A-Za-z0-9]*" FILENAME CornellBox/${shader})
	addShader(${shader} ${CMAKE_CURRENT_BINARY_DIR}/Data/Shaders/CornellBox/${FILENAME}.spv spv1.4)
endforeach()
add_executable( CornellBox  "Samples/Source Files/CornellBox/main.cpp")
AutoSPIRV(CornellBox)
target_link_libraries( CornellBox LavaCake )
set_property( TARGET CornellBox PROPERTY FOLDER "Samples" )

endif()


