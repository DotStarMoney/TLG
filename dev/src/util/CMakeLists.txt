cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#util::bits
add_library(util_bits
	bits.cc
	bits.h)
#_______________________________________________________________________________
#util::bits test
add_executable(util_bits_test
	bits_test.cc)
target_link_libraries(util_bits_test
	util_bits
	gtest
	gtest_main)
add_test(util_bits util_bits_test)
#_______________________________________________________________________________
#util::deleterptr
add_library(util_deleterptr INTERFACE)
target_sources(util_deleterptr INTERFACE
	deleterptr.h)
#_______________________________________________________________________________
#util::deleterptr test
add_executable(util_deleterptr_test
	deleterptr_test.cc)
target_link_libraries(util_deleterptr_test
	gtest
	gtest_main)
add_test(util_deleterptr util_deleterptr_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	util_bits
	util_bits_test
	util_deleterptr_test
	PROPERTIES FOLDER util)

