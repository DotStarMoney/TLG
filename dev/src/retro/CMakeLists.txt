cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#retro::fbcore
add_library(retro_fbcore INTERFACE)
target_sources(retro_fbcore INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/fbcore.h)
target_include_directories(retro_fbcore INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#retro::fbgfx
add_library(retro_fbgfx
	fbgfx.cc
	fbgfx.h)
target_link_libraries(retro_fbgfx
	sdl_util_cleanup
	retro_fbimg
	util_deleterptr
	absl::strings
	SDL2-static
	retro_fbcore
	glog
	glm)
#_______________________________________________________________________________
#retro::fbimg
add_library(retro_fbimg
	fbimg.cc
	fbimg.h)
target_link_libraries(retro_fbimg
	retro_fbgfx
	util_deleterptr
	util_noncopyable
	SDL2-static
	absl::strings
	stb_image
	retro_fbcore
	glog
	glm)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	retro_fbgfx
	retro_fbimg
	PROPERTIES FOLDER retro)