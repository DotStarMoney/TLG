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
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	tlg_lib_indexstagegraph_test
	PROPERTIES FOLDER tlg_lib)