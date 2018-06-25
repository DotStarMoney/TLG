cmake_minimum_required(VERSION 3.2)
project(TLG)
# -------------------------------- BUILD TARGETS -------------------------------
#_______________________________________________________________________________
#third_party::rapidxml
add_library(third_party_rapidxml INTERFACE)
target_sources(third_party_rapidxml INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/rapidxml.hpp
	${CMAKE_CURRENT_LIST_DIR}/rapidxml_iterators.hpp
	${CMAKE_CURRENT_LIST_DIR}/rapidxml_print.hpp
	${CMAKE_CURRENT_LIST_DIR}/rapidxml_utils.hpp)
target_include_directories(third_party_rapidxml INTERFACE
	${CMAKE_CURRENT_LIST_DIR})
# ----------------------------------- FOLDER -----------------------------------
# set_target_properties(
	  # <Executable or non-interface library targets here>
	# PROPERTIES FOLDER util)