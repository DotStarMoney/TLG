cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#experimental::radarconcept
add_library(experimental_radarconcept
	radarconcept.cc
	radarconcept.h)
target_link_libraries(experimental_radarconcept
	retro_fbgfx
	retro_fbimg
	physics_geometry2
	glog
	glm)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	experimental_radarconcept
	PROPERTIES FOLDER experimental)
