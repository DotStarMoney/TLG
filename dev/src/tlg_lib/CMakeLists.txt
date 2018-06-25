cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#tlg_lib::indexstagegraph
add_library(tlg_lib_indexstagegraph INTERFACE)
target_sources(tlg_lib_indexstagegraph INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/indexstagegraph.h)
target_include_directories(tlg_lib_indexstagegraph INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(tlg_lib_indexstagegraph INTERFACE
	util_loan
	util_deleterptr
	util_noncopyable
	absl::strings
	absl::base
	glog)
#_______________________________________________________________________________
#tlg_lib::indexstagegraph test
add_executable(tlg_lib_indexstagegraph_test
	indexstagegraph_test.cc)
target_link_libraries(tlg_lib_indexstagegraph_test
	tlg_lib_indexstagegraph
	gtest
	gtest_main)
add_test(tlg_lib_indexstagegraph tlg_lib_indexstagegraph_test)
#_______________________________________________________________________________
#tlg_lib::stage
add_library(tlg_lib_stage
	stage.cc
	stage.h)
target_link_libraries(tlg_lib_stage
	retro_fbimg
	util_loan
	physics_retro)
#_______________________________________________________________________________
#tlg_lib::stage
add_executable(tlg_lib_stage_test
	stage_test.cc)
target_link_libraries(tlg_lib_stage_test
	tlg_lib_stage
	gtest
	gtest_main)
add_test(tlg_lib_stage tlg_lib_stage_test)
#_______________________________________________________________________________
#tlg_lib::tileset
add_library(tlg_lib_tileset
	tileset.cc
	tileset.h)
target_link_libraries(tlg_lib_tileset
	retro_fbimg
	util_loan
	glog
	util_xml)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	tlg_lib_indexstagegraph_test
	tlg_lib_stage
	tlg_lib_stage_test
	tlg_lib_tileset
	PROPERTIES FOLDER tlg_lib)