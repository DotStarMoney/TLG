cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#experimental::radarconcept
add_library(experimental_radarconcept
	radarconcept.cc
	radarconcept.h)
target_link_libraries(experimental_radarconcept
	absl::strings
	retro_fbgfx
	retro_fbimg
	physics_geometry2
	util_random
	glog
	glm)
#experimental::cask
add_library(experimental_cask
	cask.cc
	cask.h)
target_link_libraries(experimental_cask
	util_random
	glog
	glm
	gflags
	thread_workqueue)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	experimental_radarconcept
	experimental_cask
	PROPERTIES FOLDER experimental)
