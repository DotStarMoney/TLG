cmake_minimum_required(VERSION 3.2)
project(TLG)
# ----------------------------- ADD SUBDIRECTORIES -----------------------------
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#thread::thread_pool
add_library(thread_thread_pool
	thread_pool.cc
	thread_pool.h)
target_link_libraries(thread_thread_pool
	util_loan
	util_noncopyable
	thread_thread_reservoir
	glog)
#_______________________________________________________________________________
#util::thread_pool test
add_executable(thread_thread_pool_test
	thread_pool_test.cc)
target_link_libraries(thread_thread_pool_test
	absl::synchronization
	thread_thread_pool
	gmock
	gtest_main)
add_test(thread_thread_pool thread_thread_pool_test)
#_______________________________________________________________________________
#thread::thread_reservoir
add_library(thread_thread_reservoir
	thread_reservoir.cc
	thread_reservoir.h)
target_link_libraries(thread_thread_reservoir
	util_loan
	glog)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	thread_thread_pool
	thread_thread_reservoir
	thread_thread_pool_test
	PROPERTIES FOLDER thread)
