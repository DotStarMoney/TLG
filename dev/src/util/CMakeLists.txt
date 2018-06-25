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
#util::cannonical_errors
add_library(util_cannonical_errors
	cannonical_errors.cc
	cannonical_errors.h)
target_link_libraries(util_cannonical_errors
	absl::strings)
#_______________________________________________________________________________
#util::deleterptr
add_library(util_deleterptr INTERFACE)
target_sources(util_deleterptr INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/deleterptr.h)
target_include_directories(util_deleterptr INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::deleterptr test
add_executable(util_deleterptr_test
	deleterptr_test.cc)
target_link_libraries(util_deleterptr_test
	util_deleterptr
	gtest
	gtest_main)
add_test(util_deleterptr util_deleterptr_test)
#_______________________________________________________________________________
#util::fixed_point
add_library(util_fixed_point INTERFACE)
target_sources(util_fixed_point INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/fixed_point.h)
target_include_directories(util_fixed_point INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::loan
add_library(util_loan INTERFACE)
target_sources(util_loan INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/loan.h)
target_include_directories(util_loan INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(util_loan INTERFACE
	glog)
#_______________________________________________________________________________
#util::loan test
add_executable(util_loan_test
	loan_test.cc)
target_link_libraries(util_loan_test
	util_loan
	gtest
	gtest_main)
add_test(util_loan util_loan_test)
#_______________________________________________________________________________
#util::make_cleanup
add_library(util_make_cleanup
	make_cleanup.cc
	make_cleanup.h)
#_______________________________________________________________________________
#util::make_cleanup test
add_executable(util_make_cleanup_test
	make_cleanup_test.cc)
target_link_libraries(util_make_cleanup_test
	util_make_cleanup
	gtest
	gtest_main)
add_test(util_make_cleanup util_make_cleanup_test)
#_______________________________________________________________________________
#util::noncopyable
add_library(util_noncopyable INTERFACE)
target_sources(util_noncopyable INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/noncopyable.h)
target_include_directories(util_noncopyable INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::pstruct
add_library(util_pstruct INTERFACE)
target_sources(util_pstruct INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/pstruct.h)
target_include_directories(util_pstruct INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::pstruct test
add_executable(util_pstruct_test
	pstruct_test.cc)
target_link_libraries(util_pstruct_test
	util_pstruct
	gtest
	gtest_main)
add_test(util_pstruct util_pstruct_test)
#_______________________________________________________________________________
#util::status
add_library(util_status
	status.cc
	status.h)
target_link_libraries(util_status
	util_cannonical_errors
	absl::strings
	glog)
#_______________________________________________________________________________
#util::status test
add_executable(util_status_test
	status_test.cc)
target_link_libraries(util_status_test
	util_status
	gtest
	gmock
	gtest_main)
add_test(util_status util_status_test)
#_______________________________________________________________________________
#util::status_macros
add_library(util_status_macros INTERFACE)
target_sources(util_status_macros INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/status_macros.h)
target_include_directories(util_status_macros INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::status_macros test
add_executable(util_status_macros_test
	status_macros_test.cc)
target_link_libraries(util_status_macros_test
	util_status_macros
	util_status
	util_statusor
	gtest
	gtest_main)
add_test(util_status_macros util_status_macros_test)
#_______________________________________________________________________________
#util::statusor
add_library(util_statusor INTERFACE)
target_sources(util_statusor INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/statusor.h)
target_include_directories(util_statusor INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(util_statusor INTERFACE
	util_status
	util_noncopyable
	glog)
#_______________________________________________________________________________
#util::statusor test
add_executable(util_statusor_test
	statusor_test.cc)
target_link_libraries(util_statusor_test
	util_statusor
	gtest
	gtest_main)
add_test(util_statusor util_statusor_test)
#_______________________________________________________________________________
#util::stopwatch
add_library(util_stopwatch INTERFACE)
target_sources(util_stopwatch INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/stopwatch.h)
target_include_directories(util_stopwatch INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
#_______________________________________________________________________________
#util::xml
add_library(util_xml
	xml.cc
	xml.h)
target_link_libraries(util_xml
	third_party_rapidxml
	util_noncopyable
	absl::strings
	glog)
#_______________________________________________________________________________
#util::xml test
add_executable(util_xml_test
	xml_test.cc)
target_link_libraries(util_xml_test
	util_xml
	gtest
	gtest_main)
add_test(util_xml util_xml_test)
# ----------------------------------- FOLDER -----------------------------------
set_target_properties(
	util_bits
	util_bits_test
	util_cannonical_errors
	util_deleterptr_test
	util_loan_test
	util_make_cleanup
	util_make_cleanup_test
	util_pstruct_test
	util_status
	util_status_test
	util_statusor_test
	util_status_macros_test
	util_xml
	util_xml_test
	PROPERTIES FOLDER util)