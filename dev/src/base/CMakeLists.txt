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
#base::platform
add_library(base_platform INTERFACE)
target_sources(base_platform INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/platform.h)
target_include_directories(base_platform INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#base::fixed32
add_library(base_fixed32 INTERFACE)
target_sources(base_fixed32 INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/fixed32.h)
target_include_directories(base_fixed32 INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#base::fixed32 test
add_executable(base_fixed32_test
	fixed32_test.cc)
target_link_libraries(base_fixed32_test
	base_fixed32
	gtest
	gtest_main)
add_test(base_fixed32 base_fixed32_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	base_static_type_assert_test
	base_fixed32_test
	PROPERTIES FOLDER base)

