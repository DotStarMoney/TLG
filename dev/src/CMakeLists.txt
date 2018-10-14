cmake_minimum_required(VERSION 3.2)
project(TLG)
set(LOCAL ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------- ADD SUBDIRECTORIES -----------------------------
#add_subdirectory(${LOCAL}/audio)
add_subdirectory(${LOCAL}/base)
add_subdirectory(${LOCAL}/third_party)
#add_subdirectory(${LOCAL}/storage)
add_subdirectory(${LOCAL}/thread)
add_subdirectory(${LOCAL}/util)
add_subdirectory(${LOCAL}/physics)
add_subdirectory(${LOCAL}/retro)
add_subdirectory(${LOCAL}/sdl_util)
add_subdirectory(${LOCAL}/tlg_lib)
add_subdirectory(${LOCAL}/experimental)

# -------------------------------- BUILD TARGETS -------------------------------

# TLG
#add_executable(TLG ${LOCAL}/main.cc)
#target_link_libraries(TLG
#	debug SDL2-staticd optimized SDL2-static
#	debug SDL2maind    optimized SDL2main
#	winmm.lib
#	imm32.lib
#	version.lib)
# Make a copy of resources for the binaries directly
#if(WIN32)
#	add_custom_command(TARGET TLG POST_BUILD
#		COMMAND robocopy \"$(SolutionDir)..\\res" \"$(TargetDir)res\\" /s /purge)
#endif()
#if(UNIX)
#	#
#endif()

add_executable(TLG
	main.cc)
target_link_libraries(TLG
	experimental_cask
	gflags)
add_custom_command(TARGET TLG POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_BINARY_DIR}/../res $<TARGET_FILE_DIR:TLG>/res)
add_custom_command(TARGET TLG POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_BINARY_DIR}/../res $<TARGET_FILE_DIR:TLG>/../res)
					   