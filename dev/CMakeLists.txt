cmake_minimum_required(VERSION 3.2)
project(TLG)

set(EX_PROJ_SOURCE_DIR ${CMAKE_BINARY_DIR}/deps/Source)
set(EX_PROJ_BUILD_DIR ${CMAKE_BINARY_DIR}/deps/Build)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(deps-CMakeLists.txt deps/CMakeLists.txt COPYONLY)
execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)

include_directories(${EX_PROJ_SOURCE_DIR}/SDL_EX/include
					${EX_PROJ_SOURCE_DIR}/GLM_EX/glm)

add_subdirectory(${CMAKE_BINARY_DIR}/../src)
add_subdirectory(${CMAKE_BINARY_DIR}/../inc)

set(ABSOLUTE_SRC_FILES "")
set(ABSOLUTE_HDR_FILES "")
foreach(filen ${SRC_FILES})
	list(APPEND ABSOLUTE_SRC_FILES "src/${filen}")
endforeach(filen)
foreach(filen ${HDR_FILES})
	list(APPEND ABSOLUTE_HDR_FILES "inc/${filen}")
endforeach(filen)
include_directories(${CMAKE_BINARY_DIR}/../inc)


include_directories(${CMAKE_BINARY_DIR}/inc)
add_executable(TLG ${ABSOLUTE_SRC_FILES} ${ABSOLUTE_HDR_FILES})

if(WIN32)
    target_link_libraries(TLG debug ${EX_PROJ_BUILD_DIR}/SDL_EX/Debug/SDL2-staticd${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(TLG debug ${EX_PROJ_BUILD_DIR}/SDL_EX/Debug/SDL2maind${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG optimized ${EX_PROJ_BUILD_DIR}/SDL_EX/Release/SDL2-static${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG optimized ${EX_PROJ_BUILD_DIR}/SDL_EX/Release/SDL2main${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG winmm.lib imm32.lib version.lib)
	add_custom_command(TARGET TLG POST_BUILD
		COMMAND XCOPY \"$(SolutionDir)..\\res" \"$(TargetDir)res\\" /S /Y /I)
	add_custom_command(TARGET TLG POST_BUILD
		COMMAND XCOPY \"$(SolutionDir)..\\res" \"$(TargetDir)..\\res\\" /S /Y /I)
endif()
if(UNIX)
	#
	#
endif()