cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#sdl_util::Cleanup
add_library(sdl_util_cleanup
	cleanup.h
	cleanup.cc)
target_link_libraries(sdl_util_cleanup
	SDL2-static
	glog)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	sdl_util_cleanup
	PROPERTIES FOLDER SDL_util)