cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#util::static_type_assert
add_library(base_static_type_assert INTERFACE)
target_sources(base_static_type_assert INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/static_type_assert.h)
target_include_directories(base_static_type_assert INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#base::static_type_assert test
add_executable(base_static_type_assert_test
	static_type_assert_test.cc)
target_link_libraries(base_static_type_assert_test
	base_static_type_assert
	gtest
	gtest_main)
add_test(base_static_type_assert base_static_type_assert_test)
#_______________________________________________________________________________
#util::platform
add_library(base_platform INTERFACE)
target_sources(base_platform INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/platform.h)
target_include_directories(base_platform INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	base_static_type_assert_test
	PROPERTIES FOLDER base)

