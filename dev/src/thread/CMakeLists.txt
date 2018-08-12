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
#_______________________________________________________________________________
#thread::semaphore
add_library(thread_semaphore
	semaphore.cc
	semaphore.h)
target_link_libraries(thread_semaphore
	util_noncopyable
	absl::base
	glog)	
#_______________________________________________________________________________
#thread::gateway
add_library(thread_gateway
	gateway.cc
	gateway.h)
target_link_libraries(thread_gateway
	util_noncopyable
	absl::base
	glog)
#_______________________________________________________________________________
#thread::workqueue
add_library(thread_workqueue
	workqueue.cc
	workqueue.h)
target_link_libraries(thread_workqueue
	util_noncopyable
	thread_semaphore
	thread_gateway
	glog)
#_______________________________________________________________________________
#thread::workqueue test
add_executable(thread_workqueue_test
	workqueue_test.cc)
target_link_libraries(thread_workqueue_test
	absl::synchronization
	thread_workqueue
	gmock
	gtest_main)
add_test(thread_workqueue thread_workqueue_test)
#_______________________________________________________________________________
#thread::affinitizingscheduler
add_library(thread_affinitizingscheduler
	affinitizingscheduler.cc
	affinitizingscheduler.h)
target_link_libraries(thread_affinitizingscheduler
	util_noncopyable
	thread_workqueue
	util_random
	absl::base
	glog)
#_______________________________________________________________________________
#thread::affinitizingscheduler test
add_executable(thread_affinitizingscheduler_test
	affinitizingscheduler_test.cc)
target_link_libraries(thread_affinitizingscheduler_test
	gmock
	gtest_main)
add_test(thread_affinitizingscheduler thread_affinitizingscheduler_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	thread_thread_pool
	thread_thread_reservoir
	thread_thread_pool_test
	thread_semaphore
	thread_gateway
	thread_workqueue
	thread_workqueue_test
	thread_affinitizingscheduler
	thread_affinitizingscheduler_test
	PROPERTIES FOLDER thread)
