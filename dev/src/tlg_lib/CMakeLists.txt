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
add_library(tlg_lib_stagecontent
	stagecontent.cc
	stagecontent.h)
target_link_libraries(tlg_lib_stagecontent
	tlg_lib_tileset
	util_loan
	physics_retro
	util_xml
	tlg_lib_rescache
	absl::strings
	glm)
#_______________________________________________________________________________
#tlg_lib::stage test
add_executable(tlg_lib_stagecontent_test
	stagecontent_test.cc)
target_link_libraries(tlg_lib_stagecontent_test
	tlg_lib_stagecontent
	gtest
	gmock
	gtest_main)
add_test(tlg_lib_stagecontent tlg_lib_stagecontent_test)
#_______________________________________________________________________________
#tlg_lib::tileset
add_library(tlg_lib_tileset
	tileset.cc
	tileset.h)
target_link_libraries(tlg_lib_tileset
	retro_fbimg
	tlg_lib_rescache
	glog
	util_xml)
#_______________________________________________________________________________
#tlg_lib::rescache
add_library(tlg_lib_rescache
	rescache.cc
	rescache.h)
target_link_libraries(tlg_lib_rescache
	util_loan
	util_noncopyable
	glog
	absl::strings)
#_______________________________________________________________________________
#tlg_lib::rescache test
add_executable(tlg_lib_rescache_test
	rescache_test.cc)
target_link_libraries(tlg_lib_rescache_test
	tlg_lib_rescache
	gtest
	gmock
	gtest_main)
add_test(tlg_lib_rescache tlg_lib_rescache_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	tlg_lib_indexstagegraph_test
	tlg_lib_stagecontent
	tlg_lib_stagecontent_test
	tlg_lib_tileset
	tlg_lib_rescache
	tlg_lib_rescache_test
	PROPERTIES FOLDER tlg_lib)
