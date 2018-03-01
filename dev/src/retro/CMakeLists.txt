cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#retro::fbgfx
add_library(retro_fbgfx
	fbgfx.cc
	fbgfx.h)
target_link_libraries(retro_fbgfx
	glog
	glm)
#_______________________________________________________________________________
#retro::fbimg
add_library(retro_fbimg
	fbimg.cc
	fbimg.h)
target_link_libraries(retro_fbimg
	glog
	glm)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	retro_fbgfx
	retro_fbimg
	PROPERTIES FOLDER retro)