cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#base::static_type_assert
add_library(base_static_type_assert INTERFACE)
target_sources(base_static_type_assert INTERFACE
	static_type_assert.h)
#_______________________________________________________________________________
#base::static_type_assert test
add_executable(base_static_type_assert_test
	static_type_assert_test.cc)
target_link_libraries(base_static_type_assert_test
	gtest
	gtest_main)
add_test(base_static_type_assert base_static_type_assert_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	base_static_type_assert_test
	PROPERTIES FOLDER base)

