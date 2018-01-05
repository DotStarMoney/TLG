cmake_minimum_required(VERSION 3.2)
project(TLG)

set(EX_PROJ_SOURCE_DIR ${CMAKE_BINARY_DIR}/deps/Source)
set(EX_PROJ_BUILD_DIR ${CMAKE_BINARY_DIR}/deps/Build)
set(TLG_SOURCE_DIR ${CMAKE_BINARY_DIR}/../src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(deps-CMakeLists.txt deps/CMakeLists.txt COPYONLY)
execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)

# From: https://github.com/google/googletest/blob/master/googletest/README.md {
#
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${EX_PROJ_SOURCE_DIR}/GoogleTest_EX
                 ${EX_PROJ_BUILD_DIR}/GoogleTest_EX
                 EXCLUDE_FROM_ALL)
# }

include_directories(${EX_PROJ_SOURCE_DIR}/SDL_EX/include
					${EX_PROJ_SOURCE_DIR}/GLM_EX/glm
					${EX_PROJ_SOURCE_DIR}/GoogleTest_EX/include)

add_subdirectory(${TLG_SOURCE_DIR})
include_directories(${TLG_SOURCE_DIR})

source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${HDR_FILES})

add_executable(TLG ${SRC_FILES} ${HDR_FILES})
target_link_libraries(TLG gtest_main)
if(WIN32)
    target_link_libraries(TLG debug ${EX_PROJ_BUILD_DIR}/SDL_EX/Debug/SDL2-staticd${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(TLG debug ${EX_PROJ_BUILD_DIR}/SDL_EX/Debug/SDL2maind${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG optimized ${EX_PROJ_BUILD_DIR}/SDL_EX/Release/SDL2-static${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG optimized ${EX_PROJ_BUILD_DIR}/SDL_EX/Release/SDL2main${CMAKE_STATIC_LIBRARY_SUFFIX})
	target_link_libraries(TLG winmm.lib imm32.lib version.lib)
	# Make a copy of resources for the binaries directly
	add_custom_command(TARGET TLG POST_BUILD
		COMMAND robocopy \"$(SolutionDir)..\\res" \"$(TargetDir)res\\" /s /purge)
endif()
if(UNIX)
	#
	#
endif()