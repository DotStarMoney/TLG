cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#physics::retro
add_library(physics_retro
	retro.cc
	retro.h)
target_link_libraries(physics_retro
	glog
	glm)
#_______________________________________________________________________________
#physics::retro test
add_executable(physics_retro_test
	retro_test.cc)
target_link_libraries(physics_retro_test
	physics_retro
	gtest
	gtest_main)
add_test(physics_retro physics_retro_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	physics_retro
	physics_retro_test
	PROPERTIES FOLDER physics)